/*
 * Digital Timer Firmware based on PIC18F25K22
 * 10-06-2023 by Aditya Chaudhary <ac3101282@gmail.com>
 */

#include "config.h"
#include <xc.h>
#include <math.h>
#include "i2c.h"

#define PORT 1

/*LCD Function Declarations*/
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00
#define LCD_FIRST_ROW 0x80
#define LCD_SECOND_ROW 0xC0
#define LCD_THIRD_ROW 0x94
#define LCD_FOURTH_ROW 0xD4
#define LCD_CLEAR 0x01
#define LCD_RETURN_HOME 0x02
#define LCD_ENTRY_MODE_SET 0x04
#define LCD_CURSOR_OFF 0x0C
#define LCD_UNDERLINE_ON 0x0E
#define LCD_BLINK_CURSOR_ON 0x0F
#define LCD_MOVE_CURSOR_LEFT 0x10
#define LCD_MOVE_CURSOR_RIGHT 0x14
#define LCD_TURN_ON 0x0C
#define LCD_TURN_OFF 0x08
#define LCD_SHIFT_LEFT 0x18
#define LCD_SHIFT_RIGHT 0x1E
#define LCD_TYPE 2 // 0 -> 5x7 | 1 -> 5x10 | 2 -> 2 lines

/* LCD Function Declaration */
void LCD_Init(unsigned char I2C_Add);
void IO_Expander_Write(unsigned char Data);
void LCD_Write_4Bit(unsigned char Nibble);
void LCD_CMD(unsigned char CMD);
void LCD_Set_Cursor(unsigned char ROW, unsigned char COL);
void LCD_Write_Char(char);
void LCD_Write_String(char *);
void Backlight();
void noBacklight();
void LCD_SR();
void LCD_SL();
void LCD_CLR();

unsigned char RS, i2c_add, BackLight_State = LCD_BACKLIGHT;

/*Function Declarations*/
void startUpcounter();                                         /* starts the counter from 0.0.0.0 to 9.9.9.9 and ends with OVEr */
void display(unsigned int buttonCounter, unsigned int update); /* display the stored EEPROM values. */
void seven_segment_config();                                   /* turn on all the displays. */
void seven_segment_off_config();                               /* turn off all the displays. */

/*LED Function Declarations*/
void red_led();   // turns red led on.
void green_led(); // turns green led on.
void blue_led();  // turns blue led on.

/*Timer Function Declarations*/
void stopTimer();   // stops timer with 00.00 on display.
void startTimer();  // starts timer
void stopMessage(); // display 0VEr on display.

/*EEPROM Function Declarations*/
void EEPROM_Write(unsigned char, unsigned char); /* Write byte to EEPROM */
char EEPROM_Read(unsigned char);                 /* Read byte From EEPROM */
void EEPROM_Mem_Initialise();

/* Utility Function Declaration */
unsigned char inttochar(unsigned int digit); /* converts int type to char type */
void lcd_print(unsigned char row, unsigned char col, char Data);

/*7 Segment Data array*/
unsigned char segment[11] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}, segmentCounter;
unsigned char segment_with_dot[11] = {0xBF, 0x86, 0xDB, 0xCF, 0xE6, 0xED, 0xFD, 0x87, 0xFF, 0xEF};
int hour_first_digit, hour_second_digit, minute_first_digit, minute_second_digit, DEL;
static unsigned int display_function_count = 0; // counts the number of times display function is called.

/*LCD Function Declarations*/

void LCD_Init(unsigned char I2C_Add)
{
    i2c_add = I2C_Add;
    IO_Expander_Write(0x00);
    __delay_ms(30);
    LCD_CMD(0x03);
    __delay_ms(5);
    LCD_CMD(0x03);
    __delay_ms(5);
    LCD_CMD(0x03);
    __delay_ms(5);
    LCD_CMD(LCD_RETURN_HOME);
    __delay_ms(5);
    LCD_CMD(0x20 | (LCD_TYPE << 2));
    __delay_ms(50);
    LCD_CMD(LCD_TURN_ON);
    __delay_ms(50);
    LCD_CMD(LCD_CLEAR);
    __delay_ms(50);
    LCD_CMD(LCD_ENTRY_MODE_SET | LCD_RETURN_HOME);
    __delay_ms(50);
}

