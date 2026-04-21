import time
import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import tkinter as tk
from tkinter import ttk
import serial.tools.list_ports
import signal
import sys
from matplotlib.widgets import Button

numPoints = 200  # Number of data points to display

show_raw = True
show_disp = True
show_filtered = True
show_arm = True


def get_com_ports():
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]


def find_renderer(fig):
    if hasattr(fig.canvas, "get_renderer"):
        renderer = fig.canvas.get_renderer()
    else:
        import io
        fig.canvas.print_pdf(io.BytesIO())
        renderer = fig._cachedRenderer
    return renderer


def start_plot(port):
    if not port:
        print("Please select a COM port.")
        return

    try:
        ser = serial.Serial(port, 500000)
        time.sleep(2)
        ser.reset_input_buffer()

        x_vals = []
        y_vals_raw = []
        y_vals_disp = []
        y_vals_filtered = []
        y_vals_arm = []

        fig, ax = plt.subplots(figsize=(13, 6))

        line_raw, = ax.plot([], [], 'k-', label='Raw Distance')
        line_disp, = ax.plot([], [], 'r-', label='Displacement')
        line_filtered, = ax.plot([], [], 'g-', label='Filtered Displacement')
        line_arm, = ax.plot([], [], 'b-', label='Arm Height')

        legend = ax.legend(loc='lower left')

        fig.canvas.draw()
        renderer = find_renderer(fig)
        bbox = legend.get_window_extent(renderer)
        bbox_in_fig_coords = bbox.transformed(ax.transAxes.inverted())
        button_x_coord = bbox_in_fig_coords.x0

        def toggle_show_raw(event):
            global show_raw
            show_raw = not show_raw
            line_raw.set_visible(show_raw)
            fig.canvas.draw_idle()

        def toggle_show_disp(event):
            global show_disp
            show_disp = not show_disp
            line_disp.set_visible(show_disp)
            fig.canvas.draw_idle()

        def toggle_show_filtered(event):
            global show_filtered
            show_filtered = not show_filtered
            line_filtered.set_visible(show_filtered)
            fig.canvas.draw_idle()

        def toggle_show_arm(event):
            global show_arm
            show_arm = not show_arm
            line_arm.set_visible(show_arm)
            fig.canvas.draw_idle()

        def signal_handler(sig, frame):
            print('Closing plot...')
            plt.close(fig)
            ser.close()
            sys.exit(0)

        signal.signal(signal.SIGINT, signal_handler)

        def animate(i):
            nonlocal x_vals, y_vals_raw, y_vals_disp, y_vals_filtered, y_vals_arm

            arduino_data_string = ""

            try:
                while ser.in_waiting:
                    arduino_data_string = ser.readline().decode('utf-8', errors='ignore').strip()

                if arduino_data_string:
                    try:
                        current_time, raw_dist, disp_dist, filtered_dist, arm_height = map(float, arduino_data_string.split())

                        x_vals.append(current_time)
                        y_vals_raw.append(raw_dist)
                        y_vals_disp.append(disp_dist)
                        y_vals_filtered.append(filtered_dist)
                        y_vals_arm.append(arm_height)

                        x_vals = x_vals[-numPoints:]
                        y_vals_raw = y_vals_raw[-numPoints:]
                        y_vals_disp = y_vals_disp[-numPoints:]
                        y_vals_filtered = y_vals_filtered[-numPoints:]
                        y_vals_arm = y_vals_arm[-numPoints:]

                        line_raw.set_data(x_vals, y_vals_raw)
                        line_disp.set_data(x_vals, y_vals_disp)
                        line_filtered.set_data(x_vals, y_vals_filtered)
                        line_arm.set_data(x_vals, y_vals_arm)

                        ax.relim()
                        ax.autoscale_view()

                    except ValueError:
                        print(f"Error processing data: {arduino_data_string}")

            except serial.SerialException:
                print("Serial connection lost. Attempting to reconnect...")
                ser.close()
                time.sleep(2)
                ser.open()
                print("Reconnected to serial port.")

                x_vals = []
                y_vals_raw = []
                y_vals_disp = []
                y_vals_filtered = []
                y_vals_arm = []

            return line_raw, line_disp, line_filtered, line_arm

        ani = animation.FuncAnimation(fig, animate, interval=10, blit=False, save_count=numPoints)

        plt.xlabel('Time (s)')
        plt.ylabel('Value (cm)')
        plt.title('Live Plot of Raw Distance, Displacement, Filtered Displacement, and Arm Height')

        raw_axes = fig.add_axes([button_x_coord, 0.235, 0.075, 0.03])
        b_raw = Button(raw_axes, 'Raw', color="lightgray")
        b_raw.on_clicked(toggle_show_raw)

        disp_axes = fig.add_axes([button_x_coord, 0.200, 0.075, 0.03])
        b_disp = Button(disp_axes, 'Disp', color="red")
        b_disp.on_clicked(toggle_show_disp)

        filt_axes = fig.add_axes([button_x_coord, 0.165, 0.075, 0.03])
        b_filt = Button(filt_axes, 'Filtered', color="lightgreen")
        b_filt.on_clicked(toggle_show_filtered)

        arm_axes = fig.add_axes([button_x_coord, 0.130, 0.075, 0.03])
        b_arm = Button(arm_axes, 'Arm', color="lightsteelblue")
        b_arm.on_clicked(toggle_show_arm)

        plt.switch_backend('TkAgg')
        mng = plt.get_current_fig_manager()
        try:
            mng.window.state('zoomed')
        except Exception:
            pass

        plt.show()
        ser.close()

    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")


root = tk.Tk()
root.title("Select COM Port")

label = tk.Label(root, text="Select the COM port for the Arduino:")
label.pack(pady=10)

com_ports = get_com_ports()
com_port_var = tk.StringVar()
com_port_combobox = ttk.Combobox(root, textvariable=com_port_var, values=com_ports, state="readonly")
com_port_combobox.pack(pady=10)

if com_ports:
    com_port_combobox.current(0)

start_button = tk.Button(root, text="Start Plot", command=lambda: start_plot(com_port_var.get()))
start_button.pack(pady=10)

root.mainloop()
