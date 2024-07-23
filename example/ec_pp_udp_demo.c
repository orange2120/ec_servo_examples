// SPDX-License-Identifier: (GPL-2.0 OR MIT)
/*
 * Control motor with UDP
 * Copyright 2018-2020 NXP
 * Copyright 2024 NASA Lab, NTU
 */

#include <stdio.h>
#include <unistd.h> //STDIN_FILENO
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "nservo.h"
#include "nser_pdo.h"
#include "pthread.h"

#define UDP_SERVER_PORT 3000
#define DEFAULT_PROFILE_VELOCITY 100

typedef struct axis_stat
{
    uint8_t mode;
    int32_t current_position;
    int32_t current_velocity;
    uint32_t scale;
    void *axis_status;
} axis_status_t;

typedef struct axis_pp_stat
{
    int32_t target_position;
    int32_t profile_velocity;
    int status;
} axis_pp_status_t;

enum new_pos_status
{
    nps_nop,
    nps_init,
    nps_set_new_point,
    nps_wait_set_ack,
    nps_op,
};

static axis_status_t axis_status;
static axis_pp_status_t pp_status;

static char rx_buf[64] = {0};

int socket_fd = 0;
int run = 1;

void signal_handler(int sig)
{
    run = 0;
}

void *udp_task()
{
    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);

    while (run)
    {
        int32_t position = axis_status.current_position;
        int32_t velocity = axis_status.current_velocity;

        if (recvfrom(socket_fd, rx_buf, sizeof(rx_buf), 0,
                     (struct sockaddr *)&client_addr, (socklen_t *)&client_len) < 0)
        {
            printf("[ERR] Rx fail\n");
        }
        printf("[MSG] %s\n", rx_buf);

        sscanf(rx_buf, "P%d,V%d", &position, &velocity);

        pp_status.target_position = position;
        pp_status.profile_velocity = velocity;

        // printf("Update target pos=%d, vel=%d\n", pp_status.target_position, pp_status.profile_velocity);

        printf("t_vel = %d,t_pos = %d, c_pos = %d\n",
            pp_status.profile_velocity,
            pp_status.target_position,
            axis_status.current_position);

        memset(rx_buf, '\0', sizeof(rx_buf));
    }

    pthread_exit(NULL);
}

static int axle_start(nser_axle *ns_axle, uint16_t status, uint8_t mode)
{
    int ret = 0;
    axle_state s = get_axle_state(status);
    switch (s)
    {
    case (no_ready_to_switch_on):
    case (switch_on_disable):
        nser_pdo_set_Controlword(ns_axle, contrlword_shutdown(0));
        break;
    case (ready_to_switch_on):
        nser_pdo_set_Controlword(ns_axle, contrlword_switch_on(0));
        break;
    case (switched_on):
        nser_pdo_set_Controlword(ns_axle, contrlword_enable_operation(0));
        nser_pdo_set_Modes_of_operation(ns_axle, mode);
        break;
    case (operation_enable):
        ret = 1;
        break;
    case (quick_stop_active):
    case (fault_reaction_active):
    case (fault):
    default:
        ret = -1;
    }
    return ret;
}

static int pp_update_target_position(nser_axle *ns_axis, axis_status_t *axis_status)
{
    uint16_t statusword = 0;
    uint16_t controlword = 0;
    axis_pp_status_t *status = axis_status->axis_status;
    controlword = nser_pdo_get_Controlword(ns_axis);
    statusword = nser_pdo_get_Statusword(ns_axis);

    if (axle_start(ns_axis, statusword, op_mode_pp) != 1)
        return 0;

    // printf("t_vel = %d, c_vel = %d, t_pos = %d, c_pos = %d\n",
    //         status->profile_velocity,
    //         axis_status->current_velocity,
    //         status->target_position,
    //         axis_status->current_position);

    if (ns_axis->nser_master->ns_master_state == ALL_OP)
    {
        axis_status->current_velocity = nser_pdo_get_Velocity_actual_value(ns_axis);
        axis_status->current_position = nser_pdo_get_Position_actual_value(ns_axis);
        if (status->status == 0)
        {
            status->target_position = axis_status->current_position;
            nser_pdo_set_Profile_velocity(ns_axis,
                                          status->profile_velocity);
            nser_pdo_set_Target_position(ns_axis,
                                         status->target_position);
            nser_pdo_set_Controlword(ns_axis, 0xf);
            if (status->profile_velocity != 0)
            {
                status->status = 1;
            }
        }
        else if (status->status > 0 && status->status < 20)
        {
            nser_pdo_set_Target_position(ns_axis,
                                         status->target_position);
            nser_pdo_set_Profile_velocity(ns_axis,
                                          status->profile_velocity);
            status->status++;
        }
        else if (status->status >= 20 && status->status < 30)
        {
            nser_pdo_set_Controlword(ns_axis,
                                     contrlword_new_set_point(controlword));
            status->status++;
        }
        else if (status->status == 30)
        {
            if (is_status_target_reached(statusword))
            {
                printf("t_vel = %d,t_pos = %d, c_pos = %d\n",
                    status->profile_velocity,
                    status->target_position,
                    axis_status->current_position);
                status->status = 1;
            }
            nser_pdo_set_Controlword(ns_axis, 0xf);
        }
    }
    return 0;
}

int main_task(nser_global_data *ns_data)
{
    nser_axle *ns_axis = &ns_data->ns_axles[0];
    pp_update_target_position(ns_axis, &axis_status);

    return ns_data->running;
}

int main(int argc, char **argv)
{
    pthread_t udp_tid;
    nser_global_data *ns_data;

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // if (argc > 2)
    // {
    //     if (argv[2][0] == 'n')
    //         auto_start = 0;
    // }
    char *xmlfile = argv[1];
    if (!(ns_data = nser_app_run_init(xmlfile)))
    {
        fprintf(stderr, "Failed to initialize the App\n");
        return 0;
    }

    // create socket
    socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        printf("Fail to create a socket.");
    }

    // server addr
    struct sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(UDP_SERVER_PORT)};

    // binding
    if (bind(socket_fd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Bind socket failed!");
        close(socket_fd);
        exit(0);
    }

    pthread_create(&udp_tid, NULL, udp_task, NULL);
    printf("UDP server started at :%d\n", UDP_SERVER_PORT);

    axis_status.axis_status = &pp_status;
    pp_status.profile_velocity = DEFAULT_PROFILE_VELOCITY;

    printf("Start task...\n\n");
    if (user_cycle_task_start(ns_data, main_task, 0))
    {
        fprintf(stderr, "Failed to start task\n");
        return 0;
    }

    while (run)
    {
        sched_yield();
    }
    user_cycle_task_stop(ns_data);
    nser_deactivate_all_masters(ns_data);

    if (close(socket_fd) < 0)
    {
        perror("close socket failed!");
    }
    printf("\nExit...\n");

    return 0;
}