void IO_Expander_Write(unsigned char Data)
{
    I2C2_Start();
    I2C2_Send(i2c_add);
    I2C2_Send(Data | BackLight_State);
    I2C2_Stop();
}

void LCD_Write_4Bit(unsigned char Nibble)
{
    // Get The RS Value To LSB OF Data
    Nibble |= RS;
    IO_Expander_Write(Nibble | 0x04);
    IO_Expander_Write(Nibble & 0xFB);
    __delay_us(50);
}

void LCD_CMD(unsigned char CMD)
{
    RS = 0; // Command Register Select
    LCD_Write_4Bit(CMD & 0xF0);
    LCD_Write_4Bit((CMD << 4) & 0xF0);
}

void LCD_Write_Char(char Data)
{
    RS = 1; // Data Register Select
    LCD_Write_4Bit(Data & 0xF0);
    LCD_Write_4Bit((Data << 4) & 0xF0);
}

void LCD_Write_String(char *Str)
{
    for (int i = 0; Str[i] != '\0'; i++)
        LCD_Write_Char(Str[i]);
}

void LCD_Set_Cursor(unsigned char ROW, unsigned char COL)
{
    switch (ROW)
    {
    case 2:
        LCD_CMD(0xC0 + COL - 1);
        break;
    case 3:
        LCD_CMD(0x94 + COL - 1);
        break;
    case 4:
        LCD_CMD(0xD4 + COL - 1);
        break;
    // Case 1
    default:
        LCD_CMD(0x80 + COL - 1);
    }
}

void Backlight()
{
    BackLight_State = LCD_BACKLIGHT;
    IO_Expander_Write(0);
}

void noBacklight()
{
    BackLight_State = LCD_NOBACKLIGHT;
    IO_Expander_Write(0);
}

void LCD_SL()
{
    LCD_CMD(0x18);
    __delay_us(40);
}

void LCD_SR()
{
    LCD_CMD(0x1C);
    __delay_us(40);
}

void LCD_CLR()
{
    LCD_CMD(0x01);
    __delay_us(40);
}

/*
 * led function definitions
 * @configuration : common anode configuration
 */
void red_led()
{
    LATAbits.LATA5 = 0; // red led

    LATAbits.LATA4 = 1;
    LATAbits.LATA6 = 1;
}

void green_led()
{

    LATAbits.LATA4 = 0; // green led

    LATAbits.LATA5 = 1;
    LATAbits.LATA6 = 1;
}

void blue_led()
{
    LATAbits.LATA6 = 0; // blue led

    LATAbits.LATA5 = 1;
    LATAbits.LATA4 = 1;
}

/*
 * 7 segment display function definitions
 */

/*
 * @desc : turn on all displays
 * @params : none.
 */
void seven_segment_config()
{
    /*
    LATAbits.LATA0 = 1;
    LATAbits.LATA1 = 1;
    LATAbits.LATA2 = 1;
    LATAbits.LATA3 = 1;
     */
}

/*
 * @desc : turn off all the displays.
 * @params : none.
 */
void seven_segment_off_config()
{
    /*
    LATAbits.LATA0 = 0;
    LATAbits.LATA1 = 0;
    LATAbits.LATA2 = 0;
    LATAbits.LATA3 = 0;
    */
}

/*
 * @desc : read data from eeprom and start the timer as usual.
 * @param : none.
 */
