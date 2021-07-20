//Mohit Tamang 
//LCR Meter

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "uart0.h"
#include "wait.h"
#include "adc0.h"
#include "parse.h"

//Pins
#define MEAS_LR PORTA,2
#define HIGHSIDE_R PORTA,6
#define INTEGRATE PORTA,7
#define VREF PORTC,6 //Analog comparator 0 positive input(to be set as VREF = 2.469V) (C0+)
#define C0neg PORTC,7 //Analog comparator 0 negative input (C0-)
#define PE0 PORTE,0
#define LOWSIDE_R PORTD,0
#define MEAS_C PORTD,6
#define GREEN_LED PORTF,3

//Global Variables
char ch;
uint16_t k_res = 58; uint32_t k_cap = 5100000; float k_ind = 12100; char str[100]; bool ok = true;
uint32_t inductance;

// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks for ports
    enablePort(PORTA);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTF);

    // Configure pins
    selectPinPushPullOutput(MEAS_LR);
    selectPinPushPullOutput(MEAS_C);
    selectPinPushPullOutput(LOWSIDE_R);
    selectPinPushPullOutput(HIGHSIDE_R);
    selectPinPushPullOutput(INTEGRATE);
    selectPinPushPullOutput(GREEN_LED);
}

void initTimer1()
{
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;  //turn on clocks for Timer1
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD | TIMER_TAMR_TACDIR;// configure for periodic mode and count up
    TIMER1_IMR_R &= ~TIMER_IMR_TATOIM;              // turn off interrupts
}

void initComparator0()
{
    //Comparator initialization
    SYSCTL_RCGCACMP_R |= SYSCTL_RCGCACMP_R0;//turn on clocks for Analog comparator 0
    selectPinAnalogInput(C0neg);//set alternative analog function for PC7 (C0-)
    //Comparator Configuration

    COMP_ACREFCTL_R |= 0x0000020F;//set VREF = 2.469V, EN=1, RNG=0
    /*Internal VREF is set as V+, comparator output inverted, Interrupt generated when output is high*/
    COMP_ACCTL0_R |= COMP_ACCTL0_ASRCP_REF | COMP_ACCTL0_CINV | COMP_ACCTL0_ISLVAL| COMP_ACCTL0_ISEN_RISE;
    waitMicrosecond(10);// delay for 10 us
    COMP_ACMIS_R |= 1;
    COMP_ACRIS_R |= COMP_ACRIS_IN0;
    NVIC_EN0_R |= 1<<(INT_COMP0 - 16);//Select and enable Interrupt Handler for Comparator Interrupt
    COMP_ACINTEN_R &= ~1;//Enable Comparator 0 Interrupt
}

float measureVoltage()
{
    uint16_t raw; float voltage;
    selectPinAnalogInput(PE0);//configure PE0 as analog input
    initAdc0Ss3();// Initialize ADC0
    // Use PC7 analog input with N=4 hardware sampling
    setAdc0Ss3Mux(3);
    setAdc0Ss3Log2AverageCount(2);
    raw = readAdc0Ss3();// Read sensor
    voltage = ((raw+0.5)/4096)*3.3;
    //sprintf(str, "\nVoltage: %0.2f V\n",voltage);
    //putsUart0(str);
    return voltage;
}

void measureResistance()
{
    //set MEAS_C & HIGHSIDE_R to 0 as we do not need to use those circuits to measure resistance
    setPinValue(MEAS_C, 0);
    setPinValue(HIGHSIDE_R, 0);

    //INTEGRATE is always on, for capacitor to charge/discharge
    setPinValue(INTEGRATE, 1);

    //Discharge the capacitor to 0V by creating a 9ms LOWSIDE_R pulse (DEINTEGRATE)
    setPinValue(MEAS_LR, 0);//turn off MEAS_LR circuit
    setPinValue(LOWSIDE_R, 1);//High
    waitMicrosecond(9000);//wait 9ms
    setPinValue(LOWSIDE_R, 0);//Low

    COMP_ACINTEN_R |= 1;//Enable Comparator 0 Interrupt
    //initanalogcomp0();

    //Charge the capacitor through MEAS_LR circuit and start the timer (INTEGRATE)
    setPinValue(MEAS_LR, 1);//MEAS_LR turns on, starts charging the capacitor

    //start Timer1 to record the time it takes for the capacitor to charge up to 2.469V
    TIMER1_TAV_R = 0;
    TIMER1_CTL_R |= TIMER_CTL_TAEN;
}

