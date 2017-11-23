EESchema Schematic File Version 2
LIBS:steppers-rescue
LIBS:conn
LIBS:device
LIBS:ESD_Protection
LIBS:linear
LIBS:microcontrollers
LIBS:power
LIBS:regul
LIBS:relays
LIBS:sensors
LIBS:stm32
LIBS:texas
LIBS:transistors
LIBS:switches
LIBS:drv8825
LIBS:steppers-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 2
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
L DRV8825 U4
U 1 1 5A184D34
P 5050 2950
F 0 "U4" H 5050 2250 60  0000 C CNN
F 1 "DRV8825" H 5050 2350 60  0000 C CNN
F 2 "" H 5100 2850 60  0000 C CNN
F 3 "" H 5100 2850 60  0000 C CNN
	1    5050 2950
	1    0    0    -1  
$EndComp
$Comp
L C_Small C8
U 1 1 5A184EF2
P 5050 2200
F 0 "C8" H 5060 2270 50  0000 L CNN
F 1 "10n" H 5060 2120 50  0000 L CNN
F 2 "" H 5050 2200 50  0001 C CNN
F 3 "" H 5050 2200 50  0001 C CNN
	1    5050 2200
	0    1    1    0   
$EndComp
$Comp
L C_Small C10
U 1 1 5A184FC5
P 5700 2600
F 0 "C10" H 5710 2670 50  0000 L CNN
F 1 "0.1u" H 5710 2520 50  0000 L CNN
F 2 "" H 5700 2600 50  0001 C CNN
F 3 "" H 5700 2600 50  0001 C CNN
	1    5700 2600
	-1   0    0    1   
$EndComp
$Comp
L Conn_01x08 J6
U 1 1 5A1850DE
P 6800 2800
F 0 "J6" H 6800 3200 50  0000 C CNN
F 1 "Conn_01x08" H 6800 2300 50  0000 C CNN
F 2 "" H 6800 2800 50  0001 C CNN
F 3 "" H 6800 2800 50  0001 C CNN
	1    6800 2800
	1    0    0    -1  
$EndComp
$Comp
L Conn_01x08 J5
U 1 1 5A1851C3
P 3550 2700
F 0 "J5" H 3550 3100 50  0000 C CNN
F 1 "Conn_01x08" H 3550 2200 50  0000 C CNN
F 2 "" H 3550 2700 50  0001 C CNN
F 3 "" H 3550 2700 50  0001 C CNN
	1    3550 2700
	-1   0    0    -1  
$EndComp
$Comp
L R R14
U 1 1 5A185403
P 6100 2300
F 0 "R14" V 6180 2300 50  0000 C CNN
F 1 "10k" V 6100 2300 50  0000 C CNN
F 2 "" V 6030 2300 50  0001 C CNN
F 3 "" H 6100 2300 50  0001 C CNN
	1    6100 2300
	0    1    1    0   
$EndComp
$Comp
L R R11
U 1 1 5A18545F
P 5700 3100
F 0 "R11" V 5780 3100 50  0000 C CNN
F 1 "1k5" V 5700 3100 50  0000 C CNN
F 2 "" V 5630 3100 50  0001 C CNN
F 3 "" H 5700 3100 50  0001 C CNN
	1    5700 3100
	0    1    1    0   
$EndComp
$Comp
L R R13
U 1 1 5A1854BE
P 5950 3650
F 0 "R13" V 6030 3650 50  0000 C CNN
F 1 "0.1R" V 5950 3650 50  0000 C CNN
F 2 "" V 5880 3650 50  0001 C CNN
F 3 "" H 5950 3650 50  0001 C CNN
	1    5950 3650
	-1   0    0    1   
$EndComp
$Comp
L R R12
U 1 1 5A185515
P 5750 3650
F 0 "R12" V 5830 3650 50  0000 C CNN
F 1 "0.1R" V 5750 3650 50  0000 C CNN
F 2 "" V 5680 3650 50  0001 C CNN
F 3 "" H 5750 3650 50  0001 C CNN
	1    5750 3650
	-1   0    0    1   
