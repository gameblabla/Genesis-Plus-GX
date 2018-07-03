/*
   basic, incomplete SSP160x (SSP1601?) interpreter
   with SVP memory controller emu

   (c) Copyright 2008, Grazvydas "notaz" Ignotas
   Free for non-commercial use.

   For commercial use, separate licencing terms must be obtained.

   Modified for Genesis Plus GX (Eke-Eke): added BIG ENDIAN support, fixed addr/code inversion
*/

#ifndef _SVP_H_
#define _SVP_H_

#include "shared.h"
#include "ssp16.h"

typedef struct {
  uint8_t iram_rom[0x20000]; // IRAM (0-0x7ff) and program ROM (0x800-0x1ffff)
  uint8_t dram[0x20000];
  ssp1601_t ssp1601;
} svp_t;

extern svp_t *svp;
extern int16_t SVP_cycles; 

extern void svp_init(void);
extern void svp_reset(void);
extern void svp_write_dram(uint32_t address, uint32_t data);
extern uint32_t svp_read_cell_1(uint32_t address);
extern uint32_t svp_read_cell_2(uint32_t address);

#endif
