# Bell-manager_esp32
Bell manager for work or school environment managed by web interface with ESP32 based device

Si prenda spunto da:
https://bangertech.de/sonoff-pow-elite/
https://github.com/arendst/Tasmota/issues/15856

Probabile pinout:
ESP32
 |-- GPIO_X → driver / transistor → Relè coil
 |-- GPIO_Y → LED (status) (forse con resistenza, transistor)
 |-- GPIO_Z → pulsante / tasto (es. POWER)
 |-- GPIO_LCD_Data[ ] → verso modulo LCD
 |-- GPIO_LCD_Ctrl (RS, EN, /CS, CLK, etc) → verso modulo LCD
 |-- Sensore corrente CSE7759B → circuito di condizionamento → ADC pin dell’ESP32
 |-- Alimentazione: 3.3 V (o 5 V) con regolatori / step-down da rete 230 V
 |-- Alimentazione per il display LCD (forse 5 V, con level shifters se necessario)

 
