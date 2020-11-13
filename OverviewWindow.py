import tkinter as tk


class OverviewWindow:
    def __init__(self, root):
        self.width = 600
        self.height = 120
        self.root = root
        self.font = ("helvetica", 10)
        self.status = "testing"
        self.engine_rpm = 500
        self.engine_rpm_max = 5500
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

        # Limiter Status Label
        self.limiter_status_title = tk.Label(self.deck_frame, font=self.font, fg="#000000", text="LIMITER")
        self.limiter_status_title.place(x=150, y=65, width=60, height=14)
        self.limiter_status_label_var = tk.StringVar()
        self.limiter_status_label_var.set(self.status)
        self.limiter_status_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                             textvariable=self.limiter_status_label_var)
        self.limiter_status_label.place(x=150, y=80, width=60, height=14)

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
