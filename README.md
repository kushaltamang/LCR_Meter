Designed a system capable of measuring resistance, inductance (and ESR), and capacitance. A design goal of this project is to limit the total cost of the daughterboard and components added to the TM4C123GXL evaluation board to $3 in 10k quantities. The virtual COM port on the evaluation board talks with PUTTY to provide a command line user interface.

- Five pins (MEAS_LR, MEAS_C, HIGHSIDE_R, LOWSIDE_R, INTEGRATE) connects the circuit to the RedBoard using GPIO pins (PA2, PD6, PA6, PD0, and PA7 respectively). The 5 GPIO pins are connected to the 5 transistors that turn on/off depedning upon the DUT(inductor, capacitor, reisstor).
- The DUT is connected to the comparator internal to the RedBoard using the PC7 pin. The comparator is set up to trigger an interrupt when the PC7 pin reaches Vref = 2.469 V. 
- The 4 diodes are installed for protection from large voltages destroying the device.
- The program can also automatically detect whether the DUT is an inductor, a capacitor or a reistor.

MEASURING RESISTANCE:
- To measure resistance, MEAS_C and HIGHSIDE_R are turned off as we do not need those circuits while measuring resistance. INTEGRATE is turned on for capacitor(1uF) to charge/discharge. 
- We create a LOWSIDE_R pulse by turning LOWISDE_R on, MEAS_LR off and turning LOWSIDE_R off after some time that discharges the 1uF capacitor. 
- Then, we start the timer and turn on MEAS_LR that charges the capacitor, and the comparator interrupt is triggered when the voltages reaches 2.469 V. In the interrupt handler, we record this time it takes for the capacitor voltage to reach 2.469 V and this time is directly proportional to the resistance value. 

MEASURING CAPACITANCE:
- While measuring capacitance, we use a known resistance(100kΩ) and an unknown C. In this case, INTEGRSTE and MEAS_LR circuits are turned off. 
- MEAS_C is always on to charge/discharge the capacitor. 
- A LOWSIDE_R pulse is created to discharge the capacitor. 
- Then, HIGHSIDE_R is turned on that integrated the DUT(capacitor) through the 100kΩ resistor. This time it takes for the capacitor to reach 2.469V and trigger comparator interrupt is directly proportional to the capacitor value.

MEASURING INDUCTANCE (AND ESR):
- To measure inductance and its effective series resistance(ESR), MEAS_LR and LOWSIDE_R circuits are used. 
- First, we discharge the inductor by turning off the MEAS_LR and LOWSIDE_R ciruits. 
- Then, if the current through the inductor is 0A, we turn on MEAS_LR and LOWSIDE_R circuit and integrate, and when the voltage reaches 2.469V, the comparator triggers an interrupt. This time is again proportional to the inductor value. 
- As for the ESR, we charge the inductor for some time until it reaches the maximum voltage. This voltage at Vdut = 3.3*33/(33+ESR) using the voltage divider law.

MEASURING VOLTAGE:
- The Analog-to-Digital(ADC) converter internal to the RedBoard was used in order to measure the voltage at DUT.

AUTOMATIC DETECTION:
- To automatically detect whether DUT is resistor, capacitor, or an inductor, the DUT is measured in all the modes(measureResistance, measureCapacitance, measureInductance), and the initial Voltages and final Voltages are recorded to create a decision matrix. Analyzing these voltages and fine tuning them, we can automatically detect the DUT and carry out our measurements.

The device uses UART interface to transmit commands and receive output in the CLI. 
The following commands are supported:

1. auto - auto detects the DUT
2. reset – Hardware resets 
3. voltage - returns the voltage across DUT2-DUT1. The voltage is limited to 0 to 3.3V. 
4. resistor - returns the resistance of the DUT 
5. capacitance - returns the capacitance of the DUT 
6. inductance - returns the inductance of the DUT 
7. esr - returns the ESR of the inductor under test 


