import serial
from threading import Thread
from time import sleep
import tkinter as tk
import numpy as np
from PIL import Image, ImageTk

user_input = ""
read_buffer = bytes()
serial_connected = False
# Main Variables - Received from Arduino
fuel_base_time = 1000
fuel_offset_time = 100
engine_rpm = 0

try:
    ser = serial.Serial(baudrate=9600, port="COM7")
    serial_connected = True
except serial.SerialException:
    print("could not connect to ecu com port")
    ser = None
    serial_connected = False


class MainWindow(tk.Tk):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.title("Liontronic v0.1a")
        self.update_delay = 0.1
        self.canvas = tk.Canvas(self, width=800, height=600, bg="#555555")
        self.canvas.pack()
        self.resizable(width=False, height=False)
        self.fuel_window = FuelWindow(self)
#        self.ignition_window = self.IgnitionWindow(self)
        self.fuel_window.deck_frame.place(x=10, y=10)
#        self.ignition_window.deck_frame.place(x=405, y=10)


class FuelWindow:
    def __init__(self, root):
        self.width = 385
        self.height = 120
        self.root = root
        self.font = ("helvetica", 10)
        self.status = "testing"
        # Background Frame
        self.deck_frame = tk.Frame(root, width=self.width, height=self.height, bd=10, relief="ridge")

        # Fuel Base Time Label
        self.fuel_base_time_label_var = tk.StringVar()
        self.fuel_base_time_label_var.set("0000 ms")
        self.fuel_base_time_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                             textvariable=self.fuel_base_time_label_var)
        self.fuel_base_time_label.place(x=5, y=10, width=60, height=14)

        # Fuel Offset Time Label
        self.fuel_offset_time_label_var = tk.StringVar()
        self.fuel_offset_time_label_var.set("+0000 ms")
        self.fuel_offset_time_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                               textvariable=self.fuel_offset_time_label_var)
        self.fuel_offset_time_label.place(anchor="ne", x=200, y=10, width=60, height=14)

        # Fuel Final Time Label
        self.fuel_final_time_label_var = tk.StringVar()
        self.fuel_final_time_label_var.set("0000 ms")
        self.fuel_final_time_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                              textvariable=self.fuel_final_time_label_var)
        self.fuel_final_time_label.place(anchor="ne", x=325, y=10, width=60, height=14)

        # Fuel Adjustment Meter
        self.fuel_image = self.create_fuel_image()
        self.fuel_image_display = tk.Label(self.deck_frame, image=self.fuel_image, anchor="w")
        self.fuel_image_display.place(anchor="center", relx=0.95, rely=0.4, width=10, height=70)

        # Status Label
        self.status_label_var = tk.StringVar()
        self.status_label_var.set(self.status)
        self.status_label = tk.Label(self.deck_frame, font=self.font, bg="#000000", fg="#FFFFFF",
                                     textvariable=self.status_label_var)
        self.status_label.place(anchor="center", relx=0.9, rely=0.9, width=60, height=12)

        # Fuel Up Button
        self.fuel_up_button = tk.Button(self.deck_frame, font=self.font, text="Inc",
                                        command=lambda: self.send_command("fuel up"))
        self.fuel_up_button.place(anchor="center", x=100, y=85, width=50, height=20)

        update_thread = Thread(name="update_view_thread", target=self.update_view, daemon=True)
        update_thread.start()

        # Fuel Down Button
        self.fuel_down_button = tk.Button(self.deck_frame, font=self.font, text="Dec",
                                          command=lambda: self.send_command("fuel down"))
        self.fuel_down_button.place(anchor="center", x=200, y=85, width=50, height=20)

        update_thread = Thread(name="update_view_thread", target=self.update_view, daemon=True)
        update_thread.start()

    @staticmethod
    def create_fuel_image():
        fuel_image = np.zeros((10, 2, 3), dtype=np.int8)
        fuel_image[-1, :, 1] = 255
        fuel_image = Image.fromarray(fuel_image, "RGB").resize((10, 70))
        return ImageTk.PhotoImage(fuel_image)

    @staticmethod
    def update_fuel_image():
        v_size = 25
        new_image = np.zeros((v_size, 2, 3), dtype=np.int8)
        if fuel_base_time is not None:
            fuel_base_index = int((fuel_base_time / 300) * v_size) * -1
            fuel_base_index = abs(fuel_base_index + v_size)
            fuel_base_index = min(fuel_base_index, v_size - 1)
            new_image[fuel_base_index, :, 2] = 255
        else:
            vol_image = Image.fromarray(new_image, "RGB").resize((10, 70))
            return ImageTk.PhotoImage(vol_image)
        if fuel_offset_time >= 0:
            fuel_offset_index = int(((fuel_base_time + fuel_offset_time) / 300) * v_size) * -1
            fuel_offset_index = abs(fuel_offset_index + v_size)
            for v in range(fuel_offset_index, fuel_base_index):
                new_image[v, :, 1] = 255  # // (v + 1)
        else:
            fuel_offset_index = int(((fuel_base_time + fuel_offset_time) / 300) * v_size) * -1
            fuel_offset_index = abs(fuel_offset_index + v_size)
            for v in range(fuel_base_index, fuel_offset_index):
                new_image[v, :, 0] = 255  # // (v + 1)
        vol_image = Image.fromarray(new_image, "RGB").resize((10, 70))
        return ImageTk.PhotoImage(vol_image)

    @staticmethod
    def send_command(com):
        com_to_send = 0x00
        if com == "fuel up":
            com_to_send = bytes([0])
        elif com == "fuel down":
            com_to_send = bytes([1])
        try:
            ser.write(com_to_send)
        except ValueError as e:
            print("input error:", e)

    def update_view(self):
        global fuel_base_time
        global fuel_offset_time
        while True:
            sleep(self.root.update_delay)
            self.fuel_base_time_label_var.set(str(fuel_base_time) + " ms")
            self.fuel_offset_time_label_var.set(str(fuel_offset_time) + " ms")
            self.fuel_final_time_label_var.set(str(fuel_base_time + fuel_offset_time) + " ms")
            self.fuel_image = self.update_fuel_image()
            self.fuel_image_display.configure(image=self.fuel_image)


