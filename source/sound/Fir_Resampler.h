/* Finite impulse response (FIR) resampler with adjustable FIR size */

/* Game_Music_Emu 0.5.2 */
#ifndef FIR_RESAMPLER_H
#define FIR_RESAMPLER_H

#include <stdint.h>

#define STEREO        2
#define MAX_RES       32
#define WIDTH         16
#define WRITE_OFFSET  (WIDTH * STEREO) - STEREO
#define GAIN          1.0

typedef int32_t sample_t;

extern int32_t Fir_Resampler_initialize( int32_t new_size );
extern void Fir_Resampler_shutdown( void );
extern void Fir_Resampler_clear( void );
extern double Fir_Resampler_time_ratio( double new_factor, double rolloff );
extern double Fir_Resampler_ratio( void );
extern int32_t Fir_Resampler_max_write( void );
extern sample_t* Fir_Resampler_buffer( void );
extern int32_t Fir_Resampler_written( void );
extern int32_t Fir_Resampler_avail( void );
extern void Fir_Resampler_write( int32_t count );
extern int32_t Fir_Resampler_read( sample_t* out, int32_t count );
extern int32_t Fir_Resampler_input_needed( int32_t output_count );
extern int32_t Fir_Resampler_skip_input( int32_t count );

#endif
