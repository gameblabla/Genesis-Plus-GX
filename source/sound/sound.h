/***************************************************************************************
 *  Genesis Plus
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
 *  Sound Hardware
 ****************************************************************************************/

#ifndef _SOUND_H_
#define _SOUND_H_

#include <stdint.h>

/* Function prototypes */
extern void sound_init(void);
extern void sound_reset(void);
extern void sound_restore(void);
extern int32_t sound_context_save(uint8_t *state);
extern int32_t sound_context_load(uint8_t *state, int8_t *version);
extern int32_t sound_update(uint32_t cycles);
extern void fm_reset(uint32_t cycles);
extern void fm_write(uint32_t cycles, uint32_t address, uint32_t data);
extern uint32_t fm_read(uint32_t cycles, uint32_t address);
extern void psg_write(uint32_t cycles, uint32_t data);

#endif /* _SOUND_H_ */