void measureCapacitance()
{
    //set MEAS_LR & INTEGRATE to 0 as we do not need to use those circuits to measure capacitance
    setPinValue(MEAS_LR,0);
    setPinValue(INTEGRATE, 0);

    setPinValue(MEAS_C,1); //MEAS_C is always on, for capacitor to charge/discharge

    //Discharge the capacitor to 0V by creating a 9ms LOWSIDE_R pulse (DEINTEGRATE)
    setPinValue(HIGHSIDE_R,0); //turn off HIGHSIDE_R circuit
    setPinValue(LOWSIDE_R, 1);//High
    waitMicrosecond(9000);//wait 9ms
    setPinValue(LOWSIDE_R, 0);//Low

    COMP_ACINTEN_R |= 1;//Enable Comparator 0 Interrupt

    //Charge the capacitor through HIGHSIDE_R circuit and start the timer (INTEGRATE)
    setPinValue(HIGHSIDE_R, 1);//HIGHSIDE_R turns on, starts charging the capacitor

    //start Timer1 to record the time it takes for the capacitor to charge up to 2.469V
    TIMER1_TAV_R = 0;
    TIMER1_CTL_R |= TIMER_CTL_TAEN;
}

void measureInductance()
{
    float V;
    //Set MEAS_C and LOWSIDE_R to 1 and wait some time to discharge the inductor
    setPinValue(MEAS_C,1);
    setPinValue(LOWSIDE_R, 1);

    waitMicrosecond(1000000);//wait 1000ms
    measureVoltage();
    setPinValue(MEAS_C,0);

    COMP_ACMIS_R |= 1;
    COMP_ACINTEN_R |= 1;//Enable Comparator 0 Interrupt

    //Turn on MEAS_LR and LOWSIDE_R to integrate
    setPinValue(MEAS_LR,1);
    setPinValue(LOWSIDE_R,1);

    //start Timer1 to record the time it takes for the capacitor to charge up to 2.469V
    TIMER1_TAV_R = 0;
    TIMER1_CTL_R |= TIMER_CTL_TAEN;

    waitMicrosecond(1000000);         // wait for 2 sec
    V = measureVoltage();
    if((V <= 2 && V > 1.5) && ch != 'a')
    {
        inductance = 10;
        sprintf(str, "Inductance: %u mH\n",inductance);
        putsUart0(str);
    }
    if(V > 2 && V < 2.469 && ch != 'a')
    {
        inductance = 1;
        sprintf(str, "Inductance: %u mH\n",inductance);
        putsUart0(str);
    }

}


float measureESR()
{
    float Vin = 3.3; float Vdut2; float esr;

    //Set MEAS_LR and LOWSIDE_R to 0 and wait some time to discharge the inductor
    setPinValue(MEAS_LR,0);
    setPinValue(LOWSIDE_R, 0);

    waitMicrosecond(100000);//wait 100ms

    //Turn on MEAS_LR and LOWSIDE_R to integrate
    setPinValue(MEAS_LR,1);
    setPinValue(LOWSIDE_R,1);

    waitMicrosecond(100000);

    Vdut2 = measureVoltage(); //measure voltage at Vdut2

    esr = (33 * ((Vin - Vdut2) / Vdut2));//calculate the ESR value
    if(ch != 'l')
    {
        sprintf(str, "ESR: %0.2f ohms\n",esr);
        putsUart0(str);
    }
    return esr;
}

void disablePins()
{
    setPinValue(MEAS_LR,0);
    setPinValue(LOWSIDE_R, 0);
    setPinValue(MEAS_C,0);
    setPinValue(HIGHSIDE_R, 0);
    setPinValue(INTEGRATE,0);

}

