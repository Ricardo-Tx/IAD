import sys
import serial

print(serial.__file__)

import time
import numpy as np
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QPushButton
import pyqtgraph as pg

class DataAcquisitionApp(QWidget):
    def __init__(self, app):
        super().__init__()
        self.app = app
        # Configurar a interface gráfica
        self.setWindowTitle("Aquisição de Dados")
        self.layout = QVBoxLayout()
        self.start_button = QPushButton("Start")
        self.stop_button = QPushButton("Stop")
        self.clear_button = QPushButton("Clear")
        self.layout.addWidget(self.start_button)
        self.layout.addWidget(self.stop_button)
        self.layout.addWidget(self.clear_button)

        # Configurar gráfico
        self.graph = pg.PlotWidget()
        self.layout.addWidget(self.graph)
        self.graph.setYRange(0,5)
        self.graph.setXRange(0,100)
        self.setLayout(self.layout)

        # Inicializar porta serial
        self.serial_port = serial.Serial('/dev/ttyACM0', 9600, timeout=1)

        # Conectar botões
        self.start_button.clicked.connect(self.start_acquisition)
        self.stop_button.clicked.connect(self.stop_acquisition)
        self.clear_button.clicked.connect(self.clear_acquisition)

        self.running = False
        self.x_data = []
        self.y_data = []

        self.line = self.graph.plot()

        self.bool = True

    def clear_acquisition(self):
        self.stop_acquisition()
        self.bool = True
        self.x_data.clear()
        self.y_data.clear()
        self.line.clear()
        self.app.processEvents()

    def start_acquisition(self):
        if self.bool:
            self.start_time = time.time()

        self.running = True
        self.acquire_data()

        self.bool = False

    def stop_acquisition(self):
        self.running = False

    def acquire_data(self):

        while self.running:
            self.serial_port.write(b'READ\n')  # Envia comando ao Arduino
            time.sleep(0.1)  # Aguarda resposta

            if self.serial_port.in_waiting > 0:
                data = self.serial_port.readline().decode().strip()  # Lê resposta
                print(data)
                value = float(data)  # Converte para inteiro
                self.x_data.append(time.time()-self.start_time)
                self.y_data.append(value)
                self.line = self.graph.plot(self.x_data, self.y_data, clear=True)  # Atualiza gráfico
                self.app.processEvents()

            '''except ValueError:
                    print(f"Erro recebido: {data}")  # Caso seja uma mensagem de erro'''

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = DataAcquisitionApp(app)
    window.show()
    sys.exit(app.exec_())