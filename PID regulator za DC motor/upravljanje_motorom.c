/*
 * File:   newmain.c
 * Author: student
 *
 * Created on 10 February 2023, 17:05
 */


#include <xc.h>
#include <math.h>
#pragma config FOSC = HS, WDTE= OFF, PWRTE= OFF, MCLRE = ON, CP = OFF, CPD = OFF, BOREN = OFF, CLKOUTEN = OFF 
#pragma config IESO = OFF, FCMEN = OFF, WRT = OFF, VCAPEN = OFF, PLLEN = OFF, STVREN = OFF, LVP = OFF 


#define _XTAL_FREQ 8000000
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))

const char digits[]={0b01000000, 0b11111001, 0b10100100, 0b10110000, 0b10011001, 0b10010010, 0b10000010, 0b11111000, 0b100000000, 0b10010000};

unsigned long int counter_for_potentiometer=0, counter_for_tachogenerator=0;
char ready=0, ones=0, tens=0, voltage=0;
double default_value=0, measured_value=0, y=0, x=0, x1=0, x2=0, y1=0, y2=0;

void init_ports(){
    
    TRISC=0b11111000;   // pin RC1 is analog output for PWM, 
                        // pins RC0 and RC2 are for controlling LED displays
        
    TRISD=0x00; //PORTD is digital output for writing digits
    ANSELD=0x00; 
    PORTD = 0x00;
}

void init_analog(){
    TRISA=0xFF; //PORTA is analog input
    ANSELA=0b00000101;  //pins AN0 and AN1 will be used for taking data 
                        //from potenciometer and DC motors output
    
    ADCON1bits.ADFM=0;  //left aligning
    ADCON1bits.ADCS2=1;
    ADCON1bits.ADCS1=1;
    ADCON1bits.ADCS0=1;
    ADCON1bits.ADNREF=0;    //Vref- is connected to Vss
    ADCON1bits.ADPREF1=0;   //Vref+ is connected to Vdd
    ADCON1bits.ADPREF0=0;
    ADCON0bits.ADON=1;  
    ADCON0bits.CHS4=0;  //choose pin AN0
 	ADCON0bits.CHS3=0;
 	ADCON0bits.CHS2=0;
 	ADCON0bits.CHS1=0;
 	ADCON0bits.CHS0=0;
}

void init_Timer(){
    TMR0CS=0;   //internal clock
    TMR0SE=0;   //on the rising edge
    PSA=0;      //prescaler is on 
    PS2=1;      //and is 1:256
    PS1=1;
    PS0=1;
    TMR0=6;     // Timer counts to 250;         
}

void init_interrupt(){
        // Activation of AD converter interrupt
    PEIE=1;
    ADIE=1;
    ADIF=0;
        // Activation of Timer0 interrupt
    TMR0IE=1;
    TMR0IF=0;
    GIE=1;     //Global interrupt enabled
    ADGO=1;    //first AD conversion
}

void initPWMs() {
    TRISCbits.TRISC1 = 0;
    CCP2CON = 0b00001100; //frequency of PWM is 4.9kHz
    T2CON = 0b00000101;
    PR2 = 0x65;
    CCPR2L = 0; //duty cycle  
}


//INTERRUPT  
void __interrupt function(void){ // enters every 32ms
	if (TMR0IE && TMR0IF){
		TMR0IF=0;
		TMR0=6;
		if (++counter_for_potentiometer>=3) {   
            	counter_for_potentiometer=0; 
            	ADCON0bits.CHS1=0;  // taking voltage from potenciometer
            	__delay_us(5);      // delay 5us between changing pins
            	ADGO=1; // starting AD conversion
        	}
		if (++counter_for_tachogenerator>=1){
		counter_for_tachogenerator=0;
		ADCON0bits.CHS1=1;  // taking voltage from potenciometer
            	__delay_us(5);      // delay 5us between changing pins
            	ADGO=1; // starting AD conversion
		}
    }
    
	if (ADIE && ADIF){
		ADIF=0; // flag reset
		if (!counter_for_potentiometer) default_value=ADRESH*5/255.*60; // speed is calculated in Hz
		else {
			measured_value = ADRESH*5/255.*60;
        		ready=1;
		}
    	}
}

void main(void) {

    init_ports();
    init_analog();
    init_Timer();
    initPWMs();
    init_interrupt();
    
    while(1){
        
        if (ready){
	x = default_value - measured_value; // in Hz
            
            //regulatory equation
            y = y1 + 0.02923 * x - x1 * 0.0289; //value between 0 - 10V; 
			if (y < 0) y = 0;
			else if (y > 10) y = 10;
            
            CCPR2L = y * 10;    // duty cycle set from 0 to 100
            
            x1 = x;
	y1 = y;
            ready = 0;
            
        }
        voltage = ADRESH;
        
        //  WRITING VOLTAGE VALUE ON DISPLAYS
        char ones = (5.0 * voltage / 255);
        char tens = (char)(10 * (5.0 * voltage / 255 - ones));
                
        // PIN ACTIVATION ON 0
        
        PORTCbits.RC0=0;    //activate right display for tens
        PORTCbits.RC2=1;
        LATD = digits[tens];
        __delay_ms(10);

        PORTCbits.RC0=1;    //activate left display for ones
        PORTCbits.RC2=0;
        LATD = digits[ones];
        __delay_ms(10);
     
    }
    
    return;
}