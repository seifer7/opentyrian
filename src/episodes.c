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

#pragma warning(disable : 5105)

#include "episodes.h"

#include "config.h"
#include "file.h"
#include "tinydir.h"
#include "lvllib.h"
#include "lvlmast.h"
#include "opentyr.h"
#include "mtrand.h"

/* MAIN Weapons Data */
JE_WeaponPortType *weaponPort;
JE_WeaponPortTypeOld oldWeaponPort[PORT_NUM + 1];
JE_WeaponType     weapons[WEAP_NUM + 1]; /* [0..weapnum] */

/* Items */
JE_PowerType   *powerSys;
JE_ShipType    *ships;
JE_OptionType  *options;
JE_ShieldType  *shields;
JE_SpecialType *special;

JE_PowerTypeOld   oldPowerSys[POWER_NUM + 1];
JE_ShipTypeOld    oldShips[SHIP_NUM + 1];
JE_OptionTypeOld  oldOptions[OPTION_NUM + 1]; /* [0..optionnum] */
JE_ShieldTypeOld  oldShields[SHIELD_NUM + 1];
JE_SpecialTypeOld oldSpecial[SPECIAL_NUM + 1];

/* Enemy data */
JE_EnemyDatType enemyDat;

/* EPISODE variables */
JE_byte    initial_episode_num, episodeNum = 0;
JE_boolean episodeAvail[EPISODE_MAX]; /* [1..episodemax] */
char       episode_file[13], cube_file[13];

JE_longint episode1DataLoc;

/* Tells the game whether the level currently loaded is a bonus level. */
JE_boolean bonusLevel;

/* Tells if the game jumped back to Episode 1 */
JE_boolean jumpBackToEpisode1;

bool value_in_array_je_byte(int val, JE_byte* arr, size_t n) {
	for (size_t i = 0; i < n; i++) {
		if (arr[i] == val)
			return true;
	}
	return false;
}

bool add_random_shop_item(const enum ITEM_TYPE* item_type, JE_byte* itemAvail, int index, const size_t itemAvailSize) {
	int itemCount = 0;

	switch (*item_type) {
	case ItemType_Ship:
		itemCount = ships[0].count;
		break;
	case ItemType_WeaponFront:
	case ItemType_WeaponRear:
		itemCount = weaponPort[0].count;
		break;
	case ItemType_Generator:
		itemCount = powerSys[0].count;
		break;
	case ItemType_SidekickLeft:
	case ItemType_SidekickRight:
		itemCount = options[0].count;
		break;
	case ItemType_Shield:
		itemCount = shields[0].count;
		break;
	default:
		return false;
	}

	int i = 0;
	int ii = 0;
	int itemsListCount = 0;
	JE_byte *itemList;
	itemList = (JE_byte*) malloc(sizeof(JE_byte) * itemCount);
	memset(itemList, 0, sizeof(JE_byte) * itemCount);

	switch (*item_type) {
	case ItemType_Ship:
		for (; ii < itemCount; ii++) {
			if (strcmp(ships[ii].id, "0") == 0) continue;
			if (strcmp(ships[ii].id, "") == 0) break;
			if (value_in_array_je_byte(ii, itemAvail, itemAvailSize)) continue;
			int probability = ships[ii].probability;
			int randomNum = (mt_rand() % 100);
			if (!(randomNum < probability)) continue;
			itemList[i++] = ii;
			itemsListCount++;
		}
		break;
	case ItemType_WeaponFront:
	case ItemType_WeaponRear:
		for (; ii < itemCount; ii++) {
			if (strcmp(weaponPort[ii].id, "0") == 0) continue;
			if (strcmp(weaponPort[ii].id, "") == 0) break;
			if (*item_type == ItemType_WeaponRear && weaponPort[ii].position != 1) continue;
			if (*item_type == ItemType_WeaponFront && weaponPort[ii].position == 1) continue;
			if (value_in_array_je_byte(ii, itemAvail, itemAvailSize)) continue;
			int probability = weaponPort[ii].probability;
			int randomNum = (mt_rand() % 100);
			if (!(randomNum < probability)) continue;
			itemList[i++] = ii;
			itemsListCount++;
		}
		break;
	case ItemType_Generator:
		for (; ii < itemCount; ii++) {
			if (strcmp(powerSys[ii].id, "0") == 0) continue;
			if (strcmp(powerSys[ii].id, "") == 0) break;
			if (value_in_array_je_byte(ii, itemAvail, itemAvailSize)) continue;
			int probability = powerSys[ii].probability;
			int randomNum = (mt_rand() % 100);
			if (!(randomNum < probability)) continue;
			itemList[i++] = ii;
			itemsListCount++;
		}
		break;
	case ItemType_SidekickLeft:
	case ItemType_SidekickRight:
		for (; ii < itemCount; ii++) {
			if (strcmp(options[ii].id, "0") == 0) continue;
			if (strcmp(options[ii].id, "") == 0) break;
			if (*item_type == ItemType_SidekickLeft && options[ii].position == 2) continue;
			if (*item_type == ItemType_SidekickRight && options[ii].position == 1) continue;
			if (value_in_array_je_byte(ii, itemAvail, itemAvailSize)) continue;
			int probability = options[ii].probability;
			int randomNum = (mt_rand() % 100);
			if (!(randomNum < probability)) continue;
			itemList[i++] = ii;
			itemsListCount++;
		}
		break;
	case ItemType_Shield:
		for (; ii < itemCount; ii++) {
			if (strcmp(shields[ii].id, "0") == 0) continue;
			if (strcmp(shields[ii].id, "") == 0) break;
			if (value_in_array_je_byte(ii, itemAvail, itemAvailSize)) continue;
			int probability = shields[ii].probability;
			int randomNum = (mt_rand() % 100);
			if (!(randomNum < probability)) continue;
			itemList[i++] = ii;
			itemsListCount++;
		}
		break;
	default:
		return false;
	}

	if (itemsListCount == 0) return false;

	int num = (mt_rand() % itemsListCount);
	itemAvail[index] = itemList[num];
	return true;
}

