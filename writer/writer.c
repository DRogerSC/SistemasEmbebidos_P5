/*******************************************************************************
 *
 *   i2c.c
 *
 *   Copyright (c) 2013 Shahrooz Shahparnia (sshahrooz@gmail.com)
 *
 *   Description:
 *   i2c is a command-line utility for executing i2c commands with the
 *   Broadcom bcm2835.  It was developed and tested on a Raspberry Pi single-board
 *   computer model B.  The utility is based on the bcm2835 C library developed
 *   by Mike McCauley of Open System Consultants, http://www.open.com.au/mikem/bcm2835/.
 *
 *   Invoking spincl results in a read or write I2C transfer.  Options include the
 *   the I2C clock frequency, read/write, address, and port initialization/closing
 *   procedures.  The command usage and command-line parameters are described below
 *   in the showusage function, which prints the usage if no command-line parameters
 *   are included or if there are any command-line parameter errors.  Invoking i2c
 *   requires root privilege.
 *
 *   This file contains the main function as well as functions for displaying
 *   usage and for parsing the command line.
 *
 *   Open Source Licensing GNU GPLv3
 *
 *   Building:
 * After installing bcm2835, you can build this
 * with something like:
 * gcc -o i2c i2c.c -l bcm2835
 * sudo ./i2c
 *
 * Or you can test it before installing with:
 * gcc -o i2c -I ../../src ../../src/bcm2835.c i2c.c
 * sudo ./i2c
 *
 *   History:
 *   11/05    VERSION 1.0.0: Original
 *
 *      User input parsing (comparse) and showusage\
 *      have been adapted from: http://ipsolutionscorp.com/raspberry-pi-spi-utility/
 *      mostly to keep consistence with the spincl tool usage.
 *
 *      Compile with: gcc -o i2c i2c.c bcm2835.c
 *
 *      Examples:
 *
 *           Set up ADC (Arduino: ADC1015)
 *           sudo ./i2c -s72 -dw -ib 3 0x01 0x44 0x00 (select config register, setup mux, etc.)
 *           sudo ./i2c -s72 -dw -ib 1 0x00 (select ADC data register)
 *
 *           Bias DAC (Arduino: MCP4725) at some voltage
 *           sudo ./i2c -s99 -dw -ib 3 0x60 0x7F 0xF0 (FS output is with 0xFF 0xF0)
 *           Read ADC convergence result
 *           sudo ./i2c -s72 -dr -ib 2 (FS output is 0x7FF0 with PGA1 = 1)
 *
 *      In a DAC to ADC loop back typical results are:
 *
 *      DAC    VOUT   ADC
 *      7FFh   1.6V   677h                    Note ratio is FS_ADC*PGA_GAIN/FS_DAC = 4.096/3.3 = 1.23
 *      5FFh   1.2V   4DCh
 *      8F0h   1.8V   745h
 *      9D0h   2V     7EAh
 *      000h   10mV   004h
 *
 ********************************************************************************/

#include <bcm2835.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

#define MODE_READ 0
#define MODE_WRITE 1

#define MAX_LEN 32

char wbuf[MAX_LEN];

typedef enum
{
    NO_ACTION,
    I2C_BEGIN,
    I2C_END
} i2c_init;

uint8_t init = NO_ACTION;
uint16_t clk_div = BCM2835_I2C_CLOCK_DIVIDER_148;
uint8_t slave_address = 0x00;
uint32_t len = 0;
uint8_t mode = MODE_READ;
uint8_t clkAddress = 0x68;
uint8_t tempAddress = 0x4d;

char *returnDay(uint8_t num){
    if(num == 0){
        return("Noneday");
        }
    else if(num == 1){
        return("Monday");
        }
    else if(num == 2){
        return("Tuesday");
        }
    else if(num == 3){
        return("Wednesday");
        }
    else if(num == 4){
        return("Thursday");
        }
    else if(num == 5){
        return("Friday");
        }
    else if(num == 6){
        return("Saturday");
        }
    else if(num == 7){
        return("Sunday");
        }
    else{
        return("Elseday");
        }
    }

void printRecord(int num, uint8_t record[3][8])
{
    const char days[72] = "Monday\0Tuesday\0Wednesday\0Thursday\0Friday\0Saturday\0Sunday\0";
    //const char days[] = "Monday";
    const int dictionary[7] = {0, 7, 15, 25, 34, 41, 50};
    //printf("%d\n", record[0][0]);
    
    
    if (record[num][7] == 0)
    {
        //printf("Trying to print\n");
        printf("Record%d: %d/%d/%d %s %d:%d:%d PM\n", num, record[num][0], record[num][1], record[num][2], returnDay(record[num][3]), record[num][4], record[num][5], record[num][6]);
        //printf("Day test: %s \n", days[0]);
        //printf("Record %d: %d/%d/%d %s %d:%d:%d PM\n", num, record[num][0], record[num][1], record[num][2], "ppa", record[num][4], record[num][5], record[num][6]);
    }
    else
    {
        //printf("Trying to print\n");
        printf("Record%d: %d/%d/%d %s %d:%d:%d AM\n", num, record[num][0], record[num][1], record[num][2], returnDay(record[num][3]), record[num][4], record[num][5], record[num][6]);
        //printf("Day test: %s \n", days[0]);
        //printf("Record %d: %d/%d/%d %s %d:%d:%d AM\n", num, record[num][0], record[num][1], record[num][2], "ppa", record[num][4], record[num][5], record[num][6]);
    }
}

