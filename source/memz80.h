/***************************************************************************************
 *  Genesis Plus
 *  Z80 bus controller (MD & MS compatibility mode)
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald (original code)
 *  Eke-Eke (2007-2011), additional code & fixes for the GCN/Wii port
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
#ifndef _MEMZ80_H_
#define _MEMZ80_H_

extern uint8_t  z80_md_memory_r(uint32_t  address);
extern void z80_md_memory_w(uint32_t  address, uint8_t  data);
extern uint8_t  z80_sms_memory_r(uint32_t  address);

extern uint8_t  z80_unused_port_r(uint32_t  port);
extern void z80_unused_port_w(uint32_t  port, uint8_t  data);
extern uint8_t  z80_sms_port_r(uint32_t  port);
extern void z80_sms_port_w(uint32_t  port, uint8_t  data);

#endif /* _MEMZ80_H_ */
