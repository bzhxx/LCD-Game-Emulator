/*

This program implements the main functions to use the LCD Game emulator.
It implements also sound buzzer and keyboard functions.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.
This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.

__author__ = "bzhxx"
__contact__ = "https://github.com/bzhxx"
__license__ = "GPLv3"

*/
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* Emulated system */
#include "sm510.h"
#include "sm500.h"
#include "gw_romloader.h"
#include "gw_system.h"
#include "gw_graphic.h"

static void (*device_reset)();
static void (*device_start)();
static void (*device_run)();
static void (*device_blit)(unsigned short *active_framebuffer);

static unsigned char last_joystick;

/* map functions and custom configuration */
bool gw_system_config()
{

	/* Supported System */
	/* SM500 SM5A(kb1013vk12) SM510 SM511 SM512 */

	/* init graphics parameters */
	gw_gfx_init();

	/* depending on the CPU, set functions pointers */

	// SM500
	if (strncmp(gw_head.cpu_name, ROM_CPU_SM500, 5) == 0)
	{
		device_start = sm500_device_start;
		device_reset = sm500_device_reset;
		device_run = sm500_execute_run;
		device_blit = gw_gfx_sm500_rendering;
		return true;
	}

	// SM5A
	if (strncmp(gw_head.cpu_name, ROM_CPU_SM5A, 5) == 0)
	{
		device_start = sm5a_device_start;
		device_reset = sm5a_device_reset;
		device_run = sm5a_execute_run;
		device_blit = gw_gfx_sm500_rendering;
		return true;
	}

	// SM510
	if (strncmp(gw_head.cpu_name, ROM_CPU_SM510, 5) == 0)
	{
		device_start = sm510_device_start;
		device_reset = sm510_device_reset;
		device_run = sm510_execute_run;
		device_blit = gw_gfx_sm510_rendering;
		return true;
	}

	// SM511
	if (strncmp(gw_head.cpu_name, ROM_CPU_SM511, 5) == 0)
	{
		device_start = sm510_device_start;
		device_reset = sm511_device_reset;
		device_run = sm511_execute_run;
		device_blit = gw_gfx_sm510_rendering;
		return (sm511_init_melody(gw_melody));
	}

	// SM512
	if (strncmp(gw_head.cpu_name, ROM_CPU_SM512, 5) == 0)
	{
		device_start = sm510_device_start;
		device_reset = sm511_device_reset;
		device_run = sm511_execute_run;
		device_blit = gw_gfx_sm510_rendering;
		return (sm511_init_melody(gw_melody));
	}

	return false;
}

void gw_system_reset() { device_reset(); }
void gw_system_start() { device_start(); }
void gw_system_blit(unsigned short *active_framebuffer) { device_blit(active_framebuffer); }
bool gw_system_romload() { return gw_romloader(); }

/******** Audio functions *******************/
static unsigned char mspeaker_data = 0;
static int gw_audio_buffer_idx = 0;

/* Audio buffer */
unsigned char gw_audio_buffer[GW_AUDIO_BUFFER_LENGTH * 2];
bool gw_audio_buffer_copied;

void gw_system_sound_init()
{
	/* Init Sound */
	/* clear shared audio buffer with emulator */
	memset(gw_audio_buffer, 0, sizeof(gw_audio_buffer));
	gw_audio_buffer_copied = false;

	gw_audio_buffer_idx = 0;
	mspeaker_data = 0;
}

static void gw_system_sound_melody(unsigned char data)
{
	if (gw_audio_buffer_copied)
	{
		gw_audio_buffer_copied = false;

		gw_audio_buffer_idx = gw_audio_buffer_idx - GW_AUDIO_BUFFER_LENGTH;

		if (gw_audio_buffer_idx < 0)
			gw_audio_buffer_idx = 0;

		// check if some samples have to be copied from the previous loop cycles
		if (gw_audio_buffer_idx != 0)
		{
			for (int i = 0; i < gw_audio_buffer_idx; i++)
				gw_audio_buffer[i] = gw_audio_buffer[i + GW_AUDIO_BUFFER_LENGTH];
		}
	}

	// SM511 R pin is melody output
	if (gw_melody != 0)

		mspeaker_data = data;

	// Piezo buzzer
	else
	{

		switch (gw_head.flags & FLAG_SOUND_MASK)
		{
			// R1 to piezo
		case FLAG_SOUND_R1_PIEZO:
			mspeaker_data = data & 1;
			break;

			// R2 to piezo
		case FLAG_SOUND_R2_PIEZO:
			mspeaker_data = data >> 1 & 1;
			break;

			// R1&R2 to piezo
		case FLAG_SOUND_R1R2_PIEZO:
			mspeaker_data = data & 3;
			break;

			// R1(+S1) to piezo
		case FLAG_SOUND_R1S1_PIEZO:
			mspeaker_data = (m_s_out & ~1) | (data & 1);
			break;

			// S1(+R1) to piezo, other to input mux
		case FLAG_SOUND_S1R1_PIEZO:
			mspeaker_data = (m_s_out & ~2) | (data << 1 & 2);
			break;

			// R1 to piezo
		default:
			mspeaker_data = data & 1;
		}
	}

	gw_audio_buffer[gw_audio_buffer_idx] = mspeaker_data;

	gw_audio_buffer_idx++;
}

