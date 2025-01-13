import sys
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QWidget,
    QVBoxLayout, QHBoxLayout, QFormLayout, QGroupBox,
    QLabel, QLineEdit, QPushButton, QComboBox, QCheckBox
)
from PySide6.QtCore import Qt, QTimer

# Import our new serial communication module
from SerialCommunication import SerialComm

class MotorControlWidget(QWidget):
    """
    A widget for controlling one motor (AZI, POL, or ELE).

    Each motor has a direction toggle button that displays custom text:
      - AZI:  0 => STATUS: CCW, 1 => STATUS: CW
      - POL:  0 => STATUS: CW,  1 => STATUS: CCW
      - ELE:  0 => STATUS: UP,  1 => STATUS: DOWN
    """
    def __init__(self, motor_label, motor_id, send_command_callback, parent=None):
        super().__init__(parent)
        self.motor_label = motor_label   # "AZI", "POL", or "ELE"
        self.motor_id = motor_id         # 1, 2, or 3
        self.send_command_callback = send_command_callback
        self.steps_per_rev = None

        # Choose initial text based on motor_label => direction=0
        if self.motor_label == "AZI":
            # direction=0 => "STATUS: CCW"
            self.direction_btn = QPushButton("STATUS: CCW")
        elif self.motor_label == "POL":
            # direction=0 => "STATUS: CW"
            self.direction_btn = QPushButton("STATUS: CW")
        else:  # "ELE"
            # direction=0 => "STATUS: UP"
            self.direction_btn = QPushButton("STATUS: UP")

        self.direction_btn.clicked.connect(self.toggle_direction)

        # RPM input
        self.rpm_le = QLineEdit("100")

        # Steps input + angle label
        if self.motor_label == "POL":
            self.steps_le = QLineEdit("2000")
        else:
            self.steps_le = QLineEdit("200")

        self.angle_label = QLabel("N/A°")
        self.steps_le.textEdited.connect(self.update_angle_label)

        # Normal mode buttons
        self.set_rpm_btn = QPushButton("Set RPM")
        self.set_rpm_btn.clicked.connect(self.set_rpm)

        self.move_steps_btn = QPushButton("Move Steps")
        self.move_steps_btn.clicked.connect(self.move_steps)

        # Sweep mode
        self.sweep_checkbox = QCheckBox("Enable Sweep")
        self.sweep_steps_le = QLineEdit("100")
        self.sweep_delay_le = QLineEdit("1000")
        self.sweep_checkbox.stateChanged.connect(self.toggle_sweep_mode)
        self.sweep_timer = QTimer()
        self.sweep_timer.timeout.connect(self.perform_sweep)

        # Stop button
        self.stop_btn = QPushButton(f"Stop {motor_label}")
        self.stop_btn.clicked.connect(self.stop_motor)

        # Layout
        form_layout = QFormLayout()
        form_layout.addRow("Direction Toggle:", self.direction_btn)
        form_layout.addRow("RPM:", self.rpm_le)

        # Steps row
        steps_layout = QHBoxLayout()
        steps_layout.addWidget(self.steps_le)
        steps_layout.addWidget(self.angle_label)
        steps_widget = QWidget()
        steps_widget.setLayout(steps_layout)
        form_layout.addRow("Steps:", steps_widget)

        # Normal mode buttons
        normal_btn_layout = QHBoxLayout()
        normal_btn_layout.addWidget(self.set_rpm_btn)
        normal_btn_layout.addWidget(self.move_steps_btn)

        # Sweep layout
        sweep_layout = QFormLayout()
        sweep_layout.addRow("Sweep Steps:", self.sweep_steps_le)
        sweep_layout.addRow("Sweep Delay (ms):", self.sweep_delay_le)

        group = QGroupBox(f"{motor_label} Motor")
        v_layout = QVBoxLayout()
        v_layout.addLayout(form_layout)
        v_layout.addLayout(normal_btn_layout)
        v_layout.addWidget(self.sweep_checkbox)
        v_layout.addLayout(sweep_layout)
        v_layout.addWidget(self.stop_btn)
        group.setLayout(v_layout)

        main_layout = QVBoxLayout()
        main_layout.addWidget(group)
        self.setLayout(main_layout)

    def toggle_direction(self):
        current_text = self.direction_btn.text().strip()

        if self.motor_label == "AZI":
            if current_text == "STATUS: CCW":
                new_text = "STATUS: CW"
                new_val = "1"
            else:
                new_text = "STATUS: CCW"
                new_val = "0"

        elif self.motor_label == "POL":
            if current_text == "STATUS: CW":
                new_text = "STATUS: CCW"
                new_val = "1"
            else:
                new_text = "STATUS: CW"
                new_val = "0"

        else:  # ELE
            if current_text == "STATUS: UP":
                new_text = "STATUS: DOWN"
                new_val = "1"
            else:
                new_text = "STATUS: UP"
                new_val = "0"

        self.direction_btn.setText(new_text)
        cmd = f"D{self.motor_id} {new_val}\n"
        self.send_command_callback(cmd)

    def set_rpm(self):
        rpm = self.rpm_le.text().strip()
        if rpm.isdigit() and int(rpm) > 0:
            cmd = f"F{self.motor_id} {rpm}\n"
            self.send_command_callback(cmd)
        else:
            print("Invalid RPM value.")

    def move_steps(self):
        steps_text = self.steps_le.text().strip()
        if steps_text.isdigit() and int(steps_text) > 0:
            cmd = f"S{self.motor_id} {steps_text}\n"
            self.send_command_callback(cmd)
        else:
            print("Invalid steps value.")

    def stop_motor(self):
        cmd = f"X{self.motor_id}\n"
        self.send_command_callback(cmd)

    def update_angle_label(self):
        if self.steps_per_rev and self.steps_le.text().isdigit():
            steps = int(self.steps_le.text())
            angle = (steps * 360.0) / self.steps_per_rev
            self.angle_label.setText(f"{angle:.2f}°")
        else:
            self.angle_label.setText("N/A°")

    def set_steps_per_rev(self, steps_per_rev):
        self.steps_per_rev = steps_per_rev
        self.update_angle_label()

    def toggle_sweep_mode(self, state):
        enabled = (state == Qt.Checked)
        self.rpm_le.setEnabled(not enabled)
        self.steps_le.setEnabled(not enabled)
        self.set_rpm_btn.setEnabled(not enabled)
        self.move_steps_btn.setEnabled(not enabled)
        self.direction_btn.setEnabled(not enabled)

        self.sweep_steps_le.setEnabled(enabled)
        self.sweep_delay_le.setEnabled(enabled)

        cmd = f"W {self.motor_id} {1 if enabled else 0}\n"
        self.send_command_callback(cmd)

        if enabled:
            delay_str = self.sweep_delay_le.text().strip()
            if delay_str.isdigit() and int(delay_str) > 0:
                self.sweep_timer.start(int(delay_str))
            else:
                print("Invalid sweep delay.")
        else:
            self.sweep_timer.stop()

    def perform_sweep(self):
        steps_str = self.sweep_steps_le.text().strip()
        if steps_str.isdigit() and int(steps_str) > 0:
            cmd = f"S{self.motor_id} {steps_str}\n"
            self.send_command_callback(cmd)
        else:
            print("Invalid sweep steps. Stopping sweep.")
            self.sweep_timer.stop()

