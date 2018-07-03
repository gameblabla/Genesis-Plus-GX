/***************************************************************************************
 *  Genesis Plus
 *  Sega Sports Pad support
 *
 *  Copyright Eke-Eke (2007-2011)
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

#include "shared.h"

static struct
{
  uint8_t State;
  uint8_t Counter;
} sportspad[2];

void sportspad_reset(int32_t index)
{
  input.analog[index << 2][0] = 128;
  input.analog[index << 2][1] = 128;
  sportspad[index].State = 0x40;
  sportspad[index].Counter = 0;
}

static inline uint8_t sportspad_read(int32_t port)
{
  /* Buttons 1(B) & 2(C) status (active low) */
  uint8_t temp = ~(input.pad[port] & 0x30);

  /* Pad index */
  int32_t index = port >> 2;

  /* Clear low bits */
  temp &= 0x70;

  /* Detect current state */
  switch (sportspad[index].Counter & 3)
  {
    case 1:
    {
      /* X position high bits */
      temp |= (input.analog[port][0] >> 4) & 0x0F;
      break;
    }

    case 2:
    {
      /* X position low bits */
      temp |= input.analog[port][0] & 0x0F;
      break;
    }

    case 3:
    {
      /* Y position high bits */
      temp |= (input.analog[port][1] >> 4) & 0x0F;
      break;
    }

    default:
    {
      /* Y position low bits */
      temp |= input.analog[port][1] & 0x0F;
      break;
    }
  }

  return temp;
}

static inline void sportspad_write(int32_t index, uint8_t data, uint8_t mask)
{
  /* update bits set as output only */
  data = (sportspad[index].State & ~mask) | (data & mask);

  /* check TH transitions */
  if ((data ^ sportspad[index].State) & 0x40)
  {
    sportspad[index].Counter++;
  }

  /* update internal state */
  sportspad[index].State = data;
}

uint8_t sportspad_1_read(void)
{
  return sportspad_read(0);
}

uint8_t sportspad_2_read(void)
{
  return sportspad_read(4);
}

void sportspad_1_write(uint8_t data, uint8_t mask)
{
  sportspad_write(0, data, mask);
}

void sportspad_2_write(uint8_t data, uint8_t mask)
{
  sportspad_write(1, data, mask);
}
