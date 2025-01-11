import serial
import serial.tools.list_ports

class SerialComm:
    """
    This class handles all the serial communication logic:
      - Listing ports
      - Connecting / Disconnecting
      - Sending commands
      - Reading data
    """

    def __init__(self, baudrate=115200, timeout=0.1):
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial_port = None

    def refresh_ports(self):
        """
        Returns a list of available serial port names.
        """
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def connect(self, port_name):
        """
        Attempts to open the specified serial port at self.baudrate.
        Returns True if successful, or raises an Exception on failure.
        """
        if self.serial_port and self.serial_port.is_open:
            self.disconnect()

        self.serial_port = serial.Serial(port_name, self.baudrate, timeout=self.timeout)
        if self.serial_port.is_open:
            return True
        else:
            raise IOError(f"Could not open port {port_name}")

    def disconnect(self):
        """
        Closes the serial port if open.
        """
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.serial_port = None

    def is_connected(self):
        """
        Returns True if the serial port is open, False otherwise.
        """
        return (self.serial_port is not None) and self.serial_port.is_open

    def send_command(self, cmd):
        """
        Sends a command (string) over the serial port, if open.
        """
        if self.is_connected():
            self.serial_port.write(cmd.encode('ascii'))
        else:
            raise IOError("Serial port not connected.")

    def read_data(self):
        """
        Reads available data from the serial port (non-blocking).
        Returns a stripped line or empty string if nothing is available.
        """
        if self.is_connected():
            line = self.serial_port.readline().decode('ascii', errors='ignore').strip()
            return line
        return ""

    def close(self):
        """
        Alias for disconnect().
        """
        self.disconnect()
