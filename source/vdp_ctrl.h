/***************************************************************************************
 *  Genesis Plus
 *  Video Display Processor (68k & Z80 CPU interface)
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

#ifndef _VDP_H_
#define _VDP_H_

/* VDP context */
extern uint8_t reg[0x20];
extern uint8_t sat[0x400];
extern uint8_t vram[0x10000];
extern uint8_t cram[0x80];
extern uint8_t vsram[0x80];
extern uint8_t hint_pending;
extern uint8_t vint_pending;
extern uint8_t m68k_irq_state;
extern uint16_t status;
extern uint32_t dma_length;

/* Global variables */
extern uint16_t ntab;
extern uint16_t ntbb;
extern uint16_t ntwb;
extern uint16_t satb;
extern uint16_t hscb;
extern uint8_t bg_name_dirty[0x800];
extern uint16_t bg_name_list[0x800];
extern uint16_t bg_list_index;
extern uint8_t bg_pattern_cache[0x80000];
extern uint8_t hscroll_mask;
extern uint8_t playfield_shift;
extern uint8_t playfield_col_mask;
extern uint16_t playfield_row_mask;
extern uint8_t odd_frame;
extern uint8_t im2_flag;
extern uint8_t interlaced;
extern uint8_t vdp_pal;
extern uint16_t v_counter;
extern uint16_t vc_max;
extern uint16_t hscroll;
extern uint16_t vscroll;
extern uint16_t lines_per_frame;
extern int32_t fifo_write_cnt;
extern uint32_t fifo_lastwrite;
extern uint32_t hvc_latch;
extern const uint8_t *hctab;

/* Function pointers */
extern void (*vdp_68k_data_w)(uint32_t data);
extern void (*vdp_z80_data_w)(uint32_t data);
extern uint32_t (*vdp_68k_data_r)(void);
extern uint32_t (*vdp_z80_data_r)(void);

/* Function prototypes */
extern void vdp_init(void);
extern void vdp_reset(void);
extern int32_t vdp_context_save(uint8_t *state);
extern int32_t vdp_context_load(uint8_t *state);
extern void vdp_dma_update(uint32_t cycles);
extern void vdp_68k_ctrl_w(uint32_t data);
extern void vdp_z80_ctrl_w(uint32_t data);
extern uint32_t vdp_ctrl_r(uint32_t cycles);
extern uint32_t vdp_hvc_r(uint32_t cycles);
extern void vdp_test_w(uint32_t data);
extern int32_t vdp_68k_irq_ack(int32_t int_level);

#endif /* _VDP_H_ */
