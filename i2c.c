/*
 * File:   i2c.c
 * Author: sudar
 *
 * Created on May 30, 2023, 3:20 PM
 */


#include <xc.h>
#include "i2c.h"

/*******************************************************************************
 * Function:        void I2C2_Init(void)
 * Description:     Configure I2C module
 * Precondition:    None
 * Parameters:      None
 * Return Values:   None
 * Remarks:         None
 ******************************************************************************/
void I2C2_Init(void){	
    SDA2_DIR = I2C_INPUT;        
    SCK2_DIR = I2C_INPUT;	
    
    
    ANSELBbits.ANSB2 = 0; //configure the input pin RB2 as digital (very important)
    ANSELBbits.ANSB1 = 0; //configure the input pin RB1 as digital (very important)
    
    SSP2STAT = 0b10000000;		// Slew Rate control is disabled
     /*
   * WCOL:0
   * SSPOV:0
   * SSPEN:1 -> Enables Serial Port & configures the SDA and SCL as serial port pins
   * CKP:0
   * SSPM3:SSPM0:1000 -> I2C Master Mode, clock=FOSC/(4*(SSPADD+1))
   * FOSC = 64MHZ, and to get a clock of 100Khz => SSPADD = 159u
   */	
	SSP2CON1 = 0b00101000;		// Select and enable I2C in master mode
    SSP2ADD  = 159u; //((_XTAL_FREQ/4000)/I2C_SPEED) - 1;	
}



/*******************************************************************************
 * Function:        void I2C_Start(void)
 * Description:     Sends start bit sequence
 * Precondition:    None
 * Parameters:      None
 * Return Values:   None
 * Remarks:         None
 ******************************************************************************/
void I2C2_Start(void){
	SSP2CON2bits.SEN = I2C_HIGH;			
	while(!PIR3bits.SSP2IF);		
	PIR3bits.SSP2IF = I2C_LOW;	
}



/*******************************************************************************
 * Function:        void I2C_ReStart(void)
 * Description:     Sends restart bit sequence
 * Precondition:    None
 * Parameters:      None
 * Return Values:   None
 * Remarks:         None
 ******************************************************************************/
void I2C2_ReStart(void){
	SSP2CON2bits.RSEN = I2C_HIGH;			
	while(!PIR3bits.SSP2IF);		
	PIR3bits.SSP2IF = I2C_LOW;			
}


/*******************************************************************************
 * Function:        void I2C_Stop(void)
 * Description:     Sends stop bit sequence
 * Precondition:    None
 * Parameters:      None
 * Return Values:   None
 * Remarks:         None
 ******************************************************************************/
void I2C2_Stop(void){
	SSP2CON2bits.PEN = 1;			
	while(!PIR3bits.SSP2IF);		
	PIR3bits.SSP2IF = 0;				
}



/*******************************************************************************
 * Function:        void I2C_Send_ACK(void)
 * Description:     Sends ACK bit sequence
 * Precondition:    None
 * Parameters:      None
 * Return Values:   None
 * Remarks:         None
 ******************************************************************************/
void I2C2_Send_ACK(void){
	SSP2CON2bits.ACKDT = I2C_LOW;			
	SSP2CON2bits.ACKEN = I2C_HIGH;			
	while(!PIR3bits.SSP2IF);		
	PIR3bits.SSP2IF = I2C_LOW;			
}


/*******************************************************************************
 * Function:        void I2C_Send_NACK(void)
 * Description:     Sends NACK bit sequence
 * Precondition:    None
 * Parameters:      None
 * Return Values:   None
 * Remarks:         None
 ******************************************************************************/
void I2C2_Send_NACK(void){
	SSP2CON2bits.ACKDT = I2C_HIGH;			
	SSP2CON2bits.ACKEN = I2C_HIGH;			
	while(!PIR3bits.SSP2IF);		
	PIR3bits.SSP2IF = I2C_LOW;	
}



/*******************************************************************************
 * Function:        unsigned char I2C_Write(unsigned char BYTE)
 * Description:     Transfers one byte
 * Precondition:    None
 * Parameters:      BYTE = Value for slave device
 * Return Values:   Return ACK/NACK from slave
 * Remarks:         None
 ******************************************************************************/
unsigned char I2C2_Send(unsigned char BYTE){
	SSP2BUF = BYTE;                 
	while(!PIR3bits.SSP2IF);		
	PIR3bits.SSP2IF = I2C_LOW;			
	return SSP2CON2bits.ACKSTAT;    
}


/*******************************************************************************
 * Function:        unsigned char I2C_Read(void)
 * Description:     Reads one byte
 * Precondition:    None
 * Parameters:      None
 * Return Values:   Return received byte
 * Remarks:         None
 ******************************************************************************/
unsigned char I2C2_Read(void){
	SSP2CON2bits.RCEN = I2C_HIGH;			
	while(!PIR3bits.SSP2IF);		
	PIR3bits.SSP2IF = I2C_LOW;			
    return SSP2BUF;      
}