void startTimer()
{

    int RESET = 0;  // variable which will help to break loop, when button 3 is pressed (represents stop timer).
    int timeUp = 0; // help to break loop, also show stop message.
    int hour_first_flag = 0;
    int minute_first_flag = 0;  // variable which will reset minute digits to 59.
    int minute_second_flag = 0; // variable which will reset minute digits to 59.

    /* reset all displays */
    LATAbits.LATA0 = 0;
    LATAbits.LATA1 = 0;
    LATAbits.LATA2 = 0;
    LATAbits.LATA3 = 0;

    /* read data from eeprom */
    hour_first_digit = EEPROM_Read(0x0A);  // reads data from address location 0x0A
    hour_second_digit = EEPROM_Read(0x0B); // reads data from address location 0x0B

    for (hour_first_digit = hour_first_digit; hour_first_digit > -1; hour_first_digit--) // hour first digit
    {
        seven_segment_config(); // turn on all displays

        hour_first_flag = hour_first_flag + 1;

        if (hour_first_flag > 1)
            hour_second_digit = 9;

        for (hour_second_digit = hour_second_digit; hour_second_digit > -1; hour_second_digit--) // hour second digit
        {

            minute_first_flag = minute_first_flag + 1; // For the first time it will be 0 + 1 = 1, but if it is > 1 that means timer should start from 59

            seven_segment_config(); // turn on all displays

            if (minute_first_flag > 1) // this means minute timer should start from 59
            {
                minute_first_digit = 5; // this is important
            }
            else // Original value passed
            {
                minute_first_digit = EEPROM_Read(0x0C);
                minute_second_digit = EEPROM_Read(0x0D);
            }

            for (minute_first_digit = minute_first_digit; minute_first_digit > -1; minute_first_digit--) // minute first digit
            {
                minute_second_flag = minute_second_flag + 1;

                if (minute_second_flag > 1)
                    minute_second_digit = 9;

                // how this will get reset to 9.
                for (minute_second_digit = minute_second_digit; minute_second_digit > -1; minute_second_digit--) // minute second digit
                {
                    // check whether hour first digit and hour second digit is zero or not.
                    // TIME OVER condition.
                    if ((hour_first_digit == 0) && (hour_second_digit == 0) && (minute_first_digit == 0) && (minute_second_digit == 0))
                    {
                        timeUp = 1; // we will use different variable to represent time over.
                        break;
                    }

                    for (DEL = 4990; DEL > 0; DEL--) // To create approximate 60 second delay, we need to pass 4975, as we are running with 64MHz.
                    {

                        green_led();        // turn green led on
                        LATCbits.LATC3 = 1; // Turn LED panel off (relay off)

                        //  DISPLAY-1 :
                        LATAbits.LATA0 = 1;                // TURN ON DISPLAY-1
                        PORTB = segment[hour_first_digit]; // Find Code and send it to the PORT
                        __delay_ms(3);                     // DELAY for turning on the display
                        LATAbits.LATA0 = 0;                // TURN OFF DISPLAY-1

                        // DISPLAY-2 :

                        LATAbits.LATA1 = 1;                 // TURN ON DISPLAY-2
                        PORTB = segment[hour_second_digit]; // Find Code and send it to the PORT
                        __delay_ms(3);                      // DELAY for turning on the display
                        LATAbits.LATA1 = 0;                 // TURN OFF DISPLAY-2

                        // MINUTE DISPLAY
                        //  DISPLAY-3 :
                        LATAbits.LATA2 = 1;                  // TURN ON DISPLAY-3
                        PORTB = segment[minute_first_digit]; // Find Code and send it to the PORT
                        __delay_ms(3);                       // DELAY for turning on the display
                        LATAbits.LATA2 = 0;                  // TURN OFF DISPLAY-3

                        // DISPLAY-4 :
                        LATAbits.LATA3 = 1;                   // TURN ON DISPLAY-4
                        PORTB = segment[minute_second_digit]; // Find Code and send it to the PORT
                        __delay_ms(3);                        // DELAY for turning on the display
                        LATAbits.LATA3 = 0;                   // TURN OFF DISPLAY-4

                        LATAbits.LATA7 = 0; // buzzer - off

                        if (DEL % 79 == 0) // Display dot pointer at regular intervals
                        {
                            LATAbits.LATA1 = 1; // TURN ON DISPLAY-2
                            PORTB = 0x80;       // Find Code and send it to the PORT
                            __delay_ms(3);      // DELAY for turning on the display
                            LATAbits.LATA1 = 0; // TURN OFF DISPLAY-2
                        }

                        // Check state of stop_timer button
                        if (PORTCbits.RC2 == 0)
                        {
                            RESET = 1; // Set the reset flag.
                            break;
                        }
                    } // end delay loop

                    if (RESET || timeUp)
                        break;

                } // End minute second digit loop

                if (RESET || timeUp)
                    break;

            } // End Minute first digit loop

            if (RESET || timeUp)
                break;

        } // end hour second digit loop
    }     // end hour first digit loop

    LATAbits.LATA7 = 1; // turn buzzer on

    if (RESET)
    {                // stop timer button pressed
        stopTimer(); // call stoptimer function irrespective of timer status.
    }
    else
    {
        LATCbits.LATC3 = 0; // Turn LED panel off (relay off)
        stopMessage();      // display OVEr message and back to normal state.
    }

} // End start timer function

