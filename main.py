import serial
from time import sleep
from threading import Thread
import tkinter as tk
from OverviewWindow import OverviewWindow
from FuelWindow import FuelWindow
from IgnitionWindow import IgnitionWindow

user_input = ""
serial_connected = False

try:
    ser = serial.Serial(baudrate=115200, port="COM7")
    serial_connected = True
except serial.SerialException:
    print("could not connect to ecu com port")
    ser = None
    serial_connected = False


class MainWindow(tk.Tk):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.title("Liontronic v0.5a")
        self.update_delay = 0.2
        self.canvas = tk.Canvas(self, width=615, height=600, bg="#555555")
        self.canvas.pack()
        self.resizable(width=False, height=False)
        self.overview_window = OverviewWindow(self)
        self.overview_window.deck_frame.place(x=10, y=10)
        self.fuel_window = FuelWindow(self)
        self.fuel_window.deck_frame.place(x=10, y=140)
        self.ignition_window = IgnitionWindow(self)
        self.ignition_window.deck_frame.place(x=10, y=270)

    @staticmethod
    def get_user_input():
        global user_input
        print("Ready for user input")
        while ser is not None:
            user_input = input()
            if user_input != "" and user_input.isdigit() and len(user_input) == 3:
                try:
                    com = bytes([int(user_input[0])])
                    data = bytes([int(user_input[1]), int(user_input[2])])
                    app_window.send_data(com=com, data=data)
                except ValueError as e:
                    print("input error:", e)
            elif user_input == "?":
                print("listing options:")
                print("")
            else:
                print("invalid input")

    @staticmethod
    def send_data(com=bytes([0]), data=bytes([0, 0])):
        com_to_send = com + data
        try:
            for b in com_to_send:
                if type(b) == int:
                    b = bytes([b])
                ser.write(b)
                # print("sending", b)
        except (ValueError, AttributeError) as e:
            print("input error:", e)
            if type(e) is AttributeError:
                print("Possible no communication to ECU")

    @staticmethod
    def receive_data():
        if ser is not None:
            print("serial receive thread started")
            while ser is not None:
                try:
                    # first byte of data describes which data is being received
                    # 0x00 = engine_rpm
                    # 0x01 = fuel_base
                    # 0x02 = fuel_trim
                    # 0x03 = ignition_advance
                    # 0x04 = ignition_advance_trim
                    # 0x05 = throttle_pos
                    # 0x06 = current test variable
                    com = ser.read(size=1)
                    # next 2 bytes are the 16bit signed value
                    raw = ser.read(size=2)
                    value = int.from_bytes(raw, byteorder="little", signed=True)
                    value_unsigned = int.from_bytes(raw, byteorder="little")
                    if com == bytes([0]):
                        app_window.update_data("engine_rpm", value_unsigned)
                    elif com == bytes([1]):
                        app_window.update_data("fuel_base", value)
                    elif com == bytes([2]):
                        app_window.update_data("fuel_trim", value)
                    elif com == bytes([3]):
                        app_window.update_data("ignition_advance", value)
                    elif com == bytes([4]):
                        app_window.update_data("ignition_advance_trim", value)
                    elif com == bytes([5]):
                        app_window.update_data("throttle_pos", value_unsigned)
                    elif com == bytes([6]):
                        app_window.update_data("status", value_unsigned)
                except UnicodeDecodeError as e:
                    print("error receiving data", e)
            else:
                print("serial receive thread has ended")

    def update_data(self, name, value):
        if name == "engine_rpm":
            self.overview_window.engine_rpm = value
            self.overview_window.engine_rpm_label_var.set(str(value))
        elif name == "fuel_base":
            self.fuel_window.fuel_base = value
            self.fuel_window.fuel_base_label_var.set(str(value))
        elif name == "fuel_trim":
            self.fuel_window.fuel_trim = value
            self.fuel_window.fuel_trim_label_var.set(str(value))
        elif name == "ignition_advance":
            self.ignition_window.ignition_advance_us = value
            self.ignition_window.ignition_advance_us_label_var.set(str(value))
            deg_advance = 0
            if self.overview_window.engine_rpm > 0:
                deg_advance = self.overview_window.calculate_degrees_from_advance_in_us(value)
            self.ignition_window.ignition_advance_deg = deg_advance
            self.ignition_window.ignition_advance_deg_label_var.set(str(deg_advance))
        elif name == "ignition_advance_trim":
            self.ignition_window.ignition_advance_trim_deg = value
            self.ignition_window.ignition_advance_trim_deg_label_var.set(str(value))
            us_advance = 0
            if self.overview_window.engine_rpm > 0:
                us_advance = self.overview_window.calculate_advance_from_deg(value)
            self.ignition_window.ignition_advance_trim_us = us_advance
            self.ignition_window.ignition_advance_trim_us_label_var.set(str(us_advance))
        elif name == "throttle_pos":
            self.overview_window.throttle_pos = value
            self.overview_window.throttle_pos_label_var.set(str(value))
        elif name == "status":
            self.overview_window.status = value
            self.overview_window.status_label_var.set(str(value))

    def win_update_func(self):
        print("window update thread started")
        while ser is not None:
            sleep(self.update_delay)
            self.send_data(com=bytes([0]), data=bytes([0, 0]))
            self.send_data(com=bytes([0]), data=bytes([0, 1]))
            self.send_data(com=bytes([0]), data=bytes([0, 2]))
            self.send_data(com=bytes([0]), data=bytes([0, 3]))
            self.send_data(com=bytes([0]), data=bytes([0, 4]))
            self.send_data(com=bytes([0]), data=bytes([0, 5]))
            self.send_data(com=bytes([0]), data=bytes([0, 6]))


app_window = MainWindow()

window_update_thread = Thread(name="win_update_thread", target=app_window.win_update_func, daemon=True)
window_update_thread.start()
comm_receive_thread = Thread(name="receive_thread", target=app_window.receive_data, daemon=True)
comm_receive_thread.start()
comm_send_thread = Thread(name="user_input_thread", target=app_window.get_user_input, daemon=True)
comm_send_thread.start()
app_window.mainloop()
