#include <SDL/SDL.h>
#include <stdint.h>
#include "shared.h"
#include "sms_ntsc.h"
#include "md_ntsc.h"

#define SOUND_FREQUENCY 48000
#define SOUND_SAMPLES_SIZE  2048

#define VIDEO_WIDTH  320 
#define VIDEO_HEIGHT 240

int32_t joynum = 0;
int32_t log_error   = 0;
int32_t debug_on    = 0;
int32_t use_sound   = 1;
int32_t n_snd;

/* sound */

struct 
{
  int8_t* current_pos;
  int8_t* buffer;
  int32_t current_emulated_samples;
} sdl_sound;

static void sdl_sound_callback(void *userdata, uint8_t *stream, int32_t len)
{
	if(sdl_sound.current_emulated_samples < len) 
	{
		memset(stream, 0, len);
	}
	else 
	{
		memcpy(stream, sdl_sound.buffer, len);
		/* loop to compensate desync */
		do 
		{
			sdl_sound.current_emulated_samples -= len;
		} while(sdl_sound.current_emulated_samples > 2 * len);
		memcpy(sdl_sound.buffer,
		sdl_sound.current_pos - sdl_sound.current_emulated_samples,
		sdl_sound.current_emulated_samples);
		sdl_sound.current_pos = sdl_sound.buffer + sdl_sound.current_emulated_samples;
	}
}

static int32_t sdl_sound_init()
{
	SDL_AudioSpec as_desired, as_obtained;
	
	SDL_memset(&as_desired, 0, sizeof(as_desired));
	SDL_memset(&as_obtained, 0, sizeof(as_obtained));
	
	as_desired.freq     = SOUND_FREQUENCY;
	as_desired.format   = AUDIO_S16LSB;
	as_desired.channels = 2;
	as_desired.samples  = SOUND_SAMPLES_SIZE;
	as_desired.callback = sdl_sound_callback;

	if(SDL_OpenAudio(&as_desired, &as_obtained) == -1) {
		printf("SDL Audio open failed\n");
		return 0;
	}

	if(as_desired.samples != as_obtained.samples) {
		printf("SDL Audio wrong setup\n");
		return 0;
	}

	sdl_sound.current_emulated_samples = 0;
	n_snd = SOUND_SAMPLES_SIZE * 2 * sizeof(int16_t) * 11;
	sdl_sound.buffer = (int8_t*)malloc(n_snd);
	if(!sdl_sound.buffer) 
	{
		printf("Can't allocate audio buffer\n");
		return 0;
	}
	memset(sdl_sound.buffer, 0, n_snd);
	sdl_sound.current_pos = sdl_sound.buffer;
	
	return 1;
}

static void sdl_sound_update()
{
	int32_t i;
	int16_t* p;
	int32_t size = audio_update();

	if (use_sound)
	{
		SDL_LockAudio();
		p = (int16_t*)sdl_sound.current_pos;
		for(i = 0; i < size; ++i) {
			*p = snd.buffer[0][i];
			++p;
			*p = snd.buffer[1][i];
			++p;
		}
		sdl_sound.current_pos = (int8_t*)p;
		sdl_sound.current_emulated_samples += size * 2 * sizeof(int16_t);
		SDL_UnlockAudio();
	}
}

static void sdl_sound_close()
{
	SDL_UnlockAudio();
	SDL_PauseAudio(1);
	if (sdl_sound.buffer) 
		free(sdl_sound.buffer);
	SDL_CloseAudio();
}

/* video */
md_ntsc_t *md_ntsc;
sms_ntsc_t *sms_ntsc;

struct {
  SDL_Surface* surf_screen;
  SDL_Rect srect;
  SDL_Rect drect;
  uint32_t frames_rendered;
} sdl_video;

static int32_t sdl_video_init()
{
	sdl_video.surf_screen  = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, 16, SDL_HWSURFACE);
	sdl_video.frames_rendered = 0;
	SDL_ShowCursor(0);
	return 1;
}

static void sdl_video_update()
{
	system_frame(0);
	SDL_Flip(sdl_video.surf_screen);
	++sdl_video.frames_rendered;
}

static void sdl_video_close()
{
	if (sdl_video.surf_screen)
		SDL_FreeSurface(sdl_video.surf_screen);
}


