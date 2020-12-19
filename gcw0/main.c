#ifdef __WIN32__
#include <windows.h>
#else
#define MessageBox(owner, text, caption, type) printf("%s: %s\n", caption, text)
#endif

#include <SDL.h>
#include <SDL_thread.h>

#include "shared.h"
#include "sms_ntsc.h"
#include "md_ntsc.h"
#include "utils.h"

#include <SDL_ttf.h>
#ifndef DINGOO
#include <SDL_image.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#define CHUNKSIZE 1024
#define PRINTSETTING(px, py, setting) \
{ \
    textSurface = TTF_RenderText_Solid(ttffont, setting, selected_text_color); \
    drect.x = px - textSurface->w; \
    drect.y = py; \
    drect.w = textSurface->w; \
    drect.h = textSurface->h; \
    SDL_BlitSurface(textSurface, NULL, menuSurface, &drect); \
    SDL_FreeSurface(textSurface); \
}
#define SOUND_FREQUENCY 44100
#define SOUND_SAMPLES_SIZE 2048
#define SOUND_FREQUENCY_VR 11025 //reduce sound frequency to allow svp chip emulation
#define SOUND_SAMPLES_SIZE_VR 512

#if defined(USE_8BPP_RENDERING)
#define BPP 8
#elif defined(USE_16BPP_RENDERING)
#define BPP 16
#elif defined(USE_32BPP_RENDERING)
#define BPP 32
#endif
#define VIDEO_WIDTH  320
#define VIDEO_HEIGHT 240

unsigned int joynum = 0;
uint fullscreen = 1; /* SDL_FULLSCREEN */
uint8  clearSoundBuf  = 0;
uint8  do_once        = 1;
uint32 gcw0_w         = 320;
uint32 gcw0_h         = 240;
uint8  goto_menu      = 0;
uint8  show_lightgun  = 0;
uint8  virtua_racing  = 0;
uint   post           = 0;
uint8  frameskip      = 0;
uint8  afA            = 0;
uint8  afB            = 0;
uint8  afC            = 0;
uint8  afX            = 0;
uint8  afY            = 0;
uint8  afZ            = 0;

//uint16 *bitmapline[240];

time_t current_time;
char rom_filename[256];
#ifdef SDL2
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
#endif
const char *cursor[4]=
{
    "./CLASSIC_01_RED.png", //doesn't flash (for epileptics it's default)
    "./CLASSIC_02.png",     //square flashing red and white
    "./CLASSIC_01.png",
    "./SQUARE_02.png",
};

/* Sound */
struct
{
    char* current_pos;
    char* buffer;
    unsigned int current_emulated_samples;
} sdl_sound;

static uint8 brm_format[0x40] =
{
    0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x00,0x00,0x00,0x00,0x40,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x53,0x45,0x47,0x41,0x5f,0x43,0x44,0x5f,0x52,0x4f,0x4d,0x00,0x01,0x00,0x00,0x00,
    0x52,0x41,0x4d,0x5f,0x43,0x41,0x52,0x54,0x52,0x49,0x44,0x47,0x45,0x5f,0x5f,0x5f
};

static short soundframe[SOUND_SAMPLES_SIZE];