size_t item_idx_from_jebyte(const enum ITEM_TYPE* item_type, const JE_byte* id) {
	char c[11] = "";
	sprintf(c, "%d", *id);
	return item_idx(item_type, c);
}

size_t item_idx(const enum ITEM_TYPE* item_type, const char* id) {
	JE_byte index = -1;
	get_item_index(item_type, id, &index);
	return (size_t) index;
}

bool get_item_index_from_int(const enum ITEM_TYPE* item_type, const int* id, JE_byte* output) {
	char c[11] = "";
	sprintf(c, "%d", *id);
	return get_item_index(item_type, c, output);
}

bool get_item_index(const enum ITEM_TYPE* item_type, const char* id, JE_byte* output) {
	size_t index = -1;
	size_t i = -1;
	bool end = false;
	for (;;) {
		switch (*item_type) {
		case ItemType_Ship:
			if (i == -1) i = ships[0].count;
			if (strcmp(ships[i].id, id) == 0) index = i;
			break;
		case ItemType_WeaponFront:
		case ItemType_WeaponRear:
			if (i == -1) i = weaponPort[0].count;
			if (strcmp(weaponPort[i].id, id) == 0) index = i;
			break;
		case ItemType_Generator:
			if (i == -1) i = powerSys[0].count;
			if (strcmp(powerSys[i].id, id) == 0) index = i;
			break;
		case ItemType_Shield:
			if (i == -1) i = shields[0].count;
			if (strcmp(shields[i].id, id) == 0) index = i;
			break;
		case ItemType_SidekickLeft:
		case ItemType_SidekickRight:
			if (i == -1) i = options[0].count;
			if (strcmp(options[i].id, id) == 0) index = i;
			break;
		case ItemType_Special:
			if (i == -1) i = special[0].count;
			if (strcmp(special[i].id, id) == 0) index = i;
			break;
		default:
			end = true;
			break;
		}
		if (end == true || index != -1 || i == 0) break;
		i--;
	}
	if (index != -1) {
		*output = index;
		return true;
	}
	return false;
}

void parse_value_pair(const char* buffer, size_t* index, char* key, char* value) {
	size_t i = *index;
	bool key_done = false;
	size_t vi = 0;
	size_t ki = 0;

	for (; ; ++i) {
		char c = buffer[i];
		if(!key_done) key[ki++] = c;
		if (key_done) value[vi++] = c;
		if (c == '=') {
			key_done = true;
			key[ki-1] = '\0';
		}
		else if (c == '\0' || c == '\r' || c == '\n') {
			value[vi-1] = '\0';
			break;
		}
	}
}

