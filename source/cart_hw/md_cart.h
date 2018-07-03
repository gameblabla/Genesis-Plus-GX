/****************************************************************************
 *  Genesis Plus
 *  Mega Drive cartridge hardware support
 *
 *  Copyright (C) 2007-2011  Eke-Eke (GCN/Wii port)
 *
 *  Lots of protection mechanism have been discovered by Haze
 *  (http://haze.mameworld.info/)
 *
 *  Realtec mapper has been figured out by TascoDeluxe
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ***************************************************************************/

#ifndef _MD_CART_H_
#define _MD_CART_H_

/* Lock-On cartridge type */
#define TYPE_GG 0x01  /* Game Genie */
#define TYPE_AR 0x02  /* (Pro) Action Replay */
#define TYPE_SK 0x03  /* Sonic & Knuckles */

/* Special hardware (0x01 reserved for SMS 3-D glasses) */
#define HW_J_CART   0x02
#define HW_LOCK_ON  0x04

/* Cartridge extra hardware */
typedef struct
{
  uint8_t regs[4];                                            /* internal registers (R/W) */
  uint32_t mask[4];                                           /* registers address mask */
  uint32_t addr[4];                                           /* registers address */
  uint16_t realtec;                                           /* realtec mapper */
  uint16_t bankshift;                                         /* cartridge with bankshift mecanism reseted on software reset */
  int32_t (*time_r)(int32_t address);             /* !TIME signal ($a130xx) read handler  */
  void (*time_w)(uint32_t address, uint32_t data);  /* !TIME signal ($a130xx) write handler */
  int32_t (*regs_r)(int32_t address);             /* cart hardware registers read handler  */
  void (*regs_w)(int32_t address, int32_t data);  /* cart hardware registers write handler */
} T_CART_HW;

/* Cartridge type */
typedef struct
{
  uint8_t *rom;     /* ROM area */
  uint8_t *base;    /* ROM base (saved for OS/Cartridge ROM swap) */
  uint32_t romsize; /* ROM size */
  uint32_t mask;    /* ROM mask */
  uint8_t special;  /* Lock-On, J-Cart or SMS 3-D glasses hardware */
  T_CART_HW hw;   /* Extra mapping hardware */
} T_CART;

/* global variables */
extern T_CART cart;

/* Function prototypes */
extern void md_cart_init(void);
extern void md_cart_reset(int32_t hard_reset);
extern int32_t md_cart_context_save(uint8_t *state);
extern int32_t md_cart_context_load(uint8_t *state);

#endif


