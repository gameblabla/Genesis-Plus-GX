/***************************************************************************************
 *  Genesis Plus
 *  Video Display Processor (Mode 4 & Mode 5 rendering)
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

#ifndef _RENDER_H_
#define _RENDER_H_

/* Global variables */
extern uint8_t object_count;

/* Function prototypes */
extern void render_init(void);
extern void render_reset(void);
extern void render_line(int32_t line);
extern void blank_line(int32_t line, int32_t offset, int32_t width);
extern void remap_line(int32_t line);
extern void window_clip(uint32_t data, uint32_t sw);
extern void render_bg_m4(int32_t line, int32_t width);
extern void render_bg_m5(int32_t line, int32_t width);
extern void render_bg_m5_vs(int32_t line, int32_t width);
extern void render_bg_m5_im2(int32_t line, int32_t width);
extern void render_bg_m5_im2_vs(int32_t line, int32_t width);
extern void render_obj_m4(int32_t max_width);
extern void render_obj_m5(int32_t max_width);
extern void render_obj_m5_ste(int32_t max_width);
extern void render_obj_m5_im2(int32_t max_width);
extern void render_obj_m5_im2_ste(int32_t max_width);
extern void parse_satb_m4(int32_t line);
extern void parse_satb_m5(int32_t line);
extern void update_bg_pattern_cache_m4(int32_t index);
extern void update_bg_pattern_cache_m5(int32_t index);
#ifdef NGC
extern void color_update(int32_t index, uint32_t data);
#endif

/* Function pointers */
extern void (*render_bg)(int32_t line, int32_t width);
extern void (*render_obj)(int32_t max_width);
extern void (*parse_satb)(int32_t line);
extern void (*update_bg_pattern_cache)(int32_t index);
#ifndef NGC
extern void (*color_update)(int32_t index, uint32_t data);
#endif

#endif /* _RENDER_H_ */