/*
 * @desc : display 00.00, stop the timer.
 * @params : none.
 */
void stopTimer()
{

    red_led(); // red led to indicate stop timer.

    LATCbits.LATC3 = 0; // Turn LED panel off (relay off)

    /*Display 00.00*/
    seven_segment_config(); // turn on all displays.

    segmentCounter = 0;
    PORTB = segment[segmentCounter]; // segment[0] = 0x3F
    __delay_ms(100);                 // 100msec delay

    LATAbits.LATA7 = 0; // buzzer - off
}

/*
 * @desc : display OVEr, when timer ends.
 * @params : none.
 */
void stopMessage()
{
    unsigned int DEL;

    red_led(); // red led to indicate that timer is over.

    LCD_Init((0x38 << 1));

    LCD_Set_Cursor(1, 7);
    LCD_Write_String("OVER");
    
    __delay_ms(500);

    LCD_CLR();
    LATAbits.LATA7 = 0;
       
}

/*
 * @desc: starts counter at device startup : 0000 - 9999
 * @params : none
 */
void startUpcounter()
{
    // display numbers from 0000 - 9999 on all displays.
    seven_segment_config();
    unsigned int displaypos, actualpos;

    for (segmentCounter = 0; segmentCounter < 2; segmentCounter++)
    {
        LATAbits.LATA7 = 1; // buzzer - on

        for (displaypos = 3; displaypos < 7; displaypos++)
        {
            actualpos = displaypos * 2;

            lcd_print(1, actualpos, inttochar(segmentCounter)); /* print digit */
            lcd_print(1, actualpos + 1, '.');                   /*print dot after a number*/
        }
        __delay_ms(500); /*500 milli-sec delay*/

        LATAbits.LATA7 = 0; // buzzer - off
        __delay_ms(500);    /*500 milli-sec delay*/
    }

    LCD_CLR();

    stopMessage();
   
}

/* EEPROM Function Definitions */

/*
 * @desc: write data to eeprom.
 * @params : address, data.
 * @return : none.
 */

void EEPROM_Write(unsigned char address, unsigned char data)
{
    /*Write Operation*/
    EEADR = address;      /* Write address to the EEADR register*/
    EEDATA = data;        /* Copy data to the EEDATA register for write */
    EECON1bits.EEPGD = 0; /* Access data EEPROM memory*/
    EECON1bits.CFGS = 0;  /* Access flash program or data memory*/
    EECON1bits.WREN = 1;  /* Allow write to the memory*/
    INTCONbits.GIE = 0;   /* Disable global interrupt*/

    /* Below sequence in EECON2 Register is necessary
    to write data to EEPROM memory*/
    EECON2 = 0x55;
    EECON2 = 0xAA;

    EECON1bits.WR = 1;  /* Start writing data to EEPROM memory*/
    INTCONbits.GIE = 1; /* Enable interrupt*/

    while (PIR2bits.EEIF == 0)
        ;              /* Wait for write operation complete */
    PIR2bits.EEIF = 0; /* Reset EEIF for further write operation */
}

