/*==================================================================
  File Name: command_interpreter.c
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This files contains command handling from UART
  ------------------------------------------------------------------
  This file is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
 
  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this software. If not, see <http://www.gnu.org/licenses/>.
  ================================================================== */ 
#include <ctype.h>
#include "command_interpreter.h"
#include "rgb_platform_stm8s207.h"
#include "uart.h"
#include "scheduler.h"
#include "eep.h"
#include "pixel.h"
#include "i2c_bb.h"
#include "i2c_ds3231_bb.h"

extern  task_struct task_list[]; // struct with all tasks
extern  uint8_t     max_tasks;
extern  Time        dt;          // Struct with time and date values, updated every sec.
extern  bool        dst_active;  // true = Daylight Saving Time active

char    rs232_inbuf[UART_BUFLEN];     // buffer for RS232 commands
uint8_t rs232_ptr = 0;                // index in RS232 buffer

extern  char    lk1[];      // Text for top horizontal line
extern  uint8_t lk1c[];     // Colour for every character in lk1[]
extern  char    lk2[];      // Text for bottom horizontal line
extern  uint8_t lk2c[];     // Colour for every character in lk2[]

/*-----------------------------------------------------------------------------
  Purpose  : Scan all devices on the I2C bus on all channels of the PCA9544
  Variables: 
         ch: the I2C channel number, 0 is the main channel
 Returns  : -
  ---------------------------------------------------------------------------*/
void i2c_scan(enum I2C_CH ch)
{
    char    s[50]; // needed for printing to serial terminal
    uint8_t x = 0;
    int     i;     // Leave this as an int!
    
    sprintf(s,"I2C[%1d]: ",ch);
    uart1_printf(s); // print to UART1
    for (i = 0x02; i < 0xff; i+=2)
    {
        if (i2c_start_bb(ch,i) == I2C_ACK)
        {
            sprintf(s,"0x%0x ",i);
            uart1_printf(s); // print to UART1
            x++;
        } // if
        i2c_stop_bb(ch);
    } // for
    if (!x) 
    {
        uart1_printf("-"); // print to UART1
    } // if	
    uart1_printf("\n");
} // i2c_scan()

/*-----------------------------------------------------------------------------
  Purpose  : Non-blocking RS232 command-handler via the USB port
  Variables: -
  Returns  : [NO_ERR, ERR_CMD, ERR_NUM, ERR_I2C]
  ---------------------------------------------------------------------------*/
uint8_t rs232_command_handler(void)
{
  char    ch;
  static uint8_t cmd_rcvd = 0;
  
  if (!cmd_rcvd && uart1_kbhit())
  {     // A new character has been received
        ch = uart1_getc(); // get character
	switch (ch)
	{
            case '\r': break;
            case '\n': cmd_rcvd  = 1;
                       rs232_inbuf[rs232_ptr] = '\0';
                       rs232_ptr = 0;
                       break;
            default  : if (rs232_ptr < UART_BUFLEN-1)
			    rs232_inbuf[rs232_ptr++] = ch;
		       else 
                       {   // buffer is full
                           cmd_rcvd = 1;
                           rs232_inbuf[UART_BUFLEN-1] = '\0';
                           rs232_ptr = 0; // remove input
                       } // else
                       break;
	} // switch
  } // if
  if (cmd_rcvd)
  {
      cmd_rcvd = 0;
      return execute_single_command(rs232_inbuf);
  } // if
  else return NO_ERR;
} // rs232_command_handler()

/*-----------------------------------------------------------------------------
  Purpose  : list all tasks and send result to the UART or ETH
  Variables: -
 Returns   : -
  ---------------------------------------------------------------------------*/
void list_all_tasks(void)
{
    uint8_t index = 0;
    char    s[50];
    
    //uart1_printf("Task-Name,T(ms),Stat,T(ms),M(ms)\n");
    //go through the active tasks
    if(task_list[index].Period != 0)
    {
        while ((index < MAX_TASKS) && (task_list[index].Period != 0))
        {
            sprintf(s,"%s,%d,%x,%d,%d\n", task_list[index].Name, 
                    task_list[index].Period  , task_list[index].Status, 
                    task_list[index].Duration, task_list[index].Duration_Max);
            uart1_printf(s);
            index++;
        } // while
    } // if
} // list_all_tasks()

