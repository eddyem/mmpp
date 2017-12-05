Management of two stepper motors
================================

Based on STM32F030F4P6

## Pinout

|Pin | Type |         Role            |
|:---|:----:|:------------------------|
|PA0 | AIN  | Steppers current        |
|PA1 | AIN  | Input voltage 12V       |
|PA2 | AIN  | EndSwitch2 of motor1    |
|PA3 | AIN  | EndSwitch1 of motor1    |
|PA4 | PUPD | Tim14Ch1 - motor1 steps |
|PA5 | PUPD | Motor2 enable           |
|PA6 | PUPD | Tim3Ch1 - motor2 steps  |
|PA7 | PUPD | Motor2 direction        |
|PA9 | OD   | USART1 Tx               |
|PA10| FIN  | USART1 Rx               |
|PA13| AIN  | EndSwitch1 of motor2    |
|PA14| AIN  | EndSwitch2 of motor2    |
|PB1 | PUPD | Turn off motors' power  |
|PF0 | PUPD | Motor1 enable           |
|PF1 | PUPD | Motor1 direction        |

## Protocol
