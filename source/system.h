/***************************************************************************************
 *  Genesis Plus
 *  Virtual System Emulation
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

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#define SYSTEM_PBC        0x00
#define SYSTEM_GENESIS    0x01
#define SYSTEM_MEGADRIVE  0x02
#define SYSTEM_PICO       0x03

#define MCYCLES_PER_LINE  3420

#define Z80_CYCLE_OFFSET  550 /* horizontal timings offset when running in SMS mode */

typedef struct
{
  uint8_t *data;      /* Bitmap data */
  int32_t width;        /* Bitmap width */
  int32_t height;       /* Bitmap height */
  int32_t depth;        /* Color depth (8-32 bits) */
  int32_t pitch;        /* Width of bitmap in bytes */
  int32_t granularity;  /* Size of each pixel in bytes */
  int32_t remap;        /* 1= Translate pixel data */
  struct
  {
    int32_t x;          /* X offset of viewport within bitmap */
    int32_t y;          /* Y offset of viewport within bitmap */
    int32_t w;          /* Width of viewport */
    int32_t h;          /* Height of viewport */
    int32_t ow;         /* Previous width of viewport */
    int32_t oh;         /* Previous height of viewport */
    int32_t changed;    /* 1= Viewport width or height have changed */
  } viewport;
} t_bitmap;

typedef struct
{
  int32_t sample_rate;  /* Output Sample rate (8000-48000) */
  float frame_rate; /* Output Frame rate (usually 50 or 60 frames per second) */
  int32_t enabled;      /* 1= sound emulation is enabled */
  int32_t buffer_size;  /* Size of sound buffer (in bytes) */
  int16_t *buffer[2]; /* Signed 16-bit stereo sound data */
  struct
  {
    int32_t *pos;
    int32_t *buffer;
  } fm;
  struct
  {
    int16_t *pos;
    int16_t *buffer;
  } psg;
} t_snd;


/* Global variables */
extern t_bitmap bitmap;
extern t_snd snd;
extern uint32_t mcycles_z80;
extern uint32_t mcycles_68k;
extern uint32_t mcycles_vdp;
extern uint8_t system_hw;

/* Function prototypes */
extern int32_t audio_init(int32_t samplerate,float framerate);
extern void audio_reset(void);
extern void audio_shutdown(void);
extern int32_t audio_update(void);
extern void audio_set_equalizer(void);
extern void system_init(void);
extern void system_reset(void);
extern void system_shutdown(void);
extern void (*system_frame)(int32_t do_skip);

#endif /* _SYSTEM_H_ */