/*
 *@desc : read data from eeprom.
 *@params : address
 *@return : data
 *@return_type : char
 */

char EEPROM_Read(unsigned char address)
{
    /*Read operation*/
    EEADR = address;      /* Read data at location 0x00*/
    EECON1bits.WREN = 0;  /* WREN bit is clear for Read operation*/
    EECON1bits.EEPGD = 0; /* Access data EEPROM memory*/
    EECON1bits.RD = 1;    /* To Read data of EEPROM memory set RD=1*/
    return (EEDATA);
}

/* Default display function definition */
/*
 *@desc : display data in edit mode and update mode.
 *@params : buttonCounter, update.
 *@return : none
 */
void display(unsigned int buttonCounter, unsigned int update)
{
 
    display_function_count = display_function_count + 1; // Increment when this function is called.

    unsigned char hour_first_digit, hour_second_digit, minute_first_digit, minute_second_digit;
    unsigned int hour_f_digit, hour_s_digit, minute_f_digit, minute_s_digit;
    
    /* read stored hour and minute data from EEPROM */
    /*
    hour_first_digit = inttochar(EEPROM_Read(0x0A));    // read eeprom data at address location 0x0A
    hour_second_digit = inttochar(EEPROM_Read(0x0B));   // read eeprom data at address location 0x0B
    minute_first_digit = inttochar(EEPROM_Read(0x0C));  // read eeprom data at address location 0x0C
    minute_second_digit = inttochar(EEPROM_Read(0x0D)); // read eeprom data at address location 0x0D
    */

        /* display stored hour and minute data  */
       LCD_Set_Cursor(1, 6); //cursor position for digit-1
       
        if (buttonCounter == 1)
        {
            LCD_Write_Char(' ');
             __delay_ms(200);
        }else{
            LCD_Write_Char(inttochar(EEPROM_Read(0x0A)));       
        }

        // digit 2
        LCD_Set_Cursor(1, 8); // cursor position for digit-2
       
        if (buttonCounter == 2)
        {
            LCD_Write_Char(' ');
            __delay_ms(200);
        }else{
            LCD_Write_Char(inttochar(EEPROM_Read(0x0B)));
            
            //print dot
            LCD_Set_Cursor(1,9);
            LCD_Write_Char(':');
        }

        // MINUTE DISPLAY
        // digit 3
         LCD_Set_Cursor(1, 10); //cursor position for digit-3
         
        if (buttonCounter == 3)
        {
            LCD_Write_Char(' ');
            __delay_ms(200);
        }else{
            LCD_Write_Char(inttochar(EEPROM_Read(0x0C)));
        }

        // digit 4
        LCD_Set_Cursor(1, 12); // cursor position for digit-4
        
        if (buttonCounter == 4)
        {
            LCD_Write_Char(' ');
            __delay_ms(200);
        }else{
            LCD_Write_Char(inttochar(EEPROM_Read(0x0D)));
        }

        LATAbits.LATA7 = 0; // buzzer - off

        
        if ((display_function_count % 12 == 0) || (display_function_count % 6 == 0)) // once this condition passes, following digit will be visible..
        {
            switch (buttonCounter) // button counter
            {
            case 1:
                LCD_Set_Cursor(1, 6);
                LCD_Write_Char(inttochar(EEPROM_Read(0x0A)));
                
                __delay_ms(200);
                break;

            case 2:
                LCD_Set_Cursor(1, 8);
                LCD_Write_Char(inttochar(EEPROM_Read(0x0B)));

                __delay_ms(200);
                break;

            case 3:
                LCD_Set_Cursor(1, 10);
                LCD_Write_Char(inttochar(EEPROM_Read(0x0C)));

                __delay_ms(200);
                break;

            case 4:
                LCD_Set_Cursor(1, 12);
                LCD_Write_Char(inttochar(EEPROM_Read(0x0D)));

                __delay_ms(200);
                break;

            default:
                break;
            } // end switch statement.
        }     // end if statement.

    // Update digits.
    if (update && (display_function_count % 2 == 0))
    {
        switch (buttonCounter)
        {
        case 1:
            hour_f_digit = EEPROM_Read(0x0A) + 1; // increment the value
            
            if (hour_f_digit > 9)
                hour_f_digit = 0;             // if it exceeds 9, reset it to 0.
            EEPROM_Write(0x0A, hour_f_digit); // update and store the value in eeprom
            __delay_ms(50);
            break;
        case 2:
            hour_s_digit = EEPROM_Read(0x0B) + 1; // increment the value
            
            if (hour_s_digit > 9)
                hour_s_digit = 0;             // if it exceeds 9, reset it to 0.
            EEPROM_Write(0x0B, hour_s_digit); // update and store the value in eeprom
            __delay_ms(50);
            break;
        case 3:
            minute_f_digit = EEPROM_Read(0x0C) + 1; // increment the value
            
            if (minute_f_digit > 5)
                minute_f_digit = 0;             // if it exceeds 5, reset it to 0.
            EEPROM_Write(0x0C, minute_f_digit); // update and store the value in eeprom
            __delay_ms(50);
            break;
        case 4:
            minute_s_digit = EEPROM_Read(0x0D) + 1; // increment the value
            
            if (minute_s_digit > 9)
                minute_s_digit = 0;             // if it exceeds 9, reset it to 0.
            EEPROM_Write(0x0D, minute_s_digit); // update and store the value in eeprom
            __delay_ms(50);
            break;
        default:
            break;
        }
    }
        
       
    // reset display function counter.
    if (display_function_count > 1000)
        display_function_count = 0;
}

