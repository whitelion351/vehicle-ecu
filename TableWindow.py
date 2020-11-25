# v1.2a
import tkinter as tk
from time import sleep
from threading import Thread


class TableWindow:
    def __init__(self, root, title):
        self.width = 600
        self.height = 550
        self.root = root
        self.font = ("helvetica", 10)
        self.font_big = ("helvetica", 20, "bold", "italic")
        self.title = title
        self.is_active = True
        self.cells_x = 11
        self.cells_y = 11
        self.current_x = 10
        self.current_y = 10

        # Background Frame
        self.deck_frame = tk.Frame(root, width=self.width, height=self.height, bd=10, relief="ridge")
        self.deck_frame.place(x=10, y=10)

        # Title Label
        self.title_label = tk.Label(self.deck_frame, font=self.font_big, anchor="w", text=title)
        self.title_label.place(x=20, y=10, width=220, height=30)

        # Table Cell Generation
        self.table_cells = [[Cell(self, x, y) for y in range(self.cells_y)] for x in range(self.cells_x)]

        # Close Button
        self.close_button = tk.Button(self.deck_frame, font=self.font, text="CLOSE", command=self.close_window)
        self.close_button.place(x=525, y=5, width=50, height=20)

        print("table window created")
        update_thd = Thread(name="table_update_thread", target=self.table_window_update_func, daemon=True)
        update_thd.start()

    def close_window(self):
        self.is_active = False
        sleep(self.root.update_delay)
        self.deck_frame.destroy()
        del self
        print("table window closed")

    def table_window_update_func(self):
        print("table_window update thread started")
        while self.is_active:
            sleep(self.root.update_delay)
            this_x, this_y = self.get_current_cell()
            if self.is_active and (this_x != self.current_x or this_y != self.current_y):
                self.change_current_cell(this_x, this_y)
        print("table_window update thread ended")

    def get_current_cell(self):
        this_x = int(self.root.overview_window.throttle_pos / 10)
        this_y = int(self.root.overview_window.engine_rpm / 500)
        # print(this_x, this_y)
        return this_x, this_y

    def change_current_cell(self, this_x, this_y):
        self.table_cells[self.current_x][self.current_y].my_label.configure(bg="#777777")
        self.table_cells[this_x][this_y].my_label.configure(bg="#77FF77")
        self.current_x = this_x
        self.current_y = this_y


class Cell:
    def __init__(self, table, this_x, this_y):
        self.my_x = this_x
        self.my_y = this_y
        self.my_label_var = tk.StringVar()
        self.my_label = tk.Label(table.deck_frame, bg="#777777", textvariable=self.my_label_var)
        if table.title == "FUEL TABLE":
            value = ord(table.root.fuel_window.fuel_map_raw[(self.my_y*11)+self.my_x]) * 100
        elif table.title == "SPARK TABLE":
            value = ord(table.root.ignition_window.ignition_map_raw[(self.my_y*11)+self.my_x])
        else:
            value = 0
        self.my_label_var.set(str(value))
        self.my_label.place(x=this_x * 50 + 20, y=this_y * 30 + 50, width=40, height=20)
