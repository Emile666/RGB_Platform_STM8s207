#ifndef _RGB_PLATFORM_STM8S207_H_
#define _RGB_PLATFORM_STM8S207_H_
/*==================================================================
  File Name: rgb_platform_stm8s207.h
  Author   : Emile
  ------------------------------------------------------------------
  Purpose  : This is the header-file for rgb_platform_stm8s207.c
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
  along with this software.  If not, see <http://www.gnu.org/licenses/>.

  |---------|      |---------|---------|bit 15
  |         |      |         |         |
  | PCB Y7  | .... | PCB Y1  | PCB Y0  |
  |         |      |         |   uC    |
  ----------|      |---------|---------|bit 0
  127      112    31       16 15      0 <- ROWS 
  ================================================================== */ 
#include "stm8_hw_init.h"
#include <string.h>
#include "delay.h"         /* for delay_msec() */
#include "i2c_bb.h"
#include "uart.h"
#include "command_interpreter.h"
#include "scheduler.h"

void    lichtkrant(void);
void    lichtkrant1(void);
void    lichtkrant2(void);
void    color_text_input(char *s, uint8_t *scol);
void    test_playfield(void);
void    print_revision_nr(void);
uint8_t read_dip_switches(void);
void    check_and_set_summertime(void);

#endif /* _RGB_PLATFORM_STM8S207_H_ */