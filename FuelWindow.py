import tkinter as tk
from PIL import Image, ImageTk
import numpy as np


class FuelWindow:
    def __init__(self, root):
        self.width = 600
        self.height = 120
        self.root = root
        self.font = ("helvetica", 10)
        self.fuel_base = 0
        self.fuel_trim = 0
        # Background Frame
        self.deck_frame = tk.Frame(root, width=self.width, height=self.height, bd=10, relief="ridge")

        # Fuel Base Time Label
        self.fuel_base_title = tk.Label(self.deck_frame, font=self.font, text="BASE")
        self.fuel_base_title.place(x=10, y=5, width=60, height=14)
        self.fuel_base_label_var = tk.StringVar()
        self.fuel_base_label_var.set("0000 us")
        self.fuel_base_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                        textvariable=self.fuel_base_label_var)
        self.fuel_base_label.place(x=10, y=20, width=60, height=14)

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

        # Duty Cycle Meter
        self.duty_cycle_image = self.create_duty_cycle_image()
        self.duty_cycle_image_display = tk.Label(self.deck_frame, image=self.duty_cycle_image)
        self.duty_cycle_image_display.place(x=560, y=5, width=10, height=90)

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
        fuel_base_index = int((self.fuel_base / 100) * v_size) * -1
        fuel_base_index = abs(fuel_base_index + v_size)
        fuel_base_index = min(fuel_base_index, v_size - 1)
        new_image[fuel_base_index:, 0, 1] = 255
        vol_image = Image.fromarray(new_image, "RGB").resize((10, 70))
        return ImageTk.PhotoImage(vol_image)
