/* 
    SN76489 emulation
    by Maxim in 2001 and 2002
*/

#ifndef _SN76489_H_
#define _SN76489_H_

#include <stdint.h>

/* Function prototypes */

extern void SN76489_Init(double PSGClockValue, int32_t SamplingRate);
extern void SN76489_Reset(void);
extern void SN76489_Shutdown(void);
extern void SN76489_SetContext(uint8_t *data);
extern void SN76489_GetContext(uint8_t *data);
extern uint8_t *SN76489_GetContextPtr(void);
extern int32_t SN76489_GetContextSize(void);
extern void SN76489_Write(int32_t data);
extern void SN76489_Update(int16_t *buffer, int32_t length);
extern void SN76489_BoostNoise(int32_t boost);

#endif /* _SN76489_H_ */