void gw_writeR(unsigned char data) { gw_system_sound_melody(data); };

/******** Keys functions **********************/
/*
 S[8]xK[4],B and BA keys input function
 8 buttons, state known using gw_get_buttons()
 gw_get_buttons() : returns 
 left         | (up << 1)   | (right << 2) | (down << 3) |
 (a << 4)     | (b << 5)    | (time << 6)  | (game << 7) |
 (pause << 8) | (power << 9)
	
 We use only 8bits LSB, pause and power are ignored
	
*/

// default value is 1 (Pull-up)
unsigned char gw_readB()
{

	unsigned int keys_pressed = gw_get_buttons() & 0xff;

	if (keys_pressed == 0)
		return 1;

	if (gw_keyboard[9] == keys_pressed)
		return 0;

	return 1;
}

// default value is 1 (Pull-up)
unsigned char gw_readBA()
{

	unsigned int keys_pressed = gw_get_buttons() & 0xff;

	if (keys_pressed == 0)
		return 1;

	if (gw_keyboard[8] == keys_pressed)
		return 0;

	return 1;
}

// default value is 0 (Pull-down)
unsigned char gw_readK(unsigned char io_S)
{

	unsigned char io_K = 0;

	unsigned int keys_pressed = gw_get_buttons() & 0xff;

	if (keys_pressed == 0)
		return 0;

	for (int Sx = 0; Sx < 8; Sx++)
	{
		if (((io_S >> Sx) & 0x1) != 0)
		{


			if (gw_keyboard_multikey[Sx])
			{
				//joystick case
				unsigned int joystick_dir = (gw_keyboard[Sx] & 0x000000ff) |
											((gw_keyboard[Sx] >> 8) & 0x000000ff) |
											((gw_keyboard[Sx] >> 16) & 0x000000ff) |
											((gw_keyboard[Sx] >> 24) & 0x000000ff);

				if (joystick_dir ==  (GW_BUTTON_UP | GW_BUTTON_DOWN | GW_BUTTON_RIGHT | GW_BUTTON_LEFT))
				{
					if (keys_pressed == GW_BUTTON_LEFT )
						keys_pressed = (last_joystick & (0xFF-GW_BUTTON_RIGHT)) | GW_BUTTON_LEFT;

					if (keys_pressed == GW_BUTTON_RIGHT)
						keys_pressed = (last_joystick & (0xFF-GW_BUTTON_LEFT)) | GW_BUTTON_RIGHT;

					if (keys_pressed == GW_BUTTON_DOWN)
						keys_pressed = (last_joystick & (0xFF-GW_BUTTON_UP)) | GW_BUTTON_DOWN;

					if (keys_pressed == GW_BUTTON_UP)
						keys_pressed = (last_joystick & (0xFF-GW_BUTTON_DOWN)) | GW_BUTTON_UP;

					last_joystick= keys_pressed;
				}

				if (((gw_keyboard[Sx] & 0x000000ff) == (keys_pressed)))
					io_K |= 0x1;
				if (((gw_keyboard[Sx] & 0x0000ff00) == (keys_pressed << 8)))
					io_K |= 0x2;
				if (((gw_keyboard[Sx] & 0x00ff0000) == (keys_pressed << 16)))
					io_K |= 0x4;
				if (((gw_keyboard[Sx] & 0xff000000) == (keys_pressed << 24)))
					io_K |= 0x8;

			//single key mode
			} else
			{
				if (((gw_keyboard[Sx] & 0x000000ff) & (keys_pressed)) != 0)
					io_K |= 0x1;
				if (((gw_keyboard[Sx] & 0x0000ff00) & (keys_pressed << 8)) != 0)
					io_K |= 0x2;
				if (((gw_keyboard[Sx] & 0x00ff0000) & (keys_pressed << 16)) != 0)
					io_K |= 0x4;
				if (((gw_keyboard[Sx] & 0xff000000) & (keys_pressed << 24)) != 0)
					io_K |= 0x8;
			}
		}
	}

	//case of R/S output is not used to poll buttons (used R2 or S2 configuration)
	if (io_S == 0)
	{
		if (((gw_keyboard[1] & 0x000000ff) & (keys_pressed)) != 0)
			io_K |= 0x1;
		if (((gw_keyboard[1] & 0x0000ff00) & (keys_pressed << 8)) != 0)
			io_K |= 0x2;
		if (((gw_keyboard[1] & 0x00ff0000) & (keys_pressed << 16)) != 0)
			io_K |= 0x4;
		if (((gw_keyboard[1] & 0xff000000) & (keys_pressed << 24)) != 0)
			io_K |= 0x8;
	}

	return io_K & 0xf;
}

/* external function use to execute some clock cycles */
int gw_system_run(int clock_cycles)
{
	// check if a key is pressed to wakeup the system
	// set K input lines active state
	m_k_active = (gw_get_buttons() != 0);

	//1 CPU operation in 2 clock cycles
	if (m_clk_div == 2)
		m_icount += (clock_cycles / 2);

	//1 CPU operation in 4 clock cycles
	if (m_clk_div == 4)
		m_icount += (clock_cycles / 4);

	device_run();

	// return cycles really executed according to divider
	return m_icount * m_clk_div;
}

/* Shutdown gw */
void gw_system_shutdown(void)
{
	/* change audio frequency to default audio clock */
	//gw_set_audio_frequency(48000);
}