void read_item_file(FILE* file, enum ITEM_TYPE item_type) {
	size_t item_i = -1;
	size_t buffer_cap = 128;
	char* buffer = malloc(buffer_cap * sizeof(char));
	if (buffer == NULL) exit(1); // TODO: Error handling
	size_t buffer_end = 1;
	buffer[buffer_end - 1] = '\0';

	for (size_t line = 0, next_line = 0; ; line = next_line)
	{
		char first_char = '\0';
		/* find beginning of next line */
		while (next_line < buffer_end)
		{
			char c = buffer[next_line];
			if (first_char == '\0') first_char = buffer[next_line];

			if (c == '\0' && next_line == buffer_end - 1)
			{
				if (line > 0)
				{
					/* shift to front */
					memmove(&buffer[0], &buffer[line], buffer_end - line);
					buffer_end -= line;
					next_line -= line;
					line = 0;
				}
				else if (buffer_end > 1)
				{
					/* need larger capacity */
					buffer_cap *= 2;
					char* new_buffer = realloc(buffer, buffer_cap * sizeof(char));
					if (new_buffer == NULL) exit(1); // TODO: Error handling
					buffer = new_buffer;
				}

				size_t read = fread(&buffer[buffer_end - 1], sizeof(char), buffer_cap - buffer_end, file);
				if (read == 0)
					break;

				buffer_end += read;
				buffer[buffer_end - 1] = '\0';
			}
			else
			{
				++next_line;

				if (c == '\n' || c == '\r')
				{
					break;
				}
			}
		}

		/* if at end of file */
		if (next_line == line)
			break;

		size_t i = line;

		assert(i <= next_line);

		char key[30] = "";
		char value[255] = "";
		char* tempptr;

		parse_value_pair(buffer, &i, key, value);
		if (item_i == -1) {
			item_i = 0;
			bool done = false;
			for (;;) {
				switch (item_type) {
					case ItemType_Ship:
						if (strcmp(ships[item_i].id, "") == 0) done = true;
						break;
					case ItemType_WeaponFront:
					case ItemType_WeaponRear:
						if (strcmp(weaponPort[item_i].id, "") == 0) done = true;
						break;
					case ItemType_Generator:
						if (strcmp(powerSys[item_i].id, "") == 0) done = true;
						break;
					case ItemType_Shield:
						if (strcmp(shields[item_i].id, "") == 0) done = true;
						break;
					case ItemType_SidekickLeft:
					case ItemType_SidekickRight:
						if (strcmp(options[item_i].id, "") == 0) done = true;
						break;
					case ItemType_Special:
						if (strcmp(special[item_i].id, "") == 0) done = true;
						break;
					case ItemType_Shot:
						// TODO
						break;
					case ItemType_Count:
						break;
				}
				if (done) break;
				item_i++;
			}
		}
		
		switch (item_type) {
			case ItemType_Ship:
				if (strcmp(key, "id") == 0) {
					memcpy(ships[item_i].id, value, sizeof(ships[item_i].id)-1);
				}
				else if (strcmp(key, "name") == 0) {
					memcpy(ships[item_i].name, value, sizeof(ships[item_i].name)-1);
				}
				else if (strcmp(key, "shipgraphic") == 0) {
					ships[item_i].shipgraphic = atoi(value);
				}
				else if (strcmp(key, "itemgraphic") == 0) {
					ships[item_i].itemgraphic = atoi(value);
				}
				else if (strcmp(key, "ani") == 0) {
					ships[item_i].ani = atoi(value);
				}
				else if (strcmp(key, "spd") == 0) {
					ships[item_i].spd = atoi(value);
				}
				else if (strcmp(key, "dmg") == 0) {
					ships[item_i].dmg = atoi(value);
				}
				else if (strcmp(key, "cost") == 0) {
					ships[item_i].cost = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphic") == 0) {
					ships[item_i].bigshipgraphic = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphx") == 0) {
					ships[item_i].bigshipgraphx = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphy") == 0) {
					ships[item_i].bigshipgraphy = atoi(value);
				}
				else if (strcmp(key, "probability") == 0) {
					ships[item_i].probability = atoi(value);
				}
			break;
			case ItemType_WeaponFront:
			case ItemType_WeaponRear:
				if (strcmp(key, "id") == 0) {
					memcpy(weaponPort[item_i].id, value, sizeof(weaponPort[item_i].id) - 1);
				}
				else if (strcmp(key, "name") == 0) {
					memcpy(weaponPort[item_i].name, value, sizeof(weaponPort[item_i].name) - 1);
				}
				else if (strcmp(key, "opnum") == 0) {
					weaponPort[item_i].opnum = atoi(value);
				}
				else if (strcmp(key, "op0") == 0) {
					int j = 0, temp;
					while (str_pop_int(value, &temp) && j < COUNTOF(weaponPort[item_i].op[0]))
						weaponPort[item_i].op[0][j++] = temp;
				}
				else if (strcmp(key, "op1") == 0) {
					int j = 0, temp;
					while (str_pop_int(value, &temp) && j < COUNTOF(weaponPort[item_i].op[1]))
						weaponPort[item_i].op[1][j++] = temp;
				}
				else if (strcmp(key, "cost") == 0) {
					weaponPort[item_i].cost = strtol(value, &tempptr, 10);
				}
				else if (strcmp(key, "itemgraphic") == 0) {
					weaponPort[item_i].itemgraphic = atoi(value);
				}
				else if (strcmp(key, "poweruse") == 0) {
					weaponPort[item_i].poweruse = atoi(value);
				}
				else if (strcmp(key, "position") == 0) {
					weaponPort[item_i].position = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphic") == 0) {
					weaponPort[item_i].bigshipgraphic = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphfrntx") == 0) {
					weaponPort[item_i].bigshipgraphfrntx = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphfrnty") == 0) {
					weaponPort[item_i].bigshipgraphfrnty = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphrearx") == 0) {
					weaponPort[item_i].bigshipgraphrearx = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphreary") == 0) {
					weaponPort[item_i].bigshipgraphreary = atoi(value);
				}
				else if (strcmp(key, "probability") == 0) {
					weaponPort[item_i].probability = atoi(value);
				}
				break;
			case ItemType_Generator:
				if (strcmp(key, "id") == 0) {
					memcpy(powerSys[item_i].id, value, sizeof(powerSys[item_i].id)-1);
				}
				else if (strcmp(key, "name") == 0) {
					memcpy(powerSys[item_i].name, value, sizeof(powerSys[item_i].name)-1);
				}
				else if (strcmp(key, "itemgraphic") == 0) {
					powerSys[item_i].itemgraphic = atoi(value);
				}
				else if (strcmp(key, "power") == 0) {
					powerSys[item_i].power = atoi(value);
				}
				else if (strcmp(key, "speed") == 0) {
					powerSys[item_i].speed = atoi(value);
				}
				else if (strcmp(key, "cost") == 0) {
					powerSys[item_i].cost = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphic") == 0) {
					powerSys[item_i].bigshipgraphic = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphx") == 0) {
					powerSys[item_i].bigshipgraphx = atoi(value);
				}
				else if (strcmp(key, "bigshipgraphy") == 0) {
					powerSys[item_i].bigshipgraphy = atoi(value);
				}
				else if (strcmp(key, "probability") == 0) {
					powerSys[item_i].probability = atoi(value);
				}
			break;
			case ItemType_Shield:
				if (strcmp(key, "id") == 0) {
					memcpy(shields[item_i].id, value, sizeof(shields[item_i].id)-1);
				}
				else if (strcmp(key, "name") == 0) {
					memcpy(shields[item_i].name, value, sizeof(shields[item_i].name)-1);
				}
				else if (strcmp(key, "itemgraphic") == 0) {
					shields[item_i].itemgraphic = atoi(value);
				}
				else if (strcmp(key, "tpwr") == 0) {
					shields[item_i].tpwr = atoi(value);
				}
				else if (strcmp(key, "mpwr") == 0) {
					shields[item_i].mpwr = atoi(value);
				}
				else if (strcmp(key, "cost") == 0) {
					shields[item_i].cost = atoi(value);
				}
				else if (strcmp(key, "probability") == 0) {
					shields[item_i].probability = atoi(value);
				}
				break;
			case ItemType_SidekickLeft:
			case ItemType_SidekickRight:
				if (strcmp(key, "id") == 0) {
					memcpy(options[item_i].id, value, sizeof(options[item_i].id) - 1);
				}
				else if (strcmp(key, "name") == 0) {
					memcpy(options[item_i].name, value, sizeof(options[item_i].name) - 1);
				}
				else if (strcmp(key, "itemgraphic") == 0) {
					options[item_i].itemgraphic = atoi(value);
				}
				else if (strcmp(key, "pwr") == 0) {
					options[item_i].pwr = atoi(value);
				}
				else if (strcmp(key, "cost") == 0) {
					options[item_i].cost = atoi(value);
				}
				else if (strcmp(key, "tr") == 0) {
					options[item_i].tr = atoi(value);
				}
				else if (strcmp(key, "option") == 0) {
					options[item_i].option = atoi(value);
				}
				else if (strcmp(key, "opspd") == 0) {
					options[item_i].opspd = atoi(value);
				}
				else if (strcmp(key, "ani") == 0) {
					options[item_i].ani = atoi(value);
				}
				else if (strcmp(key, "gr") == 0) {
					int j = 0, temp;
					while (str_pop_int(value, &temp) && j < COUNTOF(options[item_i].gr))
						options[item_i].gr[j++] = temp;
				}
				else if (strcmp(key, "wport") == 0) {
					options[item_i].wport = atoi(value);
				}
				else if (strcmp(key, "wpnum") == 0) {
					options[item_i].wpnum = atoi(value);
				}
				else if (strcmp(key, "ammo") == 0) {
					options[item_i].ammo = atoi(value);
				}
				else if (strcmp(key, "stop") == 0) {
					options[item_i].stop = atoi(value);
				}
				else if (strcmp(key, "icongr") == 0) {
					options[item_i].icongr = atoi(value);
				}
				else if (strcmp(key, "position") == 0) {
					options[item_i].position = atoi(value);
				}
				else if (strcmp(key, "probability") == 0) {
					options[item_i].probability = atoi(value);
				}
				break;
			case ItemType_Special:
				if (strcmp(key, "id") == 0) {
					memcpy(special[item_i].id, value, sizeof(special[item_i].id) - 1);
				}
				else if (strcmp(key, "name") == 0) {
					memcpy(special[item_i].name, value, sizeof(special[item_i].name) - 1);
				}
				else if (strcmp(key, "itemgraphic") == 0) {
					special[item_i].itemgraphic = atoi(value);
				}
				else if (strcmp(key, "pwr") == 0) {
					special[item_i].pwr = atoi(value);
				}
				else if (strcmp(key, "stype") == 0) {
					special[item_i].stype = atoi(value);
				}
				else if (strcmp(key, "wpn") == 0) {
					special[item_i].wpn = atoi(value);
				}
				break;
			case ItemType_Shot:
				// TODO
				break;
			case ItemType_Count:
				break;
		}
	
		first_char = '\0';
	}

	free(buffer);
}

