import sys
import serial
import serial.tools.list_ports
from serial import SerialException
import time
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QCheckBox, QLineEdit, QMessageBox, QComboBox, QLabel, QSpacerItem, QSizePolicy
from PyQt5.QtCore import QTimer
import pyqtgraph as pg
import os


class Channel():
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

    def update(self):
        self.lines[-1].setData(self.x_datas[-1], self.y_datas[-1])

    def new_line(self):
        self.x_datas.append([])
        self.y_datas.append([])
        self.lines.append(self.graph.plot(pen=self.color))

    def __iadd__(self, obj):
        self.x_datas[-1].append(obj[0])
        self.y_datas[-1].append(obj[1])
        return self



class AcquisitionApp(QWidget):
    x_width = 50
    checkboxes_default = 0b111111

    def __init__(self, app):
        super().__init__()
        self.app = app

        # Layout da interface gráfica
        self.layout = QVBoxLayout()

        # Serial layout
        self.serial_widget = QWidget()
        self.serial_widget.setObjectName("serial_widget")
        self.serial_widget.setStyleSheet("QWidget#serial_widget { background-color: lightgray; }")
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

        self.layout.addItem(QSpacerItem(100,50,QSizePolicy.Expanding, QSizePolicy.Minimum))

        # Buttons layout
        # Create checkboxes for A0 to A5
        self.button_layout = QHBoxLayout()
        self.checkboxes = []  # List to store checkbox references
        colors = []
        for i in range(6):
            col_bright = pg.intColor(i, 6).name()
            col_pale = pg.intColor(i, 6, sat=128).name()
            colors.append(col_bright)  # Dynamically choose a color for the line
            checkbox = QCheckBox(f"A{i}")
            checkbox.setStyleSheet(f"QCheckBox {{ background-color : {col_pale}; }}")
            checkbox.setChecked(bool(AcquisitionApp.checkboxes_default & (1 << (5-i))))
            self.button_layout.addWidget(checkbox)
            self.checkboxes.append(checkbox)
        self.layout.addLayout(self.button_layout)

        # Start/Stop/Clear buttons
        for command in ["start", "stop", "clear"]:
            btn = QPushButton(command.title())
            btn.clicked.connect(getattr(self,"on_"+command+"_acquisition"))
            self.layout.addWidget(btn)
            setattr(self, command+"_button", btn)

        # Configuração do gráfico
        self.graph = pg.PlotWidget()
        self.graph.getPlotItem().hideButtons()
        self.graph.getPlotItem().getViewBox().setMouseEnabled(x=False, y=False)
        self.graph.setBackground('k')
        self.graph.setLabel('left', 'Voltage (V)')
        self.graph.setLabel('bottom', 'Elapsed time (s)')
        self.graph.setYRange(0, 5.2, padding=0)  # Definir o intervalo do eixo Y (voltagem de 0 a 5V)
        self.graph.setXRange(0, AcquisitionApp.x_width, padding=0)  # Limitar o número de pontos no eixo X
        self.layout.addWidget(self.graph)

        self.line_edit = QLineEdit()
        self.line_edit.setPlaceholderText("Run command")
        self.layout.addWidget(self.line_edit) #Caixa de texto de 1 linha para enviar mensagens ao arduino

        self.setLayout(self.layout)

        # Variáveis para controlo do gráfico
        self.set_state("stopped")
        self.channels = []
        for i in range(6):
            self.channels.append(Channel(self.graph, colors[i]))

        # serial
        self.port = ""
        self.port_connected = False
        self.check_connection()

        # Inicialização dos temporizadores
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

        # commands
        self.line_edit.returnPressed.connect(self.message)

        self.start_button.setEnabled(self.port_connected)
        self.stop_button.setEnabled(False)
        self.clear_button.setEnabled(False)


    def set_state(self, s):
        self.state = s
        self.setWindowTitle(f"Acquisition App ({self.state.upper()})")  # Configuração do título da janela


    def check_connection(self):
        ports = serial.tools.list_ports.comports()
        coms = [k[0] for k in ports]
        if self.port == "":
            self.port = coms[0]

        found = self.port in coms
        
        if not self.ports_combobox.view().isVisible():
            self.ports_combobox.clear()
            self.ports_combobox.addItems([k[0]+": "+k[1] for k in ports])
            self.ports_combobox.setMinimumWidth(self.ports_combobox.view().sizeHintForColumn(0) + 20)
            cur_index = 0
            if found:
                cur_index = coms.index(self.port)
            self.ports_combobox.setCurrentIndex(cur_index)

        if found != self.port_connected:
            self.port_connected = not self.port_connected
            
            if found:
                if not self.on_connect():
                    color = 'red'
                else:
                    color = 'green'
            else:
                self.on_disconnect()
                color = 'orange'

            code = {
                'green': "OK",
                'orange': "DISCONNECTED",
                'red': "ERROR",
            }

            self.serial_widget.setStyleSheet(f"QWidget#serial_widget {{ background-color: {color}; }}")
            self.status_label.setText(self.port + " " + code[color])

            self.line_edit.setEnabled(color == 'green')
            self.start_button.setEnabled(color == 'green')


    def on_port_select(self, i):
        new_port = self.ports_combobox.itemText(i).split(": ")[0]
        if new_port == self.port:
            return

        if self.state == 'running':
            self.on_stop_acquisition()
        self.serial_port.close()
        self.port_connected = False
        self.port = new_port
        

    def on_connect(self):
        print("CONNECT")
        try:
            self.serial_port = serial.Serial(self.port, 38400, timeout=1)
        except:
            return False
        self.message(force=True)
        
        if self.state == 'halted':
            self.on_start_acquisition()
        return True


    def on_disconnect(self):
        print("DISCONNECT")
        if self.state == 'running':
            self.on_stop_acquisition()
            self.set_state('halted')


    def get_true_voltage(self):
        self.serial_port.write(b"defget(TRUE_VOLTAGE)\n")  # Enviar comando para o Arduino
        time.sleep(0.1)  # Aguardar uma pequena quantidade de tempo para a resposta

        if self.serial_port.in_waiting > 0:
            data = self.serial_port.readline().decode().rstrip()  # Ler a resposta
            volt = float(data.split(' ')[1])
            self.graph.setYRange(0, volt*1.04, padding=0)


    def message(self, force=False):
        text = self.line_edit.text()
        if not text and not force:
            return
        text += "\n"

        self.line_edit.clear()
        self.serial_port.write(text.encode("ascii"))  # Enviar comando para o Arduino
        time.sleep(0.5)  # Aguardar uma pequena quantidade de tempo para a resposta

        # print(text[:-1])
        lines = []
        # Verificar se há dados disponíveis na porta serial
        while self.serial_port.in_waiting > 0:
            data = self.serial_port.readline().decode().rstrip()  # Ler a resposta
            lines.append(data)
            # print(">>>", data)

        msg = QMessageBox()
        statuses = {
            "ERROR": QMessageBox.Critical,
            "WARN": QMessageBox.Warning,
            "INFO": QMessageBox.Information,
        }
        status = next((k for k in statuses if lines[0].startswith(k+": ")), None)
        if len(lines) == 1 and status != None:
            msg.setWindowTitle(status)
            msg.setIcon(statuses[status])
            msg.setText(lines[0].split(": ")[1])
        else:
            msg.setWindowTitle("RESULT")
            msg.setText("<b>"+text+"</b><br><br>"+'<br>'.join([l.replace('\t','&nbsp;'*4) for l in lines]))
        msg.exec_()


    def on_clear_acquisition(self):
        # Checar se a aquisição está rodando
        # if self.acquisition.state:
        #     msg = QMessageBox()
        #     msg.setIcon(QMessageBox.Critical)
        #     msg.setText("Error: Stop acquisition before clearing data.")
        #     msg.setWindowTitle("Error")
        #     msg.exec_()
        #     return
        if self.state == 'halted':
            self.set_state('stopped')

        for chn in self.channels:
            chn.clear()
        self.graph.clear()
        self.graph.setXRange(0, AcquisitionApp.x_width, padding=0)
        self.app.processEvents()  # Atualizar a interface gráfica
        self.clear_button.setEnabled(False)


    def on_start_acquisition(self):
        # Marcar o tempo de início da aquisição
        # if not self.acquisition.state:
        #     self.start_time = time.time()
        # self.acquisition.state = True  # Indica que a aquisição está rodando
        if not self.channels[0].lines:
            self.start_time = time.time()
        for chn in self.channels:
            chn.new_line()
            
        self.set_state("running")
        self.acquire_data_timer.start()  # Iniciar o temporizador de coleta de dados

        for check in self.checkboxes:
            check.setEnabled(False)
        self.stop_button.setEnabled(True)
        self.start_button.setEnabled(False)
        self.clear_button.setEnabled(False)
        

    def on_stop_acquisition(self):
        self.set_state("stopped")
        # self.acquisition.state = False  # Indica que a aquisição foi parada
        self.acquire_data_timer.stop()  # Parar o temporizador de coleta de dados

        for check in self.checkboxes:
            check.setEnabled(True)
        self.stop_button.setEnabled(False)
        self.start_button.setEnabled(True)
        self.clear_button.setEnabled(True)


    def acquire_data(self):
        # Se a aquisição estiver rodando, solicite os dados do Arduino
        if self.state != "running":
            return
        
        # Prepare the command based on selected checkboxes
        command = ''
        refs = []
        for checkbox in self.checkboxes:
            if checkbox.isChecked():
                refs.append(len(command))
                command += "1"
            else:
                command += "0"

        try:
            # Send the command to the Arduino
            self.serial_port.write(f'analog(0b{command})\n'.encode())  # Enviar comando para o Arduino
            time.sleep(0.1)  # Aguardar uma pequena quantidade de tempo para a resposta

            # Verificar se há dados disponíveis na porta serial
            if self.serial_port.in_waiting > 0:
                data = self.serial_port.readline().decode().strip()  # Ler a resposta
                values = [float(v) for v in data.split(',')[:-1]]  # Separar os valores por vírgula
                values.reverse()

                # Atualizar os dados para o gráfico
                for i, j in enumerate(refs):
                    t = time.time() - self.start_time
                    self.channels[j] += (t, values[i])
                    self.channels[j].update()
                    if t > AcquisitionApp.x_width:
                        self.graph.setXRange(t-AcquisitionApp.x_width, t, padding=0)

                self.app.processEvents()  # Atualizar a interface gráfica
        except Exception as e:
            print("EOINJKL", e)
            pass



if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = AcquisitionApp(app)  # Criar a janela de aquisição de dados
    window.show()  # Mostrar a janela
    sys.exit(app.exec_())  # Iniciar o loop do aplicativo
