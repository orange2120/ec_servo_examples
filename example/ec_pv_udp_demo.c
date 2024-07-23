// SPDX-License-Identifier: (GPL-2.0 OR MIT)
/*
 * Copyright 2018-2020 NXP
 * Copyright 2024 NASA Lab, NTU
 */

#include <stdio.h>
#include <unistd.h>     //STDIN_FILENO
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

int run = 1;

static char rx_buf[32];
int socket_fd = 0;

static int32_t speed = 0;

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
        if (recvfrom(socket_fd, rx_buf, sizeof(rx_buf), 0,
                     (struct sockaddr *)&client_addr, (socklen_t *)&client_len) < 0)
        {
            printf("[ERR] Rx fail\n");
        }
        printf("[MSG] %s\n", rx_buf);

        sscanf(rx_buf, "V%d", &speed);

        printf("Update target velocity=%d\n", speed);

        memset(rx_buf, '\0', sizeof(rx_buf));
    }

    pthread_exit(NULL);
}


static int update_target_velocity(nser_global_data *ns_data)
{
    uint16_t statusword = 0;

    if (ns_data->ns_masteter[0].ns_master_state == ALL_OP) {
        statusword = nser_pdo_get_Statusword(&ns_data->ns_axles[0]);
        if (is_status_target_reached(statusword)) {

            printf("Currect speed is %d rpm \n", speed *60 /4000);
            nser_pdo_set_Target_velocity(&ns_data->ns_axles[0], speed);
            printf("Next target speed is %d rpm \n", speed *60 /4000);
        }
    }
    return ns_data->running;

}

int main( int argc, char **argv)
{
    pthread_t udp_tid;
    nser_global_data *ns_data;
    char *xmlfile = argv[1];

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    if (!(ns_data = nser_app_run_init(xmlfile))) {
        fprintf(stderr, "Failed to initialize the App\n");
        return -1;
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

    if (user_cycle_task_start(ns_data,update_target_velocity ,1))
    {
        fprintf(stderr, "Failed to start task\n");
        return 0;
    }

    while (run) {
     sched_yield();
    }
    user_cycle_task_stop(ns_data);
    nser_deactivate_all_masters(ns_data);

    if (close(socket_fd) < 0)
    {
        perror("close socket failed!");
    }

    printf("\nExit...\n");
}


