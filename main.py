# v1.1a
import serial
from time import sleep
from threading import Thread
import tkinter as tk
from OverviewWindow import OverviewWindow
from FuelWindow import FuelWindow
from IgnitionWindow import IgnitionWindow

# "COM7" or similar for windows, "/dev/ttyUSB0" or similar for linux
serial_port = "COM7"
user_input = ""
serial_connected = False

try:
    ser = serial.Serial(baudrate=115200, port=serial_port)
    serial_connected = True
except serial.SerialException:
    print("could not connect to ecu com port")
    ser = None
    serial_connected = False


class MainWindow(tk.Tk):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.title("Liontronic v1.1a")
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
        self.weight = 1179  # this should be in kilograms
        self.tire_radius = 0.351  # this should be in meters

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
        com_to_send = bytes([255]) + com + data
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
                    # 0x00 = flags
                    # 0x01 = fuel_duration
                    # 0x02 = fuel_trim
                    # 0x03 = ignition_advance
                    # 0x04 = ignition_advance_trim
                    # 0x05 = throttle_pos
                    # 0x06 = engine_rpm
                    # 0x07 = engine_rpm_max
                    com = ser.read(size=1)
                    if com != bytes([255]):  # make sure we're synced up in the data stream
                        continue
                    com = ser.read(size=1)
                    # next 2 bytes are the 16bit signed value
                    raw = ser.read(size=2)
                    value = int.from_bytes(raw, byteorder="little", signed=True)
                    value_unsigned = int.from_bytes(raw, byteorder="little")
                    # print("received:", com, raw)
                    if com == bytes([0]):
                        app_window.update_data("flags", raw)
                    elif com == bytes([1]):
                        app_window.update_data("fuel_duration", value)
                    elif com == bytes([2]):
                        app_window.update_data("fuel_trim", value)
                    elif com == bytes([3]):
                        app_window.update_data("ignition_advance", value)
                    elif com == bytes([4]):
                        app_window.update_data("ignition_advance_trim", value)
                    elif com == bytes([5]):
                        app_window.update_data("throttle_pos", value_unsigned)
                    elif com == bytes([6]):
                        app_window.update_data("engine_rpm", value_unsigned)
                    elif com == bytes([7]):
                        app_window.update_data("engine_rpm_max", value_unsigned)
                except UnicodeDecodeError as e:
                    print("error receiving data", e)
            else:
                print("serial receive thread has ended")

    def update_data(self, name, value):
        if name == "flags":
            value1 = value[0]
            value2 = value[1]
            self.overview_window.status = value
            if value2 == 0:
                if value1 == 0:
                    self.overview_window.sync_status_label_var.set("Searching")
                    self.overview_window.sync_losses += 1
                    self.overview_window.sync_losses_label_var.set(str(self.overview_window.sync_losses))
                elif value1 == 1:
                    self.overview_window.sync_status_label_var.set("Locked")
                else:
                    self.overview_window.sync_status_label_var.set("?!!")
            elif value2 == 1:
                if value1 == 0:
                    self.overview_window.limiter_status_label_var.set("Normal")
                elif value1 == 1:
                    self.overview_window.limiter_status_label_var.set("Active")
                else:
                    self.overview_window.limiter_status_label_var.set("?!!")
        elif name == "fuel_duration":
            self.fuel_window.fuel_duration = value
            self.fuel_window.fuel_duration_label_var.set(str(value))
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
        elif name == "engine_rpm":
            # Acceleration = Velocity_Delta / Time : meters per sec / sec
            # Force = Mass x Acceleration : Kilograms * Acceleration
            # Torque = Force x Lever Radius : Force * Meters
            # Horsepower = (Torque * Engine RPM) / 5252
            self.overview_window.velocity_change = self.overview_window.engine_rpm - value
            torque_n_m = (((self.overview_window.velocity_change * self.update_delay) * self.weight) * self.tire_radius)
            torque_ft_lbs = int(torque_n_m * 0.7375621493)
            horsepower = int((torque_ft_lbs * value) / 5252)
            # print("RPM:", self.overview_window.engine_rpm, "HP:", horsepower, "T:", torque_ft_lbs)
            self.overview_window.engine_rpm = value
            self.overview_window.engine_rpm_label_var.set(str(value))
        elif name == "engine_rpm_max":
            self.overview_window.engine_rpm_max = value
            self.overview_window.engine_rpm_max_label_var.set(str(value))

    def win_update_func(self):
        print("window update thread started")
        # get initial parameters that arnt updated constantly
        # fuel trim, ignition trim, rpm limit
        # TODO: get the fuel and spark maps
        sleep(2)  # wait for arduino to reboot
        self.send_data(com=bytes([0]), data=bytes([0, 0]))
        self.send_data(com=bytes([0]), data=bytes([1, 0]))
        self.send_data(com=bytes([0]), data=bytes([0, 2]))
        self.send_data(com=bytes([0]), data=bytes([0, 4]))
        self.send_data(com=bytes([0]), data=bytes([0, 7]))
        sleep(1)
        while ser is not None:
            sleep(self.update_delay)
            self.send_data(com=bytes([0]), data=bytes([1, 0]))
            self.send_data(com=bytes([0]), data=bytes([0, 1]))
            self.send_data(com=bytes([0]), data=bytes([0, 3]))
            self.send_data(com=bytes([0]), data=bytes([0, 5]))
            self.send_data(com=bytes([0]), data=bytes([0, 6]))
            if self.fuel_window.fuel_duration > 0:
                im = self.fuel_window.update_duty_cycle_image()
                self.fuel_window.duty_cycle_image_display.configure(image=im)
                y_pos = int(float(self.fuel_window.duty_cycle_percent_label_var.get()))
                y_pos = (y_pos * -1) + 100
                self.fuel_window.duty_cycle_percent_label.place(anchor="w", y=y_pos)


app_window = MainWindow()

window_update_thread = Thread(name="win_update_thread", target=app_window.win_update_func, daemon=True)
window_update_thread.start()
comm_receive_thread = Thread(name="receive_thread", target=app_window.receive_data, daemon=True)
comm_receive_thread.start()
comm_send_thread = Thread(name="user_input_thread", target=app_window.get_user_input, daemon=True)
comm_send_thread.start()
app_window.mainloop()