static void sdl_sound_update(int enabled)
{
    unsigned int size = audio_update(soundframe) ;

    if (enabled)
    {
        unsigned int i = 0;
        short *out;

        SDL_LockAudio();
        out = (short*)sdl_sound.current_pos;

        unsigned int n = (size+15)/16;
        switch (size % 16) {
        case 0: do { {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 15:     {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 14:     {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 13:     {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 12:     {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 11:     {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 10:     {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 9:      {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 8:      {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 7:      {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 6:      {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 5:      {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 4:      {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 3:      {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 2:      {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        case 1:      {*out++ = soundframe[i++];*out++ = soundframe[i++];}
        } while ( --n > 0 );
        }

        sdl_sound.current_pos = (char*)out;
        sdl_sound.current_emulated_samples += (size + size) * sizeof(short);
        SDL_UnlockAudio();
    }
}

static void sdl_sound_callback(void *userdata, Uint8 *stream, int len)
{
    unsigned int ces = sdl_sound.current_emulated_samples;
    if(clearSoundBuf)
    {
        clearSoundBuf--;
        memset(stream, 0, len);
    } else
    if (ces < len)
    {
        if(config.optimisations && !goto_menu)
        {
            if(!frameskip) frameskip++;
            memcpy(stream, sdl_sound.buffer, len);
            memcpy(sdl_sound.buffer, sdl_sound.current_pos - ces, ces);
            sdl_sound.current_pos = sdl_sound.buffer + ces;
        }
        else if(config.skip_prevention && !goto_menu)
        {
            memcpy(stream, sdl_sound.buffer, len);
            memcpy(sdl_sound.buffer, sdl_sound.current_pos - ces, ces);
            sdl_sound.current_pos = sdl_sound.buffer + ces;
        }
        else
            memset(stream, 0, len);
    }
    else
    {
        static int count = 10;
        if(config.optimisations)
        {
          if(!(--count))
          {
            if(frameskip) frameskip--;
            count = 9;
          }
        } else frameskip = 0;
        unsigned int len_times_two = len + len;
        memcpy(stream, sdl_sound.buffer, len);

        /* loop to compensate desync */
        do
        {
            ces -= len;
        }
        while(ces > len_times_two);

        sdl_sound.current_emulated_samples = ces;
        memcpy(sdl_sound.buffer, sdl_sound.current_pos - ces, ces);
        sdl_sound.current_pos = sdl_sound.buffer + ces;
    }
}

static int sdl_sound_init()
{
    /* Create sound buffer in stack */
    char n[SOUND_SAMPLES_SIZE * 2 * sizeof(short) * 20];

    SDL_AudioSpec as_desired, as_obtained;

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        MessageBox(NULL, "SDL Audio initialization failed", "Error", 0);
        return 0;
    }

    if (strstr(rominfo.international,"Virtua Racing"))
    {
        as_desired.freq     = SOUND_FREQUENCY_VR; //speed hack
        as_desired.samples  = SOUND_SAMPLES_SIZE_VR;
    }
    else
    {
        as_desired.freq     = SOUND_FREQUENCY;
        as_desired.samples  = SOUND_SAMPLES_SIZE;
    }
    as_desired.format   = AUDIO_S16LSB;
    as_desired.channels = 2;
    as_desired.callback = sdl_sound_callback;

    if (SDL_OpenAudio(&as_desired, &as_obtained) == -1)
    {
        MessageBox(NULL, "SDL Audio open failed", "Error", 0);
        return 0;
    }

    if (as_desired.samples != as_obtained.samples)
    {
        MessageBox(NULL, "SDL Audio wrong setup", "Error", 0);
        return 0;
    }

    sdl_sound.current_emulated_samples = 0;
    sdl_sound.buffer = (char*)n;
    if (!sdl_sound.buffer)
    {
        MessageBox(NULL, "Can't allocate audio buffer", "Error", 0);
        return 0;
    }
    sdl_sound.current_pos = sdl_sound.buffer;
    return 1;
}

static void sdl_sound_close()
{
    SDL_PauseAudio(1);
    SDL_Delay(50);
    SDL_CloseAudio();
}

static void sdl_joystick_init()
{
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0)
        MessageBox(NULL, "SDL Joystick initialization failed", "Error", 0);
    else
        MessageBox(NULL, "SDL Joystick initialisation successful", "Success", 0);
    return;
}

/* video */
md_ntsc_t *md_ntsc;
sms_ntsc_t *sms_ntsc;


struct
{
    SDL_Surface* surf_bitmap;
    SDL_Surface* surf_screen;
    SDL_Rect srect;
    SDL_Rect drect;
    SDL_Rect my_srect; //for blitting small portions of the screen (custom blitter)
    SDL_Rect my_drect; //
    Uint32 frames_rendered;
} sdl_video;

static int sdl_video_init()
{
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
        MessageBox(NULL, "SDL Video initialization failed", "Error", 0);
        return 0;
    }
#ifdef SDL2
    window                = SDL_CreateWindow("genplus", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 240,  SDL_WINDOW_OPENGL);
    renderer              = SDL_CreateRenderer(window, -1, 0);
    sdl_video.surf_bitmap = SDL_CreateRGBSurface(0, 320, 240, 32, 0, 0, 0, 0);    /* You will need to change the pixelformat if using 32bits etc */
    texture               = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 320, 240);
#else
#ifdef DINGOO
    sdl_video.surf_screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, BPP, SDL_SWSURFACE | SDL_FULLSCREEN);
#else //GCWZERO
    if     (config.renderer == 0) //Triple buffering
        sdl_video.surf_screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, BPP, SDL_HWSURFACE | SDL_FULLSCREEN | SDL_TRIPLEBUF);
    else if(config.renderer == 1) //Double buffering
        sdl_video.surf_screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, BPP, SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF);
    else if(config.renderer == 2) //Software rendering
        sdl_video.surf_screen = SDL_SetVideoMode(VIDEO_WIDTH, VIDEO_HEIGHT, BPP, SDL_SWSURFACE | SDL_FULLSCREEN                );
#endif
    sdl_video.surf_bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, VIDEO_WIDTH, VIDEO_HEIGHT, BPP, 0, 0, 0, 0);
    SDL_ShowCursor(0);
#endif //SDL2
    sdl_video.frames_rendered = 0;
    return 1;
}

enum {FRAME_START = 0, FRAME_GEN = 1, FRAME_BLIT = 2, FRAME_FLIP = 3};
static int frame_progress = FRAME_START;
static uint skipval   = 0;

static void sdl_render_frame(void)
{
    if (system_hw == SYSTEM_MCD)
    {
      if(frameskip)
      {
        if (frame_progress == FRAME_START)
        {
            system_frame_scd(0); //render frame
            frame_progress = FRAME_GEN;
            skipval = 0;
        } else
        {
            system_frame_scd(frameskip); //skip frame render
            ++skipval;
        }
      } else
      {
            system_frame_scd(0); //render frame
            frame_progress = FRAME_GEN;
      }
    }
    else if ((system_hw & SYSTEM_PBC) == SYSTEM_MD)
    {
      if(frameskip)
      {
        if (frame_progress == FRAME_START)
        {
            system_frame_gen(0);
            frame_progress = FRAME_GEN;
            skipval = 0;
        } else
        {
            system_frame_gen(frameskip);
            ++skipval;
        }
      } else
      {
            system_frame_gen(0);
            frame_progress = FRAME_GEN;
      }
    }
    else
    {
      if(frameskip)
      {
        if (frame_progress == FRAME_START)
        {
            system_frame_sms(0);
            frame_progress = FRAME_GEN;
            skipval = 0;
        } else
        {
            system_frame_sms(1);
            ++skipval;
        }
      }
      else
      {
            system_frame_sms(0);
            frame_progress = FRAME_GEN;
      }
    }
}

static void viewport_size_changed(void)
{
    if (bitmap.viewport.changed & 1)
    {
        bitmap.viewport.changed &= ~1;

        /* source bitmap */
        //remove left bar with SMS roms
        if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
        {
            if (config.smsmaskleftbar) sdl_video.srect.x = 8;
            else                       sdl_video.srect.x = 0;
        }
        else   sdl_video.srect.x = 0;
        sdl_video.srect.y = 0;
        sdl_video.srect.w = bitmap.viewport.w + (2 * bitmap.viewport.x);
        sdl_video.srect.h = bitmap.viewport.h + (2 * bitmap.viewport.y);
        if (sdl_video.srect.w > VIDEO_WIDTH)
        {
            sdl_video.srect.x = (sdl_video.srect.w - VIDEO_WIDTH) / 2;
            sdl_video.srect.w = VIDEO_WIDTH;
            if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
                if (config.smsmaskleftbar)
                    sdl_video.srect.x += 8;
        }
        if (sdl_video.srect.h > VIDEO_HEIGHT)
        {
            sdl_video.srect.y = (sdl_video.srect.h - VIDEO_HEIGHT) / 2;
            sdl_video.srect.h = VIDEO_HEIGHT;
        }

        /* Destination bitmap */
        sdl_video.drect.w = sdl_video.srect.w;
        sdl_video.drect.h = sdl_video.srect.h;
        if (config.gcw0_fullscreen) sdl_video.drect.x = 0;
        else                        sdl_video.drect.x = (VIDEO_WIDTH  - sdl_video.drect.w) / 2;
        sdl_video.drect.y = (VIDEO_HEIGHT - sdl_video.drect.h) / 2;

        /* clear destination surface */
#ifdef SDL2
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_RenderClear(renderer);
#else
        int i;
        for(i = 3; i != 0; --i)
        {
            SDL_FillRect(sdl_video.surf_screen, 0, 0);
            if(config.renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
            else                    SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
        }
#endif
    }
}

static void sdl_flip_screen(void)
{
    if ( !frameskip || skipval )
    {
#ifdef SDL2
        SDL_UpdateTexture(texture, NULL, bitmap.data, 320 * sizeof(Uint32));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
#else
        if(frame_progress == FRAME_BLIT)
        {
          if(config.renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
          else                    SDL_UpdateRect(sdl_video.surf_screen, sdl_video.my_drect.x, sdl_video.my_drect.y, sdl_video.my_drect.w, sdl_video.my_drect.h);
          frame_progress = FRAME_START;
        }
        else if(show_lightgun)
        {
          if(config.renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
          else                    SDL_UpdateRect(sdl_video.surf_screen, sdl_video.my_drect.x, sdl_video.my_drect.y, sdl_video.my_drect.w, sdl_video.my_drect.h);
          frame_progress = FRAME_START;
        }
#endif
        skipval = 0;
    }
}

static void sdl_video_update()
{
    sdl_render_frame();

    /* Viewport size changed */
    viewport_size_changed();

    /* IPU scaling for gg/sms roms */
#ifndef SDL2
    if (config.gcw0_fullscreen)
    {
        if ( (gcw0_w != sdl_video.drect.w) || (gcw0_h != sdl_video.drect.h) )
        {
            if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
            {
                if (config.smsmaskleftbar)
                {
                    sdl_video.srect.w = sdl_video.srect.w - 8;
                    sdl_video.drect.w = sdl_video.srect.w;
                    sdl_video.drect.x = 4;
                }
                else
                {
                    sdl_video.srect.w = sdl_video.srect.w ;
                    sdl_video.drect.w = sdl_video.srect.w;
                    sdl_video.drect.x = 0;
                }

            }
            else
            {
                sdl_video.drect.x = 0;
                sdl_video.drect.w = sdl_video.srect.w;
            }

            if (strstr(rominfo.international,"Virtua Racing"))
            {
                sdl_video.srect.y = (sdl_video.srect.h - VIDEO_HEIGHT) / 2 + 24;
                sdl_video.drect.h = sdl_video.srect.h = 192;
                sdl_video.drect.y = 0;
            }
            else
            {
                sdl_video.drect.h = sdl_video.srect.h;
                sdl_video.drect.y = 0;
            }

            gcw0_w = sdl_video.drect.w;
            gcw0_h = sdl_video.drect.h;

            if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
            {
#ifdef SDL2
#else
#ifdef DINGOO
                sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_SWSURFACE                );
#else
                if      (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_HWSURFACE | SDL_TRIPLEBUF);
                else if (config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);
                else if (config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_SWSURFACE                );
#endif //DINGOO
#endif //SDL2
            }
            else
            {
#ifdef SDL2
#else
#ifdef DINGOO
                sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_SWSURFACE                );
#else
                if           (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, BPP, SDL_HWSURFACE | SDL_TRIPLEBUF);
                else if      (config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);
                else if      (config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, BPP, SDL_SWSURFACE                );
#endif //DINGOO
#endif //SDL2
            }
        }
    }

    if (show_lightgun && !config.gcw0_fullscreen)  //hack to remove cursor corruption if over game screen edge
        SDL_FillRect(sdl_video.surf_screen, 0, 0);

    if (!frameskip || skipval )
    {
      if(frame_progress == FRAME_GEN)
      {
        {
          SDL_BlitSurface(sdl_video.surf_bitmap, &sdl_video.srect, sdl_video.surf_screen, &sdl_video.drect);
          skipval = 0;
        }
        frame_progress = FRAME_BLIT;
      }
      else if (show_lightgun)
      {
        SDL_BlitSurface(sdl_video.surf_bitmap, &sdl_video.srect, sdl_video.surf_screen, &sdl_video.drect);
        skipval = 1;
        frame_progress = FRAME_BLIT;
      }
    }
#endif //!SDL2
    /* Add scanlines to Game Gear games if requested */
    if ( (system_hw == SYSTEM_GG) && config.gg_scanlines)
    {
        SDL_Surface *scanlinesSurface;
        scanlinesSurface = IMG_Load("./scanlines.png");
        SDL_BlitSurface(scanlinesSurface, NULL, sdl_video.surf_screen, &sdl_video.drect);
	SDL_FreeSurface(scanlinesSurface);
    }
    if (show_lightgun)
    {
        /* Remove previous cursor from black bars */
        if (config.gcw0_fullscreen)
        {
            if (config.smsmaskleftbar && system_hw == SYSTEM_SMS2)
            {
                SDL_Rect srect;
                srect.x = srect.y = 0;
                srect.w = 4;
                srect.h = 192;
                SDL_FillRect(sdl_video.surf_screen, &srect, SDL_MapRGB(sdl_video.surf_screen->format, 0, 0, 0));
                srect.x = 252;
                SDL_FillRect(sdl_video.surf_screen, &srect, SDL_MapRGB(sdl_video.surf_screen->format, 0, 0, 0));
            }
        }
        /* Get mouse coordinates (absolute values) */
        int x,y;
        SDL_GetMouseState(&x,&y);

        SDL_Rect lrect;
        lrect.x = x-7;
        lrect.y = y-7;
        lrect.w = lrect.h = 15;

        SDL_Surface *lightgunSurface;
        lightgunSurface = IMG_Load(cursor[config.cursor]);
        static int lightgun_af = 0;
        SDL_Rect srect;
        srect.y = 0;
        srect.w = srect.h = 15;

        /* Only show cursor if movement occurred within 3 seconds */
        time_t current_time2;
        current_time2 = time(NULL);

        if (lightgun_af >= 10)
        {
            srect.x = 0;
            if ( ( current_time2 - current_time ) < 3 )
                SDL_BlitSurface(lightgunSurface, &srect, sdl_video.surf_screen, &lrect);
        }
        else
        {
            if (config.cursor != 0) srect.x = 15;
            else                    srect.x = 0;
            if ( ( current_time2 - current_time ) < 3 )
                SDL_BlitSurface(lightgunSurface, &srect, sdl_video.surf_screen, &lrect);
        }
        lightgun_af++;
        if (lightgun_af == 20) lightgun_af = 0;
        SDL_FreeSurface(lightgunSurface);
    } //show_lightgun

    sdl_flip_screen();

    if (++sdl_video.frames_rendered == 3)
        sdl_video.frames_rendered = 0;
}

static void sdl_video_close()
{
    if (sdl_video.surf_bitmap)
        SDL_FreeSurface(sdl_video.surf_bitmap);
    if (sdl_video.surf_screen)
        SDL_FreeSurface(sdl_video.surf_screen);
}

/* Timer Sync */

struct
{
    SDL_sem* sem_sync;
    unsigned ticks;
} sdl_sync;

Uint32 sdl_sync_timer_callback(Uint32 interval, void *param)
{
    SDL_SemPost(sdl_sync.sem_sync);
    post++;
    return interval;
}

static int sdl_sync_init()
{
#ifdef SDL2
    if (SDL_InitSubSystem(SDL_INIT_TIMER|SDL_INIT_EVENTS) < 0)
#else
    if (SDL_InitSubSystem(SDL_INIT_TIMER|SDL_INIT_EVENTTHREAD) < 0)
#endif
    {
        MessageBox(NULL, "SDL Timer initialization failed", "Error", 0);
        return 0;
    }

    sdl_sync.sem_sync = SDL_CreateSemaphore(0);
    sdl_sync.ticks = 0;
    return 1;
}

static void sdl_sync_close()
{
    if (sdl_sync.sem_sync)
        SDL_DestroySemaphore(sdl_sync.sem_sync);
}

static const uint16 vc_table[4][2] =
{
    /* NTSC, PAL */
    {0xDA , 0xF2},  /* Mode 4 (192 lines) */
    {0xEA , 0x102}, /* Mode 5 (224 lines) */
    {0xDA , 0xF2},  /* Mode 4 (192 lines) */
    {0x106, 0x10A}  /* Mode 5 (240 lines) */
};
#ifdef SDL2
static int sdl_control_update(SDL_Keycode keystate)
#else
static int sdl_control_update(SDLKey keystate)
#endif
{
    return 1;
}

static void shutdown(void)
{
    FILE *fp;

    if (system_hw == SYSTEM_MCD)
    {
        /* save internal backup RAM (if formatted) */
        char brm_file[256];
        if (!memcmp(scd.bram + 0x2000 - 0x20, brm_format + 0x20, 0x20))
        {
            sprintf(brm_file,"%s/%s", get_save_directory(), "scd.brm");
            fp = fopen(brm_file, "wb");
            if (fp!=NULL)
            {
                fwrite(scd.bram, 0x2000, 1, fp);
                fclose(fp);
            }
        }

        /* save cartridge backup RAM (if formatted) */
        if (scd.cartridge.id)
        {
            if (!memcmp(scd.cartridge.area + scd.cartridge.mask + 1 - 0x20, brm_format + 0x20, 0x20))
            {
                sprintf(brm_file,"%s/%s", get_save_directory(), "cart.brm");
                fp = fopen(brm_file, "wb");
                if (fp!=NULL)
                {
                    fwrite(scd.cartridge.area, scd.cartridge.mask + 1, 1, fp);
                    fclose(fp);
                }
            }
        }
    }

    if (sram.on) //DJK Most of this code is modified from ../gx/fileio/file_slot.c
    {
        /* save SRAM */
        char save_file[256];
        if (rom_filename[0] != '\0')
        {
            sprintf(save_file,"%s/%s.srm", get_save_directory(), rom_filename);
            fp = fopen(save_file, "wb");
            if (fp!=NULL)
            {
                /* max. supported SRAM size */
                unsigned long filesize = 0x10000;

                uint8_t *buffer = 0;

                /* only save modified SRAM size */
                do
                {
                    if (sram.sram[filesize-1] != 0xff)
                        break;
                }
                while (--filesize > 0);

                /* only save if SRAM has been modified */
                if ((filesize != 0) || (crc32(0, &sram.sram[0], 0x10000) != sram.crc))
                {
                    printf("\nSaving SRAM");
                    /* allocate buffer */
                    buffer = (uint8_t *)memalign(32, filesize);
                    if (!buffer)
                        printf("\nDEBUG Warning, no buffer found!");

                    /* copy SRAM data */
                    memcpy(buffer, sram.sram, filesize);

                    /* update CRC */
                    sram.crc = crc32(0, sram.sram, 0x10000);

                    int done = 0;

                    /* Write from buffer (2k blocks) */
                    while (filesize > CHUNKSIZE)
                    {
                         fwrite(buffer + done, CHUNKSIZE, 1, fp);
                         done += CHUNKSIZE;
                         filesize -= CHUNKSIZE;
                    }

                    /* Write remaining bytes */
                    fwrite(buffer + done, filesize, 1, fp);
                    done += filesize;
                }
            fclose(fp);
            }
        }
    }

    audio_shutdown();
    error_shutdown();

    sdl_sound_close();
    sdl_video_close();
    sdl_sync_close();
    SDL_Quit();
}

void gcw0_savestate(int slot)
{
    char save_state_file[256];
    sprintf(save_state_file,"%s/%s.gp%d", get_save_directory(), rom_filename, slot);
    FILE *f = fopen(save_state_file,"wb");
    if (f)
    {
        uint8 buf[STATE_SIZE];
        int len = state_save(buf);
        fwrite(&buf, len, 1, f);
        fclose(f);
    }
}

void gcw0_loadstate(int slot)
{
    char save_state_file[256];
    sprintf(save_state_file,"%s/%s.gp%d", get_save_directory(), rom_filename, slot);
    FILE *f = fopen(save_state_file,"rb");
    if (f)
    {
        uint8 buf[STATE_SIZE];
        fread(&buf, STATE_SIZE, 1, f);
        state_load(buf);
        fclose(f);
    }
}

static void gcw0menu(void)
{
    SDL_PauseAudio(1);
    enum {MAINMENU = 0, GRAPHICS_OPTIONS = 10, REMAP_OPTIONS = 20, SAVE_STATE = 30, LOAD_STATE = 40, MISC_OPTIONS = 50, AUTOFIRE_OPTIONS = 60, SOUND_OPTIONS = 70};
    static int menustate = MAINMENU;
    static int menu_fade = 0;
    static int start_menu = 0;
    static int resizeScreen = 0;
    static int renderer;
    SDL_Rect srect;
    SDL_Rect drect;
    renderer = config.renderer;

    /* Menu text */
    const char *gcw0menu_mainlist[10]=     { "Resume game", "Autofire toggle", "Save state", "Load state", "Graphics options", "Sound options", "Remap buttons", "Misc. Options", "Reset", "Quit" };
    const char *gcw0menu_autofirelist[7]=  { "Return to main menu", "Toggle A", "Toggle B", "Toggle C", "Toggle X", "Toggle Y", "Toggle Z" };
    const char *gcw0menu_gfxlist[5]=       { "Return to main menu", "Renderer", "Scaling", "Keep aspect ratio", "Scanlines (GG)" };
    const char *gcw0menu_sndlist[4]=       { "Return to main menu", "Sound", "FM sound (SMS)", "Stop lag" };
    const char *gcw0menu_numericlist[5]=   { "0", "1", "2", "3", "4" };
    const char *gcw0menu_optimisations[2]= { "Off", "On" };
    const char *gcw0menu_renderer[3]=      { "HW Triple Buf", "HW Double Buf", "Software" };
    const char *gcw0menu_onofflist[2]=     { "Off", "On" };
    const char *gcw0menu_deadzonelist[7]=  { " 0", " 5,000", " 10,000", " 15,000", " 20,000", " 25,000", " 30,000" };
    const char *gcw0menu_remapoptions[9]=  { "Return to main menu", "A", "B", "C", "X", "Y", "Z", "Start", "Mode" };
    const char *gcw0menu_savestate[10]=    { "Back to main menu", "Save state 1 (Quicksave)", "Save state 2", "Save state 3", "Save state 4", "Save state 5", "Save state 6", "Save state 7", "Save state 8", "Save state 9" };
    const char *gcw0menu_loadstate[10]=    { "Back to main menu", "Load state 1 (Quickload)", "Load state 2", "Load state 3", "Load state 4", "Load state 5", "Load state 6", "Load state 7", "Load state 8", "Load state 9" };
    const char *gcw0menu_misc[8]=          { "Back to main menu", "Optimisations (MCD/VR)", "Resume on Save/Load", "A-stick", "A-stick deadzone", "Lock-on(MD)", "Lightgun speed", "Lightgun Cursor" };
    const char *lock_on_desc[4]=           { "Off", "Game Genie", "Action Replay", " Sonic&Knuckles" };

    /* Save current configuration settings */
    save_current_config();

    /* Setup fonts */
    TTF_Init();
    TTF_Font *ttffont = NULL;
    ttffont = TTF_OpenFont("./ProggyTiny.ttf", 16);
    SDL_Color text_color_dull = {70, 70, 70};
    SDL_Color text_color = {180, 180, 180};
    SDL_Color selected_text_color = {23, 86, 155}; //selected colour = Sega blue ;)

    /* Set up surfaces */
    SDL_Surface *gameSurface   = SDL_CreateRGBSurface(SDL_SWSURFACE, sdl_video.surf_screen->w, sdl_video.surf_screen->h, BPP, 0, 0, 0, 0);
    SDL_Surface *menuSurface   = SDL_CreateRGBSurface(SDL_SWSURFACE, sdl_video.surf_screen->w, sdl_video.surf_screen->h, BPP, 0, 0, 0, 0);
    SDL_Surface *bgSurface     = NULL;

    /* Preserve last frame of emulation */
    SDL_BlitSurface(sdl_video.surf_bitmap, &sdl_video.srect, gameSurface, &sdl_video.drect);

    /* Blank screen */
    SDL_FillRect(sdl_video.surf_screen, 0, 0);

    /* Show background hardware image */
    switch(system_hw)
    {
    case SYSTEM_PICO:
        bgSurface = IMG_Load( "./PICO.png" ); break;
    case SYSTEM_SG: //SG-1000 I&II
    case SYSTEM_SGII:
        bgSurface = IMG_Load( "./SG1000.png" ); break;
    case SYSTEM_MARKIII: //Mark III & Sega Master System I&II & Megadrive with power base converter
    case SYSTEM_SMS:
    case SYSTEM_GGMS:
    case SYSTEM_SMS2:
    case SYSTEM_PBC:
        bgSurface = IMG_Load( "./SMS.png" ); break;
    case SYSTEM_GG:
        bgSurface = IMG_Load( "./GG.png" ); break;
    case SYSTEM_MD:
        bgSurface = IMG_Load( "./MD.png" ); break;
    case SYSTEM_MCD:
        bgSurface = IMG_Load( "./MCD.png" ); break;
    default:
        bgSurface = IMG_Load( "./MD.png" ); break;
    }

    /* Start menu loop */
    while(goto_menu)
    {
        static int selectedoption = 0;
        char remap_text[256];
        char load_state_screenshot[256];
        int savestate = 0;
        time_t current_time;
        char* c_time_string;

        /* Obtain current time. */
        current_time = time(NULL);

        /* Convert to local time format. */
        c_time_string = ctime(&current_time);

        /* Initialise surfaces */
        SDL_Surface *textSurface;
        SDL_Surface *menuBackground;

        /* Blit the last emulation frame */
        SDL_BlitSurface(gameSurface, NULL, sdl_video.surf_screen, NULL);

        if (start_menu < 30)
        {
            /* Blit slice of emulation frame */
            srect.x = drect.x = 0;
            srect.w = drect.w = menuSurface->w;
            srect.y = 0;
            srect.h = start_menu * (menuSurface->h / 30);
            drect.y = menuSurface->h - srect.h;
            drect.h = srect.h;

            SDL_BlitSurface(gameSurface, &srect, menuSurface, &drect);
        } else
            SDL_BlitSurface(gameSurface, NULL, menuSurface, NULL);
#ifndef SDL2
        /* Fade background in slowly */
        if(menu_fade < 240) menu_fade += 5;
        SDL_SetAlpha(bgSurface, SDL_SRCALPHA, menu_fade);
#endif
        /* Blit the background image */
        srect.w = menuSurface->w;
        srect.h = menuSurface->h;
        srect.x = (bgSurface->w - srect.w) / 2;
        srect.y = (bgSurface->h - srect.h) / 2;
        SDL_BlitSurface(bgSurface, &srect, menuSurface, NULL);

        /* Fill menu box */
        menuBackground = SDL_CreateRGBSurface(SDL_SWSURFACE,
            (menuSurface->w <= 160 ? menuSurface->w : ((menuSurface->w + 160) / 2)),
            (menuSurface->h <= 144 ? menuSurface->h : 185), BPP, 0, 0, 0, 0);
        drect.x = (menuSurface->w - menuBackground->w) / 2;
        drect.y = (menuBackground->h >= 185 ? ((menuSurface->h - 185) / 2) : 0);
        drect.w = menuBackground->w;
        drect.h = menuBackground->h;
        SDL_FillRect(menuBackground, 0, 0);
#ifndef SDL2
        if(menu_fade < 205) SDL_SetAlpha(menuBackground, SDL_SRCALPHA, 255 - menu_fade);
        else SDL_SetAlpha(menuBackground, SDL_SRCALPHA, 50);
#endif
        SDL_BlitSurface(menuBackground, NULL, menuSurface, &drect);
        SDL_FreeSurface(menuBackground);

        /* Show title */
        if(menuSurface->h > 12 * 12)
        {
            drect.x = menuSurface->w / 2 - 6 * 8;
            drect.y = (menuSurface->h - 12 * 12) / 3;
            drect.w = 100;
            drect.h = 50;
            textSurface = TTF_RenderText_Solid(ttffont, "Genesis Plus GX", text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
            SDL_FreeSurface(textSurface);
        }

        /* Show time */
        drect.x = menuSurface->w - (19 * 6);
        drect.y = 0;
        drect.w = 100;
        drect.h = 50;
        textSurface = TTF_RenderText_Solid(ttffont, c_time_string, text_color_dull);
        SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
        SDL_FreeSurface(textSurface);

        /* Show version */
        if(menuSurface->w >= 43 * 6)
        {
            drect.x = drect.y = 0;
            drect.w = 100;
            drect.h = 50;
            textSurface = TTF_RenderText_Solid(ttffont, "Build date " __DATE__, text_color_dull);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
            SDL_FreeSurface(textSurface);
        }

        switch(menustate)
        {
        case MAINMENU:
        {
            int i;
            for(i = 0; i < sizeof(gcw0menu_mainlist)/sizeof(gcw0menu_mainlist[0]); i++)
            {
                drect.x = (menuSurface->w - 160) / 2;
                drect.y = (menuSurface->h - 10 * 12) / 2 + (12*i);
                drect.w = 100;
                drect.h = 50;
                if (i == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_mainlist[i], selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_mainlist[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                SDL_FreeSurface(textSurface);
            }
            break;
        }
        case GRAPHICS_OPTIONS:
        {
            int i;
            for(i = 0; i < sizeof(gcw0menu_gfxlist)/sizeof(gcw0menu_gfxlist[0]); i++)
            {
                drect.x = (menuSurface->w - 160) / 2;
                drect.y = (menuSurface->h - 10 * 12) / 2 + (12*i);
                drect.w = 100;
                drect.h = 50;
                if ((i + 10) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_gfxlist[i], selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_gfxlist[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                SDL_FreeSurface(textSurface);
            }
            /* Display On/Off */
            int tempX = drect.x;
            int tempY = (menuSurface->h - 10 * 12) / 2;
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 1), gcw0menu_renderer[configTemp.renderer]);         //Renderer
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 2), gcw0menu_onofflist[configTemp.gcw0_fullscreen]); //Scaling
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 3), gcw0menu_onofflist[configTemp.keepaspectratio]); //Aspect ratio
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 4), gcw0menu_onofflist[configTemp.gg_scanlines]);    //Scanlines
            break;
        }
        case SOUND_OPTIONS:
        {
            int i;
            for(i = 0; i < sizeof(gcw0menu_sndlist)/sizeof(gcw0menu_sndlist[0]); i++)
            {
                drect.x = (menuSurface->w - 160) / 2;
                drect.y = (menuSurface->h - 10 * 12) / 2 + (12*i);
                drect.w = 100;
                drect.h = 50;
                if ((i + 70) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_sndlist[i], selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_sndlist[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                SDL_FreeSurface(textSurface);
            }

            /* Display On/Off */
            int tempX = drect.x;
            int tempY = (menuSurface->h - 10 * 12) / 2;
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 1), gcw0menu_onofflist[configTemp.use_sound]);       //Sound
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 2), gcw0menu_onofflist[configTemp.ym2413]);          //FM Sound (SMS)
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 3), gcw0menu_onofflist[configTemp.skip_prevention]); //Stop Skipping
            break;
        }
        case REMAP_OPTIONS:
        {
            sprintf(remap_text, "%s", "GenPlus");
            drect.x = (menuSurface->w - 160) / 2;
            drect.y = (menuSurface->h - 10 * 12) / 2 + 24;
            drect.w = 100;
            drect.h = 50;
            textSurface = TTF_RenderText_Solid(ttffont, remap_text, text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
            SDL_FreeSurface(textSurface);
            sprintf(remap_text, "%s", "GCW-Zero");
            drect.x = (menuSurface->w + 160) / 2 - 6 * 8;
            textSurface = TTF_RenderText_Solid(ttffont, remap_text, text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
            SDL_FreeSurface(textSurface);

            int i = 0;
            sprintf(remap_text, gcw0menu_remapoptions[i]);
            drect.x = (menuSurface->w - 160) / 2;
            drect.y = (menuSurface->h - 10 * 12) / 2;
            drect.w = 100;
            drect.h = 50;
            if ((i + 20) == selectedoption)
                textSurface = TTF_RenderText_Solid(ttffont, remap_text, selected_text_color);
            else
                textSurface = TTF_RenderText_Solid(ttffont, remap_text, text_color);
            SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
            SDL_FreeSurface(textSurface);

            for(i = 1; i < sizeof(gcw0menu_remapoptions)/sizeof(gcw0menu_remapoptions[0]); i++)
            {
                sprintf(remap_text, gcw0menu_remapoptions[i]);
                drect.x = (menuSurface->w - 160) / 2;
                drect.y = (menuSurface->h - 10 * 12) / 2 + (12 * i) + 24;
                drect.w = 100;
                drect.h = 50;
                if ((i + 20) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, remap_text, selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, remap_text, text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                SDL_FreeSurface(textSurface);

                sprintf(remap_text, gcw0_get_key_name(configTemp.buttons[i - 1]));
                drect.x = (menuSurface->w + 160) / 2 - 6 * 8;
                if ((i + 20) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, remap_text, selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, remap_text, text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                SDL_FreeSurface(textSurface);
            }
            break;
        }
        case SAVE_STATE:
        {
            /* Show saved BMP as background if available */
            sprintf(load_state_screenshot,"%s/%s.%d.bmp", get_save_directory(), rom_filename, selectedoption - 30);
            SDL_Surface* screenshot;
            screenshot = SDL_LoadBMP(load_state_screenshot);
            if (screenshot)
            {
                drect.x = (menuSurface->w - screenshot->w) / 2;
                drect.y = (menuSurface->h - screenshot->h) / 2;
                drect.w = screenshot->w;
                drect.h = screenshot->h;
                SDL_BlitSurface(screenshot, NULL, menuSurface, &drect);

                /* Fill menu box */
                menuBackground = SDL_CreateRGBSurface(SDL_SWSURFACE,
                    (menuSurface->w <= 160 ? menuSurface->w : ((menuSurface->w + 160) / 2)),
                    (menuSurface->h <= 144 ? menuSurface->h : 185), BPP, 0, 0, 0, 0);
                drect.x = (menuSurface->w - menuBackground->w) / 2;
                drect.y = (menuBackground->h >= 185 ? ((menuSurface->h - 185) / 2) : 0);
                drect.w = menuBackground->w;
                drect.h = menuBackground->h;
                SDL_FillRect(menuBackground, 0, 0);
#ifndef SDL2
                SDL_SetAlpha(menuBackground, SDL_SRCALPHA, 180);
#endif
                SDL_BlitSurface(menuBackground, NULL, menuSurface, &drect);
                SDL_FreeSurface(menuBackground);
            }

            /* Show title */
            if(menuSurface->h > 12 * 12)
            {
                drect.x = menuSurface->w / 2 - 6 * 8;
                drect.y = (menuSurface->h - 12 * 12) / 3;
                drect.w = 100;
                drect.h = 50;
                textSurface = TTF_RenderText_Solid(ttffont, "Genesis Plus GX", text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                SDL_FreeSurface(textSurface);
            }

            int i;
            for(i = 0; i < sizeof(gcw0menu_savestate)/sizeof(gcw0menu_savestate[0]); i++)
            {
                drect.x = (menuSurface->w - 160) / 2;
                drect.y = (menuSurface->h - 10 * 12) / 2 + (12*i);
                drect.w = 100;
                drect.h = 50;
                if ((i + 30) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_savestate[i], selected_text_color);
	        else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_savestate[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                SDL_FreeSurface(textSurface);
            }
            if (screenshot) SDL_FreeSurface(screenshot);
            savestate = 1;
        }
        case LOAD_STATE:
        {
#ifndef DINGOO
            if (!savestate)
            {
                /* Show saved BMP as background if available */
                sprintf(load_state_screenshot,"%s/%s.%d.bmp", get_save_directory(), rom_filename, selectedoption - 40);
                SDL_Surface* screenshot;
                screenshot = SDL_LoadBMP(load_state_screenshot);
                if (screenshot)
                {
                    drect.x = (menuSurface->w - screenshot->w) / 2;
                    drect.y = (menuSurface->h - screenshot->h) / 2;
                    drect.w = screenshot->w;
                    drect.h = screenshot->h;
                    SDL_BlitSurface(screenshot, NULL, menuSurface, &drect);

                    /* Fill menu box */
                    menuBackground = SDL_CreateRGBSurface(SDL_SWSURFACE,
                        (menuSurface->w <= 160 ? menuSurface->w : ((menuSurface->w + 160) / 2)),
                        (menuSurface->h <= 144 ? menuSurface->h : 185), BPP, 0, 0, 0, 0);
                    drect.x = (menuSurface->w - menuBackground->w) / 2;
                    drect.y = (menuBackground->h >= 185 ? ((menuSurface->h - 185) / 2) : 0);
                    drect.w = menuBackground->w;
                    drect.h = menuBackground->h;
                    SDL_FillRect(menuBackground, 0, 0);
#ifndef SDL2
                    SDL_SetAlpha(menuBackground, SDL_SRCALPHA, 180);
#endif
                    SDL_BlitSurface(menuBackground, NULL, menuSurface, &drect);
                    SDL_FreeSurface(menuBackground);
                }

                /* Show title */
                if(menuSurface->h > 12 * 12)
                {
                    drect.x = menuSurface->w / 2 - 6 * 8;
                    drect.y = (menuSurface->h - 12 * 12) / 3;
                    drect.w = 100;
                    drect.h = 50;
                    textSurface = TTF_RenderText_Solid(ttffont, "Genesis Plus GX", text_color);
                    SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                    SDL_FreeSurface(textSurface);
                }

                int i;
                for(i = 0; i < sizeof(gcw0menu_loadstate)/sizeof(gcw0menu_loadstate[0]); i++)
                {
                    drect.x = (menuSurface->w - 160) / 2;
                    drect.y = (menuSurface->h - 10 * 12) / 2 + (12*i);
                    drect.w = 100;
                    drect.h = 50;
                    if ((i + 40) == selectedoption)
	                textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_loadstate[i], selected_text_color);
	            else
	                textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_loadstate[i], text_color);
                    SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                    SDL_FreeSurface(textSurface);
                }
                if (screenshot) SDL_FreeSurface(screenshot);
            }
            savestate = 0;
#endif //!DINGOO
            break;
        }
        case MISC_OPTIONS:
        {
            int i;
            for(i = 0; i < sizeof(gcw0menu_misc)/sizeof(gcw0menu_misc[0]); i++)
            {
                drect.x = (menuSurface->w - 160) / 2;
                drect.y = (menuSurface->h - 10 * 12) / 2 + (12 * i);
                drect.w = 100;
                drect.h = 50;
                if ((i + 50) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_misc[i], selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_misc[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                SDL_FreeSurface(textSurface);
            }
            /* Display On/Off */
            int tempX = drect.x;
            int tempY = (menuSurface->h - 10 * 12) / 2;
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 1), gcw0menu_optimisations[configTemp.optimisations]); //Optimisations
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 2), gcw0menu_onofflist[configTemp.sl_autoresume]);     //Save/load autoresume
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 3), gcw0menu_onofflist[configTemp.a_stick]);           //A-stick
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 4), gcw0menu_deadzonelist[configTemp.deadzone]);       //A-stick Sensitivity
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 5), lock_on_desc[configTemp.lock_on]);                 //Display Lock-on Types
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 6), gcw0menu_numericlist[configTemp.lightgun_speed]);  //Lightgun speed

            /* Lightgun Cursor */
            drect.x = menuSurface->w - tempX - 16;
            drect.y = tempY + (12 * 7);
            SDL_Surface *lightgunSurface;
            lightgunSurface = IMG_Load(cursor[configTemp.cursor]);
            static int lightgun_af_demo = 0;

            srect.x = srect.y = 0;
            srect.w = srect.h = 15;
            if (lightgun_af_demo >= 10 && configTemp.cursor != 0) srect.x = 15;
            lightgun_af_demo++;
            if (lightgun_af_demo == 20) lightgun_af_demo = 0;
            SDL_BlitSurface(lightgunSurface, &srect, menuSurface, &drect);
            SDL_FreeSurface(lightgunSurface);
            break;
        }
        case AUTOFIRE_OPTIONS:
        {
            int i;
            for(i = 0; i < sizeof(gcw0menu_autofirelist)/sizeof(gcw0menu_autofirelist[0]); i++)
            {
                drect.x = (menuSurface->w - 160) / 2;
                drect.y = (menuSurface->h - 10 * 12) / 2 + (12 * i);
                drect.w = 100;
                drect.h = 50;
                if ((i + 60) == selectedoption)
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_autofirelist[i], selected_text_color);
                else
                    textSurface = TTF_RenderText_Solid(ttffont, gcw0menu_autofirelist[i], text_color);
                SDL_BlitSurface(textSurface, NULL, menuSurface, &drect);
                SDL_FreeSurface(textSurface);
            }
            /* Display On/Off */
            int tempX = drect.x;
            int tempY = (menuSurface->h - 10 * 12) / 2;
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 1), gcw0menu_onofflist[afA]); //A
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 2), gcw0menu_onofflist[afB]); //B
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 3), gcw0menu_onofflist[afC]); //C
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 4), gcw0menu_onofflist[afX]); //X
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 5), gcw0menu_onofflist[afY]); //Y
            PRINTSETTING(menuSurface->w - tempX, tempY + (12 * 6), gcw0menu_onofflist[afZ]); //Z
            break;
        }

