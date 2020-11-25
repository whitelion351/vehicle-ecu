# v1.2a
import tkinter as tk
from TableWindow import TableWindow


class IgnitionWindow:
    def __init__(self, root):
        self.width = 600
        self.height = 120
        self.root = root
        self.font = ("helvetica", 10)
        self.ignition_map_rows = 11
        self.ignition_map_cols = 11
        self.ignition_map_raw = [0 for _ in range(self.ignition_map_rows*self.ignition_map_cols)]

        # Background Frame
        self.deck_frame = tk.Frame(root, width=self.width, height=self.height, bd=10, relief="ridge")

        self.ignition_advance_us = 0
        self.ignition_advance_deg = 0
        self.ignition_advance_trim_us = 0
        self.ignition_advance_trim_deg = 0

        # Ignition Advance Label
        self.ignition_advance_us_title = tk.Label(self.deck_frame, font=self.font, text="ADV (uS)")
        self.ignition_advance_us_title.place(x=10, y=5, width=60, height=14)
        self.ignition_advance_us_label_var = tk.StringVar()
        self.ignition_advance_us_label_var.set("0")
        self.ignition_advance_us_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                                  textvariable=self.ignition_advance_us_label_var)
        self.ignition_advance_us_label.place(x=10, y=20, width=60, height=14)
        self.ignition_advance_deg_label_var = tk.StringVar()
        self.ignition_advance_deg_label_var.set("0")
        self.ignition_advance_deg_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                                   textvariable=self.ignition_advance_deg_label_var)
        self.ignition_advance_deg_label.place(x=10, y=35, width=60, height=14)

        # Ignition Advance Trim Label
        self.ignition_advance_trim_us_title = tk.Label(self.deck_frame, font=self.font, text="TRIM (uS)")
        self.ignition_advance_trim_us_title.place(x=80, y=5, width=60, height=14)
        self.ignition_advance_trim_us_label_var = tk.StringVar()
        self.ignition_advance_trim_us_label_var.set("0")
        self.ignition_advance_trim_us_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                                       textvariable=self.ignition_advance_trim_us_label_var)
        self.ignition_advance_trim_us_label.place(x=80, y=20, width=60, height=14)
        self.ignition_advance_trim_deg_label_var = tk.StringVar()
        self.ignition_advance_trim_deg_label_var.set("0")
        self.ignition_advance_trim_deg_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                                        textvariable=self.ignition_advance_trim_deg_label_var)
        self.ignition_advance_trim_deg_label.place(x=80, y=35, width=60, height=14)

        # Trim Down Button
        self.trim_down_button = tk.Button(self.deck_frame, font=self.font, text="-1",
                                          command=lambda: self.root.send_data(com=bytes([2]), data=bytes([0, 0])))
        self.trim_down_button.place(x=150, y=35, width=40, height=14)

        # Trim Up Button
        self.trim_up_button = tk.Button(self.deck_frame, font=self.font, text="+1",
                                        command=lambda: self.root.send_data(com=bytes([2]), data=bytes([0, 1])))
        self.trim_up_button.place(x=200, y=35, width=40, height=14)

        # Spark Map Button
        self.spark_map_button = tk.Button(self.deck_frame, font=self.font, text="MAP TABLE",
                                          command=self.display_table_window)
        self.spark_map_button.place(x=250, y=35, width=80, height=15)

    def display_table_window(self):
        TableWindow(self.root, "SPARK TABLE")
