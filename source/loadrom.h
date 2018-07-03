/***************************************************************************************
 *  Genesis Plus
 *  ROM Loading Support
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald (original code)
 *  Eke-Eke (2007,2008,2009), additional code & fixes for the GCN/Wii port
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
 *
 ****************************************************************************************/
#ifndef _LOADROM_H_
#define _LOADROM_H_

#define MAXROMSIZE 10485760

typedef struct
{
  int8_t consoletype[18];         /* Genesis or Mega Drive */
  int8_t copyright[18];           /* Copyright message */
  int8_t domestic[50];            /* Domestic name of ROM */
  int8_t international[50];       /* International name of ROM */
  int8_t ROMType[4];              /* Educational or Game */
  int8_t product[14];             /* Product serial number */
  uint16_t checksum;      /* ROM Checksum (header) */
  uint16_t realchecksum;  /* ROM Checksum (calculated) */
  uint32_t romstart;        /* ROM start address */
  uint32_t romend;          /* ROM end address */
  uint8_t country[18];             /* Country flag */
  uint16_t peripherals;           /* Supported peripherals */
} ROMINFO;


/* Global variables */
extern ROMINFO rominfo;
extern int8_t rom_filename[256];

/* Function prototypes */
extern int32_t load_rom(int8_t *filename);
extern void region_autodetect(void);
extern int8_t *get_company(void);
extern int8_t *get_peripheral(int32_t index);

#endif /* _LOADROM_H_ */