void load_items(void) {
	for (int item_type = 0; item_type < ItemType_Count; item_type++) {
		if (item_type == ItemType_WeaponRear) continue;
		if (item_type == ItemType_SidekickRight) continue;
		tinydir_dir dir;

		int numItems = 0;
		char itemFileExt[7] = "";
		switch (item_type) {
			case ItemType_Ship:
				tinydir_open_sorted(&dir, TINYDIR_STRING("./data/items/ship"));
				strcpy(itemFileExt, "tyrshp");
				break;
			case ItemType_WeaponFront:
				tinydir_open_sorted(&dir, TINYDIR_STRING("./data/items/weapon"));
				strcpy(itemFileExt, "tyrwep");
				break;
			case ItemType_Generator:
				tinydir_open_sorted(&dir, TINYDIR_STRING("./data/items/generator"));
				strcpy(itemFileExt, "tyrgen");
				break;
			case ItemType_Shield:
				tinydir_open_sorted(&dir, TINYDIR_STRING("./data/items/shield"));
				strcpy(itemFileExt, "tyrshd");
				break;
			case ItemType_SidekickLeft:
				tinydir_open_sorted(&dir, TINYDIR_STRING("./data/items/sidekick"));
				strcpy(itemFileExt, "tyrsid");
				break;
			case ItemType_Special:
				tinydir_open_sorted(&dir, TINYDIR_STRING("./data/items/special"));
				strcpy(itemFileExt, "tyrspc");
				break;
			default:
				continue;
			break;
		}

		for (size_t i = 0; i < dir.n_files; i++)
		{
			tinydir_file file;
			if (tinydir_readfile_n(&dir, &file, i) == -1)
			{
				continue; // Failed to get acces to item file
			}
			if (file.is_dir) continue;
			if (_tinydir_strcmp(itemFileExt, file.extension) != 0) continue;
			numItems++;
		}

		switch (item_type) {
		case ItemType_Ship:
			ships = calloc(numItems, sizeof(*ships));
			ships[0].count = numItems;
			break;
		case ItemType_WeaponFront:
			weaponPort = calloc(numItems, sizeof(*weaponPort));
			weaponPort[0].count = numItems;
			break;
		case ItemType_Generator:
			powerSys = calloc(numItems, sizeof(*powerSys));
			powerSys[0].count = numItems;
			break;
		case ItemType_Shield:
			shields = calloc(numItems, sizeof(*shields));
			shields[0].count = numItems;
			break;
		case ItemType_SidekickLeft:
			options = calloc(numItems, sizeof(*options));
			options[0].count = numItems;
			break;
		case ItemType_Special:
			special = calloc(numItems, sizeof(*special));
			special[0].count = numItems;
			break;
		default:
			continue;
			break;
		}

		for (size_t i = 0, s = 0; i < dir.n_files && s < numItems; i++) {
			tinydir_file file;
			if (tinydir_readfile_n(&dir, &file, i) == -1)
			{
				continue; // Failed to get acces to ship file
			}
			if (file.is_dir) continue;
			if (_tinydir_strcmp(itemFileExt, file.extension) != 0) continue;

			FILE* itemFile = fopen(file.path, "r");
			read_item_file(itemFile, item_type);

			s++;
		}
		tinydir_close(&dir);
	}
}