class MainWindow(QMainWindow):
    """
    Main window controlling:
      - SerialComm usage (connect/disconnect, read data)
      - Steps/Rev
      - "Disable after motion" => Z <0/1>
      - Stop All => X0
      - Three motors (AZI, POL, ELE)
    """
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Motor Control: AZI=CCW↔CW, POL=CW↔CCW, ELE=UP↔DOWN")

        # Create the serial communication object
        from SerialCommunication import SerialComm
        self.serialComm = SerialComm()

        self.steps_per_rev = None

        # Serial port selection
        self.port_cb = QComboBox()
        self.refresh_ports()
        self.connect_btn = QPushButton("Connect")
        self.connect_btn.clicked.connect(self.toggle_connection)

        # Steps/Rev display
        self.steps_rev_label = QLabel("Steps/Rev: N/A | Angular Res: N/A°")

        # Steps/Rev setting
        self.set_spr_le = QLineEdit("200")
        self.set_spr_btn = QPushButton("Set Steps/Rev")
        self.set_spr_btn.clicked.connect(self.set_steps_per_rev_command)

        # "Disable after motion"
        self.disable_after_motion_cb = QCheckBox("Disable after motion")
        self.disable_after_motion_cb.stateChanged.connect(self.toggle_disable_after_motion)

        # Stop all
        self.stop_all_btn = QPushButton("Stop All")
        self.stop_all_btn.clicked.connect(self.stop_all_motors)

        # Status label
        self.status_label = QLabel("Status: Ready")

        # Motor widgets
        self.motorAZI = MotorControlWidget("AZI", 1, self.send_command)
        self.motorPOL = MotorControlWidget("POL", 2, self.send_command)
        self.motorELE = MotorControlWidget("ELE", 3, self.send_command)

        # Layout for serial port
        serial_layout = QHBoxLayout()
        serial_layout.addWidget(QLabel("Serial Port:"))
        serial_layout.addWidget(self.port_cb)
        serial_layout.addWidget(self.connect_btn)

        # Steps/Rev + disable + stopAll
        set_spr_layout = QHBoxLayout()
        set_spr_layout.addWidget(QLabel("New Steps/Rev:"))
        set_spr_layout.addWidget(self.set_spr_le)
        set_spr_layout.addWidget(self.set_spr_btn)
        set_spr_layout.addWidget(self.disable_after_motion_cb)
        set_spr_layout.addWidget(self.stop_all_btn)

        # Motors row
        motors_layout = QHBoxLayout()
        motors_layout.addWidget(self.motorAZI)
        motors_layout.addWidget(self.motorPOL)
        motors_layout.addWidget(self.motorELE)

        main_widget = QWidget()
        main_layout = QVBoxLayout()
        main_layout.addLayout(serial_layout)
        main_layout.addWidget(self.steps_rev_label)
        main_layout.addLayout(set_spr_layout)
        main_layout.addWidget(self.status_label)
        main_layout.addLayout(motors_layout)
        main_widget.setLayout(main_layout)

        self.setCentralWidget(main_widget)

        # Timer for reading serial
        self.serial_read_timer = QTimer()
        self.serial_read_timer.timeout.connect(self.read_serial_data)

    def refresh_ports(self):
        self.port_cb.clear()
        ports = self.serialComm.refresh_ports()
        for port_name in ports:
            self.port_cb.addItem(port_name)

    def toggle_connection(self):
        if self.serialComm.is_connected():
            self.serialComm.disconnect()
            self.connect_btn.setText("Connect")
            self.update_status("Disconnected")
            self.serial_read_timer.stop()
        else:
            port_name = self.port_cb.currentText()
            if port_name:
                try:
                    self.serialComm.connect(port_name)
                    self.connect_btn.setText("Disconnect")
                    self.update_status(f"Connected to {port_name}, requesting steps/rev...")
                    self.serial_read_timer.start(100)
                    # Maybe do some initial commands
                    self.request_steps_per_rev()
                    self.disable_after_motion_cb.setChecked(True)
                except Exception as e:
                    self.update_status(f"Failed to open {port_name}: {e}")

    def send_command(self, cmd):
        try:
            self.serialComm.send_command(cmd)
            self.update_status(f"Sent: {cmd.strip()}")
        except Exception as e:
            self.update_status(f"Error sending command: {e}")

    def request_steps_per_rev(self):
        self.send_command("G\n")

    def set_steps_per_rev_command(self):
        val_str = self.set_spr_le.text().strip()
        if val_str.isdigit() and int(val_str) > 0:
            self.send_command(f"R {val_str}\n")
            QTimer.singleShot(1, self.request_steps_per_rev)
        else:
            self.update_status("Invalid steps per revolution value.")

    def toggle_disable_after_motion(self, state):
        enabled = (state == Qt.CheckState.Checked.value)
        cmd = f"Z {1 if enabled else 0}\n"
        self.send_command(cmd)

    def stop_all_motors(self):
        self.send_command("X0\n")

    def read_serial_data(self):
        # Poll for new lines from the serial
        line = self.serialComm.read_data()
        if line:
            self.update_status(f"Received: {line}")
            if line.isdigit():
                steps_per_rev = int(line)
                self.steps_per_rev = steps_per_rev
                angle_res = 360.0 / steps_per_rev
                self.steps_rev_label.setText(
                    f"Steps/Rev: {steps_per_rev} | Angular Res: {angle_res:.2f}°"
                )
                # Update each motor widget
                self.motorAZI.set_steps_per_rev(steps_per_rev)
                self.motorPOL.set_steps_per_rev(steps_per_rev)
                self.motorELE.set_steps_per_rev(steps_per_rev)

    def update_status(self, msg):
        self.status_label.setText(f"Status: {msg}")
        print(msg)

def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
