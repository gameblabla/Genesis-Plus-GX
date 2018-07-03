/*
**
** File: ym2413.c - software implementation of YM2413
**                  FM sound generator type OPLL
**
** Copyright (C) 2002 Jarek Burczynski
**
** Version 1.0
**
*/

#ifndef _H_YM2413_
#define _H_YM2413_

extern void YM2413Init(double clock, int32_t rate);
extern void YM2413ResetChip(void);
extern void YM2413Update(int32_t *buffer, int32_t length);
extern void YM2413Write(uint32_t a, uint32_t v);
extern uint32_t YM2413Read(uint32_t a);
extern int8_t *YM2413GetContextPtr(void);
extern uint32_t YM2413GetContextSize(void);
extern void YM2413Restore(uint8_t *buffer);


#endif /*_H_YM2413_*/