void JE_loadItemDat(void)
{
	FILE *f = NULL;
	
	if (episodeNum <= 3)
	{
		f = dir_fopen_die(data_dir(), "tyrian.hdt", "rb");
		fread_s32_die(&episode1DataLoc, 1, f);
		fseek(f, episode1DataLoc, SEEK_SET);
	}
	else
	{
		// episode 4 stores item data in the level file
		f = dir_fopen_die(data_dir(), levelFile, "rb");
		fseek(f, lvlPos[lvlNum-1], SEEK_SET);
	}

	JE_word itemNum[7]; /* [1..7] */
	fread_u16_die(itemNum, 7, f);

	for (int i = 0; i < WEAP_NUM + 1; ++i)
	{
		fread_u16_die(&weapons[i].drain,           1, f);
		fread_u8_die( &weapons[i].shotrepeat,      1, f);
		fread_u8_die( &weapons[i].multi,           1, f);
		fread_u16_die(&weapons[i].weapani,         1, f);
		fread_u8_die( &weapons[i].max,             1, f);
		fread_u8_die( &weapons[i].tx,              1, f);
		fread_u8_die( &weapons[i].ty,              1, f);
		fread_u8_die( &weapons[i].aim,             1, f);
		fread_u8_die(  weapons[i].attack,          8, f);
		fread_u8_die(  weapons[i].del,             8, f);
		fread_s8_die(  weapons[i].sx,              8, f);
		fread_s8_die(  weapons[i].sy,              8, f);
		fread_s8_die(  weapons[i].bx,              8, f);
		fread_s8_die(  weapons[i].by,              8, f);
		fread_u16_die( weapons[i].sg,              8, f);
		fread_s8_die( &weapons[i].acceleration,    1, f);
		fread_s8_die( &weapons[i].accelerationx,   1, f);
		fread_u8_die( &weapons[i].circlesize,      1, f);
		fread_u8_die( &weapons[i].sound,           1, f);
		fread_u8_die( &weapons[i].trail,           1, f);
		fread_u8_die( &weapons[i].shipblastfilter, 1, f);

		#ifdef DO_DECYRPT
		char filename[60];
		snprintf(filename, sizeof(filename), "data\\shots\\%d.tyrsht", i);
		if (!dir_file_exists(get_user_directory(), filename)) {
			FILE* outF = dir_fopen(get_user_directory(), filename, "w");
			fprintf(outF, "id=%d\n", i);
			fprintf(outF, "drain=%d\n", weapons[i].drain);
			fprintf(outF, "shotrepeat=%d\n", weapons[i].shotrepeat);
			fprintf(outF, "multi=%d\n", weapons[i].multi);
			fprintf(outF, "weapani=%d\n", weapons[i].weapani);
			fprintf(outF, "max=%d\n", weapons[i].max);
			fprintf(outF, "tx=%d\n", weapons[i].tx);
			fprintf(outF, "ty=%d\n", weapons[i].ty);
			fprintf(outF, "aim=%d\n", weapons[i].aim);
			fputs("attack=", outF);
			for (int ii = 0; ii < COUNTOF(weapons[i].attack); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", weapons[i].attack[ii]);
			}
			fputs("\n", outF);
			fputs("del=", outF);
			for (int ii = 0; ii < COUNTOF(weapons[i].del); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", weapons[i].del[ii]);
			}
			fputs("\n", outF);
			fputs("sx=", outF);
			for (int ii = 0; ii < COUNTOF(weapons[i].sx); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", weapons[i].sx[ii]);
			}
			fputs("\n", outF);
			fputs("sy=", outF);
			for (int ii = 0; ii < COUNTOF(weapons[i].sy); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", weapons[i].sy[ii]);
			}
			fputs("\n", outF);
			fputs("bx=", outF);
			for (int ii = 0; ii < COUNTOF(weapons[i].bx); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", weapons[i].bx[ii]);
			}
			fputs("\n", outF);
			fputs("by=", outF);
			for (int ii = 0; ii < COUNTOF(weapons[i].by); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", weapons[i].by[ii]);
			}
			fputs("\n", outF);
			fputs("sg=", outF);
			for (int ii = 0; ii < COUNTOF(weapons[i].sg); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", weapons[i].sg[ii]);
			}
			fputs("\n", outF);
			fprintf(outF, "acceleration=%d\n", weapons[i].acceleration);
			fprintf(outF, "accelerationx=%d\n", weapons[i].accelerationx);
			fprintf(outF, "circlesize=%d\n", weapons[i].circlesize);
			fprintf(outF, "sound=%d\n", weapons[i].sound);
			fprintf(outF, "trail=%d\n", weapons[i].trail);
			fprintf(outF, "shipblastfilter=%d\n", weapons[i].shipblastfilter);
			fclose(outF);
		}
		#endif
	}
	
	for (int i = 0; i < PORT_NUM + 1; ++i)
	{
		Uint8 nameLen;
		fread_u8_die( &nameLen,                   1, f);
		fread_die(    &oldWeaponPort[i].name,    1, 30, f);
		oldWeaponPort[i].name[MIN(nameLen, 30)] = '\0';
		fread_u8_die( &oldWeaponPort[i].opnum,       1, f);
		fread_u16_die( oldWeaponPort[i].op[0],      11, f);
		fread_u16_die( oldWeaponPort[i].op[1],      11, f);
		fread_u16_die(&oldWeaponPort[i].cost,        1, f);
		fread_u16_die(&oldWeaponPort[i].itemgraphic, 1, f);
		fread_u16_die(&oldWeaponPort[i].poweruse,    1, f);

		#ifdef DO_DECYRPT
		char filename[60];
		snprintf(filename, sizeof(filename), "data\\items\\weapon\\%d.tyrwep", i);
		if (!dir_file_exists(get_user_directory(), filename)) {
			FILE* outF = dir_fopen(get_user_directory(), filename, "w");
			fprintf(outF, "id=%d\n", i);
			fprintf(outF, "name=%s\n", oldWeaponPort[i].name);
			fprintf(outF, "opnum=%d\n", oldWeaponPort[i].opnum);
			fputs("op0=", outF);
			for (int ii = 0; ii < COUNTOF(oldWeaponPort[i].op[0]); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", oldWeaponPort[i].op[0][ii]);
			}
			fputs("\n", outF);
			fputs("op1=", outF);
			for (int ii = 0; ii < COUNTOF(oldWeaponPort[i].op[1]); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", oldWeaponPort[i].op[1][ii]);
			}
			fputs("\n", outF);
			fprintf(outF, "cost=%d\n", oldWeaponPort[i].cost);
			fprintf(outF, "itemgraphic=%d\n", oldWeaponPort[i].itemgraphic);
			fprintf(outF, "poweruse=%d\n", oldWeaponPort[i].poweruse);
			fclose(outF);
		}
		#endif
	}

	for (int i = 0; i < SPECIAL_NUM + 1; ++i)
	{
		Uint8 nameLen;
		fread_u8_die( &nameLen,                1, f);
		fread_die(    &oldSpecial[i].name,    1, 30, f);
		oldSpecial[i].name[MIN(nameLen, 30)] = '\0';
		fread_u16_die(&oldSpecial[i].itemgraphic, 1, f);
		fread_u8_die( &oldSpecial[i].pwr,         1, f);
		fread_u8_die( &oldSpecial[i].stype,       1, f);
		fread_u16_die(&oldSpecial[i].wpn,         1, f);

		#ifdef DO_DECYRPT
		char filename[60];
		snprintf(filename, sizeof(filename), "data\\items\\special\\%d.tyrspc", i);
		if (!dir_file_exists(get_user_directory(), filename)) {
			FILE* outF = dir_fopen(get_user_directory(), filename, "w");
			fprintf(outF, "id=%d\n", i);
			fprintf(outF, "name=%s\n", oldSpecial[i].name);
			fprintf(outF, "itemgraphic=%d\n", oldSpecial[i].itemgraphic);
			fprintf(outF, "pwr=%d\n", oldSpecial[i].pwr);
			fprintf(outF, "stype=%d\n", oldSpecial[i].stype);
			fprintf(outF, "wpn=%d\n", oldSpecial[i].wpn);
			fclose(outF);
		}
		#endif
	}

	for (int i = 0; i < POWER_NUM + 1; ++i)
	{
		Uint8 nameLen;
		fread_u8_die( &nameLen,                 1, f);
		fread_die(    &oldPowerSys[i].name,    1, 30, f);
		oldPowerSys[i].name[MIN(nameLen, 30)] = '\0';
		fread_u16_die(&oldPowerSys[i].itemgraphic, 1, f);
		fread_u8_die( &oldPowerSys[i].power,       1, f);
		fread_s8_die( &oldPowerSys[i].speed,       1, f);
		fread_u16_die(&oldPowerSys[i].cost,        1, f);

		#ifdef DO_DECYRPT
		char filename[60];
		snprintf(filename, sizeof(filename), "data\\items\\generator\\%d.tyrgen", i);
		if (!dir_file_exists(get_user_directory(), filename)) {
			FILE* outF = dir_fopen(get_user_directory(), filename, "w");
			fprintf(outF, "id=%d\n", i);
			fprintf(outF, "name=%s\n", oldPowerSys[i].name);
			fprintf(outF, "itemgraphic=%d\n", oldPowerSys[i].itemgraphic);
			fprintf(outF, "power=%d\n", oldPowerSys[i].power);
			fprintf(outF, "speed=%d\n", oldPowerSys[i].speed);
			fprintf(outF, "cost=%d\n", oldPowerSys[i].cost);
			fclose(outF);
		}
		#endif
	}

	for (int i = 0; i < SHIP_NUM + 1; ++i)
	{
		Uint8 nameLen;
		fread_u8_die( &nameLen,                 1, f);
		fread_die(    &oldShips[i].name,       1, 30, f);
		oldShips[i].name[MIN(nameLen, 30)] = '\0';
		fread_u16_die(&oldShips[i].shipgraphic,    1, f);
		fread_u16_die(&oldShips[i].itemgraphic,    1, f);
		fread_u8_die( &oldShips[i].ani,            1, f);
		fread_s8_die( &oldShips[i].spd,            1, f);
		fread_u8_die( &oldShips[i].dmg,            1, f);
		fread_u16_die(&oldShips[i].cost,           1, f);
		fread_u8_die( &oldShips[i].bigshipgraphic, 1, f);

		#ifdef DO_DECYRPT
		char filename[60];
		snprintf(filename, sizeof(filename), "data\\items\\ship\\%d.tyrshp", i);
		if (!dir_file_exists(get_user_directory(), filename)) {
			FILE* outF = dir_fopen(get_user_directory(), filename, "w");
			fprintf(outF, "id=%d\n", i);
			fprintf(outF, "name=%s\n", oldShips[i].name);
			fprintf(outF, "shipgraphic=%d\n", oldShips[i].shipgraphic);
			fprintf(outF, "itemgraphic=%d\n", oldShips[i].itemgraphic);
			fprintf(outF, "ani=%d\n", oldShips[i].ani);
			fprintf(outF, "spd=%d\n", oldShips[i].spd);
			fprintf(outF, "dmg=%d\n", oldShips[i].dmg);
			fprintf(outF, "cost=%d\n", oldShips[i].cost);
			fprintf(outF, "bigshipgraphic=%d\n", oldShips[i].bigshipgraphic);
			fclose(outF);
		}
		#endif
	}

	for (int i = 0; i < OPTION_NUM + 1; ++i)
	{
		Uint8 nameLen;
		fread_u8_die(  &nameLen,                1, f);
		fread_die(     &oldOptions[i].name,    1, 30, f);
		oldOptions[i].name[MIN(nameLen, 30)] = '\0';
		fread_u8_die(  &oldOptions[i].pwr,         1, f);
		fread_u16_die( &oldOptions[i].itemgraphic, 1, f);
		fread_u16_die( &oldOptions[i].cost,        1, f);
		fread_u8_die(  &oldOptions[i].tr,          1, f);
		fread_u8_die(  &oldOptions[i].option,      1, f);
		fread_s8_die(  &oldOptions[i].opspd,       1, f);
		fread_u8_die(  &oldOptions[i].ani,         1, f);
		fread_u16_die(  oldOptions[i].gr,         20, f);
		fread_u8_die(  &oldOptions[i].wport,       1, f);
		fread_u16_die( &oldOptions[i].wpnum,       1, f);
		fread_u8_die(  &oldOptions[i].ammo,        1, f);
		fread_bool_die(&oldOptions[i].stop,           f);
		fread_u8_die(  &oldOptions[i].icongr,      1, f);

		#ifdef DO_DECYRPT
		char filename[60];
		snprintf(filename, sizeof(filename), "data\\items\\sidekick\\%d.tyrsid", i);
		if (!dir_file_exists(get_user_directory(), filename)) {
			FILE* outF = dir_fopen(get_user_directory(), filename, "w");
			fprintf(outF, "id=%d\n", i);
			fprintf(outF, "name=%s\n", oldOptions[i].name);
			fprintf(outF, "itemgraphic=%d\n", oldOptions[i].itemgraphic);
			fprintf(outF, "pwr=%d\n", oldOptions[i].pwr);
			fprintf(outF, "cost=%d\n", oldOptions[i].cost);
			fprintf(outF, "tr=%d\n", oldOptions[i].tr);
			fprintf(outF, "option=%d\n", oldOptions[i].option);
			fprintf(outF, "opspd=%d\n", oldOptions[i].opspd);
			fprintf(outF, "ani=%d\n", oldOptions[i].ani);
			fputs("gr=", outF);
			for (int ii = 0; ii < COUNTOF(oldOptions[i].gr); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", oldOptions[i].gr[ii]);
			}
			fputs("\n", outF);
			fprintf(outF, "wport=%d\n", oldOptions[i].wport);
			fprintf(outF, "wpnum=%d\n", oldOptions[i].wpnum);
			fprintf(outF, "ammo=%d\n", oldOptions[i].ammo);
			fprintf(outF, "stop=%d\n", oldOptions[i].stop);
			fprintf(outF, "icongr=%d\n", oldOptions[i].icongr);
			fclose(outF);
		}
		#endif
	}

	for (int i = 0; i < SHIELD_NUM + 1; ++i)
	{
		Uint8 nameLen;
		fread_u8_die( &nameLen,                1, f);
		fread_die(    &oldShields[i].name,    1, 30, f);
		oldShields[i].name[MIN(nameLen, 30)] = '\0';
		fread_u8_die( &oldShields[i].tpwr,        1, f);
		fread_u8_die( &oldShields[i].mpwr,        1, f);
		fread_u16_die(&oldShields[i].itemgraphic, 1, f);
		fread_u16_die(&oldShields[i].cost,        1, f);

		#ifdef DO_DECYRPT
		char filename[60];
		snprintf(filename, sizeof(filename), "data\\items\\shield\\%d.tyrshd", i);
		if (!dir_file_exists(get_user_directory(), filename)) {
			FILE* outF = dir_fopen(get_user_directory(), filename, "w");
			fprintf(outF, "id=%d\n", i);
			fprintf(outF, "name=%s\n", oldShields[i].name);
			fprintf(outF, "tpwr=%d\n", oldShields[i].tpwr);
			fprintf(outF, "mpwr=%d\n", oldShields[i].mpwr);
			fprintf(outF, "itemgraphic=%d\n", oldShields[i].itemgraphic);
			fprintf(outF, "cost=%d\n", oldShields[i].cost);
			fclose(outF);
		}
		#endif
	}
	
	for (int i = 0; i < ENEMY_NUM + 1; ++i)
	{
		fread_u8_die( &enemyDat[i].ani,           1, f);
		fread_u8_die(  enemyDat[i].tur,           3, f);
		fread_u8_die(  enemyDat[i].freq,          3, f);
		fread_s8_die( &enemyDat[i].xmove,         1, f);
		fread_s8_die( &enemyDat[i].ymove,         1, f);
		fread_s8_die( &enemyDat[i].xaccel,        1, f);
		fread_s8_die( &enemyDat[i].yaccel,        1, f);
		fread_s8_die( &enemyDat[i].xcaccel,       1, f);
		fread_s8_die( &enemyDat[i].ycaccel,       1, f);
		fread_s16_die(&enemyDat[i].startx,        1, f);
		fread_s16_die(&enemyDat[i].starty,        1, f);
		fread_s8_die( &enemyDat[i].startxc,       1, f);
		fread_s8_die( &enemyDat[i].startyc,       1, f);
		fread_u8_die( &enemyDat[i].armor,         1, f);
		fread_u8_die( &enemyDat[i].esize,         1, f);
		fread_u16_die( enemyDat[i].egraphic,     20, f);
		fread_u8_die( &enemyDat[i].explosiontype, 1, f);
		fread_u8_die( &enemyDat[i].animate,       1, f);
		fread_u8_die( &enemyDat[i].shapebank,     1, f);
		fread_s8_die( &enemyDat[i].xrev,          1, f);
		fread_s8_die( &enemyDat[i].yrev,          1, f);
		fread_u16_die(&enemyDat[i].dgr,           1, f);
		fread_s8_die( &enemyDat[i].dlevel,        1, f);
		fread_s8_die( &enemyDat[i].dani,          1, f);
		fread_u8_die( &enemyDat[i].elaunchfreq,   1, f);
		fread_u16_die(&enemyDat[i].elaunchtype,   1, f);
		fread_s16_die(&enemyDat[i].value,         1, f);
		fread_u16_die(&enemyDat[i].eenemydie,     1, f);

		#ifdef DO_DECYRPT
		char filename[60];
		snprintf(filename, sizeof(filename), "data\\enemy\\%d.tyremy", i);
		if (!dir_file_exists(get_user_directory(), filename)) {
			FILE* outF = dir_fopen(get_user_directory(), filename, "w");
			fprintf(outF, "id=%d\n", i);
			fprintf(outF, "ani=%d\n", enemyDat[i].ani);
			fputs("tur=", outF);
			for (int ii = 0; ii < COUNTOF(enemyDat[i].tur); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", enemyDat[i].tur[ii]);
			}
			fputs("\n", outF);
			fputs("freq=", outF);
			for (int ii = 0; ii < COUNTOF(enemyDat[i].freq); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", enemyDat[i].freq[ii]);
			}
			fputs("\n", outF);
			fprintf(outF, "xmove=%d\n", enemyDat[i].xmove);
			fprintf(outF, "ymove=%d\n", enemyDat[i].ymove);
			fprintf(outF, "xaccel=%d\n", enemyDat[i].xaccel);
			fprintf(outF, "yaccel=%d\n", enemyDat[i].yaccel);
			fprintf(outF, "xcaccel=%d\n", enemyDat[i].xcaccel);
			fprintf(outF, "ycaccel=%d\n", enemyDat[i].ycaccel);
			fprintf(outF, "startx=%d\n", enemyDat[i].startx);
			fprintf(outF, "starty=%d\n", enemyDat[i].starty);
			fprintf(outF, "startxc=%d\n", enemyDat[i].startxc);
			fprintf(outF, "startyc=%d\n", enemyDat[i].startyc);
			fprintf(outF, "armor=%d\n", enemyDat[i].armor);
			fprintf(outF, "esize=%d\n", enemyDat[i].esize);
			fputs("egraphic=", outF);
			for (int ii = 0; ii < COUNTOF(enemyDat[i].egraphic); ii++) {
				if (ii > 0) fputs(" ", outF);
				fprintf(outF, "%d", enemyDat[i].egraphic[ii]);
			}
			fputs("\n", outF);
			fprintf(outF, "explosiontype=%d\n", enemyDat[i].explosiontype);
			fprintf(outF, "animate=%d\n", enemyDat[i].animate);
			fprintf(outF, "shapebank=%d\n", enemyDat[i].shapebank);
			fprintf(outF, "xrev=%d\n", enemyDat[i].xrev);
			fprintf(outF, "yrev=%d\n", enemyDat[i].yrev);
			fprintf(outF, "dgr=%d\n", enemyDat[i].dgr);
			fprintf(outF, "dlevel=%d\n", enemyDat[i].dlevel);
			fprintf(outF, "dani=%d\n", enemyDat[i].dani);
			fprintf(outF, "elaunchfreq=%d\n", enemyDat[i].elaunchfreq);
			fprintf(outF, "elaunchtype=%d\n", enemyDat[i].elaunchtype);
			fprintf(outF, "value=%d\n", enemyDat[i].value);
			fprintf(outF, "eenemydie=%d\n", enemyDat[i].eenemydie);
			fclose(outF);
		}
		#endif
	}
	
	fclose(f);
}