/*-----------------------------------------------------------------------------
  Purpose  : This routine reads the time and date from the DS3231 RTC and 
             prints this info to the uart.
  Variables: -
  Returns  : -
  ---------------------------------------------------------------------------*/
void print_date_and_time(void)
{
    char s2[40]; // Used for printing to UART
    uart1_printf("DS3231: ");
    sprintf(s2," %d-%d-%d, %d:%d.%d",
               dt.day , dt.mon, dt.year,
               dt.hour, dt.min, dt.sec);
    uart1_printf(s2);
    sprintf(s2," dow:%d, dst:%d\n",
               dt.dow, dst_active);
    uart1_printf(s2);
} // print_date_and_time()

/*-----------------------------------------------------------------------------
  Purpose: interpret commands which are received via the USB serial terminal:
   - S0           : Ebrew hardware revision number (also disables delayed-start)
     S2           : List all connected I2C devices  
     S3           : List all tasks
 
  Variables: 
          s: the string that contains the command from RS232 serial port 0
  Returns  : [NO_ERR, ERR_CMD, ERR_NUM, ERR_I2C] or ack. value for command
  ---------------------------------------------------------------------------*/
uint8_t execute_single_command(char *s)
{
   uint8_t  num  = atoi(&s[1]); // convert number in command (until space is found)
   uint8_t  rval = NO_ERR;
   char     s2[40]; // Used for printing to RS232 port
   char     *s1;
   uint8_t  d,m,h,sec;
   uint16_t y;
   const char sep[] = ":-.";
   
   switch (tolower(s[0]))
   {
        case 'd': // Set Date, 1 = Get Date
		 switch (num)
		 {
                    case 0: // Set Date
			    s1 = strtok(&s[3],sep);
                            d  = atoi(s1);
                            s1 = strtok(NULL ,sep);
                            m  = atoi(s1);
                            s1 = strtok(NULL ,sep);
                            y  = atoi(s1);
                            sprintf(s2,"Date: %d-%d-%d\n",d,m,y);
                            uart1_printf(s2);
                            ds3231_setdate(d,m,y); // write to DS3231 IC
                            break;
                    case 1: // Set Time
                            s1      = strtok(&s[3],sep);
                            h       = atoi(s1);
                            s1      = strtok(NULL ,sep);
                            m       = atoi(s1);
                            s1      = strtok(NULL ,sep);
                            sec     = atoi(s1);
                            sprintf(s2,"Time: %d:%d:%d\n",h,m,sec);
                            uart1_printf(s2);
                            ds3231_settime(h,m,sec); // write to DS3231 IC
                            break;
                    case 2: // Get Date & Time
                            print_date_and_time(); 
                            break;
                   default: break;
                 } // switch
                 break;

        case 's': // System commands
               rval = 67 + num;
               switch (num)
               {
                   case 0: // Ebrew revision
                       print_revision_nr(); // print revision number
                       break;
                   case 2: // List all I2C devices
                       i2c_scan(I2C_CH0);
                       break;
                   case 3: // List all tasks
                       list_all_tasks(); 
                       break;	
                   default: rval = ERR_NUM;
                   break;
               } // switch
               break;
               
	   case 't': // Text input for lichtkrant
               rval = 67 + num;
               switch (num)
               {
                   case 0: // Text for top-level row
                       strcpy(lk1,&s[3]);
                       eep_write_string(EEP_TEXT1,lk1);
                       color_text_input(lk1,lk1c);
                       eep_write_string(EEP_COL1,(char *)lk1c);
                       break;
                   case 1: // Text for bottom-level row
                       strcpy(lk2,&s[3]);
                       eep_write_string(EEP_TEXT2,lk2);
                       color_text_input(lk2,lk2c);
                       eep_write_string(EEP_COL2,(char *)lk2c);
                       break;
                   default: rval = ERR_NUM;
                   break;
               } // switch
               break;
               
	   default: 
               rval = ERR_CMD;
               sprintf(s2,"ERR.CMD[%s]\n",s);
               uart1_printf(s2);
               break;
   } // switch
   return rval;	
} // execute_single_command()