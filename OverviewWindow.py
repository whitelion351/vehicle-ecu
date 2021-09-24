# v1.2a
import tkinter as tk
import numpy as np
from PIL import Image, ImageTk


class OverviewWindow:
    def __init__(self, root):
        self.width = 600
        self.height = 120
        self.root = root
        self.font = ("helvetica", 10)
        self.status = "testing"
        self.engine_rpm = 500
        self.engine_rpm_max = 5500
        self.sync_losses = 0
        self.velocity_change = 0

        self.throttle_pos = 0
        # Background Frame
        self.deck_frame = tk.Frame(root, width=self.width, height=self.height, bd=10, relief="ridge")

        # engine rpm
        self.engine_rpm_title = tk.Label(self.deck_frame, font=self.font, fg="#000000", text="RPM")
        self.engine_rpm_title.place(x=10, y=5, width=60, height=14)
        self.engine_rpm_label_var = tk.StringVar()
        self.engine_rpm_label_var.set("500")
        self.engine_rpm_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                         textvariable=self.engine_rpm_label_var)
        self.engine_rpm_label.place(x=10, y=20, width=60, height=14)

        # rpm limit
        self.engine_rpm_max_title = tk.Label(self.deck_frame, font=self.font, fg="#000000", text="LIMIT")
        self.engine_rpm_max_title.place(x=80, y=5, width=60, height=14)
        self.engine_rpm_max_label_var = tk.StringVar()
        self.engine_rpm_max_label_var.set("the moon")
        self.engine_rpm_max_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                             textvariable=self.engine_rpm_max_label_var)
        self.engine_rpm_max_label.place(x=80, y=20, width=60, height=14)

        # Trim Down Button
        self.trim_down_button = tk.Button(self.deck_frame, font=self.font, text="-500",
                                          command=lambda: self.root.send_data(com=bytes([3]), data=bytes([0, 0])))
        self.trim_down_button.place(x=150, y=20, width=40, height=14)

        # Trim Up Button
        self.trim_up_button = tk.Button(self.deck_frame, font=self.font, text="+500",
                                        command=lambda: self.root.send_data(com=bytes([3]), data=bytes([0, 1])))
        self.trim_up_button.place(x=200, y=20, width=40, height=14)

        # throttle_pos
        self.throttle_pos_title = tk.Label(self.deck_frame, font=self.font, fg="#000000", text="THR%")
        self.throttle_pos_title.place(x=10, y=65, width=60, height=14)
        self.throttle_pos_label_var = tk.StringVar()
        self.throttle_pos_label_var.set("0 %")
        self.throttle_pos_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                           textvariable=self.throttle_pos_label_var)
        self.throttle_pos_label.place(x=10, y=80, width=60, height=14)

        # Sync Status Label
        self.sync_status_title = tk.Label(self.deck_frame, font=self.font, fg="#000000", text="SYNC")
        self.sync_status_title.place(x=80, y=65, width=60, height=14)
        self.sync_status_label_var = tk.StringVar()
        self.sync_status_label_var.set(self.status)
        self.sync_status_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                          textvariable=self.sync_status_label_var)
        self.sync_status_label.place(x=80, y=80, width=60, height=14)

        # Sync Losses Label
        self.sync_losses_title = tk.Label(self.deck_frame, font=self.font, fg="#000000", text="L")
        self.sync_losses_title.place(x=141, y=65, width=30, height=14)
        self.sync_losses_label_var = tk.StringVar()
        self.sync_losses_label_var.set(self.sync_losses)
        self.sync_losses_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                          textvariable=self.sync_losses_label_var)
        self.sync_losses_label.place(x=141, y=80, width=30, height=14)

        # Limiter Status Label
        self.limiter_status_title = tk.Label(self.deck_frame, font=self.font, fg="#000000", text="LIMITER")
        self.limiter_status_title.place(x=180, y=65, width=60, height=14)
        self.limiter_status_label_var = tk.StringVar()
        self.limiter_status_label_var.set(self.status)
        self.limiter_status_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                             textvariable=self.limiter_status_label_var)
        self.limiter_status_label.place(x=180, y=80, width=60, height=14)

        # HP and Torque Graph
        self.hp_graph_title_var = tk.StringVar()
        self.hp_graph_title_var.set("HP(red) TORQUE(blue)")
        self.hp_graph_title = tk.Label(self.deck_frame, font=self.font, fg="#FFFFFF", bg="#000000",
                                       textvariable=self.hp_graph_title_var)
        self.hp_graph_title.place(x=250, y=5, width=320, height=14)
        self.hp_graph_values = []
        self.hp_graph_img = self.create_graph_image()
        self.hp_graph = tk.Label(self.deck_frame, font=self.font, bg="#000000", image=self.hp_graph_img)
        self.hp_graph.place(x=250, y=20, width=320, height=70)

    def update_graph(self, hp, t):
        self.hp_graph_title_var.set(f"{hp} HP(red) {t} TORQUE(blue)")
        self.hp_graph_img = self.update_graph_image(hp, t)
        self.hp_graph.configure(image=self.hp_graph_img)

    def create_graph_image(self):
        v_size = 36
        h_size = 320
        self.hp_graph_values = np.zeros((v_size, h_size, 3), dtype=np.int8)
        self.hp_graph_values[18, :, 1] = 255
        self.hp_graph_values[:, 160, 0] = 255
        graph_image = Image.fromarray(self.hp_graph_values, "RGB").resize((320, 70))
        return ImageTk.PhotoImage(graph_image)

    def update_graph_image(self, hp, t):
        self.hp_graph_values[:, :-1, :] = self.hp_graph_values[:, 1:, :]
        val = (36 / 1000)
        hp_pos = int((val * hp) * -1) + 35
        if hp_pos < 0:
            hp_pos = 0
        if hp_pos > 35:
            hp_pos = 35
        t_pos = int((val * t) * -1) + 35
        if t_pos < 0:
            t_pos = 0
        if t_pos > 35:
            t_pos = 35
        self.hp_graph_values[:, -1, :] = 0
        self.hp_graph_values[hp_pos, -1:, 0] = 255
        self.hp_graph_values[t_pos, -1:, 2] = 255
        graph_image = Image.fromarray(self.hp_graph_values, "RGB").resize((320, 70))
        return ImageTk.PhotoImage(graph_image)

    def calculate_degrees_from_advance_in_us(self, advance_in_us=0):
        single_rev_time = (1 / (self.engine_rpm / 60)) * 1000000
        single_deg_time = single_rev_time / 360
        total_degrees_of_advance = advance_in_us / single_deg_time
        return round(total_degrees_of_advance, ndigits=1)

    def calculate_advance_from_deg(self, deg=0):
        single_rev_time = (1 / (self.engine_rpm / 60)) * 1000000
        single_deg_time = single_rev_time / 360
        total_advance_in_us = single_deg_time * deg
        return int(total_advance_in_us)
