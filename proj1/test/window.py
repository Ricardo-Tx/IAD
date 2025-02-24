import sys
import serial
import time
import numpy as np
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QPushButton
import pyqtgraph as pg

class DataAcquisitionApp(QWidget):
    def __init__(self):
        super().__init__()

        # Configurar a interface gráfica
        self.setWindowTitle("Aquisição de Dados")
        self.layout = QVBoxLayout()
        self.start_button = QPushButton("Start")
        self.stop_button = QPushButton("Stop")
        self.layout.addWidget(self.start_button)
        self.layout.addWidget(self.stop_button)
        
        # Configurar gráfico
        self.graph = pg.PlotWidget()
        self.layout.addWidget(self.graph)
        self.setLayout(self.layout)

        # Inicializar porta serial
        self.serial_port = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

        # Conectar botões
        self.start_button.clicked.connect(self.start_acquisition)
        self.stop_button.clicked.connect(self.stop_acquisition)

        self.running = False
        self.x_data = []
        self.y_data = []

    def start_acquisition(self):
        self.running = True
        self.acquire_data()

    def stop_acquisition(self):
        self.running = False

    def acquire_data(self):
        while self.running:
            self.serial_port.write(b'READ\n')  # Envia comando ao Arduino
            time.sleep(0.1)  # Aguarda resposta

            if self.serial_port.in_waiting > 0:
                data = self.serial_port.readline().decode().strip()  # Lê resposta
                try:
                    value = int(data)  # Converte para inteiro
                    self.x_data.append(time.time())
                    self.y_data.append(value)
                    self.graph.plot(self.x_data, self.y_data, clear=True)  # Atualiza gráfico
                except ValueError:
                    print(f"Erro recebido: {data}")  # Caso seja uma mensagem de erro

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = DataAcquisitionApp()
    window.show()
    sys.exit(app.exec_())