/* other menu's go here */

        default:
            break;
        }

        /* Update display */
        if (start_menu < 30)
        {
            /* Blit slice of emulation frame */
            srect.x = drect.x = 0;
            srect.w = drect.w = menuSurface->w;
            srect.y = menuSurface->h - start_menu * (menuSurface->h / 30);
            srect.h = menuSurface->h - srect.y;
            drect.y = 0;
            drect.h = srect.h;

            SDL_BlitSurface(menuSurface, &srect, sdl_video.surf_screen, &drect);
            start_menu++;
        } else
            SDL_BlitSurface(menuSurface, NULL, sdl_video.surf_screen, NULL);
#ifdef SDL2
        SDL_RenderPresent(renderer);
#else
        if(renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
        else             SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
#endif
        /* Check for user input */
#ifndef SDL2
        SDL_EnableKeyRepeat(0, 0);
#endif
        static int keyheld = 0;
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_KEYDOWN:
                sdl_control_update(event.key.keysym.sym);
                break;
            case SDL_KEYUP:
                keyheld = 0;
                break;
            default:
                break;
            }
        }
        if (event.type == SDL_KEYDOWN && !keyheld)
        {
            keyheld++;
            uint8 *keystate2;
#ifdef SDL2
            keystate2 = SDL_GetKeyboardState(NULL);
#else
            keystate2 = SDL_GetKeyState(NULL);
#endif
            if (menustate == REMAP_OPTIONS)
            {
                /* REMAP_OPTIONS needs to capture all input */
#ifdef SDL2
                SDL_Keycode pressed_key = 0;
#else
                SDLKey pressed_key = 0;
#endif
                if      (keystate2[SDLK_RETURN]   ) pressed_key = SDLK_RETURN;
                else if (keystate2[SDLK_LCTRL]    ) pressed_key = SDLK_LCTRL;
                else if (keystate2[SDLK_LALT]     ) pressed_key = SDLK_LALT;
                else if (keystate2[SDLK_LSHIFT]   ) pressed_key = SDLK_LSHIFT;
                else if (keystate2[SDLK_SPACE]    ) pressed_key = SDLK_SPACE;
                else if (keystate2[SDLK_TAB]      ) pressed_key = SDLK_TAB;
                else if (keystate2[SDLK_BACKSPACE]) pressed_key = SDLK_BACKSPACE;
                else if (keystate2[SDLK_ESCAPE]   ) pressed_key = SDLK_ESCAPE;

                if (pressed_key)
                {
                    if (selectedoption == 20)
                    {
                        if(pressed_key == SDLK_LCTRL || pressed_key == SDLK_LALT) //return to main menu
                        {
                            SDL_Delay(130);
                            menustate = MAINMENU;
                        }
                    }
                    else
                    {
                        switch(selectedoption)
                        {
                        case 21:;//button a remap
                            configTemp.buttons[A]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 22:;//button b remap
                            configTemp.buttons[B]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 23:;//button c remap
                            configTemp.buttons[C]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 24:;//button x remap
                            configTemp.buttons[X]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 25:;//button y remap
                            configTemp.buttons[Y]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 26:;//button z remap
                            configTemp.buttons[Z]     = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 27:;//button start remap
                            configTemp.buttons[START] = (pressed_key==SDLK_ESCAPE)? 0: pressed_key; break;
                        case 28:;//button mode remap
                            configTemp.buttons[MODE]  = pressed_key;                                break;
                        }
                        SDL_Delay(130);
//                        config_save();
                    }
                }
            } //remap menu

            if (keystate2[SDLK_DOWN])
            {
                /* Warp to top of menu? */
                selectedoption++;
                if      (selectedoption == GRAPHICS_OPTIONS) selectedoption = MAINMENU;
                else if (selectedoption == 15              ) selectedoption = GRAPHICS_OPTIONS;
                else if (selectedoption == 29              ) selectedoption = REMAP_OPTIONS;
                else if (selectedoption == LOAD_STATE      ) selectedoption = SAVE_STATE;
                else if (selectedoption == MISC_OPTIONS    ) selectedoption = LOAD_STATE;
                else if (selectedoption == 58              ) selectedoption = MISC_OPTIONS;
                else if (selectedoption == 67              ) selectedoption = AUTOFIRE_OPTIONS;
                else if (selectedoption == 74              ) selectedoption = SOUND_OPTIONS;
                if (start_menu >= 30) SDL_Delay(100);
    	    }
            else if (keystate2[SDLK_UP])
            {
                /* Warp to bottom of menu? */
                if      (selectedoption ==  MAINMENU       ) selectedoption = GRAPHICS_OPTIONS; //main menu
                else if (selectedoption ==  GRAPHICS_OPTIONS) selectedoption = 15; //graphics menu
                else if (selectedoption ==  REMAP_OPTIONS   ) selectedoption = 29; //remap menu
                else if (selectedoption ==  SAVE_STATE      ) selectedoption = LOAD_STATE; //save menu
                else if (selectedoption ==  LOAD_STATE      ) selectedoption = MISC_OPTIONS; //load menu
                else if (selectedoption ==  MISC_OPTIONS    ) selectedoption = 58; //misc menu
                else if (selectedoption ==  AUTOFIRE_OPTIONS) selectedoption = 67; //autofire menu
                else if (selectedoption ==  SOUND_OPTIONS   ) selectedoption = 74; //sound menu
                selectedoption--;
                if (start_menu >= 30) SDL_Delay(100);
            }
	    else if (keystate2[SDLK_LALT] && menustate != REMAP_OPTIONS)
            {
                /* Quit current menu */
                SDL_Delay(130);
                switch(menustate)
                {
                case AUTOFIRE_OPTIONS:
                    menustate = MAINMENU; selectedoption = 1; break;
                case SAVE_STATE:
                    menustate = MAINMENU; selectedoption = 2; break;
                case LOAD_STATE:
                    menustate = MAINMENU; selectedoption = 3; break;
                case GRAPHICS_OPTIONS:
                    menustate = MAINMENU; selectedoption = 4; break;
                case SOUND_OPTIONS:
                    menustate = MAINMENU; selectedoption = 5; break;
                case MAINMENU:
                    if (selectedoption == REMAP_OPTIONS) selectedoption = 6;
                    else                      goto_menu =  selectedoption = 0;
	            break;
                case MISC_OPTIONS:
                    menustate = MAINMENU; selectedoption = 7; break;
                }
            }
            else if (keystate2[SDLK_LCTRL] && menustate != REMAP_OPTIONS)
            {
                SDL_Delay(130);
                switch(selectedoption)
                {
                case 0: //Resume
	            goto_menu=0; selectedoption=0; break;
                case 1: //Autofire menu
                    menustate = selectedoption = AUTOFIRE_OPTIONS; break;
                case 2: //Save
                    menustate = selectedoption = SAVE_STATE; break;
                case 3: //Load
                    menustate = selectedoption = LOAD_STATE; break;
                case 4: //Graphics
                    menustate = selectedoption = GRAPHICS_OPTIONS; break;
                case 5: //Sound
                    menustate = selectedoption = SOUND_OPTIONS; break;
                case 6: //Remap
                    menustate = selectedoption = REMAP_OPTIONS; break;
                case 7: //Misc.
                    menustate = selectedoption = MISC_OPTIONS; break;
                case 8: //Reset
                    goto_menu = selectedoption = MAINMENU; system_reset(); break;
                case 9: //Quit
                    config_save(); exit(0); break;
                case 10: //Back to main menu
                    menustate = MAINMENU; selectedoption = 4; break;
                case 11: //Renderer
                    if (configTemp.renderer >= sizeof(gcw0menu_renderer) / sizeof(gcw0menu_renderer[0]) - 1) configTemp.renderer = 0;
                    else configTemp.renderer ++;
                    break;
                case 12: //Scaling
                    configTemp.gcw0_fullscreen = !configTemp.gcw0_fullscreen; resizeScreen = 1; break;
                case 13: //Keep aspect ratio
                    configTemp.keepaspectratio = !configTemp.keepaspectratio; do_once = resizeScreen = 1; break;
                case 14: //Scanlines (GG)
                    configTemp.gg_scanlines = !configTemp.gg_scanlines; break;
                case 20: //Back to main menu
                    selectedoption = 6; break;
                case 30: //Back to main menu
                    menustate = MAINMENU; selectedoption = 2; break;
                case 31:
                case 32:
                case 33:
                case 34:
                case 35:
                case 36:
                case 37:
                case 38:
                case 39:;//Save state 1-9
                    SDL_Delay(120);
                    gcw0_savestate(selectedoption - 30);

                    /* Save BMP screenshot */
                    char save_state_screenshot[256];
                    sprintf(save_state_screenshot,"%s/%s.%d.bmp", get_save_directory(), rom_filename, selectedoption-30);
                    SDL_SaveBMP(gameSurface, save_state_screenshot);
                    if (configTemp.sl_autoresume) menustate = goto_menu = selectedoption = MAINMENU;
                    break;
                case 40: //Back to main menu
                    menustate = MAINMENU; selectedoption = 3; break;
                case 41:
                case 42:
                case 43:
                case 44:
                case 45:
                case 46:
                case 47:
                case 48:
                case 49:;//Load state 1-9
                    SDL_Delay(120);
                    gcw0_loadstate(selectedoption - 40);
                    if (configTemp.sl_autoresume) menustate = goto_menu = selectedoption = MAINMENU;
                    break;
                case 50: //return to main menu
                    menustate = MAINMENU; selectedoption = 7; break;
                case 51: //Optimisations
                    if(configTemp.optimisations >= sizeof(gcw0menu_optimisations) / sizeof(gcw0menu_optimisations[0]) - 1) configTemp.optimisations = 0;
                    else configTemp.optimisations ++;
                    break;
                case 52: //toggle auto resume when save/loading
                    configTemp.sl_autoresume = !configTemp.sl_autoresume; break;
                case 53: //toggle A-Stick
                    configTemp.a_stick = !configTemp.a_stick; break;
                case 54: //toggle A-Stick deadzone
                    if(configTemp.deadzone >= sizeof(gcw0menu_deadzonelist) / sizeof(gcw0menu_deadzonelist[0]) - 1) configTemp.deadzone = 0;
                    else configTemp.deadzone++;
                    break;
                case 55: //toggle or change lock-on device
                    if(configTemp.lock_on >= sizeof(lock_on_desc) / sizeof(lock_on_desc[0]) - 1) configTemp.lock_on = 0;
                    else configTemp.lock_on++;
                    break;
                case 56: //Lightgun speed
                    if(configTemp.lightgun_speed >= sizeof(gcw0menu_numericlist) / sizeof(gcw0menu_numericlist[0]) - 1) configTemp.lightgun_speed = 1;
                    else configTemp.lightgun_speed++;
                    break;
                case 57: //Change lightgun cursor
                    if(configTemp.cursor >= sizeof(cursor) / sizeof(cursor[0]) - 1) configTemp.cursor = 0;
                    else configTemp.cursor++;
                    break;
                case 60: //return to main menu
                    menustate = MAINMENU; selectedoption = 1; break;
                case 61: //Toggle autofire for button A
                    afA = !afA; break;
                case 62: //Toggle autofire for button B
                    afB = !afB; break;
                case 63: //Toggle autofire for button C
                    afC = !afC; break;
                case 64: //Toggle autofire for button X
                    afX = !afX; break;
                case 65: //Toggle autofire for button Y
                    afY = !afY; break;
                case 66: //Toggle autofire for button Z
                    afZ = !afZ; break;
                case 70: //return to main menu
                    menustate = MAINMENU; selectedoption = 5; break;
                case 71: //Toggle sound on/off
                    configTemp.use_sound = !configTemp.use_sound; break;
                case 72: //Toggle high quality FM for SMS
                    configTemp.ym2413 = !configTemp.ym2413; break;
                case 73: //Stop skipping
                    configTemp.skip_prevention = !configTemp.skip_prevention; break;
                default: //this should never happen
	            break;
                }
            }
        }
    }//menu loop

    /* Save configuration */
    //config_save();

    /* Update display */
    drect.x = drect.y = srect.x = 0;
    drect.w = srect.w = menuSurface->w;
    while(start_menu)
    {
        SDL_BlitSurface(gameSurface, NULL, sdl_video.surf_screen, NULL);
        srect.h = start_menu-- * (menuSurface->h / 30);
        srect.y = menuSurface->h - srect.h;
        drect.h = srect.h;
#ifndef SDL2
        /* Fade background out slowly */
        if(menu_fade) menu_fade -= 5;
        SDL_SetAlpha(menuSurface, SDL_SRCALPHA, menu_fade);
#endif
        SDL_BlitSurface(menuSurface, &srect, sdl_video.surf_screen, &drect);
#ifdef SDL2
        SDL_RenderPresent(renderer);
#else
        if(renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
        else             SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
#endif
    }
    clearSoundBuf = 2;
    menu_fade = 0;
    TTF_CloseFont (ttffont);
    SDL_FreeSurface(menuSurface);
    SDL_FreeSurface(bgSurface);
    SDL_FreeSurface(gameSurface);

    if(resizeScreen)
    {
        resizeScreen--;
        bitmap.viewport.changed = 1; //change screen res if required
        if (config.gcw0_fullscreen)
        {
            if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
            {
                gcw0_w = sdl_video.drect.w;
                gcw0_h = sdl_video.drect.h;
#ifdef SDL2
#else
#ifdef DINGOO
                sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_SWSURFACE                );
#else
                if     (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_HWSURFACE | SDL_TRIPLEBUF);
                else if(config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);
                else if(config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_SWSURFACE                );
#endif //DINGOO
#endif
            }
            else
            {
                sdl_video.drect.w = sdl_video.srect.w;
                sdl_video.drect.h = sdl_video.srect.h;
                sdl_video.drect.y = 0;
                sdl_video.drect.x = sdl_video.drect.y = 0;

                gcw0_w = sdl_video.drect.w;
                gcw0_h = sdl_video.drect.h;
#ifdef SDL2
#else
#ifdef DINGOO
                sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_SWSURFACE                );
#else
                if     (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, BPP, SDL_HWSURFACE | SDL_TRIPLEBUF);
                else if(config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);
                else if(config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(gcw0_w,gcw0_h, BPP, SDL_SWSURFACE                );
#endif //DINGOO
#endif
            }
        } else
        {
#ifdef SDL2
#else
#ifdef DINGOO
                sdl_video.surf_screen  = SDL_SetVideoMode(256,gcw0_h, BPP, SDL_SWSURFACE                );
#else
                if     (config.renderer == 0) sdl_video.surf_screen  = SDL_SetVideoMode(320,240, BPP, SDL_HWSURFACE | SDL_TRIPLEBUF);
                else if(config.renderer == 1) sdl_video.surf_screen  = SDL_SetVideoMode(320,240, BPP, SDL_HWSURFACE | SDL_DOUBLEBUF);
                else if(config.renderer == 2) sdl_video.surf_screen  = SDL_SetVideoMode(320,240, BPP, SDL_SWSURFACE                );
#endif //DINGOO
#endif
        }

    /* Clear screen */

#ifdef SDL2
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        int i;
        for(i = 0; i < 3; i++)
        {
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
        }
#else
        int i;
        for(i = 0; i < 3; i++)
        {
            SDL_FillRect(sdl_video.surf_screen, 0, 0);
            if(config.renderer < 2) SDL_Flip      (sdl_video.surf_screen            );
            else                    SDL_UpdateRect(sdl_video.surf_screen, 0, 0, 0, 0);
        }
#endif
    }

    /* Reset semaphore to avoid temporary speedups on menu exit */
    if (post)
    {
        do
        {
            SDL_SemWait(sdl_sync.sem_sync);
        }
        while(--post);
    }

    SDL_PauseAudio(!config.use_sound);

}


