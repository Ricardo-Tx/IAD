import sys
import serial
import time
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QPushButton, QMessageBox
from PyQt5.QtCore import QTimer
import pyqtgraph as pg

class DataAcquisitionApp(QWidget):
    def __init__(self, app):
        super().__init__()
        self.app = app
        self.setWindowTitle("Aquisição de Dados")  # Configuração do título da janela

        # Layout da interface gráfica
        self.layout = QVBoxLayout()
        self.start_button = QPushButton("Start")
        self.stop_button = QPushButton("Stop")
        self.clear_button = QPushButton("Clear")
        self.layout.addWidget(self.start_button)
        self.layout.addWidget(self.stop_button)
        self.layout.addWidget(self.clear_button)

        # Configuração do gráfico
        self.graph = pg.PlotWidget()
        self.layout.addWidget(self.graph)
        self.graph.setYRange(0, 5)  # Definir o intervalo do eixo Y (voltagem de 0 a 5V)
        self.graph.setXRange(0, 100)  # Limitar o número de pontos no eixo X
        self.setLayout(self.layout)

        # Inicialização da porta serial (adaptar para a porta correta do seu sistema)
        self.serial_port = serial.Serial('/dev/ttyACM0', 9600, timeout=1)

        # Conexão dos botões
        self.start_button.clicked.connect(self.start_acquisition)
        self.stop_button.clicked.connect(self.stop_acquisition)
        self.clear_button.clicked.connect(self.clear_acquisition)

        # Variáveis para controle do gráfico
        self.running = False
        self.x_data = []  # Armazenar os valores do tempo
        self.y_data1 = []  # Armazenar os valores do sensor 1
        self.y_data2 = []  # Armazenar os valores do sensor 2

        # Linhas do gráfico para os dois sensores
        self.line1 = self.graph.plot(pen='b')  # Linha azul para o sensor 1
        self.line2 = self.graph.plot(pen='m')  # Linha rosa para o sensor 2

        # Inicialização do temporizador para aquisição de dados
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.acquire_data)  # Função chamada periodicamente
        self.timer.setInterval(100)  # Aquisição a cada 100 ms

        self.first_time  = 0

        # Flags de controle
        self.acquisition_running = False  # Controle se a aquisição está rodando

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
        self.y_data1.clear()
        self.y_data2.clear()
        self.line1.clear()
        self.line2.clear()
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
            self.serial_port.write(b'analog(0b100001)\n')  # Enviar comando para o Arduino
            time.sleep(0.1)  # Aguardar uma pequena quantidade de tempo para a resposta

            # Verificar se há dados disponíveis na porta serial
            if self.serial_port.in_waiting > 0:
                time.sleep(1)  # Aguardar uma pequena quantidade de tempo para a resposta
                data = self.serial_port.readline().decode().strip()  # Ler a resposta
                print(data)
                if data.startswith('INFO'):
                    return
                values = data.split(',')  # Separar os valores por vírgula

                values = values[:-1]
                print(values)

                for i in range(len(values)):
                    values[i] = float(values[i])

                    # Atualizar os dados para o gráfico
                    self.x_data.append(time.time() - self.start_time + self.first_time)  # Tempo desde o início
                    self.y_data1.append(value1)  # Valor do primeiro sensor
                    self.y_data2.append(value2)  # Valor do segundo sensor
                    """
                    # Limitar o número de pontos no gráfico (máximo de 100)
                    if len(self.x_data) > 100:
                        self.x_data.pop(0)
                        self.y_data1.pop(0)
                        self.y_data2.pop(0)
                    """
                    # Atualizar as linhas no gráfico
                    self.line1.setData(self.x_data, self.y_data1)  # Atualizar linha do sensor 1
                    self.line2.setData(self.x_data, self.y_data2)  # Atualizar linha do sensor 2
                    self.app.processEvents()  # Atualizar a interface gráfica

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = DataAcquisitionApp(app)  # Criar a janela de aquisição de dados
    window.show()  # Mostrar a janela
    sys.exit(app.exec_())  # Iniciar o loop do aplicativo
