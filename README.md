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
TBA