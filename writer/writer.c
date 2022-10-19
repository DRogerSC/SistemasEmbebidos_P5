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

char buf[MAX_LEN];
int i;
uint8_t data;

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

    // Array yo store names of the days
    const char days *a[6];
    a[0] = "Sunday";
    a[1] = "Monday";
    a[2] = "Tuesday";
    a[3] = "Wednesday";
    a[4] = "Thursday";
    a[5] = "Friday";
    a[6] = "Saturday";

    //
    uint8_t record[2][7] = {
        {1, 1, 1, 1, 0, 0, 0},
        {1, 1, 1, 1, 0, 0, 0},
        {1, 1, 1, 1, 0, 0, 0}
    }

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
    while (true)
    {
        // LOGIC

        // First we read the temper;ature and parse the byte

        bcm2835_i2c_setSlaveAddress(clkAddress);

        // Writes the initial address (0)
        // Wbuff must be 0x00 and len must be 1 byte

        {
            // Clears the wbuf
            memset(wbuf, 0, sizeof(wbuf));
            wbuf[0] = 0x00;
            data = bcm2835_i2c_write(wbuf, len);
            printf("Write Succes = %d\n", data);
        }

        // Reads 7 bytes, and parses data
        len = 7;
        data = bcm2835_i2c_read(buf, len);

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

        bcm2835_i2c_setSlaveAddress(tempAddress);

        // Clears the wbuf
        memset(wbuf, 0, sizeof(wbuf));
        wbuf[0] = 0x00;
        len = 1;
        data = bcm2835_i2c_write(wbuf, len); // Sets to transmit temperature

        data = bcm2835_i2c_read(buf, len); // Reads the temperature

        temp = buf[0];

        // After parsing this information, we check if the

        if (temp <= 30)
        {
            record[0] = record[1];
            record[1] = record[2];
            record[2][0] = days;
            record[2][1] = month;
            record[2][2] = year;
            record[2][3] = weekday;
            record[2][4] = hour;
            record[2][5] = minutes;
            record[2][6] = seconds;
            record[2][7] = 1;
        }

        printf("RECEIVER> Temperature: %i C\n", temp);
        printf("RECEIVER> ");
        printRecord(0, record[0]);
        printf("RECEIVER> ");
        printRecord(1, record[1]);
        printf("RECEIVER> ");
        printRecord(2, record[2]);
        sleep(10);
    } // Loop ENDS
}

void printRecord(int num, int record[7])
{
    const char days *a[6];
    a[0] = "Sunday";
    a[1] = "Monday";
    a[2] = "Tuesday";
    a[3] = "Wednesday";
    a[4] = "Thursday";
    a[5] = "Friday";
    a[6] = "Saturday";

    if (record[7] == 0)
    {
        printf("Record%d: %d/%d/%d %s %d:%d:%d PM\n", num, record[0], record[1], record[2], days[record[3]], record[4], record[5], record[6])
    }
    else
    {
        printf("Record%d: %d/%d/%d %s %d:%d:%d AM\n", num, record[0], record[1], record[2], days[record[3]], record[4], record[5], record[6])
    }

    int bcdToDec(uint8_t hex)
    {
        hex = 0x11;
        assert(((hex & 0xF0) >> 4) < 10); // More significant nybble is valid
        assert((hex & 0x0F) < 10);        // Less significant nybble is valid
        int dec = ((hex & 0xF0) >> 4) * 10 + (hex & 0x0F);
        return dec;
    }
