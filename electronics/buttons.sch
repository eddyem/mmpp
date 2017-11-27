EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:switches
LIBS:relays
LIBS:motors
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L SW_Push SW1
U 1 1 5A1C8495
P 3500 1850
F 0 "SW1" H 3550 1950 50  0000 L CNN
F 1 "SW_Push" H 3500 1790 50  0000 C CNN
F 2 "Buttons_Switches_THT.pretty:SW_PUSH_6mm_h13mm" H 3500 2050 50  0001 C CNN
F 3 "" H 3500 2050 50  0001 C CNN
	1    3500 1850
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x02 J1
U 1 1 5A1C86CD
P 4100 1850
F 0 "J1" H 4100 1950 50  0000 C CNN
F 1 "Conn_01x02" H 4100 1650 50  0000 C CNN
F 2 "my_footprints:SMD2pin" H 4100 1850 50  0001 C CNN
F 3 "" H 4100 1850 50  0001 C CNN
	1    4100 1850
	1    0    0    -1  
$EndComp
Wire Wire Line
	3900 1850 3700 1850
Wire Wire Line
	3300 1850 3300 1950
Wire Wire Line
	3300 1950 3900 1950
$Comp
L SW_Push SW2
U 1 1 5A1C8853
P 3500 2300
F 0 "SW2" H 3550 2400 50  0000 L CNN
F 1 "SW_Push" H 3500 2240 50  0000 C CNN
F 2 "Buttons_Switches_THT.pretty:SW_PUSH_6mm_h13mm" H 3500 2500 50  0001 C CNN
F 3 "" H 3500 2500 50  0001 C CNN
	1    3500 2300
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x02 J2
U 1 1 5A1C8859
P 4100 2300
F 0 "J2" H 4100 2400 50  0000 C CNN
F 1 "Conn_01x02" H 4100 2100 50  0000 C CNN
F 2 "my_footprints:SMD2pin" H 4100 2300 50  0001 C CNN
F 3 "" H 4100 2300 50  0001 C CNN
	1    4100 2300
	1    0    0    -1  
$EndComp
Wire Wire Line
	3900 2300 3700 2300
Wire Wire Line
	3300 2300 3300 2400
Wire Wire Line
	3300 2400 3900 2400
$Comp
L SW_Push SW3
U 1 1 5A1C8B9A
P 3500 2750
F 0 "SW3" H 3550 2850 50  0000 L CNN
F 1 "SW_Push" H 3500 2690 50  0000 C CNN
F 2 "Buttons_Switches_THT.pretty:SW_PUSH_6mm_h13mm" H 3500 2950 50  0001 C CNN
F 3 "" H 3500 2950 50  0001 C CNN
	1    3500 2750
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x02 J3
U 1 1 5A1C8BA0
P 4100 2750
F 0 "J3" H 4100 2850 50  0000 C CNN
F 1 "Conn_01x02" H 4100 2550 50  0000 C CNN
F 2 "my_footprints:SMD2pin" H 4100 2750 50  0001 C CNN
F 3 "" H 4100 2750 50  0001 C CNN
	1    4100 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	3900 2750 3700 2750
Wire Wire Line
	3300 2750 3300 2850
Wire Wire Line
	3300 2850 3900 2850
$Comp
L SW_Push SW4
U 1 1 5A1C8BA9
P 3500 3200
F 0 "SW4" H 3550 3300 50  0000 L CNN
F 1 "SW_Push" H 3500 3140 50  0000 C CNN
F 2 "Buttons_Switches_THT.pretty:SW_PUSH_6mm_h13mm" H 3500 3400 50  0001 C CNN
F 3 "" H 3500 3400 50  0001 C CNN
	1    3500 3200
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x02 J4
U 1 1 5A1C8BAF
P 4100 3200
F 0 "J4" H 4100 3300 50  0000 C CNN
F 1 "Conn_01x02" H 4100 3000 50  0000 C CNN
F 2 "my_footprints:SMD2pin" H 4100 3200 50  0001 C CNN
F 3 "" H 4100 3200 50  0001 C CNN
	1    4100 3200
	1    0    0    -1  
$EndComp
Wire Wire Line
	3900 3200 3700 3200
Wire Wire Line
	3300 3200 3300 3300
Wire Wire Line
	3300 3300 3900 3300
$EndSCHEMATC