void quicksaveState()
{
    /* Save to quicksave slot */
    char save_state_file[256];
    sprintf(save_state_file,"%s/%s.gp1", get_save_directory(), rom_filename);
        FILE *f = fopen(save_state_file,"wb");
        if (f)
        {
            uint8 buf[STATE_SIZE];
            int len = state_save(buf);
            fwrite(&buf, len, 1, f);
            fclose(f);
        }

    /* Save BMP screenshot */
    char save_state_screenshot[256];
    sprintf(save_state_screenshot,"%s/%s.1.bmp", get_save_directory(), rom_filename);
    SDL_Surface* screenshot;
    if (!config.gcw0_fullscreen)
    {
        screenshot = SDL_CreateRGBSurface(SDL_SWSURFACE, sdl_video.srect.w, sdl_video.srect.h, BPP, 0, 0, 0, 0);
        SDL_Rect temp;
        temp.x = temp.y = 0;
        temp.w = sdl_video.srect.w;
        temp.h = sdl_video.srect.h;
        SDL_BlitSurface(sdl_video.surf_bitmap, &temp, screenshot, &temp);
        SDL_SaveBMP(screenshot, save_state_screenshot);
        SDL_FreeSurface(screenshot);
    }
    else
    {
        screenshot = SDL_CreateRGBSurface(SDL_SWSURFACE, gcw0_w, gcw0_h, BPP, 0, 0, 0, 0);
        SDL_Rect temp;
        temp.x = temp.y = 0;
        temp.w = gcw0_w;
        temp.h = gcw0_h;
        SDL_BlitSurface(sdl_video.surf_bitmap, &temp, screenshot, &temp);
        SDL_SaveBMP(screenshot, save_state_screenshot);
        SDL_FreeSurface(screenshot);
    }
}

