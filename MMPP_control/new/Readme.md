Simple CLI control tool allowing bash scripting 
===================================


## Command line arguments

	-A, --absmove        absolute move (without this flag moving is relative)
	-E, --reset          reset given mcu (may be included several times)
	-L, --lin1=arg       move polaroid linear stage to N steps
	-R, --rot1=arg       rotate polaroid to given angle
	-S, --stop           stop any moving
	-a, --sendraw=arg    send RAW string to port and read the answer
	-b, --baudrate=arg   TTY baudrate
	-d, --comdev=arg     terminal device filename
	-h, --help           show this help
	-l, --lin2=arg       move waveplate linear stage to N steps
	-p, --pidfile=arg    PID-file name
	-q, --quiet          don't show anything @screen from stdout
	-r, --rot2=arg       rotate lambda/4 to given angle
	-s, --status         get device status
	-t, --temp           show temperature of both MCU
	-w, --wait           wait while all previous moving ends
	-y, --async          asynchronous moving - don't wait




## Keywords in *quiet* mode


- - temperature of x'th MCU;
- POLTEMP=35
L4TEMP=32.9
- POLMOTOR0=SLEEP
POLPOS0=0
POLESW00=RLSD
POLESW01=RLSD
POLMOTOR1=SLEEP
POLPOS1=0
POLESW10=RLSD
POLESW11=RLSD
POLSTEPSLEFT0
POLSTEPSLEFT1
L4MOTOR0=SLEEP
L4POS0=-40001
L4ESW00=RLSD
L4ESW01=RLSD
L4MOTOR1=SLEEP
L4POS1=-40001
L4ESW10=RLSD
L4ESW11=RLSD
L4STEPSLEFT0
L4STEPSLEFT1

