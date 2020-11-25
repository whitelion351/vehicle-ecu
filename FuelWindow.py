# v1.2a
import tkinter as tk
from PIL import Image, ImageTk
import numpy as np
from TableWindow import TableWindow


class FuelWindow:
    def __init__(self, root):
        self.width = 600
        self.height = 120
        self.root = root
        self.font = ("helvetica", 10)
        self.fuel_duration = 0
        self.fuel_trim = 0
        self.fuel_map_rows = 11
        self.fuel_map_cols = 11
        self.fuel_map_raw = [0 for _ in range(self.fuel_map_rows*self.fuel_map_cols)]

        # Background Frame
        self.deck_frame = tk.Frame(root, width=self.width, height=self.height, bd=10, relief="ridge")

        # Fuel Base Time Label
        self.fuel_duration_title = tk.Label(self.deck_frame, font=self.font, text="INJ TIME")
        self.fuel_duration_title.place(x=10, y=5, width=60, height=14)
        self.fuel_duration_label_var = tk.StringVar()
        self.fuel_duration_label_var.set("0000 us")
        self.fuel_duration_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                            textvariable=self.fuel_duration_label_var)
        self.fuel_duration_label.place(x=10, y=20, width=60, height=14)

        # Fuel Trim Label
        self.fuel_trim_title = tk.Label(self.deck_frame, font=self.font, text="TRIM")
        self.fuel_trim_title.place(x=80, y=5, width=60, height=14)
        self.fuel_trim_label_var = tk.StringVar()
        self.fuel_trim_label_var.set("+0000 us")
        self.fuel_trim_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                        textvariable=self.fuel_trim_label_var)
        self.fuel_trim_label.place(x=80, y=20, width=60, height=14)

        # Trim Down Button
        self.trim_down_button = tk.Button(self.deck_frame, font=self.font, text="-100",
                                          command=lambda: self.root.send_data(com=bytes([1]), data=bytes([0, 0])))
        self.trim_down_button.place(x=150, y=20, width=40, height=14)

        # Trim Up Button
        self.trim_up_button = tk.Button(self.deck_frame, font=self.font, text="+100",
                                        command=lambda: self.root.send_data(com=bytes([1]), data=bytes([0, 1])))
        self.trim_up_button.place(x=200, y=20, width=40, height=14)

        # Fuel Map Button
        self.fuel_map_button = tk.Button(self.deck_frame, font=self.font, text="MAP TABLE",
                                         command=self.display_table_window)
        self.fuel_map_button.place(x=250, y=20, width=80, height=14)

        # Duty Cycle Meter
        self.duty_cycle_title = tk.Label(self.deck_frame, font=self.font, text="DUTY")
        self.duty_cycle_title.place(x=513, y=5, width=50, height=14)
        self.duty_cycle_image = self.create_duty_cycle_image()
        self.duty_cycle_image_display = tk.Label(self.deck_frame, image=self.duty_cycle_image)
        self.duty_cycle_image_display.place(x=560, y=0, width=10, height=100)
        self.duty_cycle_percent_label_var = tk.StringVar()
        self.duty_cycle_percent_label_var.set("0")
        self.duty_cycle_percent_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                                 textvariable=self.duty_cycle_percent_label_var)
        self.duty_cycle_percent_label.place(x=519, y=50, width=40, height=14)

    def display_table_window(self):
        TableWindow(self.root, "FUEL TABLE")

    @staticmethod
    def create_duty_cycle_image():
        v_size = 50
        fuel_image = np.zeros((v_size, 1, 3), dtype=np.int8)
        fuel_image[-1, 0, 1] = 255
        fuel_image = Image.fromarray(fuel_image, "RGB").resize((10, 100))
        return ImageTk.PhotoImage(fuel_image)

    def update_duty_cycle_image(self):
        v_size = 50
        new_image = np.zeros((v_size, 1, 3), dtype=np.int8)
        if self.root.overview_window.engine_rpm != 0:
            total_time = (1 / (self.root.overview_window.engine_rpm / 60 / 2)) * 1000000
            dc_fraction = (self.fuel_duration / total_time)
            dc_percent = round(dc_fraction * 100, ndigits=1)
        else:
            total_time = 100
            dc_fraction = (1 / total_time)
            dc_percent = round(dc_fraction * 100, ndigits=1)
        self.duty_cycle_percent_label_var.set(str(dc_percent))
        fuel_dur_index = int(dc_fraction * -v_size)
        fuel_dur_index = abs(fuel_dur_index + v_size)
        fuel_dur_index -= 1
        fuel_dur_index = min(fuel_dur_index, v_size - 1)
        new_image[fuel_dur_index:, 0, 1] = 255
        final_image = Image.fromarray(new_image, "RGB").resize((10, 100))
        return ImageTk.PhotoImage(final_image)