static int32_t sdl_control_update(SDLKey keystate)
{
    switch (keystate)
    {
      case SDLK_TAB:
      {
        system_init();
        system_reset();
        break;
      }
      case SDLK_F3:
      {
        config.render ^=1;
        break;
      }

      case SDLK_F5:
      {
        log_error ^= 1;
        break;
      }

      case SDLK_F7:
      {
        FILE *f = fopen("game.gpz","r+b");
        if (f)
        {
          uint8_t buf[STATE_SIZE];
          fread(&buf, STATE_SIZE, 1, f);
          state_load(buf);
          fclose(f);
        }
        break;
      }

      case SDLK_F8:
      {
        FILE *f = fopen("game.gpz","w+b");
        if (f)
        {
          uint8_t buf[STATE_SIZE];
          state_save(buf);
          fwrite(&buf, STATE_SIZE, 1, f);
          fclose(f);
        }
        break;
      }

      case SDLK_F9:
      {
        vdp_pal ^= 1;

        /* save YM2612 context */
        uint8_t *temp = malloc(YM2612GetContextSize());
        if (temp)
          memcpy(temp, YM2612GetContextPtr(), YM2612GetContextSize());

        /* reinitialize all timings */
        audio_init(snd.sample_rate, snd.frame_rate);
        system_init();

        /* restore YM2612 context */
        if (temp)
        {
          YM2612Restore(temp);
          free(temp);
        }
        
        /* reinitialize VC max value */
        static const uint16_t vc_table[4][2] = 
        {
          /* NTSC, PAL */
          {0xDA , 0xF2},  /* Mode 4 (192 lines) */
          {0xEA , 0x102}, /* Mode 5 (224 lines) */
          {0xDA , 0xF2},  /* Mode 4 (192 lines) */
          {0x106, 0x10A}  /* Mode 5 (240 lines) */
        };
        vc_max = vc_table[(reg[1] >> 2) & 3][vdp_pal];

        /* reinitialize display area */
        bitmap.viewport.changed = 3;
        break;
      }

      case SDLK_F10:
      {
        gen_reset(1);
        gen_reset(0);
        break;
      }

      case SDLK_F11:
      {
        config.overscan ^= 1;
        bitmap.viewport.changed = 3;
        break;
      }

      case SDLK_F12:
      {
        joynum = (joynum + 1) % MAX_DEVICES;
        while (input.dev[joynum] == NO_DEVICE)
        {
          joynum = (joynum + 1) % MAX_DEVICES;
        }
        break;
      }

      case SDLK_ESCAPE:
      {
        return 0;
      }

      default:
        break;
    }

   return 1;
}

