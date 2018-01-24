Management of two stepper motors
================================

Based on STM32F030F4P6

## Pinout

|Pin | Type |         Role            |
|:---|:----:|:------------------------|
|PA0 | AIN  | Steppers current        |
|PA1 | AIN  | Input voltage 12V       |
|PA2 | AIN  | EndSwitch1 of motor0    |
|PA3 | AIN  | EndSwitch0 of motor0    |
|PA4 | PUPD | Tim14Ch1 - motor0 steps |
|PA5 | PUPD | Motor1 enable           |
|PA6 | PUPD | Tim3Ch1 - motor1 steps  |
|PA7 | PUPD | Motor1 direction        |
|PA9 | OD   | USART1 Tx               |
|PA10| FIN  | USART1 Rx               |
|PA13| FINPU| EndSwitch0 of motor1    |
|PA14| FINPU| EndSwitch1 of motor1    |
|PB1 | PUPD | Turn off current sensor |
|PF0 | PUPD | Motor0 enable           |
|PF1 | PUPD | Motor0 direction        |

## Protocol

Any command have format "# CMD", where # is device ID (the_conf.devID) or "-1" if there's
only one device at the bus. After it can be any number of spaces and command symbols.
If the command recognized and it's right, `ALL OK` would be returnded. If the command is bad, answer
`BADCMD`. Error in command sintax: `ERR`. If the answer would take more than one line it would
be ends by `DATAEND`.

### First symbol of commands

* (nothing except spaces) - *ping* command, device with given ID answers: `ALIVE`
* **G** - one of the *Getters* (depending on next text)
* **M** - motors' manipulation
* **R** - make *MCU software reboot* (next `status` getter will return `SOFTRESET=1`)
* **S** - one of the *Setters*
* **W** - *write flash* command, use it to save new configuration data

All wrong commands will return `BADCMD`, commands with bad format or wrong number return `ERR`.
Commands with lot of output ends with `DATAEND`. Some commands (like flash write) will return
just `ALL OK` if succeed.

### Getters

Getters returning more than one field ends with `DATAEND` meaning that's all data.

* **A** - get *ADC value*
    * **D** - Vdd value (*100 Volts), e.g. `VDD=330`
    * **I** - total motors' current (*100 Amperes), e.g. `IMOT=123`
    * **M** - motor's voltage (*100 Volts), e.g. `VMOT=1193`
* **C** - get *current configuration*, e.g.

```
    CONFSZ=36
    DEVID=0
    V12NUM=1
    V12DEN=10
    I12NUM=1
    I12DEN=1
    V33NUM=1
    V33DEN=1
    ESWTHR=150
    MOT0SPD=60
    MOT1SPD=60
    USARTSPD=115200
    REVERSE0=0
    REVERSE1=0
    MAXSTEPS0=0
    MAXSTEPS1=0
    DATAEND
```

All variables here are fields of `user_conf` struct.


* **R** - get *raw ADC* values:
    * 0 - Steppers current
    * 1 - Input voltage 12V
    * 2 - EndSwitch2 of motor1
    * 3 - EndSwitch1 of motor1
    * 4 - inner temperature
    * 5 - vref

E.g.:

```
ADC[0]=4095
ADC[1]=2340
ADC[2]=4095
ADC[3]=4087
ADC[4]=1665
ADC[5]=1532
DATAEND
```



* **S** - get *motors' status*, e.g.

```
SOFTRESET=1
MOTOR0=STOP
POS0=-1
ESW00=ERR
ESW01=BTN
MOTOR1=STOP
POS1=-1
ESW10=HALL
ESW11=HALL

```

* **T** - get *MCU temperature*, e.g. `TEMP=365`


### Motors' manipulation
Next char should be '0' or '1' --- motor's number. If wrong, `Num>1` answer would be returned.
There's only two commands in this section:

* **Mnum** - move motor to *num* steps. Errors:
    * **BadSteps** - *num* isn't a number
    * **IsMoving** - motor is still moving, stop it before
    * **OnEndSwitch** - the motor is on end-switch limiting moving in given direction
    * **ZeroMove** - user wants to move for 0 steps
    * **TooBigNumber** - amount of steps is greater than `MAXSTEPSx`
* **S** - stop motor


### Setters
Change of any setter takes place in MCU RAM immediately. To store them permanently run
*write flash* command.

* **C#num** - set current *speed* to *num* for motor #
* **D num** - set *denominator* to number *num*
* **E num** - set *numerator*
* **I num** - set *device ID*
* **M#num** - set maxsteps (*num* is 1..65535) for motor `#`
* **P num** - properties of internal pullup (0 - disabled, other or without `num` - enabled)
* **R#num** - set reverse for motor # (*num* == 0 turns reverse off, *num* == 1 turns it on)
* **S#num** - set *speed* (`motspd`) to *num* for motor #
* **T num** - set *end-switches threshold* (in ADU, near 0 for Hall switch, 2048 for user button
    and 4096 for released state)
* **U num** - set *USART speed* to *num* bits per second

### Motor speed setters
To set motor speed to **N** steps per second, give command `C` or `S` with argument equal to
3000/N. E.g. to set current speed for DevID=0, motor0 to 50 steps per second give command `0SC050`.

### Denominator and numerator setters
Have naxt letter similar to ADC getter (**D** - Vdd, **I** - motors' I, or **M** - motors' U).
The value of numerator shouldn't be very large as uint32_t used in multiplications.

### Device ID setter
After this command device will immediately change it's ID, if you "lost" device after this
procedure you should reboot it or (if there's only one device on the bus) call it by "universal ID"
(-1).

### USART speed setter
The USART speed will be changed after next reset (e.g. by *MCU software reboot* command), so it
don't work without storing in the flash. Check it twice before writing as wrong numbers can make device
lost until next re-flashing.
