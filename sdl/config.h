
#ifndef _CONFIG_H_
#define _CONFIG_H_

/****************************************************************************
 * Config Option 
 *
 ****************************************************************************/
typedef struct 
{
  uint8_t padtype;
} t_input_config;

typedef struct 
{
  uint8_t hq_fm;
  uint8_t filter;
  uint8_t psgBoostNoise;
  uint8_t dac_bits;
  int16_t psg_preamp;
  int16_t fm_preamp;
  int16_t lp_range;
  int16_t low_freq;
  int16_t high_freq;
  int16_t lg;
  int16_t mg;
  int16_t hg;
  float rolloff;
  uint8_t region_detect;
  uint8_t force_dtack;
  uint8_t addr_error;
  uint8_t tmss;
  uint8_t lock_on;
  uint8_t hot_swap;
  uint8_t romtype;
  uint8_t invert_mouse;
  uint8_t gun_cursor[2];
  uint8_t overscan;
  uint8_t ntsc;
  uint8_t render;
  int16_t ym2413_enabled;
  t_input_config input[MAX_INPUTS];
} t_config;

/* Global variables */
extern t_config config;
extern void set_config_defaults(void);

#endif /* _CONFIG_H_ */

