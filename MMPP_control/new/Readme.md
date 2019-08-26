Simple CLI control tool allowing bash scripting 
===============================================


## Command line arguments

- `-A, --absmove`--      absolute moving, use this flag to point that steps amount (parameters of `-L`, `-l`, `-R` or `-r`) are given in absolute position (without this flag steps meaning as relative);
- `-E, --reset`--`       reset given mcu (may be included several times), each `-Ex` (where `x` is controller ID) will send signal to reset given MCU;
- `-L, --lin1=arg`--     move polaroid linear stage to N steps;
- `-R, --rot1=arg`--     rotate polaroid to given angle (angle in degrees);
- `-S, --stop`--         stop any moving (with this flag each MCU will receive a signal to stop any moving);
- `-a, --sendraw=arg`--  send RAW string to port and read the answer, if you want to send something that can't be reached by standard arguments, use this flag;
- `-b, --baudrate=arg`-- TTY baudrate (raw number, e.g. 115200), default speed is 9600;
- `-d, --comdev=arg`--   terminal device filename, default path is `/dev/ttyUSB0`;
- `-h, --help`--         show this help;
- `-l, --lin2=arg`--     move wave-plate linear stage to N steps;
- `-p, --pidfile=arg`--  PID-file name (default is `/tmp/MMPP_control.pid`);
- `-q, --quiet`--        "almost quiet" mode for external calls (show minimum on stdout, output will be like `param=value`);
- `-r, --rot2=arg`--     rotate lambda/4 to given angle;
- `-s, --status`--       get device status;
- `-t, --temp`--         show approximate temperature of both MCU;
- `-w, --wait`--         wait while all previous moving ends, this is useful if you ran this tool several times with `-y` and want to wait until all moving ends;
- `-y, --async`--        asynchronous moving, the tool won't wait and will exit after all commands being sent.


## Output in normal mode

When "quiet" mode inactive, the output is human-readable and have a lot of information. E.g. "status" request in case of success will show something like

	Pol: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS  || L/4: M0ST M0LEFT M0POS  - M1ST M1LEFT M1POS 
	Pol: STOP      0      0 - STOP      0  29700 || L/4: STOP      0      0 - STOP      0      0
	ESW00 ESW01 ESW10 ESW11 || ESW00 ESW01 ESW10 ESW11
	 HALL  RLSD  RLSD  RLSD ||  HALL  RLSD  HALL  RLSD 

The first and third rows are headers. To the left of `||` delimiter is polaroid data, to the right -- wave-plate data.
The second row shows values of linear stage (`M0ST`) and rotator (`M1ST`) motors state, steps left (`MxLEFT`) and current positions (`MxPOS`). The last row shows Hall sensors state for all end-switches (`HALL` -- end-switch is active, `RLSD` end-switch is inactive and for linear stage there could be third value: `BTN` meaning that user button pressed).

When sending raw data you will get some additional information, e.g.:

./MMPP_control -a 1GC

	Send raw string: 1GC
	Receive: CONFSZ=36
	DEVID=1
	V12NUM=605
	V12DEN=94
	I12NUM=3
	I12DEN=4
	V33NUM=1
	V33DEN=1
	ESWTHR=500
	MOT0SPD=3
	MOT1SPD=5
	MAXSTEPS0=50000
	MAXSTEPS1=50000
	USARTSPD=9600
	INTPULLUP=1
	REVERSE0=1
	REVERSE1=0
	USTEPS=16
	ACCDECSTEPS=50
	DATAEND

(this is a request for settings stored in device flash memory).

When moving, the output would be like first two strings of `-s` with dynamically changed values of steps left and current position of moving parts.

The output of temperature request would be like:

	./MMPP_control -t   
	MCU1 temperature: 39.6 degC
	MCU2 temperature: 35.1 degC


## Keywords in *quiet* mode

This mode is useful for batch files and running from external soft. Its main difference is no unuseful information and data format like `parameter=value`. For example, temperature request have output like: 

	POLTEMP=35
	L4TEMP=32.9

Where `POLTEMP` is temperature of polarisator controller, L4TEMP -- temperature of wave-plate controller.
When moving, there's no output, only waiting for moving ends. It's better to run this tool with `-y` flag and asynchronously start it later with `-s` flag to receive current state.
The output of `MMPP_control -qs` will be like:

	POLMOTOR0=MOVE
	POLSTEPSLEFT0=742
	POLPOS0=4558
	POLESW00=RLSD
	POLESW01=RLSD
	POLMOTOR1=MOVE
	POLSTEPSLEFT1=7289
	POLPOS1=32411
	POLESW10=RLSD
	POLESW11=RLSD
	L4MOTOR0=MOVE
	L4STEPSLEFT0=642
	L4POS0=4358
	L4ESW00=RLSD
	L4ESW01=RLSD
	L4MOTOR1=MOVE
	L4STEPSLEFT1=1542
	L4POS1=6458
	L4ESW10=RLSD
	L4ESW11=RLSD

Where

- `xxxMOTOR0` and `xxxMOTOR1` are linear stage and rotator motors of polaroid (`POL`) or wave-plate (`L4`) controller;
- `xxxSTEPSLEFTy` are amount of steps left (these values are displayed only when corresponding motor is moving);
- `xxxPOSy` are current positions (in steps!!!);
- `xxxESWyz` are end-switches values (`y==0` -- linear stage, `y==1` -- rotator; `z==0` -- zero's end-switch, `z==1` -- limiting end-switch).


**Warning**! Be careful: positions of rotators have different values. When you call this tool to move any rotator to given angle, you should point this angle in degrees. When you get state of device, you will see this angle in steps! These parameters defined in `tty_procs.h`:

	#define STEPSREV1           (36000)
	#define STEPSREV2           (28800)
	
This mean that polaroid rotator have 36000 steps per revolution (100 steps per degree), wave-plate rotator have 28800 steps per revolution (80 steps per degree).