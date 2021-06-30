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
#ifndef _GW_SYSTEM_H_
#define _GW_SYSTEM_H_

#define GW_SCREEN_WIDTH 320
#define GW_SCREEN_HEIGHT 240

/* refresh rate 128Hz */
#define GW_REFRESH_RATE 128

/* System clock = Audio clock = 32768Hz */
#define GW_SYS_FREQ 32768U
#define GW_AUDIO_FREQ GW_SYS_FREQ

#define GW_AUDIO_BUFFER_LENGTH (GW_AUDIO_FREQ / GW_REFRESH_RATE)

/* emulated system : number of clock cycles per loop */
#define GW_SYSTEM_CYCLES (GW_AUDIO_FREQ / GW_REFRESH_RATE)

/* Main API to emulate the system */

// Configure, start and reset (in this order)
bool gw_system_config();
void gw_system_start();
void gw_system_reset();

// Run some clock cycles and refresh the display
int gw_system_run(int clock_cycles);
void gw_system_blit(unsigned short *active_framebuffer);

// Audio init
void gw_system_sound_init();

// ROM loader
bool gw_system_romload();

// get buttons state
unsigned int gw_get_buttons();

/* Dummy API for load/save state and network support */
// void gw_system_netplay_callback(netplay_event_t event, void *arg);
// bool gw_system_SaveState(char *pathName);
// bool gw_system_LoadState(char *pathName);

/* shared audio buffer between host and emulator */
extern unsigned char gw_audio_buffer[GW_AUDIO_BUFFER_LENGTH * 2];
extern bool gw_audio_buffer_copied;

/* Output of emulated system  */
void gw_writeR(unsigned char data);

/* Input of emulated system */
unsigned char gw_readK(unsigned char S);
unsigned char gw_readBA();
unsigned char gw_readB();

#define GW_BUTTON_LEFT  1
#define GW_BUTTON_UP    (1 << 1)
#define GW_BUTTON_RIGHT (1 << 2)
#define GW_BUTTON_DOWN  (1 << 3)
#define GW_BUTTON_A     (1 << 4)
#define GW_BUTTON_B     (1 << 5)
#define GW_BUTTON_TIME  (1 << 6)
#define GW_BUTTON_GAME  (1 << 7)

#endif /* _GW_SYSTEM_H_ */
