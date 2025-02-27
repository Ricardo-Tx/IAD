# IAD
Repositório para o desenvolvimento dos projetos na UC de *Instrumentação e Aquisição de Dados*, por parte do grupo E no ano letivo 2024/2025.


## Projeto 1
### Pasta de projeto Arduino `analog_serial_pi`:
Contém o código responsável pela leitura das tensões analógicas e da comunicação via *serial* (USB).

Funcionalidades a implementar ou ponderar:
- Leitura e escrita de determinados parâmetros de medição (tensão real, oversampling, frequência de medição, etc.) no **EEPROM** do Arduino (memória não volátil).
- Comunicação através de comandos mais completos (por ex. `measure(rate, ...)`).
- Estudo do erro associado às medições e redução do erro por oversampling. Recursos úteis nesta [página](https://www.skillbank.co.uk/arduino/adc1.htm) e site.
- Aplicação de pré-filtros IIR diretamente no Arduino, possivelmente com recurso a comandos específicos.
- Comando `help()` que expõe todos os outros comandos disponíveis.

### Aplicação `PyQt`:
TBA


## Projeto 2

### LED sensores de luz
É possível usar LEDs como elementos detetores de luz num comprimento de onda semelhante ao da luz que emitiam originalmente. Assim, poderiam-se utilizar uma série de LEDs de cores diferentes para caracterizar de modo grosseiro o espetro da luz incidente. 

Um artigo no
[hackaday](https://hackaday.com/2024/07/23/you-can-use-leds-as-sensors-too/) explica o funcionamento em mais detalhe.

### Calculadora com relês
Criação de uma calculadora com arduino, teclas de entrada e um ecrã de saída (7 segment, LCD, OLED, etc.), mas onde os cálculos são realizados através de portas lógicas externas realizadas com relês. 

Exemplo:
[Relay Calculator](https://www.youtube.com/watch?v=6daApHNHT20)

### Radar ESP32
Utilização de 1 ou mais módulos *ESP32* para análise da intensidade de sinais WiFi ou Bluetooth, de modo a caracterizar a existência de obstáculos ou distância entre os módulos.

Exemplo: [ESP32 Motion Detection](https://www.youtube.com/watch?v=7XwgxTXPJKM)

### Eletrodoméstico IoT
Modificar um eletrodoméstico ou aparelho, por exemplo uma máquina de café, para realizar ações ou responder com o estado através de WiFi.