/*
 *@desc : converts type (int to char)
 */
unsigned char inttochar(unsigned int digit)
{
    /*
     * ASCII representation of 0 is 48, any digit added to 48 will be char representation of that number.
     */
    return digit + '0';
}

/*
 * @desc : Collective function to display some character on lcd.
 */
void lcd_print(unsigned char row, unsigned char col, char Data)
{
    /*
     * 1. Set cursor.
     * 2. Print
     */
    LCD_Set_Cursor(row, col); /* set cursor */
    LCD_Write_Char(Data);     /*write data*/
}


void EEPROM_Mem_Initialise(){
    unsigned char eeprom_addr, flag_addr = 0x0F; //address location
    unsigned int mem_check = 0, i = 1;
    
    __delay_ms(1000);
    
    //below loop will continue until faulty reading 
    while((EEPROM_Read(0x0A) == 0) && (EEPROM_Read(0x0B) == 0) && (EEPROM_Read(0x0C) == 0) && (EEPROM_Read(0x0D) == 0) && (EEPROM_Read(0x0F) == 0)){
            __delay_ms(500);
    }
        
    if(EEPROM_Read(0x0F) == 1)
            mem_check = 1; 
    
    if(mem_check != 1){
        
        for(eeprom_addr = 0x0A; eeprom_addr < 0x0E; eeprom_addr++){
            EEPROM_Write(eeprom_addr, 0);
            __delay_ms(20);                        
        }
        
        EEPROM_Write(flag_addr, 1);
         __delay_ms(20);
    }
    
     __delay_ms(1000);
}

/*
 *@desc : main function
 */