void measureAuto()
{
    disablePins();
    waitMicrosecond(1000000);
    float initV; float finalV;
    initV = measureVoltage();
    measureResistance();
    waitMicrosecond(1000000);
    finalV = measureVoltage();
    putsUart0("DUT: ");
    if(initV < 0.1 | initV > 2.5)
    {
        //putsUart0("R or C");
        measureCapacitance();
        finalV = measureVoltage();
        putsUart0("CAPACITOR\n");
        ok = false;
        ch = 'c';
    }
    else if(finalV > 3)
    {
        measureInductance();
        finalV = measureVoltage();
        if(finalV < 1)
        {
            putsUart0("RESISTOR\n");
            ok = false;
            ch = 'r';
        }
        else
        {
            putsUart0("INDUCTOR\n");
            measureESR();
            ok = false;
            ch = 'l';
        }
    }
    else
    {
        putsUart0("CAPACITOR\n");
    }
}


//When capacitor charges up to 2.469V, comparator interrupt gets triggered, and stop the timer
void comparator0ISR()
{
    //measureVoltage();
    uint32_t timer;
    uint32_t resistance;uint32_t capacitance;

    TIMER1_CTL_R &= ~(TIMER_CTL_TAEN);//Stop the timer
    timer = (TIMER1_TAV_R); // get the timer value
    NVIC_EN0_R &= ~(1<<(INT_COMP0 - 16));//turn off comparator interrupt
    COMP_ACINTEN_R &= ~1;//Disable Comparator 0 Interrupt

    //Print timer value
    //sprintf(str, "\nTimer: %4u\n",timer);
   // putsUart0(str);

    if(ch == 'r')
    {
        resistance = timer/k_res;
        sprintf(str, "Resistance: %4u ohms\n>",resistance);
        putsUart0(str);
    }

    if(ch == 'c')
    {
       // setPinValue(HIGHSIDE_R, 0);//turn off HIGHSIDE_R circuit to discharge the capacitor
        capacitance = timer/k_cap;
        sprintf(str, "Capacitance: %4u uF\n>",capacitance);
        putsUart0(str);
    }

    if(ch == 'l')
    {
        float esr;
        esr = measureESR();
        inductance = (k_ind*(esr+33))/timer;
        sprintf(str, "Inductance: %u uH\n>",inductance - 64);
        putsUart0(str);
    }
}

int main(void)
{
    USER_DATA data;
    //Initialize Hardware
    initHw();
    initTimer1();

    initUart0();//configures UART0
    setUart0BaudRate(115200, 40e6); // sets data-transmission rate of UART0
    putsUart0("LCR MEASUREMENT DEVICE: \n");
    while(true)
    {
        if(ok)
        {
            putsUart0(">");
            getsUart0(&data);//receives chars from the user interface, processing special characters such as backspace, and writes resultant string into the buffer
            parseFields(&data);
            if(isCommand(&data,"auto",0)) ch = 'a';
            if(isCommand(&data,"esr",0)) ch = 'e';
            if(isCommand(&data,"resistor",0)) ch = 'r';
            if(isCommand(&data,"inductance",0)) ch = 'l';
            if(isCommand(&data,"capacitance",0)) ch = 'c';
            if(isCommand(&data,"voltage",0)) ch = 'v';
            if(isCommand(&data,"reset",0)) ch = 's';

        }
        initComparator0();
        if( ch == 'r')
        {
            putsUart0("\nMEASURING RESISTANCE\n");
            measureResistance();
            ok = true;
        }

        if( ch == 'c')
        {
            putsUart0("\nMEASURING CAPACITANCE\n");
            measureCapacitance();
            ok = true;
        }

        if(ch == 'v')
        {
            float voltage;
            putsUart0("\nMEASURING VOLTAGE\n");
            voltage = measureVoltage();
            sprintf(str, "\nVoltage: %0.2f V\n",voltage);
            putsUart0(str);
        }

        if(ch =='l')
        {
            putsUart0("\nMEASURING INDUCTANCE\n");
            measureInductance();
            ok = true;
        }

        if(ch=='e')
        {
            putsUart0("\nMEASURING ESR\n");
            measureESR();
        }

        if(ch=='a')
        {
            putsUart0("\nAuto Mode\n");
            measureAuto();
        }

        if(ch =='s')
        {
            putsUart0("\nHARDWARE RESET\n");
            disablePins();
            initHw();
            initTimer1();
            initUart0();//configures UART0
            setUart0BaudRate(115200, 40e6); // sets data-transmission rate of UART0
        }
    }
}
