/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "helptext.h"

#include "config.h"
#include "localization.h"
#include "episodes.h"
#include "file.h"
#include "fonthand.h"
#include "menus.h"
#include "opentyr.h"
#include "video.h"

#include <assert.h>
#include <string.h>

const JE_byte menuHelp[MENU_MAX][11] = /* [1..maxmenu, 1..11] */
{
	{  1, 34,  2,  3,  4,  5,                  0, 0, 0, 0, 0 },
	{  6,  7,  8,  9, 10, 11, 11, 12,                0, 0, 0 },
	{ 13, 14, 15, 15, 16, 17, 12,                 0, 0, 0, 0 },
	{                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{  4, 30, 30,  3,  5,                   0, 0, 0, 0, 0, 0 },
	{                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 16, 17, 15, 15, 12,                   0, 0, 0, 0, 0, 0 },
	{ 31, 31, 31, 31, 32, 12,                  0, 0, 0, 0, 0 },
	{  4, 34,  3,  5,                    0, 0, 0, 0, 0, 0, 0 }
};

JE_byte verticalHeight = 7;
JE_byte helpBoxColor = 12;
JE_byte helpBoxBrightness = 1;
JE_byte helpBoxShadeType = FULL_SHADE;

char keyName[8][18];                                                     /* [1..8] of string [17] */
char topicName[6][21];                                                   /* [1..6] of string [20] */
char mainMenuHelp[HELPTEXT_MAINMENUHELP_COUNT][66];                      /* [1..34] of string [65] */
char inGameText[6][21];                                                  /* [1..6] of string [20] */
char detailLevel[6][13];                                                 /* [1..6] of string [12] */
char gameSpeedText[5][13];                                               /* [1..5] of string [12] */
char inputDevices[3][13];                                                /* [1..3] of string [12] */
char networkText[HELPTEXT_NETWORKTEXT_COUNT][HELPTEXT_NETWORKTEXT_SIZE]; /* [1..4] of string [20] */
char difficultyNameB[11][21];                                            /* [0..9] of string [20] */
char joyButtonNames[5][21];                                              /* [1..5] of string [20] */
char superShips[HELPTEXT_SUPERSHIPS_COUNT][26];                          /* [0..10] of string [25] */
char specialName[HELPTEXT_SPECIALNAME_COUNT][10];                        /* [1..9] of string [9] */
char destructHelp[25][22];                                               /* [1..25] of string [21] */
char weaponNames[17][17];                                                /* [1..17] of string [16] */
char destructModeName[DESTRUCT_MODES][13];                               /* [1..destructmodes] of string [12] */
char shipInfo[HELPTEXT_SHIPINFO_COUNT][2][256];                          /* [1..13, 1..2] of string */
char menuInt[MENU_MAX+1][11][18];                                        /* [0..14, 1..11] of string [17] */

static void decrypt_string(char *s, size_t len)
{
	static const unsigned char crypt_key[] = { 204, 129, 63, 255, 71, 19, 25, 62, 1, 99 };

	if (len == 0)
		return;

	for (size_t i = len - 1; ; --i)
	{
		s[i] ^= crypt_key[i % sizeof(crypt_key)];
		if (i == 0)
			break;
		s[i] ^= s[i - 1];
	}
}

void read_encrypted_pascal_string(char *s, size_t size, FILE *f)
{
	Uint8 len;
	char buffer[255];

	fread_u8_die(&len, 1, f);
	fread_die(buffer, 1, len, f);

	if (size == 0)
		return;

	decrypt_string(buffer, len);

	assert(len < size);

	len = MIN(len, size - 1);
	memcpy(s, buffer, len);
	s[len] = '\0';
}

void skip_pascal_string(FILE *f)
{
	Uint8 len;
	char buffer[255];

	fread_u8_die(&len, 1, f);
	fread_die(buffer, 1, len, f);
}

void JE_helpBox(SDL_Surface *screen,  int x, int y, const char *message, unsigned int boxwidth)
{
	JE_byte startpos, endpos, pos;
	JE_boolean endstring;

	char substring[256];

	if (strlen(message) == 0)
	{
		return;
	}

	pos = 1;
	endpos = 0;
	endstring = false;

	do
	{
		startpos = endpos + 1;

		do
		{
			endpos = pos;
			do
			{
				pos++;
				if (pos == strlen(message))
				{
					endstring = true;
					if ((unsigned)(pos - startpos) < boxwidth)
					{
						endpos = pos + 1;
					}
				}

			} while (!(message[pos-1] == ' ' || endstring));

		} while (!((unsigned)(pos - startpos) > boxwidth || endstring));

		SDL_strlcpy(substring, message + startpos - 1, MIN((size_t)(endpos - startpos + 1), sizeof(substring)));
		JE_textShade(screen, x, y, substring, helpBoxColor, helpBoxBrightness, helpBoxShadeType);

		y += verticalHeight;

	} while (!endstring);

	if (endpos != pos + 1)
	{
		JE_textShade(screen, x, y, message + endpos, helpBoxColor, helpBoxBrightness, helpBoxShadeType);
	}

	helpBoxColor = 12;
	helpBoxShadeType = FULL_SHADE;
}

void JE_HBox(SDL_Surface *screen, int x, int y, unsigned int  messagenum, unsigned int boxwidth)
{
	JE_helpBox(screen, x, y, _n("HELPTEXT_%d",messagenum-1), boxwidth);
}

void JE_loadHelpText(void)
{
	const unsigned int menuInt_entries[MENU_MAX + 1] = { -1, 7, 9, 8, -1, -1, 11, -1, -1, -1, 6, 4, 6, 7, 5 };
	
	FILE *f = dir_fopen_die(data_dir(), "tyrian.hdt", "rb");
	fread_s32_die(&episode1DataLoc, 1, f);

	/*Online Help*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < HELPTEXT_HELPTXT_COUNT; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Planet names*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < HELPTEXT_PNAME_COUNT; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Miscellaneous text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < HELPTEXT_MISCTEXT_COUNT; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Little Miscellaneous text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < HELPTEXT_MISCTEXTB_COUNT; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Key names*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[6]; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Main Menu*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < HELPTEXT_MAINMENU_COUNT; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Event text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < HELPTEXT_OUTPUTS_COUNT; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Help topics*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(topicName); ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Main Menu Help*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(mainMenuHelp); ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Menu 1 - Main*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[1]; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Menu 2 - Items*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[2]; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Menu 3 - Options*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[3]; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*InGame Menu*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(inGameText); ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Detail Level*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(detailLevel); ++i)
		read_encrypted_pascal_string(detailLevel[i], sizeof(detailLevel[i]), f);
	skip_pascal_string(f);

	/*Game speed text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(gameSpeedText); ++i)
		read_encrypted_pascal_string(gameSpeedText[i], sizeof(gameSpeedText[i]), f);
	skip_pascal_string(f);

	// episode names
	skip_pascal_string(f);
	for (unsigned int i = 0; i < HELPTEXT_EPISODE_MENU_COUNT; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	// difficulty names
	skip_pascal_string(f);
	for (unsigned int i = 0; i < HELPTEXT_DIFFICULTY_MENU_COUNT; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	// gameplay mode names
	skip_pascal_string(f);
	for (unsigned int i = 0; i < HELPTEXT_GAMEPLAY_MENU_COUNT; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Menu 10 - 2Player Main*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[10]; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Input Devices*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(inputDevices); ++i)
		read_encrypted_pascal_string(inputDevices[i], sizeof(inputDevices[i]), f);
	skip_pascal_string(f);

	/*Network text*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(networkText); ++i)
		read_encrypted_pascal_string(networkText[i], sizeof(networkText[i]), f);
	skip_pascal_string(f);

	/*Menu 11 - 2Player Network*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[11]; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*HighScore Difficulty Names*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(difficultyNameB); ++i)
		read_encrypted_pascal_string(difficultyNameB[i], sizeof(difficultyNameB[i]), f);
	skip_pascal_string(f);

	/*Menu 12 - Network Options*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[12]; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Menu 13 - Joystick*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[13]; ++i)
		skip_pascal_string(f);
	skip_pascal_string(f);

	/*Joystick Button Assignments*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(joyButtonNames); ++i)
		read_encrypted_pascal_string(joyButtonNames[i], sizeof(joyButtonNames[i]), f);
	skip_pascal_string(f);

	/*SuperShips - For Super Arcade Mode*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(superShips); ++i)
		read_encrypted_pascal_string(superShips[i], sizeof(superShips[i]), f);
	skip_pascal_string(f);

	/*SuperShips - For Super Arcade Mode*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(specialName); ++i)
		read_encrypted_pascal_string(specialName[i], sizeof(specialName[i]), f);
	skip_pascal_string(f);

	/*Secret DESTRUCT game*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(destructHelp); ++i)
		read_encrypted_pascal_string(destructHelp[i], sizeof(destructHelp[i]), f);
	skip_pascal_string(f);

	/*Secret DESTRUCT weapons*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(weaponNames); ++i)
		read_encrypted_pascal_string(weaponNames[i], sizeof(weaponNames[i]), f);
	skip_pascal_string(f);

	/*Secret DESTRUCT modes*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(destructModeName); ++i)
		read_encrypted_pascal_string(destructModeName[i], sizeof(destructModeName[i]), f);
	skip_pascal_string(f);

	/*NEW: Ship Info*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < COUNTOF(shipInfo); ++i)
	{
		read_encrypted_pascal_string(shipInfo[i][0], sizeof(shipInfo[i][0]), f);
		read_encrypted_pascal_string(shipInfo[i][1], sizeof(shipInfo[i][1]), f);
	}
	skip_pascal_string(f);

	/*Menu 12 - Network Options*/
	skip_pascal_string(f);
	for (unsigned int i = 0; i < menuInt_entries[14]; ++i)
		skip_pascal_string(f);

	fclose(f);

	// Fill the topicName array with translater strings (help menu)
	__(topicName[0], "HELPMENU_HELP_MENU", sizeof(topicName[0]));
	__(topicName[1], "HELPMENU_1P_GAME_MENU", sizeof(topicName[1]));
	__(topicName[2], "HELPMENU_2P_GAME_MENU", sizeof(topicName[2]));
	__(topicName[3], "HELPMENU_UPGRADE_SHIP", sizeof(topicName[3]));
	__(topicName[4], "HELPMENU_OPTIONS", sizeof(topicName[4]));
	__(topicName[5], "HELPMENU_EXIT_HELP", sizeof(topicName[5]));

	// Fill the menuInt arrays with translated strings
	__(menuInt[1][0], "1PFMENU_GAME_MENU", sizeof(menuInt[1][0]));
	__(menuInt[1][1], "1PFMENU_DATA", sizeof(menuInt[1][1]));
	__(menuInt[1][2], "1PFMENU_SHIP_SPECS", sizeof(menuInt[1][2]));
	__(menuInt[1][3], "1PFMENU_UPGRADE_SHIP", sizeof(menuInt[1][3]));
	__(menuInt[1][4], "1PFMENU_OPTIONS", sizeof(menuInt[1][4]));
	__(menuInt[1][5], "1PFMENU_PLAY_NEXT_LVL", sizeof(menuInt[1][5]));
	__(menuInt[1][6], "1PFMENU_QUIT", sizeof(menuInt[1][6]));

	__(menuInt[2][0], "UPGMENU_UPGRADE_SHIP", sizeof(menuInt[2][0]));
	__(menuInt[2][1], "UPGMENU_SHIP_TYPE", sizeof(menuInt[2][1]));
	__(menuInt[2][2], "UPGMENU_FRONT_GUN", sizeof(menuInt[2][2]));
	__(menuInt[2][3], "UPGMENU_REAR_GUN", sizeof(menuInt[2][3]));
	__(menuInt[2][4], "UPGMENU_SHIELD", sizeof(menuInt[2][4]));
	__(menuInt[2][5], "UPGMENU_GENERATOR", sizeof(menuInt[2][5]));
	__(menuInt[2][6], "UPGMENU_LEFT_SIDEKICK", sizeof(menuInt[2][6]));
	__(menuInt[2][7], "UPGMENU_RIGHT_SIDEKICK", sizeof(menuInt[2][7]));
	__(menuInt[2][8], "UPGMENU_DONE", sizeof(menuInt[2][8]));

	__(menuInt[3][0], "OPTMENU_OPTIONS", sizeof(menuInt[3][0]));
	__(menuInt[3][1], "OPTMENU_LOAD", sizeof(menuInt[3][1]));
	__(menuInt[3][2], "OPTMENU_SAVE", sizeof(menuInt[3][2]));
	__(menuInt[3][3], "OPTMENU_MUSIC", sizeof(menuInt[3][3]));
	__(menuInt[3][4], "OPTMENU_SOUND", sizeof(menuInt[3][4]));
	__(menuInt[3][5], "OPTMENU_JOYSTICK", sizeof(menuInt[3][5]));
	__(menuInt[3][6], "OPTMENU_KEYBOARD", sizeof(menuInt[3][6]));
	__(menuInt[3][7], "OPTMENU_DONE", sizeof(menuInt[3][7]));

	__(menuInt[6][0], "KEY_CONFIG", sizeof(menuInt[6][0]));
	__(menuInt[6][1], "KEY_UP", sizeof(menuInt[6][1]));
	__(menuInt[6][2], "KEY_DOWN", sizeof(menuInt[6][2]));
	__(menuInt[6][3], "KEY_LEFT", sizeof(menuInt[6][3]));
	__(menuInt[6][4], "KEY_RIGHT", sizeof(menuInt[6][4]));
	__(menuInt[6][5], "KEY_FIRE", sizeof(menuInt[6][5]));
	__(menuInt[6][6], "KEY_CHANGEFIRE", sizeof(menuInt[6][6]));
	__(menuInt[6][7], "KEY_LEFTSIDEKICK", sizeof(menuInt[6][7]));
	__(menuInt[6][8], "KEY_RIGHTSIDEKICK", sizeof(menuInt[6][8]));
	__(menuInt[6][9], "KEY_RESET_TO_DEFAULT", sizeof(menuInt[6][9]));
	__(menuInt[6][10], "KEY_DONE", sizeof(menuInt[6][10]));

	__(menuInt[10][0], "2PMENU_GAME_MENU", sizeof(menuInt[10][0]));
	__(menuInt[10][1], "2PMENU_PLAY_NEXT_LVL", sizeof(menuInt[10][1]));
	__(menuInt[10][2], "2PMENU_PLAYER_1_INPUT", sizeof(menuInt[10][2]));
	__(menuInt[10][3], "2PMENU_PLAYER_2_INPUT", sizeof(menuInt[10][3]));
	__(menuInt[10][4], "2PMENU_OPTIONS", sizeof(menuInt[10][4]));
	__(menuInt[10][5], "2PMENU_QUIT_GAME", sizeof(menuInt[10][5]));

	__(menuInt[11][0], "1PAMENU_GAME_MENU", sizeof(menuInt[11][0]));
	__(menuInt[11][1], "1PAMENU_PLAY_NEXT_LVL", sizeof(menuInt[11][1]));
	__(menuInt[11][2], "1PAMENU_OPTIONS", sizeof(menuInt[11][2]));
	__(menuInt[11][3], "1PAMENU_QUIT_GAME", sizeof(menuInt[11][3]));

	__(menuInt[12][0], "OPTMENU1_OPTIONS", sizeof(menuInt[12][0]));
	__(menuInt[12][1], "OPTMENU1_JOYSTICK", sizeof(menuInt[12][1]));
	__(menuInt[12][2], "OPTMENU1_KEYBOARD", sizeof(menuInt[12][2]));
	__(menuInt[12][3], "OPTMENU1_MUSIC", sizeof(menuInt[12][3]));
	__(menuInt[12][4], "OPTMENU1_SOUND", sizeof(menuInt[12][4]));
	__(menuInt[12][5], "OPTMENU1_DONE", sizeof(menuInt[12][5]));

	__(menuInt[13][0], "MENUTITLE_JOYSTICK", sizeof(menuInt[13][0]));

	__(menuInt[14][0], "SUPERMENU_GAME_MENU", sizeof(menuInt[14][0]));
	__(menuInt[14][1], "SUPERMENU_PLAY_NEXT_LVL", sizeof(menuInt[14][1]));
	__(menuInt[14][2], "SUPERMENU_SHIP_SPECS", sizeof(menuInt[14][2]));
	__(menuInt[14][3], "SUPERMENU_OPTIONS", sizeof(menuInt[14][3]));
	__(menuInt[14][4], "SUPERMENU_QUIT_GAME", sizeof(menuInt[14][4]));

	// Fill in mainMenuHelp array with translated strings
	for (unsigned int i = 0; i < COUNTOF(mainMenuHelp); ++i)
		__n(mainMenuHelp[i], "MENU_HINT_%d", sizeof(mainMenuHelp[i]), i);

	__(inGameText[0], "INGAMEMENU_MUSIC_VOL", sizeof(inGameText[0]));
	__(inGameText[1], "INGAMEMENU_SOUND_VOL", sizeof(inGameText[1]));
	__(inGameText[2], "INGAMEMENU_DETAIL_LVL", sizeof(inGameText[2]));
	__(inGameText[3], "INGAMEMENU_GAME_SPEED", sizeof(inGameText[3]));
	__(inGameText[4], "INGAMEMENU_RETURN_TO_GAME", sizeof(inGameText[4]));
	__(inGameText[5], "INGAMEMENU_QUIT_LVL", sizeof(inGameText[5]));
}
