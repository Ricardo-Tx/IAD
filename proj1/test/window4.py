import sys
import serial
import serial.tools.list_ports
import time
from enum import Enum
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QCheckBox, QLineEdit, QMessageBox, QComboBox, QLabel, QSpacerItem, QSizePolicy
from PyQt5.QtCore import QTimer
from PyQt5.QtGui import QColor
import pyqtgraph as pg


class AcquisitionState(Enum):
    '''state of the data acquisition'''
    STOPPED = 1         # data is not being recorded. stop button should be disabled
                        #  -> RUNNING if the user starts aquisition through the start button
                        #  -> CLEARED if the user clears data through the clear button

    RUNNING = 2         # data is being recorded. start button should be disabled
                        #  -> STOPPED if user stops acquisition through the stop button
                        #  -> HALTED if the arduino is disconnected 

    HALTED = 3          # data stopped being recorded without user instruction.
                        # start button should be disabled
                        #  -> CLEARED if the user clears data through the clear button
                        #  -> STOPPED if user stops acquisition through the stop button
                        #  -> RUNNING if data can be read again

    CLEARED = 4         # no data is present
                        #  -> RUNNING if the user starts aquisition through the start button


class SerialState(Enum):
    '''state of the serial communication. the value of each state is the associated color'''
    NONE = 'lightgray'          # initial state of the program
                                #  -> DISCONNECTED if any port is found, and set current port to it.

    DISCONNECTED = '#f59b2c'    # current serial port is not in the list or otherwise needs to be reconnected
                                #  -> OK if connection to arduino was successful, and enable start button.
                                #     if serial state is HALTED, set it to RUNNING
                                #  -> ERROR if connection to arduino was not successful

    OK = '#20a845'              # connection successful
                                #  -> DISCONNECTED if current serial port is not in the list. disable start button
                                #  -> ! ERROR (possible but currently not accounted for)

    ERROR = '#e60e0e'           # connection not successful
                                #  -> DISCONNECTED if current serial port is not in the list. disable start button
                                #  -> ! OK (possible but currently not accounted for)



class Channel():
    '''Interface for a single analog channel data and its plot'''
    def __init__(self, graph, color):
        self.color = color
        self.graph = graph
        self.x_datas = []
        self.y_datas = []
        self.lines = []
    
    def clear(self):
        for i in range(len(self.x_datas)):
            self.x_datas[i].clear()
            self.y_datas[i].clear()
            self.lines[i].clear()
        self.x_datas.clear()
        self.y_datas.clear()
        self.lines.clear()

    def new_line(self):
        self.x_datas.append([])
        self.y_datas.append([])
        self.lines.append(self.graph.plot(pen=self.color))

    def __iadd__(self, obj):
        self.x_datas[-1].append(obj[0])
        self.y_datas[-1].append(obj[1])
        self.lines[-1].setData(self.x_datas[-1], self.y_datas[-1])
        return self



