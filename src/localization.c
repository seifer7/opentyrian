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
 * \file localization.c
 * \author Kane Shaw
 * \date 2023
 * \copyright GNU General Public License v2+ or Mozilla Public License 2.0
 */
#include "localization.h"
#include "config.h"
#include "file.h"

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// Need to disable warning for error code 4774
// 'snprintf' : format string expected in argument 3 is not a string literal
// Because the value fed into snprintf needs to be dynamicly generated in functions _n and _nn.
#pragma warning( disable : 4774)

Language current_language;

static bool is_whitespace(char c)
{
	return c == '\t' || c == ' ';
}

static bool is_end(char c)
{
	return c == '\0' || c == '\n' || c == '\r';
}

static bool is_whitespace_or_end(char c)
{
	return is_whitespace(c) || is_end(c);
}

void language_oom(void)
{
	fprintf(stderr, "out of memory\n");
	exit(EXIT_FAILURE);
}

static LanguageString string_init_len(const char* s, size_t n)
{
	LanguageString string;

	if (s == NULL)
	{
		LANGUAGE_STRING_LONG_TAG(string) = true;

		string.long_buf = NULL;
	}
	else
	{
		char is_long = n >= COUNTOF(string.short_buf);

		LANGUAGE_STRING_LONG_TAG(string) = is_long;

		char* buffer = is_long ?
			string.long_buf = malloc((n + 1) * sizeof(char)) :
			string.short_buf;
		if (buffer == NULL)
			language_oom();

		memcpy(buffer, s, n * sizeof(char));
		buffer[n] = '\0';
	}

	return string;
}

static void init_line(LanguageLine* line, const char* name, size_t name_len, const char* value, size_t value_len)
{
	line->name = string_init_len(name, name_len);
	line->value = string_init_len(value, value_len);
}

LanguageLine* language_add_line_len(Language* language, const char* name, const char* value, size_t name_len, size_t value_len) {
	assert(language != NULL);
	assert(name != NULL);
	assert(value != NULL);

	LanguageLine* lines = realloc(language->lines, (language->line_count + 1) * sizeof(LanguageLine));
	if (lines == NULL)
		return NULL;

	LanguageLine* line = &lines[language->line_count];

	language->line_count += 1;
	language->lines = lines;

	init_line(line, name, name_len, value, value_len);

	return line;
}

void language_init(Language* language)
{
	assert(language != NULL);

	language->line_count = 0;
	language->lines = NULL;
}

void language_load(void) {
	Language* lang = &current_language;
	language_init(lang);
    FILE* fi;
    fi = dir_fopen_die(get_user_directory(), "data/lang/english-us.txt", "r");
	language_parse(&current_language, fi);
}

bool language_parse(Language* language, FILE* file)
{
	assert(language != NULL);
	assert(file != NULL);

	size_t buffer_cap = 128;
	char* buffer = malloc(buffer_cap * sizeof(char));
	if (buffer == NULL)
		language_oom();
	size_t buffer_end = 1;
	buffer[buffer_end - 1] = '\0';

	for (size_t line = 0, next_line = 0; ; line = next_line)
	{
		char first_char = '\0';
		bool reading_multiline = false;
		int flags = 0;
		/* find beginning of next line */
		while (next_line < buffer_end)
		{
			char c = buffer[next_line];
			if(first_char == '\0') first_char = buffer[next_line];

			if (c == '\0' && next_line == buffer_end - 1)
			{
				if (line > 0 && !reading_multiline)
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
					if (new_buffer == NULL)
						language_oom();
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
					reading_multiline = false;
					size_t i = line;
					flags = language_line_flags(buffer, &i);
					if (flags & IS_MULTILINE) {
						if (flags & HAS_MULTILINE_END) {
							break;
						}
						else {
							// We haven't got the end of the multiline yet!
							reading_multiline = true;
						}
					}
					else {
						break;
					}
				}
			}
		}

		/* if at end of file */
		if (next_line == line)
			break;

		size_t i = line;

		assert(i <= next_line);

		if (!(flags & (IS_COMMENT | IS_EMPTY)))
			language_parse_line(language, buffer, &i);
		first_char = '\0';
	}

	free(buffer);

	return language;
}

int language_line_flags(const char* buffer, size_t* index) {
	int flags = 0;
	size_t i = *index;
	char c = buffer[i];

	if (is_end(c)) {
		flags |= IS_EMPTY;
	}
	else if (c == '#') {
		flags |= IS_COMMENT;
	}
	else if (c == '%') {
		char check_if[5];
		strncpy(check_if, buffer + i, sizeof(check_if));
		check_if[4] = '\0';
		char check_endif[7];
		strncpy(check_endif, buffer + i, sizeof(check_endif));
		check_endif[6] = '\0';

		if (strcmp("%IF ", check_if) == 0) {
			flags |= IS_CONDITION_START;
		}
		else if (strcmp("%ENDIF", check_endif) == 0) {
			flags |= IS_CONDITION_END;
		}
	}
	else {
		size_t value_start = 0;
		size_t value_end = 0;
		language_parse_line_get_value_pos(&value_start, &value_end, buffer, &i);
		if (buffer[value_start] == '`') {
			flags |= IS_MULTILINE | HAS_MULTILINE_START;
		}
		if (buffer[value_end-1] == '`') {
			flags |= IS_MULTILINE | HAS_MULTILINE_END;
		}
	}

	return flags;
}

