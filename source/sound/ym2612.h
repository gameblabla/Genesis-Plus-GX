/*
**
** software implementation of Yamaha FM sound generator (YM2612/YM3438)
**
** Original code (MAME fm.c)
**
** Copyright (C) 2001, 2002, 2003 Jarek Burczynski (bujar at mame dot net)
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development
**
** Version 1.4 (final beta) 
**
** Additional code & fixes by Eke-Eke for Genesis Plus GX
**
*/

#ifndef _H_YM2612_
#define _H_YM2612_

#include <stdint.h>

extern void YM2612Init(double clock, int32_t rate);
extern void YM2612ResetChip(void);
extern void YM2612Update(int32_t *buffer, int32_t length);
extern void YM2612Write(uint32_t a, uint32_t v);
extern uint32_t YM2612Read(void);
extern uint8_t *YM2612GetContextPtr(void);
extern uint32_t YM2612GetContextSize(void);
extern void YM2612Restore(uint8_t *buffer);
extern int32_t YM2612LoadContext(uint8_t *state);
extern int32_t YM2612SaveContext(uint8_t *state);

#endif /* _YM2612_ */