int32_t sdl_input_update(void)
{
  uint8_t *keystate = SDL_GetKeyState(NULL);

  /* reset input */
  input.pad[joynum] = 0;
 
  switch (input.dev[joynum])
  {
    case DEVICE_LIGHTGUN:
    {
      /* get mouse (absolute values) */
      int32_t x,y;
      int32_t state = SDL_GetMouseState(&x,&y);

      /* Calculate X Y axis values */
      input.analog[joynum][0] = (x * bitmap.viewport.w) / VIDEO_WIDTH;
      input.analog[joynum][1] = (y * bitmap.viewport.h) / VIDEO_HEIGHT;

      /* Start,Left,Right,Middle buttons -> 0 0 0 0 START MIDDLE RIGHT LEFT */
      if(state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
      if(state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_C;
      if(state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_A;
      if(keystate[SDLK_f])  input.pad[joynum] |= INPUT_START;

      break;
    }

    case DEVICE_PADDLE:
    {
      /* get mouse (absolute values) */
      int32_t x;
      int32_t state = SDL_GetMouseState(&x, NULL);

      /* Range is [0;256], 128 being middle position */
      input.analog[joynum][0] = x * 256 /VIDEO_WIDTH;

      /* Button I -> 0 0 0 0 0 0 0 I*/
      if(state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;

      break;
    }

    case DEVICE_SPORTSPAD:
    {
      /* get mouse (relative values) */
      int32_t x,y;
      int32_t state = SDL_GetRelativeMouseState(&x,&y);

      /* Range is [0;256] */
      input.analog[joynum][0] = (uint8_t)(-x & 0xFF);
      input.analog[joynum][1] = (uint8_t)(-y & 0xFF);

      /* Buttons I & II -> 0 0 0 0 0 0 II I*/
      if(state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
      if(state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_C;

      break;
    }

    case DEVICE_MOUSE:
    {
      /* get mouse (relative values) */
      int32_t x,y;
      int32_t state = SDL_GetRelativeMouseState(&x,&y);

      /* Sega Mouse range is [-256;+256] */
      input.analog[joynum][0] = x * 2;
      input.analog[joynum][1] = y * 2;

      /* Vertical movement is upsidedown */
      if (!config.invert_mouse)
        input.analog[joynum][1] = 0 - input.analog[joynum][1];

      /* Start,Left,Right,Middle buttons -> 0 0 0 0 START MIDDLE RIGHT LEFT */
      if(state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
      if(state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_C;
      if(state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_A;
      if(keystate[SDLK_f])  input.pad[joynum] |= INPUT_START;

      break;
    }

    case DEVICE_XE_A1P:
    {
      /* A,B,C,D,Select,START,E1,E2 buttons -> E1(?) E2(?) START SELECT(?) A B C D */
      if(keystate[SDLK_a])  input.pad[joynum] |= INPUT_START;
      if(keystate[SDLK_s])  input.pad[joynum] |= INPUT_A;
      if(keystate[SDLK_d])  input.pad[joynum] |= INPUT_C;
      if(keystate[SDLK_f])  input.pad[joynum] |= INPUT_Y;
      if(keystate[SDLK_z])  input.pad[joynum] |= INPUT_B;
      if(keystate[SDLK_x])  input.pad[joynum] |= INPUT_X;
      if(keystate[SDLK_c])  input.pad[joynum] |= INPUT_MODE;
      if(keystate[SDLK_v])  input.pad[joynum] |= INPUT_Z;
      
      /* Left Analog Stick (bidirectional) */
      if(keystate[SDLK_UP])     input.analog[joynum][1]-=2;
      else if(keystate[SDLK_DOWN])   input.analog[joynum][1]+=2;
      else input.analog[joynum][1] = 128;
      if(keystate[SDLK_LEFT])   input.analog[joynum][0]-=2;
      else if(keystate[SDLK_RIGHT])  input.analog[joynum][0]+=2;
      else input.analog[joynum][0] = 128;

      /* Right Analog Stick (unidirectional) */
      if(keystate[SDLK_KP8])    input.analog[joynum+1][0]-=2;
      else if(keystate[SDLK_KP2])   input.analog[joynum+1][0]+=2;
      else if(keystate[SDLK_KP4])   input.analog[joynum+1][0]-=2;
      else if(keystate[SDLK_KP6])  input.analog[joynum+1][0]+=2;
      else input.analog[joynum+1][0] = 128;

      /* Limiters */
      if (input.analog[joynum][0] > 0xFF) input.analog[joynum][0] = 0xFF;
      else if (input.analog[joynum][0] < 0) input.analog[joynum][0] = 0;
      if (input.analog[joynum][1] > 0xFF) input.analog[joynum][1] = 0xFF;
      else if (input.analog[joynum][1] < 0) input.analog[joynum][1] = 0;
      if (input.analog[joynum+1][0] > 0xFF) input.analog[joynum+1][0] = 0xFF;
      else if (input.analog[joynum+1][0] < 0) input.analog[joynum+1][0] = 0;
      if (input.analog[joynum+1][1] > 0xFF) input.analog[joynum+1][1] = 0xFF;
      else if (input.analog[joynum+1][1] < 0) input.analog[joynum+1][1] = 0;

      break;
    }

    case DEVICE_ACTIVATOR:
    {
      if(keystate[SDLK_g])  input.pad[joynum] |= INPUT_ACTIVATOR_7L;
      if(keystate[SDLK_h])  input.pad[joynum] |= INPUT_ACTIVATOR_7U;
      if(keystate[SDLK_j])  input.pad[joynum] |= INPUT_ACTIVATOR_8L;
      if(keystate[SDLK_k])  input.pad[joynum] |= INPUT_ACTIVATOR_8U;
    }

    default:
    {
      if(keystate[SDLK_LCTRL])  input.pad[joynum] |= INPUT_A;
      if(keystate[SDLK_LALT])  input.pad[joynum] |= INPUT_B;
      if(keystate[SDLK_LSHIFT])  input.pad[joynum] |= INPUT_C;
      if(keystate[SDLK_RETURN])  input.pad[joynum] |= INPUT_START;
      if(keystate[SDLK_SPACE])  input.pad[joynum] |= INPUT_X;
      if(keystate[SDLK_TAB])  input.pad[joynum] |= INPUT_Y;
      if(keystate[SDLK_BACKSPACE])  input.pad[joynum] |= INPUT_Z;
      if(keystate[SDLK_v])  input.pad[joynum] |= INPUT_MODE;

      if(keystate[SDLK_UP])     input.pad[joynum] |= INPUT_UP;
      else
      if(keystate[SDLK_DOWN])   input.pad[joynum] |= INPUT_DOWN;
      if(keystate[SDLK_LEFT])   input.pad[joynum] |= INPUT_LEFT;
      else
      if(keystate[SDLK_RIGHT])  input.pad[joynum] |= INPUT_RIGHT;

      break;
    }
  }

  if (system_hw == SYSTEM_PICO)
  {
    /* get mouse (absolute values) */
    int32_t x,y;
    int32_t state = SDL_GetMouseState(&x,&y);

    /* Calculate X Y axis values */
    input.analog[0][0] = 0x3c  + (x * (0x17c-0x03c+1)) / VIDEO_WIDTH;
    input.analog[0][1] = 0x1fc + (y * (0x2f7-0x1fc+1)) / VIDEO_HEIGHT;
 
    /* Map mouse buttons to player #1 inputs */
    if(state & SDL_BUTTON_MMASK) pico_current++;
    if(state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_B;
    if(state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_A;
  }

  return 1;
}

static void msleep(uint8_t milisec)
{
	struct timespec req={0};
	time_t sec=(uint16_t)(milisec/1000);

	milisec=milisec-(sec*1000);
	req.tv_sec=sec;
	req.tv_nsec=milisec*1000000L;

	while(nanosleep(&req,&req)==-1)
	continue;
}


int main (int argc, char *argv[])
{
	uint8_t running = 1;
	uint32_t start;

	/* Print help if no game specified */
	if(argc < 2)
	{
		printf("Genesis Plus\\SDL by Charles MacDonald\nWWW: http://cgfm2.emuviews.com\nusage: %s gamename\n", argv[0]);
		return 0;
	}

	/* set default config */
	error_init();
	set_config_defaults();

	/* Load ROM file */
	cart.rom = malloc(10*1024*1024);
	memset(cart.rom, 0, 10*1024*1024);
	if(!load_rom(argv[1]))
	{
		printf("Error loading file `%s'.", argv[1]);
		return 0;
	}

	/* load BIOS */
	memset(bios_rom, 0, sizeof(bios_rom));
	FILE *f = fopen(OS_ROM, "rb");
	if (f!=NULL)
	{
		fread(&bios_rom, 0x800,1,f);
		fclose(f);
		int i;
		for(i = 0; i < 0x800; i += 2)
		{
			uint8_t temp = bios_rom[i];
			bios_rom[i] = bios_rom[i+1];
			bios_rom[i+1] = temp;
		}
		config.tmss |= 2;
	}

	/* initialize SDL */
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	sdl_video_init();
	if (use_sound) sdl_sound_init();

	/* initialize Genesis virtual system */
	SDL_LockSurface(sdl_video.surf_screen);
	memset(&bitmap, 0, sizeof(t_bitmap));
	bitmap.width        = 320;
	bitmap.height       = 240;
	bitmap.depth        = 16;
	bitmap.granularity  = 2;
	bitmap.pitch        = (bitmap.width * bitmap.granularity);
	bitmap.data         = sdl_video.surf_screen->pixels + (vdp_pal ? 0 : 320*16);
	SDL_UnlockSurface(sdl_video.surf_screen);
	bitmap.viewport.changed = 3;

	/* initialize emulation */
	audio_init(SOUND_FREQUENCY, vdp_pal ? 50.0 : 60.0);
	system_init();

	/* load SRAM */
	f = fopen("./game.srm", "rb");
	if (f!=NULL)
	{
		fread(sram.sram,0x10000,1, f);
		fclose(f);
	}

	/* reset emulation */
	system_reset();

	if(use_sound) SDL_PauseAudio(0);

	float real_FPS = 1000.0f/(float)(vdp_pal ? 50.0f : 60.0f);

	/* emulation loop */
	while(running)
	{
		start = SDL_GetTicks();
		sdl_video_update();
		sdl_sound_update();
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch(event.type) 
			{
				case SDL_QUIT:
					running = 0;
				break;

				case SDL_KEYDOWN:
					running = sdl_control_update(event.key.keysym.sym);
				break;
			}
		}
		
		#ifndef NOLIMIT
		if(real_FPS > (SDL_GetTicks()-start))
			msleep(real_FPS-(SDL_GetTicks()-start));
		#endif
	}

	/* save SRAM */
	f = fopen("./game.srm", "wb");
	if (f != NULL)
	{
		fwrite(sram.sram,0x10000,1, f);
		fclose(f);
	}

	system_shutdown();
	audio_shutdown();
	error_shutdown();
	if (cart.rom) free(cart.rom);
	sdl_sound_close();
	sdl_video_close();
	SDL_Quit();
	
	return 0;
}