void main(void)
{

    // Configure the oscillator(64MHz using PLL)
    OSCCON = 0x70;  // 0b01110000
    OSCTUNE = 0xC0; // 0b11000000

    // Configure the button pins
    TRISCbits.TRISC0 = 1;
    TRISCbits.TRISC1 = 1;
    TRISCbits.TRISC2 = 1;

    // Configure the input pins as digital.
    ANSELCbits.ANSC2 = 0;

    // Configure the digital pins (RGB LED))
    TRISAbits.TRISA4 = 0; // led 1
    TRISAbits.TRISA5 = 0; // led 2
    TRISAbits.TRISA6 = 0; // led 3

    TRISAbits.TRISA7 = 0; // buzzer
    TRISCbits.TRISC3 = 0; // relay

    /*By default all LEDs should be off*/
    LATAbits.LATA4 = 1;
    LATAbits.LATA5 = 1;
    LATAbits.LATA6 = 1;

    LATCbits.LATC3 = 0; // initially relay should be off.
    
    /*I2C and LCD Initialisation*/
    I2C2_Init();

    LCD_Init((0x38 << 1)); // Initialize LCD module with I2C address = 0x38

    /*Start Initial Counter*/
    startUpcounter();
   
    __delay_ms(1000);
    EEPROM_Mem_Initialise();
    
    /*EEPROM - LCD Write Read Test*/
    /*
     * 1. write some integer in EEPROM
     * 2. Display it on LCD.
     */
    /*
    EEPROM_Write(0x0F, 1); //write '1' at 0x0F.

    char test_var = EEPROM_Read(0x0F) + '0';

    LCD_Set_Cursor(2,8); // row-2, column-1
    LCD_Write_Char(test_var); //display data at 0x0F.
    */
  
    unsigned int isEditMode = 0;
    unsigned int shiftCounter = 1;
    unsigned int stop_flag = 0;
    unsigned int updateFlag = 0;
    unsigned int transition_start_counter = 0;
    unsigned int transition_end_counter = 0;
    unsigned char hour_first_digit, hour_second_digit, minute_first_digit, minute_second_digit;

    while (1)
    {
        if (PORTCbits.RC0 == 0) // button 1 clicked
        {
            LATAbits.LATA7 = 1; // buzzer - on

            isEditMode ^= 1; // toggle
            stop_flag = 0;   // reset stop flag.

            LATAbits.LATA7 = 0; // buzzer - off
        }
        else if (PORTCbits.RC1 == 0) // button 2 clicked
        {
            LATAbits.LATA7 = 1; // buzzer - on

            updateFlag = 0; // clear updateFlag
            stop_flag = 0;  // clear stop flag.

            if (isEditMode)
            { // edit mode

                __delay_ms(100); // delay for button press.

                if (shiftCounter < 4) // if button shift is less than 4, increment.
                    shiftCounter++;
                else
                    shiftCounter = 1; // equals to 4 or greater than 4, reset the counter to 1.

                display(shiftCounter, updateFlag); // display stored time, shift displays.
            }
            else
            {
                //@TODO : We have to add transition mode. (on long press)
                /*
                 * logic for long press:
                 * place a counter, and increment the counter when button is pressed.
                 * place a check for some value of counter, device should enter transition mode.
                 */

                shiftCounter = 1; // reset button shift counter.

                // green_led(); // start timer indication

                // normal mode.
                // 1. relay on
                // 2. startTimer
                // 3. relay off
                // 4. time ends indication.

                startTimer(); // start timer.
            }
        }
        else if (PORTCbits.RC2 == 0) // button 3 clicked
        {
            LATAbits.LATA7 = 1;           // buzzer - on
            transition_start_counter = 0; // reset transition start counter.

            if (isEditMode)
            {                                      // edit mode
                updateFlag = 1;                    // set update flag.
                display(shiftCounter, updateFlag); // increment digits of the respective display.
            }
            else
            {
                stop_flag = 1;

                // normal mode.
                if (stop_flag)
                    stopTimer(); // display 00.00 and restart the timer.
            }
        }
        else
        {
            transition_start_counter = 0; // reset transition start counter.

            if (isEditMode == 0) // normal mode
            {
                red_led(); // set up bits to turn on red led.

                if (stop_flag)
                {
                    stopTimer(); // stop timer
                }
                else
                {
                    display(0, 0); // display stored time in normal mode.
                }
            }
            else // edit mode
            {
                updateFlag = 0;                    // clear updateFlag
                blue_led();                        // set up bits to turn on blue led.
                display(shiftCounter, updateFlag); // display stored data in edit mode.
            }

            LATAbits.LATA7 = 0; // buzzer - off
        }
    }

    return;
}
