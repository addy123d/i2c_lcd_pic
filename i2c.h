/* 
 * File:   i2c.h
 * Author: sudar
 *
 * Created on May 30, 2023, 3:18 PM
 */

#ifndef I2C_H
#define	I2C_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <xc.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
    
/*********** G E N E R A L   D E F I N E S ************************************/
#define I2C_INPUT   1
#define I2C_LOW     0
#define I2C_HIGH    1
#define I2C_SPEED	100         // Define i2c speed kbps
    
/*********** P O R T   D E F I N E S ******************************************/
    #define SDA2        RB2				// Data pin for i2c
    #define SCK2        RB1				// Clock pin for i2c
    #define SDA2_DIR    TRISBbits.RB2   // Data pin direction
    #define SCK2_DIR	TRISBbits.RB1	// Clock pin direction  

/*********** P R O T O T Y P E S **********************************************/
void I2C2_Init(void);
void I2C2_Start(void);
void I2C2_ReStart(void);
void I2C2_Stop(void);
void I2C2_Send_ACK(void);
void I2C2_Send_NACK(void);
unsigned char I2C2_Send(unsigned char BYTE);
unsigned char I2C2_Read(void);



#ifdef	__cplusplus
}
#endif

#endif	/* I2C_H */