class AcquisitionApp(QWidget):
    '''main app'''

    # how much time in seconds of data to display at any moment
    time_range = 50

    # which analog channel checkboxes should be checked on startup
    checkboxes_default = [True, True, True, True, True, True]

    # whether the program should expect a start message from the arduino
    start_msg = True

    # QMessageBox icons associated to each possible arduino status message 
    statuses = {
        "ERROR": QMessageBox.Critical,
        "WARN": QMessageBox.Warning,
        "INFO": QMessageBox.Information,
    }

    def __init__(self, app):
        super().__init__()
        self.app = app
        self.state = AcquisitionState.CLEARED
        self.serial_state = SerialState.NONE

        # main vertical layout
        self.layout = QVBoxLayout()

        # horizontal layout with serial information
        self.serial_widget = QWidget()
        self.serial_widget.setObjectName("serial_widget")
        self.serial_layout = QHBoxLayout()
        self.serial_layout.addWidget(QLabel("Serial Port:"))
        self.ports_combobox = QComboBox()
        self.ports_combobox.activated.connect(self.on_port_select)
        self.serial_layout.addWidget(self.ports_combobox)
        self.serial_layout.addStretch(1)
        self.status_label = QLabel()
        self.serial_layout.addWidget(self.status_label)
        self.serial_widget.setLayout(self.serial_layout)
        self.layout.addWidget(self.serial_widget)

        # spacer between horizontal layout and the rest of the window
        self.layout.addItem(QSpacerItem(100,50,QSizePolicy.Expanding,QSizePolicy.Minimum))

        # horizontal layout for the analog channel checkboxes
        self.button_layout = QHBoxLayout()
        self.checkboxes = []
        colors = []
        for i in range(6):
            colors.append(pg.intColor(i, 6))
            h, _, v, _ = colors[-1].getHsv()
            checkbox = QCheckBox(f"A{i}")
            checkbox.setStyleSheet(f"QCheckBox {{ background-color : {QColor.fromHsv(h, 128, v).name()}; }}")
            checkbox.setChecked(AcquisitionApp.checkboxes_default[i])
            self.button_layout.addWidget(checkbox)
            self.checkboxes.append(checkbox)
        self.layout.addLayout(self.button_layout)

        # vertically stacked wide Start/Stop/Clear buttons
        for command in ["start", "stop", "clear"]:
            btn = QPushButton(command.title())
            btn.clicked.connect(getattr(self,"on_"+command+"_acquisition"))
            self.layout.addWidget(btn)
            setattr(self, command+"_button", btn)

        # main graph, with zooming/panning disabled
        self.graph = pg.PlotWidget()
        self.graph.getPlotItem().hideButtons()
        self.graph.getPlotItem().getViewBox().setMouseEnabled(x=False, y=False)
        self.graph.setLabel('left', 'Voltage (V)')
        self.graph.setLabel('bottom', 'Elapsed time (s)')
        self.graph.setYRange(0, 5.2, padding=0)
        self.graph.setXRange(0, AcquisitionApp.time_range, padding=0)
        self.layout.addWidget(self.graph)

        # text field at the bottom to send custom commands
        self.line_edit = QLineEdit()
        self.line_edit.setPlaceholderText("Run command")
        self.line_edit.returnPressed.connect(self.message)
        self.layout.addWidget(self.line_edit)

        self.setLayout(self.layout)


        # create analog channel objects
        self.channels = []
        for col in colors:
            self.channels.append(Channel(self.graph, col))

        # acquisition state
        self.set_acquisition_state(AcquisitionState.CLEARED)

        # serial state and initialization
        self.serial = serial.Serial(None, 38400, timeout=1)
        self.set_serial_state(SerialState.NONE)
        self.ports_list = []
        self.check_connection()

        # setup periodic function callbacks
        def setTimeout(func, interval, start=False):
            timer = QTimer(self)
            timer.timeout.connect(func)
            timer.setInterval(interval)
            if start:
                timer.start()
            setattr(self, func.__name__+"_timer", timer)
        setTimeout(self.acquire_data, 100)
        setTimeout(self.check_connection, 500, start=True)
        setTimeout(self.get_true_voltage, 10000, start=True)


    def set_acquisition_state(self, s: AcquisitionState):
        '''sets the acquisition state and updates the window title'''
        # UI changes
        self.setWindowTitle(f"Acquisition App ({s.name})")
        for check in self.checkboxes:
            check.setEnabled(s != AcquisitionState.RUNNING)
        self.stop_button.setEnabled(s in [AcquisitionState.RUNNING, AcquisitionState.HALTED])
        self.start_button.setEnabled(self.serial_state == SerialState.OK and s != AcquisitionState.RUNNING)
        self.clear_button.setEnabled(s in [AcquisitionState.STOPPED, AcquisitionState.HALTED])

        self.state = s


    def set_serial_state(self, s: SerialState):
        '''sets the serial state and updates the serial widget'''
        # UI changes
        self.serial_widget.setStyleSheet(f"QWidget#serial_widget {{ background-color: {s.value}; }}")
        text = s.name
        if self.serial.port != None:
            text = str(self.serial.port) + " " + text
        self.status_label.setText(text)
        self.line_edit.setEnabled(s == SerialState.OK)
        self.start_button.setEnabled(s == SerialState.OK and self.state != AcquisitionState.RUNNING)
        
        self.serial_state = s


    def on_port_select(self, i: int):
        '''called when the user selects a combobox item'''
        new_port = self.ports_combobox.itemText(i).split(": ")[0]
        if new_port == self.serial.port:
            return

        # refer to the state transitions
        if self.state == AcquisitionState.RUNNING:
            self.on_stop_acquisition()

        # disconnect and change ports
        self.set_serial_state(SerialState.DISCONNECTED)
        self.serial.close()
        self.serial.port = new_port
        self.check_connection(force=True)


    def check_connection(self, force=False):
        '''
        handles the serial port connection (possible disconnections or device changes).
        does nothing if the serial port list stays the same, unless if forced.
        '''
        ports = serial.tools.list_ports.comports()
        if self.ports_list == ports and not force:
            # nothing new
            return
        self.ports_list = ports
        coms = [k[0] for k in ports]

        if self.serial.port == None and coms:
            # if no port on the serial object and ports available, assign it the first one
            self.serial.port = coms[0]
            self.serial_state = SerialState.DISCONNECTED  # don't trigger UI changes

        # whether the current port is present in the list of ports
        found = self.serial.port in coms
        
        if not self.ports_combobox.view().isVisible():
            # if the combobox window is not being interacted with, update its values
            self.ports_combobox.clear()
            self.ports_combobox.addItems([k[0]+": "+k[1] for k in ports])
            self.ports_combobox.setMinimumWidth(self.ports_combobox.view().sizeHintForColumn(0) + 20)
            self.ports_combobox.setCurrentIndex(coms.index(self.serial.port) if found else 0) 

        # refer to the state transition in the declaration of SerialState
        if not found and self.serial_state in [SerialState.OK, SerialState.ERROR]:
            self.on_disconnect()
            self.set_serial_state(SerialState.DISCONNECTED)
        elif found and self.serial_state == SerialState.DISCONNECTED:
            connect_state = self.on_connect()
            self.set_serial_state(connect_state)                
        

    def on_connect(self) -> SerialState:
        '''called on serial device connect'''
        print("CONNECT")
        try:
            self.serial.open()
        except:
            return SerialState.ERROR
        
        # read welcome message
        if AcquisitionApp.start_msg:
            self.message(force=True)
        
        # refer to the state transitions
        if self.state == AcquisitionState.HALTED:
            self.on_start_acquisition()
        return SerialState.OK


    def on_disconnect(self):
        '''called on serial device disconnect'''
        print("DISCONNECT")
        self.serial.close()

        # refer to the state transitions
        if self.state == AcquisitionState.RUNNING:
            self.on_stop_acquisition()
            self.set_acquisition_state(AcquisitionState.HALTED)


    def get_true_voltage(self):
        '''scales the y axis according to the maximum voltage reported by the arduino'''
        if not self.serial.is_open:
            # can't send the command
            return
        
        # send specific command to get the true voltage
        self.serial.write(b"defget(TRUE_VOLTAGE)\n")
        time.sleep(0.1)

        if self.serial.in_waiting > 0:
            # if there's a response, update the y axis to reflect the new maximum voltage
            data = self.serial.readline().decode().rstrip()
            volt = float(data.split(' ')[1])
            self.graph.setYRange(0, volt*1.04, padding=0)


    def message(self, force=False):
        '''
        sends the text of the command text field as a command to the arduino.
        sends nothing if there's no text on the text field, unless if forced.
        '''
        text = self.line_edit.text()
        if not text and not force:
            # nothing to send
            return
        text += "\n"
        self.line_edit.clear()

        # send given command
        self.serial.write(text.encode("ascii"))
        time.sleep(2 if force else 0.5)

        lines = []
        while self.serial.in_waiting > 0:
            # while there's a response, store each line
            data = self.serial.readline().decode().rstrip()
            lines.append(data)

        # open a popup window
        msg = QMessageBox()
        status = next((k for k in AcquisitionApp.statuses if lines[0].startswith(k+": ")), None)
        if len(lines) == 1 and status != None:
            # if it's a status message, set the appropriate status icon
            msg.setWindowTitle(status)
            msg.setIcon(AcquisitionApp.statuses[status])
            msg.setText(lines[0].split(": ")[1])
        else:
            # write the original command in bold and the response below
            msg.setWindowTitle("RESULT")
            msg.setText("<b>"+text+"</b><br><br>"+'<br>'.join([l.replace('\t','&nbsp;'*4) for l in lines]))
        msg.exec_()


    def on_clear_acquisition(self):
        '''clears the acquisition data'''
        # clear everything and update x range
        for chn in self.channels:
            chn.clear()
        self.graph.clear()
        self.graph.setXRange(0, AcquisitionApp.time_range, padding=0)
        self.app.processEvents()
        
        self.set_acquisition_state(AcquisitionState.CLEARED)


    def on_start_acquisition(self):
        '''starts or restarts the data acquisition'''
        if not self.channels[0].lines:
            # if there's no previous data set the start time to now 
            self.start_time = time.time()

        # create new separate lines for each channel
        for chn in self.channels:
            chn.new_line()
            
        # starts the periodic calls to the data acquisition method
        self.acquire_data_timer.start()
        self.set_acquisition_state(AcquisitionState.RUNNING)
        

    def on_stop_acquisition(self):
        '''stops the data acquisition'''
        self.acquire_data_timer.stop()
        self.set_acquisition_state(AcquisitionState.STOPPED)


    def acquire_data(self):
        '''reads data and updates the lines'''
        # prepare the command based on selected checkboxes
        command = ''
        refs = []
        for checkbox in self.checkboxes:
            if checkbox.isChecked():
                refs.append(len(command))
                command += "1"
            else:
                command += "0"
        command = command[::-1]

        try:
            # send the command
            self.serial.write(f'analog(0b{command})\n'.encode())
            time.sleep(0.1)

            if self.serial.in_waiting > 0:
                # if there's a response, parse it and update the graph
                data = self.serial.readline().decode().strip()
                values = [float(v) for v in data.split(',')[:-1]]
                values.reverse()

                # update only the channels sent in the command
                for i, j in enumerate(refs):
                    t = time.time() - self.start_time
                    self.channels[j] += (t, values[i])
                    if t > AcquisitionApp.time_range:
                        # if time exceeds the time range set a new x range
                        self.graph.setXRange(t-AcquisitionApp.time_range, t, padding=0)

                self.app.processEvents()
        except:
            # possible state transition to SerialState.ERROR ???
            pass



if __name__ == "__main__":
    # main loop
    app = QApplication(sys.argv)
    window = AcquisitionApp(app)
    window.show()
    sys.exit(app.exec_())
