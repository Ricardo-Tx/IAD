#include <Arduino.h>

const int analogPin1 = A0; // Pino analógico onde o primeiro sensor está ligado
const int analogPin2 = A5; // Pino analógico onde o segundo sensor está ligado
const String command = "READ"; // Comando esperado

void setup() {
    Serial.begin(9600); // Inicializa a comunicação serial
}

void loop() {
    if (Serial.available() > 0) { // Verifica se há dados recebidos
        String receivedCommand = Serial.readStringUntil('\n'); // Lê o comando recebido
        receivedCommand.trim(); // Remove espaços extras

        if (receivedCommand == command) {
            // Lê o valor analógico dos dois pinos
            int sensorValue1 = analogRead(analogPin1);
            int sensorValue2 = analogRead(analogPin2);

            // Converte os valores para voltagem
            float sensorValue1_V = sensorValue1 * (5.0 / 1023.0);
            float sensorValue2_V = sensorValue2 * (5.0 / 1023.0);

            // Envia os valores de voltagem via serial, separados por uma vírgula
            Serial.print(sensorValue1_V);
            Serial.print(",");  // Separate values with a comma
            Serial.println(sensorValue2_V);
        } else {
            Serial.println("ERRO: Comando inválido!"); // Mensagem de erro
        }
    }
}
