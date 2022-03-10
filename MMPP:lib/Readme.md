MMPPlib
==================

This is a library for MMPP (multimode photometer-polarimeter) easy management. Be careful using it in multi-threading applications because almost all its functions are **not thread-safe**!

The library functions can be divided onto two parts: turrets management and motors management. The original tools for this were [HSFW_management](https://github.com/eddyem/eddys_snippets/tree/master/HSFW_management) and [MMPP_control](https://github.com/eddyem/mmpp/tree/master/MMPP_control). Now you can easily write your own C/C++ application using this library. The two examples will facilitate this job.

## HSFW turrets management

The example files `wheels.c`, `wheelscmdlnopts.c` and `wheelscmdlnopts.h` demonstrate how you can use base functions of turrets' management.

Each wheel describes by appropriate structure:

	typedef struct{
	    int fd;         // file descriptor of device
	    char *serial;   // serial number
	    char ID;        // identificator
	    char name[9];   // wheel name
	    int maxpos;     // max position
	} wheel_descr;

The first thing you need to do when start working with turrets is to discovery them. This is done by function

	int find_wheels(wheel_descr **wheels, wheel_error *err);

This function returns amount of wheels found (or 0 if none). The `wheels` parameter is array of `wheel_descr`, one element for each wheel. The second, `err`, is error state (if wheels not found):

	typedef enum{
	     WHERR_ALLOK    // no errors
	    ,WHERR_UDEV     // udev error
	    ,WHERR_CANTOPEN // can't open file device
	} wheel_error;

You can got `WHERR_CANTOPEN` error if your rights are not sufficient to open the device file.

To properly delete memory and close devices call function

	void del_wheels(wheel_descr *w, int N);

which arguments are: `w` -- array of descriptors, and `N` -- their amount in array.

After device is opened you can work with it using functions:

	wheel_status wheel_getpos(wheel_descr *w);
	bool wheel_clear_err(wheel_descr *w);
	bool move_wheel(wheel_descr *w, int filter_pos);
	bool wheel_home(wheel_descr *w);
	
The `wheel_getpos` returns not only a position but also an error code if something went wrong:

	typedef enum{
	     WHEEL_MOVING = -2  // still moving
	    ,WHEEL_ERROR  = -1  // communication error
	    ,WHEEL_POSERR = 0   // wrong wheel position
	    ,WHEEL_POS1, WHEEL_POS2, WHEEL_POS3, WHEEL_POS4, WHEEL_POS5, WHEEL_POS6
	    ,WHEEL_POS7, WHEEL_POS8, WHEEL_POS9, WHEEL_POS10
	} wheel_status;
	
So its positive values are wheel position and negative are error codes.

Other three functions returns boolean value: `false` in case of error.
You can run `wheel_clear_err` to reset error state of device.
The `move_wheel` will rotate given wheel into position `filter_pos` (if this position isn't available or in case of other error it will return `false`). 
Sometimes you may want to home wheel manually, then call `wheel_home`.

## MMPP motors management

These are the base functions you need to manage MMPP motors:

	int mmpp_tryopen(char *dev, int spd);
	void mmpp_close();
	int mot_handshake();
	bool mot_getstatus(int Nmcu, motor_state *s);
	bool get_rst(int N, bool clear);
	bool get_alive(int N);
	int stop_all();
	int get_temp(double *t1, double *t2);
	int init_motors();
	int mot_wait();
	ttysend_status tty_sendcmd(char *cmd);
	bool get_ADC(int N, ADC_state *s);
	ttysend_status movemotor(int mcu, int motnum, int steps, int absmove);
	void reset_MCU(int N);
	
There is also three special functions, don't use them without a real need:

	char *tty_get();
	int tty_send(char *cmd);
	char* tty_sendraw(char *string);
	
Here are all base types used in those functions. The motor state:

	typedef enum{
	    STP_UNKNOWN,    // wrong state
	    STP_SLEEP,      // don't moving
	    STP_ACCEL,      // start moving with acceleration
	    STP_MOVE,       // moving with constant speed
	    STP_MVSLOW,     // moving with slowest constant speed
	    STP_DECEL,      // moving with deceleration
	    STP_STOP,       // stop motor right now (by demand)
	    STP_STOPZERO,   // stop motor and zero its position (on end-switch)
	    STP_MOVE0,      // move towards 0 endswitch (negative direction)
	    STP_MOVE1       // move towards 1 endswitch (positive direction)
	} stp_state;

Base answers on `tty_sendcmd`:

	typedef enum{
	    SEND_ERR,       // communication error
	    SEND_ALLOK,     // no errors
	    SEND_ACTIVE,    // motor is still moving
	    SEND_TOOBIG,    // amount of steps too big
	    SEND_ZEROMOVE,  // give 0 steps to move
	    SEND_ESWITCH,   // staying on end-switch & try to move further
	    SEND_NEEDINIT,  // motor needs initialisation
	    SEND_NEGATMOVE, // try to move to negative position
	    SEND_OTHER,     // unknown state
	} ttysend_status;

The end- (proximity or limit) switches' state:

	typedef enum{
	    ESW_ERROR,      // wrong value
	    ESW_RELEASED,   // opened
	    ESW_HALL,       // hall sensor
	    ESW_BUTTON      // user button
	} ESW_status;

If reset state detected, you can know the reason of reset:

	typedef enum{
	    RESET_NONE,     // no sw/wd reset occured
	    RESET_SW,       // software reset have been before last status call
	    RESET_WDG       // watchdog reset -//-
	} reset_status;
	
The state of each MCU:

	typedef struct{
	    reset_status rst;           // reset status (was MCU reseted by watchdog or software?)
	    stp_state state[2];         // status of stepper motor
	    int stepsleft[2];           // steps left to reach target position
	    int curpos[2];              // current position (negative for non-initialized state or error)
	    ESW_status ESW_status[2][2];// End-switches status, [i][j], i - motor, j - esw 0 or 1
	} motor_state;

And ADC values:

	typedef struct{
	    double Vdd;     // value of Vdd (+3.3V)
	    double Imot;    // motors' current (Amperes)
	    double Vmot;    // motors' voltage (+12V)
	} ADC_state;

So, before start working with the device you need to open it with `mmpp_tryopen`, its parameter `dev` is device name, and `spd` is communication speed (9600 by default). This function returns `0` if all OK.
Opened device at the end of work should be closed by `mmpp_close`.

Run `mot_handshake` if you doubt that all OK with MCUs, this function will return `0` if at least one MCU found and `1` if none. After running this function you can run `get_alive` to determine which MCU have connection problems (`get_alive` returns `false` if given MCU didn't answer on a handshake).

The `mot_getstatus` returns `false` if cannot communicate with given MCU. Otherwise it will fill the `s` parameter. After running of this function you can determinate reset status non only by `rst` parameter, but also by function `get_rst` (its parameter `clear` should be set to `true` if you want to clear global reset status).

To stop all movement, run `stop_all`.

Just after powering the device all motors are in uninitialized condition. To initialize them you need to move all onto zero's end-switch using `init_motors`. This function is **blocking**!!! So if you want non-blocking initialization write it yourself like at example `tm.c`. This function returns `0` if all OK, or it will return the last problem MCU and motor value as `Nmcu*10+motnum`.
Another blocking function is `mot_wait` which will blocks until all motors stopped.

To move motor run `movemotor`. It have four parameters: `mcu` (MCU number, 1 or 2), `motnum` (motor number, 0 or 1), `steps` (steps amount) and `absmove` (set it to `true` if `steps` are absolute position). Its return value can tell about possible problems.

If you want to get MCU's temperature, run `get_temp`. Its both parameters are temperature values in degrees of Celsius.
The ADC state available with`get_ADC`. 

To reset MCU just run `reset_MCU`. To make certain that reset occurred, run `mot_getstatus` and check both `curpos`, they should be `-1`.

Function`tty_sendcmd` allows to send some special commands with return status analyse. If you want to make particular communications, use `tty_send` to send command without subsequent reading or `tty_sendraw` with answer reading. To read MCU's answer separately you can call `tty_get`.

For examples of these functions usage look into files `tm.c`, `tmcmdlnopts.c` and `tmcmdlnopts.h`.
