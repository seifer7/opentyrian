/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2015  The OpenTyrian Development Team
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
/*!
 * \file localization.h
 * \author Kane Shaw
 * \date 2023
 * \copyright GNU General Public License v2+ or Mozilla Public License 2.0
 */
#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef COUNTOFA
 /*!
  * \brief Calculate the number of elements in a fixed-length array.
  *
  * \param[in] a the fixed-length array
  * \return the number of elements in the array
  */
#define COUNTOFA(a) (sizeof(a) / sizeof(*(a)))
#endif

 /*! \cond suppress_doxygen */
#define LANGUAGE_STRING_LONG_TAG(s) ((s).short_buf[COUNTOFA((s).short_buf) - 1])
/*! \endcond */

#define LANGUAGE_LINE_KEY_LENGTH 25

typedef union
{
	/*!
	 * \brief The inline buffer for short strings.
	 */
	char short_buf[16];

	/*!
	 * \brief The buffer for long strings.
	 *
	 * May be \c NULL.
	 */
	char* long_buf;
} LanguageString;

typedef struct {
	LanguageString name;
	LanguageString value;
} LanguageLine;

typedef struct {
    unsigned int line_count;
    LanguageLine *lines;
} Language;

enum {
	IS_COMMENT = 1 << 0,
	IS_EMPTY = 1 << 1,
	IS_MULTILINE = 1 << 2,
	HAS_MULTILINE_START = 1 << 3,
	HAS_MULTILINE_END = 1 << 4,
	IS_CONDITION_START = 1 << 5,
	IS_CONDITION_END = 1 << 6,
};

extern Language current_language;

static inline char* language_string_to_cstr(const LanguageString* string)
{
	assert(string != NULL);
	char is_long = LANGUAGE_STRING_LONG_TAG(*string);
	return is_long ?
		string->long_buf :
		string->short_buf;
}
extern LanguageLine* language_add_line_len(Language* language, const char* name, const char* value, size_t name_len, size_t value_len);
static inline LanguageLine* language_add_line(Language* language, char* name, char* value)
{
	assert(language != NULL);
	assert(name != NULL);
	assert(value != NULL);
	return language_add_line_len(language,
		name,
		value,
		strlen(name),
		strlen(value)
	);
}

void language_oom(void);
int language_line_flags(const char* buffer, size_t* index);
bool language_parse(Language* language, FILE* file);
void language_parse_line_get_key(char* key, const char* buffer, size_t* index);
void language_parse_line_get_value_pos(size_t* start, size_t* end, const char* buffer, size_t* index);
void language_parse_line(Language* language, const char* buffer, size_t* index);
extern void language_load(void);
extern LanguageLine* language_find_line(Language* language, const char* name);
extern char* language_get_value(const LanguageLine* line);
extern char* _(const char* language_line_name);
extern char* _n(const char* language_line_name, const size_t n);
extern char* _nn(const char* language_line_name, const size_t n, const size_t nn);
extern void __(char* s, const char* language_line_name, const size_t size);
extern void __n(char* s, const char* language_line_name, size_t size, const size_t n);
extern void __nn(char* s, const char* language_line_name, size_t size, const size_t n, const size_t nn);

#endif