int bcdToDec(uint8_t hex)
{
    hex = 0x11;
    assert(((hex & 0xF0) >> 4) < 10); // More significant nybble is valid
    assert((hex & 0x0F) < 10);        // Less significant nybble is valid
    int dec = ((hex & 0xF0) >> 4) * 10 + (hex & 0x0F);
    return dec;
}

char buf[MAX_LEN];
int i;
uint8_t data;
  // Array yo store names of the days
    /*const char days = *a[6];
    a[0] = "Sunday";
    a[1] = "Monday";
    a[2] = "Tuesday";
    a[3] = "Wednesday";
    a[4] = "Thursday";
    a[5] = "Friday";
    a[6] = "Saturday";*/
    
    const char days[72] = "Monday\0Tuesday\0Wednesday\0Thursday\0Friday\0Saturday\0Sunday\0";
    //const char days[] = "Monday";
    
    const int dictionary[7] = {0, 7, 15, 25, 34, 41, 50};
    


int main(int argc, char **argv)
{
    FILE *fptr;
    fptr = fopen("E:\\cprogram\\newprogram.txt", "w");

    // Register hour
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hour;
    uint8_t month;
    uint8_t year;
    uint8_t temp;
    uint8_t day;
    uint8_t weekday;

  
    //
    uint8_t record[3][8] = {
        {1, 1, 1, 1, 0, 0, 0},
        {1, 1, 1, 1, 0, 0, 0},
        {1, 1, 1, 1, 0, 0, 0}
    };

    // Inicializacion de comunicacion I2C
    if (!bcm2835_init())
    {
        printf("bcm2835_init failed. Are you running as root??\n");
        return 1;
    }

    // I2C begin
    if (!bcm2835_i2c_begin())
    {
        printf("bcm2835_i2c_begin failed. Are you running as root??\n");
        return 1;
    }

    bcm2835_i2c_setClockDivider(clk_div);

    // Loop start (READS EVERY 10 SECONDS)
    while (1)
    {
        // LOGIC

        // First we read the temper;ature and parse the byte
        fprintf(stderr, "Slave address set to: %d\n", clkAddress);
        bcm2835_i2c_setSlaveAddress(clkAddress);

        // Writes the initial address (0)
        // Wbuff must be 0x00 and len must be 1 byte
            // Clears the wbuf
            len = 1;
            memset(wbuf, 0, sizeof(wbuf));
            wbuf[0] = 0x00;
            data = bcm2835_i2c_write(wbuf, len);
            printf("Write Success = %d\n", data);

        // Reads 7 bytes, and parses data
        len = 7;
        data = bcm2835_i2c_read(buf, len);
        printf("Read Result = %d\n", data);
        seconds = buf[0];
        minutes = buf[1];
        hour = buf[2];
        weekday = buf[3];
        day = buf[4];
        month = buf[5];
        year = buf[6];

        seconds = bcdToDec(seconds);
        minutes = bcdToDec(minutes);
        hour = bcdToDec(hour);
        weekday = bcdToDec(weekday);
        day = bcdToDec(day);
        month = bcdToDec(month);
        year = bcdToDec(year);

        // Then we parse each byte relevant to the clock register

         fprintf(stderr, "Slave address set to: %d\n", tempAddress);
        bcm2835_i2c_setSlaveAddress(tempAddress);

        // Clears the wbuf
        memset(wbuf, 0, sizeof(wbuf));
        wbuf[0] = 0x00;
        len = 1;
        data = bcm2835_i2c_write(wbuf, len); // Sets to transmit temperature
         printf("Write Success = %d\n", data);
        data = bcm2835_i2c_read(buf, len); // Reads the temperature
        printf("Read Result = %d\n", data);
        
        for (i = 0; i < MAX_LEN; i++)
        buf[i] = 'n';
        data = bcm2835_i2c_read(buf, len);
        printf("Read Result = %d\n", data);
        for (i = 0; i < MAX_LEN; i++)
        {
            if (buf[i] != 'n')
                printf("Read Buf[%d] = %x\n", i, buf[i]);
        }
    }
        
        temp = buf[0];

        // After parsing this information, we check if the

        if (temp <= 30)
        {
            for(int x = 0; x<8; x++)
            {
                record[0][x] = record[1][x];
            }
            for(int x = 0; x<8; x++)
            {
                record[1][x] = record[2][x];
            }
        
            record[2][0] = day;
            record[2][1] = month;
            record[2][2] = year;
            record[2][3] = weekday;
            record[2][4] = hour;
            record[2][5] = minutes;
            record[2][6] = seconds;
            record[2][7] = 1;
        }

        printf("RECEIVER> Temperature: %i C\n", temp);
        printf("RECEIVER> " );
        printRecord(0, record);
        printf("RECEIVER> ");
        printRecord(1, record);
        printf("RECEIVER> ");
        printRecord(2, record);
        sleep(10);
    } // Loop ENDS
}