int sdl_input_update(void)
{
#ifdef SDL2
//TODO begun but unfinished
#else

#ifdef SDL2
    uint8 *keystate = SDL_GetKeyboardState(NULL);
#else
    uint8 *keystate = SDL_GetKeyState(NULL);
#endif
    /* reset input */
    input.pad[joynum] = 0;
    if (show_lightgun)
        input.pad[4] = 0; //player2:
    switch (input.dev[4])
    {
    case DEVICE_LIGHTGUN:
        show_lightgun = 1;

        /* Get mouse coordinates (absolute values) */
        int x,y;
        SDL_GetMouseState(&x,&y);

        if (config.gcw0_fullscreen)
        {
            input.analog[4][0] =  x;
            input.analog[4][1] =  y;
        } else
        {
            input.analog[4][0] =  x - (VIDEO_WIDTH  - bitmap.viewport.w) / 2;
            input.analog[4][1] =  y - (VIDEO_HEIGHT - bitmap.viewport.h) / 2;
        }
        if (config.smsmaskleftbar) x += 8;

        /* TRIGGER, B, C (Menacer only), START (Menacer & Justifier only) */
        if (keystate[SDLK_ESCAPE])  input.pad[4] |= INPUT_START;
    default:
        break;
    }
    switch (input.dev[joynum])
    {
    case DEVICE_LIGHTGUN:
    {
#ifdef GCWZERO
        show_lightgun = 2;

        /* Get mouse coordinates (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);

        if (config.gcw0_fullscreen)
        {
            input.analog[0][0] =  x;
            input.analog[0][1] =  y;
        } else
        {
            input.analog[0][0] =  x - (VIDEO_WIDTH  - bitmap.viewport.w) / 2;
            input.analog[0][1] =  y - (VIDEO_HEIGHT - bitmap.viewport.h) / 2;
        }
        if (config.smsmaskleftbar) x += 8;
        /* TRIGGER, B, C (Menacer only), START (Menacer & Justifier only) */
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_A;
        if (state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_B;
        if (state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_C;
        if (keystate[SDLK_ESCAPE])    input.pad[0]      |= INPUT_START;
#else
        /* Get mouse coordinates (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);

        /* X axis */
        input.analog[joynum][0] =  x - (VIDEO_WIDTH  - bitmap.viewport.w) / 2;

        /* Y axis */
        input.analog[joynum][1] =  y - (VIDEO_HEIGHT - bitmap.viewport.h) / 2;

        /* TRIGGER, B, C (Menacer only), START (Menacer & Justifier only) */
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_A;
        if (state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_B;
        if (state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_C;
        if (keystate[SDLK_f])         input.pad[joynum] |= INPUT_START;
        break;
#endif
    }
#ifndef GCWZERO
    case DEVICE_PADDLE:
    {
        /* Get mouse (absolute values) */
        int x;
        int state = SDL_GetMouseState(&x, NULL);

        /* Range is [0;256], 128 being middle position */
        input.analog[joynum][0] = x * 256 / VIDEO_WIDTH;

        /* Button I ->0 0 0 0 0 0 I*/
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;

        break;
    }

    case DEVICE_SPORTSPAD:
    {
        /* Get mouse (relative values) */
        int x,y;
        int state = SDL_GetRelativeMouseState(&x,&y);

        /* Range is [0;256] */
        input.analog[joynum][0] = (unsigned char)(-x & 0xFF);
        input.analog[joynum][1] = (unsigned char)(-y & 0xFF);

        /* Buttons I & II -> 0 0 0 0 0 0 II I*/
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
        if (state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_C;

        break;
    }

    case DEVICE_MOUSE:
    {
        SDL_ShowCursor(1);
        /* Get mouse (relative values) */
        int x,y;
        int state = SDL_GetRelativeMouseState(&x,&y);

        /* Sega Mouse range is [-256;+256] */
        input.analog[joynum][0] = x * 2;
        input.analog[joynum][1] = y * 2;

        /* Vertical movement is upsidedown */
        if (!config.invert_mouse)
            input.analog[joynum][1] = 0 - input.analog[joynum][1];

        /* Start,Left,Right,Middle buttons -> 0 0 0 0 START MIDDLE RIGHT LEFT */
        if (state & SDL_BUTTON_LMASK) input.pad[joynum] |= INPUT_B;
        if (state & SDL_BUTTON_RMASK) input.pad[joynum] |= INPUT_C;
        if (state & SDL_BUTTON_MMASK) input.pad[joynum] |= INPUT_A;
        if (keystate[SDLK_f])         input.pad[joynum] |= INPUT_START;

        break;
    }

    case DEVICE_XE_1AP:
    {
        /* A,B,C,D,Select,START,E1,E2 buttons -> E1(?) E2(?) START SELECT(?) A B C D */
        if (keystate[SDLK_a])  input.pad[joynum] |= INPUT_START;
        if (keystate[SDLK_s])  input.pad[joynum] |= INPUT_A;
        if (keystate[SDLK_d])  input.pad[joynum] |= INPUT_C;
        if (keystate[SDLK_f])  input.pad[joynum] |= INPUT_Y;
        if (keystate[SDLK_z])  input.pad[joynum] |= INPUT_B;
        if (keystate[SDLK_x])  input.pad[joynum] |= INPUT_X;
        if (keystate[SDLK_c])  input.pad[joynum] |= INPUT_MODE;
        if (keystate[SDLK_v])  input.pad[joynum] |= INPUT_Z;

        /* Left Analog Stick (bidirectional) */
        if      (keystate[SDLK_UP])     input.analog[joynum][1] -=   2;
        else if (keystate[SDLK_DOWN])   input.analog[joynum][1] +=   2;
        else                            input.analog[joynum][1]  = 128;
        if      (keystate[SDLK_LEFT])   input.analog[joynum][0] -=   2;
        else if (keystate[SDLK_RIGHT])  input.analog[joynum][0] +=   2;
        else                            input.analog[joynum][0]  = 128;

        /* Right Analog Stick (unidirectional) */
        if      (keystate[SDLK_KP8])    input.analog[joynum + 1][0] -=   2;
        else if (keystate[SDLK_KP2])    input.analog[joynum + 1][0] +=   2;
        else if (keystate[SDLK_KP4])    input.analog[joynum + 1][0] -=   2;
        else if (keystate[SDLK_KP6])    input.analog[joynum + 1][0] +=   2;
        else                            input.analog[joynum + 1][0]  = 128;

        /* Limiters */
        if      (input.analog[joynum][0]     > 0xFF) input.analog[joynum][0]     = 0xFF;
        else if (input.analog[joynum][0]     < 0   ) input.analog[joynum][0]     = 0;
        if      (input.analog[joynum][1]     > 0xFF) input.analog[joynum][1]     = 0xFF;
        else if (input.analog[joynum][1]     < 0   ) input.analog[joynum][1]     = 0;
        if      (input.analog[joynum + 1][0] > 0xFF) input.analog[joynum + 1][0] = 0xFF;
        else if (input.analog[joynum + 1][0] < 0   ) input.analog[joynum + 1][0] = 0;
        if      (input.analog[joynum + 1][1] > 0xFF) input.analog[joynum + 1][1] = 0xFF;
        else if (input.analog[joynum + 1][1] < 0   ) input.analog[joynum + 1][1] = 0;
        break;
    }

    case DEVICE_PICO:
    {
        /* Get mouse (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);

        /* Calculate X Y axis values */
        input.analog[0][0] = 0x3c  + (x * (0x17c-0x03c+1)) / VIDEO_WIDTH;
        input.analog[0][1] = 0x1fc + (y * (0x2f7-0x1fc+1)) / VIDEO_HEIGHT;

        /* Map mouse buttons to player #1 inputs */
        if (state & SDL_BUTTON_MMASK) pico_current  = (pico_current + 1) & 7;
        if (state & SDL_BUTTON_RMASK) input.pad[0] |= INPUT_PICO_RED;
        if (state & SDL_BUTTON_LMASK) input.pad[0] |= INPUT_PICO_PEN;

        break;
    }

    case DEVICE_TEREBI:
    {
        /* Get mouse (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);

        /* Calculate X Y axis values */
        input.analog[0][0] = (x * 250) / VIDEO_WIDTH;
        input.analog[0][1] = (y * 250) / VIDEO_HEIGHT;

        /* Map mouse buttons to player #1 inputs */
        if (state & SDL_BUTTON_RMASK) input.pad[0] |= INPUT_B;

        break;
    }

    case DEVICE_GRAPHIC_BOARD:
    {
        /* Get mouse (absolute values) */
        int x,y;
        int state = SDL_GetMouseState(&x,&y);

        /* Calculate X Y axis values */
        input.analog[0][0] = (x * 255) / VIDEO_WIDTH;
        input.analog[0][1] = (y * 255) / VIDEO_HEIGHT;

        /* Map mouse buttons to player #1 inputs */
        if (state & SDL_BUTTON_LMASK) input.pad[0] |= INPUT_GRAPHIC_PEN;
        if (state & SDL_BUTTON_RMASK) input.pad[0] |= INPUT_GRAPHIC_MENU;
        if (state & SDL_BUTTON_MMASK) input.pad[0] |= INPUT_GRAPHIC_DO;

        break;
    }

    case DEVICE_ACTIVATOR:
    {
        if (keystate[SDLK_g])  input.pad[joynum] |= INPUT_ACTIVATOR_7L;
        if (keystate[SDLK_h])  input.pad[joynum] |= INPUT_ACTIVATOR_7U;
        if (keystate[SDLK_j])  input.pad[joynum] |= INPUT_ACTIVATOR_8L;
        if (keystate[SDLK_k])  input.pad[joynum] |= INPUT_ACTIVATOR_8U;
    }
#endif
    default:
    {
        /* Autofire */
        static int autofire = 0;
        if (afA && autofire && keystate[config.buttons[A]]) input.pad[joynum] |= INPUT_A;
        else if (!afA       && keystate[config.buttons[A]]) input.pad[joynum] |= INPUT_A;
        if (afB && autofire && keystate[config.buttons[B]]) input.pad[joynum] |= INPUT_B;
        else if (!afB       && keystate[config.buttons[B]]) input.pad[joynum] |= INPUT_B;
        if (afC && autofire && keystate[config.buttons[C]]) input.pad[joynum] |= INPUT_C;
        else if (!afC       && keystate[config.buttons[C]]) input.pad[joynum] |= INPUT_C;
        autofire = !autofire;
        if (keystate[config.buttons[START]]) input.pad[joynum] |= INPUT_START;
        if (show_lightgun)
        {
            if (keystate[config.buttons[X]]) input.pad[4]      |= INPUT_A; //player 2
            if (keystate[config.buttons[Y]]) input.pad[4]      |= INPUT_B; //player 2
            if (keystate[config.buttons[Z]]) input.pad[4]      |= INPUT_C; //player 2
        } else
        {
            if (afX && autofire && keystate[config.buttons[X]]) input.pad[joynum] |= INPUT_X;
            else if (!afX       && keystate[config.buttons[X]]) input.pad[joynum] |= INPUT_X;
            if (afY && autofire && keystate[config.buttons[Y]]) input.pad[joynum] |= INPUT_Y;
            else if (!afY       && keystate[config.buttons[Y]]) input.pad[joynum] |= INPUT_Y;
            if (afZ && autofire && keystate[config.buttons[Z]]) input.pad[joynum] |= INPUT_Z;
            else if (!afZ       && keystate[config.buttons[Z]]) input.pad[joynum] |= INPUT_Z;
        }
        if (keystate[config.buttons[MODE]])  input.pad[joynum] |= INPUT_MODE;
        if (keystate[SDLK_ESCAPE] && keystate[SDLK_RETURN])
        {
            /* Activate menu flag */
            goto_menu = 1;
        }
        if (keystate[SDLK_ESCAPE] && keystate[SDLK_TAB])
        {
            clearSoundBuf = 2;
            SDL_PauseAudio(1);
            quicksaveState();
            SDL_PauseAudio(!config.use_sound);
        }
        if (keystate[SDLK_ESCAPE] && keystate[SDLK_BACKSPACE])
        {
            clearSoundBuf = 2;
            SDL_PauseAudio(1);
            SDL_Delay(100);
            gcw0_loadstate(1);
            SDL_PauseAudio(!config.use_sound);
        }

        /* A-stick support */
        static int MoveLeft = 0, MoveRight = 0, MoveUp    = 0, MoveDown  = 0,
                   lg_left  = 0, lg_right  = 0, lg_up     = 0, lg_down   = 0;
        Sint32     x_move   = 0, y_move    = 0;

        SDL_Joystick* joy;
        if (SDL_NumJoysticks() > 0)
        {
            joy    = SDL_JoystickOpen(0);
            x_move = SDL_JoystickGetAxis(joy, 0);
            y_move = SDL_JoystickGetAxis(joy, 1);
        }

        /* Define deadzone of analogue joystick */
        int deadzone = config.deadzone * 5000;

        /* Control lightgun with A-stick if activated */
        if (show_lightgun)
        {
            lg_left = lg_right = lg_up = lg_down = 0;

            if (x_move < -deadzone || x_move > deadzone)
            {
                if (x_move < -deadzone) lg_left  = 1;
                if (x_move < -20000)    lg_left  = 3;
                if (x_move >  deadzone) lg_right = 1;
                if (x_move >  20000)    lg_right = 3;
                current_time = time(NULL); //cursor disappears after 3 seconds...
            }
            if (y_move < -deadzone || y_move > deadzone)
            {
                if (y_move < -deadzone) lg_up   = 1;
                if (y_move < -20000)    lg_up   = 3;
                if (y_move >  deadzone) lg_down = 1;
                if (y_move >  20000)    lg_down = 3;
                current_time = time(NULL);
            }

            /* Keep mouse within screen, wrap around! */
            int x,y;
            SDL_GetMouseState(&x,&y);
            if (!config.gcw0_fullscreen)
            {
                if ((x - lg_left ) < sdl_video.drect.x )               x = VIDEO_WIDTH  - sdl_video.drect.x;
                if ((y - lg_up   ) < sdl_video.drect.y )               y = VIDEO_HEIGHT - sdl_video.drect.y;
                if ((x + lg_right) > VIDEO_WIDTH  - sdl_video.drect.x) x = sdl_video.drect.x;
                if ((y + lg_down ) > VIDEO_HEIGHT - sdl_video.drect.y) y = sdl_video.drect.y;
            } else //scaling on
            {
                if ((x - lg_left) < 0)       x = gcw0_w;
                if ((y - lg_up  ) < 0)       y = gcw0_h;
                if ((x + lg_right) > gcw0_w) x = 0;
                if ((y + lg_down ) > gcw0_h) y = 0;
            }
#ifdef SDL2
//TODO
//          SDL_WarpMouseInWindow( window, ( x+ ( ( lg_right - lg_left ) * config.lightgun_speed ) ) ,
//                               ( y+ ( ( lg_down  - lg_up   ) * config.lightgun_speed ) ) );
#else
            /* Move cursor to correct location */
            SDL_WarpMouse((x + ((lg_right - lg_left) * config.lightgun_speed)),
                          (y + ((lg_down  - lg_up  ) * config.lightgun_speed)));
#endif

        } else
        /* Mirror the D-pad controls */
        if (config.a_stick)
        {
            int deadzone = config.deadzone * 5000;
            MoveLeft = MoveRight = MoveUp = MoveDown = 0;

            if (x_move < -deadzone) MoveLeft  = 1;
            if (x_move >  deadzone) MoveRight = 1;
            if (y_move < -deadzone) MoveUp    = 1;
            if (y_move >  deadzone) MoveDown  = 1;
        }
        if (show_lightgun)
        {
            if (show_lightgun == 1)
            {
                /* Genesis/MD D-pad controls player 2 */
                if (MoveUp              )  input.pad[4]      |= INPUT_UP;
                if (MoveDown            )  input.pad[4]      |= INPUT_DOWN;
                if (MoveLeft            )  input.pad[4]      |= INPUT_LEFT;
                if (MoveRight           )  input.pad[4]      |= INPUT_RIGHT;
                if (keystate[SDLK_UP]   )  input.pad[joynum] |= INPUT_UP;
                if (keystate[SDLK_DOWN] )  input.pad[joynum] |= INPUT_DOWN;
                if (keystate[SDLK_LEFT] )  input.pad[joynum] |= INPUT_LEFT;
                if (keystate[SDLK_RIGHT])  input.pad[joynum] |= INPUT_RIGHT;
            } else
            if (show_lightgun == 2)
            {
                /* SMS D-pad controls player 2 */
                if (MoveUp              )  input.pad[joynum] |= INPUT_UP;
                if (MoveDown            )  input.pad[joynum] |= INPUT_DOWN;
                if (MoveLeft            )  input.pad[joynum] |= INPUT_LEFT;
                if (MoveRight           )  input.pad[joynum] |= INPUT_RIGHT;
                if (keystate[SDLK_UP]   )  input.pad[4]      |= INPUT_UP;
                if (keystate[SDLK_DOWN] )  input.pad[4]      |= INPUT_DOWN;
                if (keystate[SDLK_LEFT] )  input.pad[4]      |= INPUT_LEFT;
                if (keystate[SDLK_RIGHT])  input.pad[4]      |= INPUT_RIGHT;
            }
        } else
        {
#ifdef SDL2
//TODO
#else
            if      (keystate[SDLK_UP]    || MoveUp   )  input.pad[joynum] |= INPUT_UP;
            else if (keystate[SDLK_DOWN]  || MoveDown )  input.pad[joynum] |= INPUT_DOWN;
            if      (keystate[SDLK_LEFT]  || MoveLeft )  input.pad[joynum] |= INPUT_LEFT;
            else if (keystate[SDLK_RIGHT] || MoveRight)  input.pad[joynum] |= INPUT_RIGHT;
#endif
        }
        }
    }
#endif
    return 1;
}

int main (int argc, char **argv)
{
    FILE *fp;
    int running = 1;
    atexit(shutdown);
    /* Print help if no game specified */
    if (argc < 2)
    {
        char caption[256];
        sprintf(caption, "Genesis Plus GX\\SDL\nusage: %s gamename\n", argv[0]);
        MessageBox(NULL, caption, "Information", 0);
        exit(1);
    }

    error_init();
    create_default_directories();

    /* Set default config */
    set_config_defaults();

    /* Using rom file name instead of crc code to save files */
    sprintf(rom_filename, "%s",  get_file_name(argv[1]));

    /* Mark all BIOS as unloaded */
    system_bios = 0;

    /* Genesis BOOT ROM support (2KB max) */
    memset(boot_rom, 0xFF, 0x800);
    fp = fopen(MD_BIOS, "rb");
    if (fp != NULL)
    {
        int i;

        /* Read BOOT ROM */
        fread(boot_rom, 1, 0x800, fp);
        fclose(fp);

        /* Check BOOT ROM */
        if (!memcmp((char *)(boot_rom + 0x120),"GENESIS OS", 10))
        {
            /* Mark Genesis BIOS as loaded */
            system_bios = SYSTEM_MD;
        }

        /* Byteswap ROM */
        for (i=0; i<0x800; i+=2)
        {
            uint8 temp = boot_rom[i];
            boot_rom[i] = boot_rom[i+1];
            boot_rom[i+1] = temp;
        }
    }

    /* Load game file */
    if (!load_rom(argv[1]))
    {
        char caption[256];
        sprintf(caption, "Error loading file `%s'.", argv[1]);
        MessageBox(NULL, caption, "Error", 0);
        exit(1);
    }

    /* Per-game configuration (speed hacks) */
    if (strstr(rominfo.international,"Virtua Racing"))
    {
        virtua_racing         = 1; //further speed hacks required
        frameskip             = 4;
        config.hq_fm          = 0;
        config.psgBoostNoise  = 0;
        config.filter         = 0;
        config.dac_bits       = 7;
    }
    else if (system_hw == SYSTEM_MCD)
    {
        frameskip             = 4;
        config.hq_fm          = 0;
        config.psgBoostNoise  = 0;
        config.filter         = 0;
        config.dac_bits       = 7;
    }
    else
    {
        config.hq_fm          = 0;
        config.psgBoostNoise  = 0;
        config.filter         = 0;
        config.dac_bits       = 7;
    }

	/* Force 50 Fps refresh rate for PAL only games */
	if (vdp_pal)
	{
		setenv("SDL_VIDEO_REFRESHRATE", "50", 0);
	}
	else
	{
		setenv("SDL_VIDEO_REFRESHRATE", "60", 0);
	}

    /* Initialize SDL */
    if (SDL_Init(0) < 0)
    {
        char caption[256];
        sprintf(caption, "SDL initialization failed");
        MessageBox(NULL, caption, "Error", 0);
        exit(1);
    }
    sdl_joystick_init();
    sdl_video_init();
    sdl_sound_init();
    sdl_sync_init();

    /* Initialize Genesis virtual system */
    if(SDL_MUSTLOCK(sdl_video.surf_bitmap))
        SDL_LockSurface(sdl_video.surf_bitmap);
    memset(&bitmap, 0, sizeof(t_bitmap));
    bitmap.width        = 320;
    bitmap.height       = 240;
#if defined(USE_8BPP_RENDERING)
    bitmap.pitch        = (bitmap.width * 1);
#elif defined(USE_15BPP_RENDERING)
    bitmap.pitch        = (bitmap.width * 2);
#elif defined(USE_16BPP_RENDERING)
    bitmap.pitch        = (bitmap.width * 2);
#elif defined(USE_32BPP_RENDERING)
    bitmap.pitch        = (bitmap.width * 4);
#endif
    bitmap.data         = sdl_video.surf_bitmap->pixels;

/*
int i = 0;
while(i  < 239)
{
    bitmapline[i]         = (uint16 *)&bitmap.data[i * bitmap.pitch];
    i++;
}
*/
    if(SDL_MUSTLOCK(sdl_video.surf_bitmap))
        SDL_UnlockSurface(sdl_video.surf_bitmap);
    bitmap.viewport.changed = 3;

    /* Initialize system hardware */
    if (strstr(rominfo.international,"Virtua Racing"))
        audio_init(SOUND_FREQUENCY_VR, (vdp_pal ? 50 : 60));
    else if (system_hw == SYSTEM_MCD)
        audio_init(SOUND_FREQUENCY,     (vdp_pal ? 50 : 60));
    else
        audio_init(SOUND_FREQUENCY, 0);
    system_init();

    /* Mega CD specific */
    char brm_file[256];
    if (system_hw == SYSTEM_MCD)
    {
        /* Load internal backup RAM */
        sprintf(brm_file,"%s/%s", get_save_directory(), "scd.brm");
        fp = fopen(brm_file, "rb");
        if (fp!=NULL)
        {
            fread(scd.bram, 0x2000, 1, fp);
            fclose(fp);
        }

        /* Check if internal backup RAM is formatted */
        if (memcmp(scd.bram + 0x2000 - 0x20, brm_format + 0x20, 0x20))
        {
            /* Clear internal backup RAM */
            memset(scd.bram, 0x00, 0x200);

            /* Internal Backup RAM size fields */
            brm_format[0x10] = brm_format[0x12] = brm_format[0x14] = brm_format[0x16] = 0x00;
            brm_format[0x11] = brm_format[0x13] = brm_format[0x15] = brm_format[0x17] = (sizeof(scd.bram) / 64) - 3;

            /* Format internal backup RAM */
            memcpy(scd.bram + 0x2000 - 0x40, brm_format, 0x40);
        }

        /* Load cartridge backup RAM */
        if (scd.cartridge.id)
        {
            sprintf(brm_file,"%s/%s", get_save_directory(), "cart.brm");
            fp = fopen(brm_file, "rb");
            if (fp!=NULL)
            {
                fread(scd.cartridge.area, scd.cartridge.mask + 1, 1, fp);
                fclose(fp);
            }

            /* Check if cartridge backup RAM is formatted */
            if (memcmp(scd.cartridge.area + scd.cartridge.mask + 1 - 0x20, brm_format + 0x20, 0x20))
            {
                /* Clear cartridge backup RAM */
                memset(scd.cartridge.area, 0x00, scd.cartridge.mask + 1);

                /* Cartridge Backup RAM size fields */
                brm_format[0x10] = brm_format[0x12] = brm_format[0x14] = brm_format[0x16] = (((scd.cartridge.mask + 1) / 64) - 3) >> 8;
                brm_format[0x11] = brm_format[0x13] = brm_format[0x15] = brm_format[0x17] = (((scd.cartridge.mask + 1) / 64) - 3) & 0xff;

                /* Format cartridge backup RAM */
                memcpy(scd.cartridge.area + scd.cartridge.mask + 1 - sizeof(brm_format), brm_format, sizeof(brm_format));
            }
        }
    }
    if (sram.on)
    {
        /* Load SRAM (max. 64 KB)*/
        char save_file[256];
        sprintf(save_file,"%s/%s.srm", get_save_directory(), rom_filename);
        fp = fopen(save_file, "rb");
        if (fp!=NULL)
        {
            fseek(fp, 0, SEEK_END);
            int size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            fread(sram.sram,size,1, fp);
            fclose(fp);
        }
    }

    /* Reset system hardware */
    system_reset();

    SDL_PauseAudio(!config.use_sound);

    /* Three frames = 50 ms (60hz) or 60 ms (50hz) */
    if (sdl_sync.sem_sync)
        SDL_AddTimer(vdp_pal ? 60 : 50, sdl_sync_timer_callback, (void *) 1);

    /* Emulation loop */
    while(running)
    {
        SDL_Event event;
        if (SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                    running = 0;
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_HOME) //Powerslider up
                        goto_menu = 1;
                default:
                    break;
            }
        }
        /*if (do_once) 
        {
            do_once = 0; //don't waste write cycles!
            if (config.keepaspectratio)
            {
                FILE* aspect_ratio_file = fopen("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", "w");
                if (aspect_ratio_file)
                {
                    fwrite("Y", 1, 1, aspect_ratio_file);
                    fclose(aspect_ratio_file);
                }
            }
            if (!config.keepaspectratio)
    	    {
                FILE* aspect_ratio_file = fopen("/sys/devices/platform/jz-lcd.0/keep_aspect_ratio", "w");
                if (aspect_ratio_file)
                {
                    fwrite("N", 1, 1, aspect_ratio_file);
                    fclose(aspect_ratio_file);
                }
            }
	}*/

        /* SMS automask leftbar if screen mode changes (eg Alex Kidd in Miracle World) */
        static int smsmaskleftbar = 0;
        if ( (system_hw == SYSTEM_MARKIII) || (system_hw == SYSTEM_SMS) || (system_hw == SYSTEM_SMS2) || (system_hw == SYSTEM_PBC) )
        {
            if (config.smsmaskleftbar != smsmaskleftbar)
            {
                /* force screen change */
                bitmap.viewport.changed = 1;
                smsmaskleftbar = config.smsmaskleftbar;
            }
        }

        sdl_video_update();
        sdl_sound_update(config.use_sound);

        if (!sdl_video.frames_rendered)
        {
            if (!goto_menu)
            {
                SDL_SemWait(sdl_sync.sem_sync);
                if (post)    --post;
                if (post)
                {
                    do
                    {
                        SDL_SemWait(sdl_sync.sem_sync);
                    }
                    while(--post);
                }
            }
            else    gcw0menu();
        }
    }
    return 0;
}
