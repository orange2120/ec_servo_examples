
import socket
import tkinter as tk
from tkinter import ttk, messagebox

DEFAULT_IP = '192.168.50.100'
# DEFAULT_IP = '127.0.0.1'
DEFAULT_PORT = 3000


DEFAULT_MODE = "PP"
DEFAULT_POS = 0
DEFAULT_VEL = 20000

HOMING_MSG_PP = 'P0,V20000'
HOMING_MSG_PV = 'V0'

class EdgeServoController:
    def __init__(self) -> None:
        self.root = tk.Tk()
        
        self.modes = ["PP", "PV"]
        self.labels = ["Mode", "IP", "Port", "Position", "Velocity"]

        self.root.geometry("275x210")
        self.root.title("Real-time-edge-servo UDP controller")

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server_addr = (DEFAULT_IP, DEFAULT_PORT)

        self.input_tb = []
        self.box_mode = None

        self.current_mode = DEFAULT_MODE

    def build_ui(self):

        self.box_mode = ttk.Combobox(self.root, value=self.modes, state= "readonly", width = 15)
        self.box_mode.current(0)
        self.box_mode.bind('<<ComboboxSelected>>', self.update_mode)

        label_mode = tk.Label(self.root, text=self.labels[0])
        label_ip   = tk.Label(self.root, text=self.labels[1])
        label_port = tk.Label(self.root, text=self.labels[2])
        label_pos  = tk.Label(self.root, text=self.labels[3])
        label_vel  = tk.Label(self.root, text=self.labels[4])

        tb_ip = tk.Text(self.root, height = 1, width = 20)
        tb_port = tk.Text(self.root, height = 1, width = 20)
        tb_pos = tk.Text(self.root, height = 1, width = 20)
        tb_vel = tk.Text(self.root, height = 1, width = 20)

        tb_ip.insert(tk.END, DEFAULT_IP)
        tb_port.insert(tk.END, DEFAULT_PORT)
        tb_pos.insert(tk.END, DEFAULT_POS)
        tb_vel.insert(tk.END, DEFAULT_VEL)

        self.input_tb = [tb_ip, tb_port, tb_pos, tb_vel]

        btn_send = tk.Button(self.root, text="Send", command=self.run_send)
        btn_home = tk.Button(self.root, text="Home", command=self.homing)
        btn_exit = tk.Button(self.root, text="Exit", command = self.root.destroy)

        label_mode.grid(row=0, column=0, pady=(10, 0))
        self.box_mode.grid(row=0, column=1, pady=(10, 0))

        # make layout
        tb_ip.grid(row=1, column=1, sticky='w', pady=(20, 0))
        tb_port.grid(row=2, column=1, sticky='w')
        tb_pos.grid(row=3, column=1, sticky='w', pady=(10, 0))
        tb_vel.grid(row=4, column=1, sticky='w')

        label_ip.grid(row=1, column=0, sticky='w', padx=(10, 0), pady=(20, 0))
        label_port.grid(row=2, column=0, sticky='w', padx=(10, 0))
        label_pos.grid(row=3, column=0, sticky='w', padx=(10, 0), pady=(10, 0))
        label_vel.grid(row=4, column=0, sticky='w', padx=(10, 0))

        btn_send.grid(row=5, column=0, sticky='sw', padx=(10, 0), pady=(10, 0))
        btn_home.grid(row=5, column=1, sticky='sw', padx=(10, 0), pady=(10, 0))
        btn_exit.grid(row=5, column=1, sticky='sw', padx=(120, 0), pady=(10, 0))

    def update_mode(self, event):
        self.current_mode = self.box_mode.get()
        if self.current_mode == "PP":
            self.input_tb[2].configure(state="normal")
        elif self.current_mode == "PV":
            self.input_tb[2].configure(state="disabled")

        print(f"Mode updated: {self.current_mode}")

    def make_msg_from_box(self):
        msg = ""

        if self.current_mode == "PP":
            pos = self.input_tb[2].get("1.0",'end-1c')
            vel = self.input_tb[3].get("1.0",'end-1c')
            msg = f"P{pos},V{vel}"

        elif self.current_mode == "PV":
            vel = self.input_tb[3].get("1.0",'end-1c')
            msg = f"V{vel}"
        return msg

    def send_cmd(self, msg):
        server_addr = self.input_tb[0].get("1.0",'end-1c')
        server_port = int(self.input_tb[1].get("1.0",'end-1c'))
        self.server_addr = (server_addr, server_port)

        print(f'[MSG] {msg}')
        self.sock.sendto(msg.encode(), self.server_addr)
 
    def run_send(self):
        self.send_cmd(self.make_msg_from_box())
        
    def homing(self):
        if self.current_mode == "PP":
            self.send_cmd(HOMING_MSG_PP)
        elif self.current_mode == "PV":
            self.send_cmd(HOMING_MSG_PV)

    def run(self):
        self.build_ui()
        tk.mainloop()


edgeServo = EdgeServoController()
edgeServo.run()