void language_parse_line_get_key(char *key, const char* buffer, size_t* index) {
	size_t i = *index;
	size_t key_start;
	size_t key_end;

	while (is_whitespace(buffer[i]))
		++i;

	// Find string key
	key_start = i;
	for (; ; ++i)
	{
		char c = buffer[i];
		if (is_whitespace(c) || c == ':') {
			key_end = i;
			break;
		}
	}

	strncpy(key, buffer + key_start, MIN(LANGUAGE_LINE_KEY_LENGTH, (key_end - key_start)));
}

void language_parse_line_get_value_pos(size_t* start, size_t* end, const char* buffer, size_t* index) {
	size_t i = *index;
	bool is_multiline = false;

	while (is_whitespace(buffer[i]))
		++i;

	// Skip string key
	for (; ; ++i)
	{
		char c = buffer[i];
		if (c == ':') break;
	}
	i++;

	// Get the string
	*start = i;
	char c = buffer[i];
	if (c == '`') is_multiline = true;

	for (; ; ++i)
	{
		char c = buffer[i];
		if (c == '\0' || (!is_multiline && is_end(c)) || (is_multiline && i > *start && is_end(c) && buffer[i-1] == '`')) break;
	}
	*end = i;
}

void language_parse_line(Language* language, const char* buffer, size_t* index) {
	size_t i = *index;
	size_t value_start = 0;
	size_t value_end = 0;
	char key[LANGUAGE_LINE_KEY_LENGTH + 1] = "";

	language_parse_line_get_key(key, buffer, &i);
	language_parse_line_get_value_pos(&value_start, &value_end, buffer, &i);
	if (buffer[value_start] == '`') value_start++, value_end--;

	language_add_line_len(language,
		key,
		&buffer[value_start],
		strlen(key),
		value_end - value_start);
}

LanguageLine* language_find_line(Language* language, const char* name)
{
	assert(language != NULL);
	assert(name != NULL);

	LanguageLine* lines_end = &language->lines[language->line_count];

	for (LanguageLine* line = &language->lines[0]; line < lines_end; ++line)
	{
		if (strcmp(language_string_to_cstr(&line->name), name) == 0)
		{
			const char* line_name = language_string_to_cstr(&line->name);
			if ((line_name == NULL || name == NULL) ? line_name == name : strcmp(language_string_to_cstr(&line->name), name) == 0)
				return line;
		}
	}

	return NULL;
}

char* language_get_value(const LanguageLine* line)
{
	return language_string_to_cstr(&line->value);
}

char* _(const char* language_line_name)
{
	LanguageLine* line = language_find_line(&current_language, language_line_name);
	if (line == NULL) return NULL;
	return language_get_value(line);
}
char* _n(const char* language_line_name, const size_t n)
{
	char line_name[25] = "";
	snprintf(line_name, sizeof(line_name), language_line_name, n);
	LanguageLine* line = language_find_line(&current_language, line_name);
	if (line == NULL) return NULL;
	return language_get_value(line);
}
char* _nn(const char* language_line_name, const size_t n, const size_t nn)
{
	char line_name[25] = "";
	snprintf(line_name, sizeof(line_name), language_line_name, n, nn);
	LanguageLine* line = language_find_line(&current_language, line_name);
	if (line == NULL) return NULL;
	return language_get_value(line);
}

void __(char* s, const char* language_line_name, size_t size)
{
	if (size == 0) return;
	const char *buffer = _(language_line_name);
	if (buffer == NULL) return;
	Uint8 len = strlen(buffer);

	if (size == 0)
		return;

	len = MIN(len, size - 1);
	memcpy(s, buffer, len);
	s[len] = '\0';
}

void __n(char* s, const char* language_line_name, size_t size, const size_t n)
{
	if (size == 0) return;
	const char* buffer = _n(language_line_name, n);
	if (buffer == NULL) return;
	Uint8 len = strlen(buffer);


	len = MIN(len, size - 1);
	memcpy(s, buffer, len);
	s[len] = '\0';
}

void __nn(char* s, const char* language_line_name, size_t size, const size_t n, const size_t nn)
{
	if (size == 0) return;
	const char* buffer = _nn(language_line_name, n, nn);
	if (buffer == NULL) return;
	Uint8 len = strlen(buffer);

	len = MIN(len, size - 1);
	memcpy(s, buffer, len);
	s[len] = '\0';
}