def get_user_input(ser_dev):
    global user_input
    print("Waiting for input")
    while True:
        user_input = input()
        if user_input != "" and user_input.isdigit():
            try:
                ser_dev.write(bytes([int(user_input, 10)]))
            except ValueError as e:
                print("input error:", e)
        elif user_input == "?":
            print("listing options:")
            print("")
        else:
            print("invalid input")


def receive_data():
    global read_buffer
    global fuel_base_time
    global fuel_offset_time
    global engine_rpm
    if ser is not None:
        while True:
            current_read = ser.read(size=1)
            read_buffer += current_read
            try:
                if read_buffer[-2:].decode("UTF8") == "\r\n":
                    read_buffer = read_buffer[:-2]
                    if len(read_buffer) != 3:
                        print("some received data was discarded. length was", len(read_buffer))
                        read_buffer = bytes()
                        continue
                    else:
                        # first byte of data describes which data is being sent
                        # 0 = fuel base time
                        # 1 = fuel offset time
                        # 2 = engine rpm
                        if read_buffer[0] == 0:
                            temp = int.from_bytes(read_buffer[1:], byteorder="little")
                            # print("fuel base time:", temp)
                            fuel_base_time = temp
                        elif read_buffer[0] == 1:
                            temp = int.from_bytes(read_buffer[1:], byteorder="little", signed=True)
                            # print("fuel offset time:", temp)
                            fuel_offset_time = temp
                        elif read_buffer[0] == 2:
                            temp = int.from_bytes(read_buffer[1:], byteorder="little")
                            print("engine rpm:", temp)
                            engine_rpm = temp
                        read_buffer = bytes()
            except UnicodeDecodeError:
                pass
    else:
        print("comm receive thread has exited")


comm_receive_thread = Thread(name="output_thread", target=receive_data, daemon=True)
comm_receive_thread.start()
comm_send_thread = Thread(name="input_thread", target=get_user_input, args=[ser], daemon=True)
comm_send_thread.start()

app_window = MainWindow()
app_window.mainloop()