void JE_initEpisode(JE_byte newEpisode)
{
	if (newEpisode == episodeNum)
		return;
	
	episodeNum = newEpisode;
	
	snprintf(levelFile,    sizeof(levelFile),    "tyrian%d.lvl",  episodeNum);
	snprintf(cube_file,    sizeof(cube_file),    "cubetxt%d.dat", episodeNum);
	snprintf(episode_file, sizeof(episode_file), "levels%d.dat",  episodeNum);
	
	JE_analyzeLevel();
	JE_loadItemDat();
}

void JE_scanForEpisodes(void)
{
	for (int i = 0; i < EPISODE_MAX; ++i)
	{
		char ep_file[20];
		snprintf(ep_file, sizeof(ep_file), "tyrian%d.lvl", i + 1);
		episodeAvail[i] = dir_file_exists(data_dir(), ep_file);
	}
}

unsigned int JE_findNextEpisode(void)
{
	unsigned int newEpisode = episodeNum;
	
	jumpBackToEpisode1 = false;
	
	while (true)
	{
		newEpisode++;
		
		if (newEpisode > EPISODE_MAX)
		{
			newEpisode = 1;
			jumpBackToEpisode1 = true;
			gameHasRepeated = true;
		}
		
		if (episodeAvail[newEpisode-1] || newEpisode == episodeNum)
		{
			break;
		}
	}
	
	return newEpisode;
}
