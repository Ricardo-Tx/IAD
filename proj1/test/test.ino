#include <Arduino.h>

const int analogPin = A0; // Pino analógico onde o sensor está ligado
const String command = "READ"; // Comando esperado

void setup() {
    Serial.begin(9600); // Inicializa a comunicação serial
}

void loop() {
    if (Serial.available() > 0) { // Verifica se há dados recebidos
        String receivedCommand = Serial.readStringUntil('\n'); // Lê o comando recebido
        receivedCommand.trim(); // Remove espaços extras

        if (receivedCommand == command) {
            int sensorValue = analogRead(analogPin); // Lê o valor analógico
            float sensorValue_V = sensorValue * 5 / 1024.0;
            Serial.println(sensorValue_V); // Envia o valor de volta
        } else {
            Serial.println("ERRO: Comando inválido!"); // Mensagem de erro
        }
    }
}
