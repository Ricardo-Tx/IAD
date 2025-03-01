import sys
import serial
import time
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QCheckBox, QLineEdit, QMessageBox
from PyQt5.QtCore import QTimer
import pyqtgraph as pg
import os

class DataAcquisitionApp(QWidget):
    def __init__(self, app):
        super().__init__()
        self.app = app
        self.setWindowTitle("Aquisição de Dados")  # Configuração do título da janela

        # Layout da interface gráfica
        self.layout = QVBoxLayout()

        # Buttons layout
        self.button_layout = QHBoxLayout()
        self.checkboxes = []  # List to store checkbox references
        self.gate_commands = ['000001', '000010', '000100', '001000', '010000', '100000']  # One-hot encoding for A0 to A5

        # Create checkboxes for A0 to A5
        for i in range(6):
            checkbox = QCheckBox(f"A{i}")
            checkbox.setStyleSheet("color: red")
            self.button_layout.addWidget(checkbox)
            self.checkboxes.append(checkbox)

        self.layout.addLayout(self.button_layout)

        # Start/Stop/Clear buttons
        self.start_button = QPushButton("Start")
        self.stop_button = QPushButton("Stop")
        self.clear_button = QPushButton("Clear")
        self.layout.addWidget(self.start_button)
        self.layout.addWidget(self.stop_button)
        self.layout.addWidget(self.clear_button)

        # Configuração do gráfico
        self.graph = pg.PlotWidget()
        self.layout.addWidget(self.graph)
        self.line_edit = QLineEdit()
        self.layout.addWidget(self.line_edit) #Caixa de texto de 1 linha para enviar mensagens ao arduino
        self.graph.setYRange(0, 5)  # Definir o intervalo do eixo Y (voltagem de 0 a 5V)
        self.graph.setXRange(0, 100)  # Limitar o número de pontos no eixo X
        self.setLayout(self.layout)

        # Inicialização da porta serial (adaptar para a porta correta do seu sistema)
        self.serial_port = serial.Serial('/dev/ttyACM0' if os.name != 'nt' else 'COM8', 9600, timeout=1)

        # Conexão dos botões
        self.start_button.clicked.connect(self.start_acquisition)
        self.stop_button.clicked.connect(self.stop_acquisition)
        self.clear_button.clicked.connect(self.clear_acquisition)

        # Variáveis para controlo do gráfico
        self.running = False
        self.x_data = []  # Armazenar os valores do tempo
        self.y_data = []  # Armazenar os valores de todos os sensores
        self.lines = []  # List to hold the plot lines for each selected gate
        for i in range(6):
            color = pg.intColor(i, 6)  # Dynamically choose a color for the line
            self.lines.append(self.graph.plot(pen=color))  # Create a new line for each sensor
            self.y_data.append([])

        # Inicialização do temporizador para aquisição de dados
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.acquire_data)  # Função chamada periodicamente
        self.timer.setInterval(100)  # Aquisição a cada 100 ms

        self.first_time = 0

        # Flags de controlo
        self.acquisition_running = False  # Controle se a aquisição está rodando

        self.line_edit.returnPressed.connect(self.message)

    def message(self):
        text = self.line_edit.text()
        self.line_edit.clear()
        self.serial_port.write(text.encode("ascii"))  # Enviar comando para o Arduino
        time.sleep(0.1)  # Aguardar uma pequena quantidade de tempo para a resposta

        # Verificar se há dados disponíveis na porta serial
        if self.serial_port.in_waiting > 0:
            data = self.serial_port.readline().decode().strip()  # Ler a resposta
            print(data)
            if data.startswith('INFO'):
                return




    def clear_acquisition(self):
        # Checar se a aquisição está rodando
        if self.acquisition_running:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error: Stop acquisition before clearing data.")
            msg.setWindowTitle("Error")
            msg.exec_()
            return

        # Limpar os dados do gráfico
        self.stop_acquisition()  # Parar a aquisição, caso esteja rodando
        self.x_data.clear()
        self.y_data.clear()
        for line in self.lines:
            line.clear()  # Clear all the plot lines
        self.lines.clear()
        self.first_time = 0
        self.app.processEvents()  # Atualizar a interface gráfica

    def start_acquisition(self):
        # Marcar o tempo de início da aquisição
        if not self.acquisition_running:
            self.start_time = time.time()

        self.running = True
        self.acquisition_running = True  # Indica que a aquisição está rodando
        self.timer.start()  # Iniciar o temporizador de coleta de dados

    def stop_acquisition(self):
        self.running = False
        self.first_time = self.x_data[-1]
        self.acquisition_running = False  # Indica que a aquisição foi parada
        self.timer.stop()  # Parar o temporizador de coleta de dados

    def acquire_data(self):
        # Se a aquisição estiver rodando, solicite os dados do Arduino
        if self.running:
            # Prepare the command based on selected checkboxes
            command = ''
            for checkbox in reversed(self.checkboxes):
                command += str(int(checkbox.isChecked()))
            print("command", command)

            # Send the command to the Arduino
            self.serial_port.write(f'analog(0b{command})\n'.encode())  # Enviar comando para o Arduino
            time.sleep(0.1)  # Aguardar uma pequena quantidade de tempo para a resposta

            # Verificar se há dados disponíveis na porta serial
            if self.serial_port.in_waiting > 0:
                data = self.serial_port.readline().decode().strip()  # Ler a resposta
                print(data)
                if data.startswith('INFO'):
                    return
                values = data.split(',')  # Separar os valores por vírgula

                values = values[:-1]



                # Converting the values to float
                for i in range(len(values)):
                    values[i] = float(values[i])
                print('values')
                print(values)

                # Atualizar os dados para o gráfico
                self.x_data.append(time.time() - self.start_time + self.first_time)  # Tempo desde o início
                for i in range(6):
                    self.y_data[i].append(values[i])
                    self.lines[i].setData(self.x_data, self.y_data[i])

                self.app.processEvents()  # Atualizar a interface gráfica

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = DataAcquisitionApp(app)  # Criar a janela de aquisição de dados
    window.show()  # Mostrar a janela
    sys.exit(app.exec_())  # Iniciar o loop do aplicativo