$EndComp
$Comp
L C_Small C9
U 1 1 5A18554B
P 5550 3650
F 0 "C9" H 5560 3720 50  0000 L CNN
F 1 "0.47u" H 5560 3570 50  0000 L CNN
F 2 "" H 5550 3650 50  0001 C CNN
F 3 "" H 5550 3650 50  0001 C CNN
	1    5550 3650
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR018
U 1 1 5A185913
P 5550 3800
F 0 "#PWR018" H 5550 3550 50  0001 C CNN
F 1 "GND" H 5550 3650 50  0000 C CNN
F 2 "" H 5550 3800 50  0001 C CNN
F 3 "" H 5550 3800 50  0001 C CNN
	1    5550 3800
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR019
U 1 1 5A1861DF
P 6050 3350
F 0 "#PWR019" H 6050 3100 50  0001 C CNN
F 1 "GND" H 6050 3200 50  0000 C CNN
F 2 "" H 6050 3350 50  0001 C CNN
F 3 "" H 6050 3350 50  0001 C CNN
	1    6050 3350
	1    0    0    -1  
$EndComp
Text Label 6350 2300 0    60   ~ 0
~SLP
Text Label 4500 2600 2    60   ~ 0
~SLP
Text HLabel 6400 2500 0    60   Input ~ 0
WMM
Text HLabel 6400 2600 0    60   Input ~ 0
GND
NoConn ~ 5450 3050
Text HLabel 6400 2700 0    60   Output ~ 0
B2
Text HLabel 6400 2800 0    60   Output ~ 0
B1
Text HLabel 6400 2900 0    60   Output ~ 0
A1
Text HLabel 6400 3000 0    60   Output ~ 0
A2
Text HLabel 6400 3100 0    60   Output ~ 0
~FAULT
Text Label 5500 2450 0    60   ~ 0
WMM
Text Label 4550 3950 0    60   ~ 0
WMM
$Comp
L C_Small C4
U 1 1 5A186907
P 4650 4200
F 0 "C4" H 4660 4270 50  0000 L CNN
F 1 "0.1u" H 4660 4120 50  0000 L CNN
F 2 "" H 4650 4200 50  0001 C CNN
F 3 "" H 4650 4200 50  0001 C CNN
	1    4650 4200
	-1   0    0    1   
$EndComp
$Comp
L C_Small C7
U 1 1 5A18698C
P 4900 4200
F 0 "C7" H 4910 4270 50  0000 L CNN
F 1 "0.1u" H 4910 4120 50  0000 L CNN
F 2 "" H 4900 4200 50  0001 C CNN
F 3 "" H 4900 4200 50  0001 C CNN
	1    4900 4200
	-1   0    0    1   
$EndComp
$Comp
L C_Small C3
U 1 1 5A1869D0
P 4400 4200
F 0 "C3" H 4410 4270 50  0000 L CNN
F 1 "4.7u" H 4410 4120 50  0000 L CNN
F 2 "" H 4400 4200 50  0001 C CNN
F 3 "" H 4400 4200 50  0001 C CNN
	1    4400 4200
	-1   0    0    1   
$EndComp
$Comp
L GND #PWR020
U 1 1 5A186BE0
P 4650 4350
F 0 "#PWR020" H 4650 4100 50  0001 C CNN
F 1 "GND" H 4650 4200 50  0000 C CNN
F 2 "" H 4650 4350 50  0001 C CNN
F 3 "" H 4650 4350 50  0001 C CNN
	1    4650 4350
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR021
U 1 1 5A186EF5
P 4500 3650
F 0 "#PWR021" H 4500 3400 50  0001 C CNN
F 1 "GND" H 4500 3500 50  0000 C CNN
F 2 "" H 4500 3650 50  0001 C CNN
F 3 "" H 4500 3650 50  0001 C CNN
	1    4500 3650
	1    0    0    -1  
$EndComp
$Comp
L POT RV1
U 1 1 5A187154
P 4200 3400
F 0 "RV1" V 4025 3400 50  0000 C CNN
F 1 "10k" V 4100 3400 50  0000 C CNN
F 2 "" H 4200 3400 50  0001 C CNN
F 3 "" H 4200 3400 50  0001 C CNN
	1    4200 3400
	1    0    0    -1  
$EndComp
Text Label 5550 3450 0    60   ~ 0
3v3
Text Label 4200 3200 0    60   ~ 0
3v3
NoConn ~ 4650 3000
Wire Wire Line
	5150 2350 5200 2350
Wire Wire Line
	4900 2350 4950 2350
Wire Wire Line
	5150 2200 5200 2200
Wire Wire Line
	5200 2200 5200 2350
Wire Wire Line
	4950 2200 4900 2200
Wire Wire Line
	4900 2200 4900 2350
Wire Wire Line
	5450 2700 5700 2700
Wire Wire Line
	5450 2450 5450 2550
Wire Wire Line
	5450 2450 5700 2450
Wire Wire Line
	5700 2450 5700 2500
