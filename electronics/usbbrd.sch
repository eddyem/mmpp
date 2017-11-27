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
LIBS:usbbrd-cache
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
L USB_B J1
U 1 1 5A1C0904
P 5300 2550
F 0 "J1" H 5100 3000 50  0000 L CNN
F 1 "USB_B" H 5100 2900 50  0000 L CNN
F 2 "Connectors.pretty:USB_B" H 5450 2500 50  0001 C CNN
F 3 "" H 5450 2500 50  0001 C CNN
	1    5300 2550
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x05 J2
U 1 1 5A1C0988
P 6250 2550
F 0 "J2" H 6250 2850 50  0000 C CNN
F 1 "Conn_01x05" H 6250 2250 50  0000 C CNN
F 2 "my_footprints:SMD5pin" H 6250 2550 50  0001 C CNN
F 3 "" H 6250 2550 50  0001 C CNN
	1    6250 2550
	1    0    0    -1  
$EndComp
NoConn ~ 5200 2950
NoConn ~ 6050 2550
Wire Wire Line
	5750 2350 5750 2650
Wire Wire Line
	5750 2650 6050 2650
Wire Wire Line
	5300 2950 5900 2950
Wire Wire Line
	5900 2950 5900 2750
Wire Wire Line
	5900 2750 6050 2750
Wire Wire Line
	5600 2550 5850 2550
Wire Wire Line
	5850 2550 5850 2350
Wire Wire Line
	5850 2350 6050 2350
Wire Wire Line
	6050 2450 5950 2450
Wire Wire Line
	5950 2450 5950 2700
Wire Wire Line
	5950 2700 5600 2700
Wire Wire Line
	5600 2700 5600 2650
Wire Wire Line
	5600 2350 5750 2350
$EndSCHEMATC