Connection ~ 5450 2500
Wire Wire Line
	5550 3550 5550 3450
Wire Wire Line
	5550 3450 5450 3450
Wire Wire Line
	5550 3800 5550 3750
Wire Wire Line
	5750 3500 5750 3350
Wire Wire Line
	5750 3350 5450 3350
Wire Wire Line
	5450 3250 5950 3250
Wire Wire Line
	5950 3250 5950 3500
Wire Wire Line
	5550 3800 5950 3800
Connection ~ 5750 3800
Connection ~ 5550 3800
Wire Wire Line
	5450 3150 5550 3150
Wire Wire Line
	5550 3150 5550 3100
Wire Wire Line
	6000 3200 6600 3200
Wire Wire Line
	6000 3200 6000 2600
Wire Wire Line
	6000 2600 6600 2600
Wire Wire Line
	6050 3200 6050 3350
Connection ~ 6050 3200
Wire Wire Line
	5850 3100 6600 3100
Wire Wire Line
	5950 3100 5900 3100
Wire Wire Line
	5950 2300 5950 3100
Connection ~ 5900 3100
Wire Wire Line
	6250 2300 6350 2300
Wire Wire Line
	3750 2600 4650 2600
Wire Wire Line
	5700 2500 6600 2500
Connection ~ 5700 2500
Wire Wire Line
	6600 2700 5800 2700
Wire Wire Line
	5800 2700 5800 2800
Wire Wire Line
	5800 2800 5450 2800
Wire Wire Line
	5450 2850 5850 2850
Wire Wire Line
	5850 2850 5850 2800
Wire Wire Line
	5850 2800 6600 2800
Wire Wire Line
	6600 2900 5450 2900
Wire Wire Line
	5450 2950 6050 2950
Wire Wire Line
	6050 2950 6050 3000
Wire Wire Line
	6050 3000 6600 3000
Wire Wire Line
	4400 4100 4900 4100
Wire Wire Line
	4550 4100 4550 3950
Connection ~ 4550 4100
Connection ~ 4650 4100
Wire Wire Line
	4400 4300 4900 4300
Wire Wire Line
	4650 4300 4650 4350
Connection ~ 4650 4300
Wire Wire Line
	4500 3400 4500 3650
Wire Wire Line
	4500 3600 4200 3600
Wire Wire Line
	4650 3350 4650 3450
Connection ~ 4650 3400
Wire Wire Line
	4200 3600 4200 3550
Connection ~ 4500 3600
Wire Wire Line
	4450 3400 4350 3400
Wire Wire Line
	4450 3150 4450 3400
Wire Wire Line
	4200 3200 4200 3250
Wire Wire Line
	4650 2500 4650 2400
Wire Wire Line
	4650 2400 3750 2400
Wire Wire Line
	3750 2500 4550 2500
Wire Wire Line
	4550 2500 4550 2550
Wire Wire Line
	4550 2550 4650 2550
Wire Wire Line
	3750 2700 4600 2700
Wire Wire Line
	4600 2700 4600 2650
Wire Wire Line
	4600 2650 4650 2650
Wire Wire Line
	4450 3150 4650 3150
Wire Wire Line
	4650 3150 4650 3100
Connection ~ 4650 3150
Wire Wire Line
	4650 3400 4500 3400
Wire Wire Line
	3750 3100 4600 3100
Wire Wire Line
	4600 3100 4600 2950
Wire Wire Line
	4600 2950 4650 2950
Wire Wire Line
	3750 3000 4550 3000
Wire Wire Line
	4550 3000 4550 2850
Wire Wire Line
	4550 2850 4650 2850
Wire Wire Line
	4650 2800 4500 2800
Wire Wire Line
	4500 2800 4500 2900
Wire Wire Line
	4500 2900 3750 2900
Wire Wire Line
	3750 2800 4450 2800
Wire Wire Line
	4450 2800 4450 2750
Wire Wire Line
	4450 2750 4650 2750
Text HLabel 3800 2400 2    60   Input ~ 0
DIR
Text HLabel 3800 2500 2    60   Input ~ 0
STEP
Text HLabel 3800 2600 2    60   Input ~ 0
~SLEEP
Text HLabel 3800 2700 2    60   Input ~ 0
~RESET
Text HLabel 3800 2800 2    60   Input ~ 0
M2
Text HLabel 3800 2900 2    60   Input ~ 0
M1
Text HLabel 3800 3000 2    60   Input ~ 0
M0
Text HLabel 3800 3100 2    60   Input ~ 0
~EN
$EndSCHEMATC
