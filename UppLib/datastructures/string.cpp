#include "string.hpp"

#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "../math/scalars.hpp"
#include "../utility/utils.hpp"

String string_create_static_with_size(const char* content, int length)
{
    String result;
    result.size = length;
    result.characters = const_cast<char*>(content);
    result.capacity = length+1;
    return result;
}

String string_create_substring_static(String* string, int start_pos, int end_pos) {
    if (start_pos >= end_pos) {
        return string_create_static("");
    }
    start_pos = math_clamp(start_pos, 0, string->size);
    end_pos = math_clamp(end_pos, 0, string->size);

    String result;
    result.characters = string->characters + start_pos;
    result.size = end_pos - start_pos;
    return result;
}

String string_create_substring(String* string, int start_index, int end_index)
{
    if (start_index > end_index) {
        return string_create_empty(32);
    }
    start_index = math_clamp(start_index, 0, string->size-1);
    end_index = math_clamp(end_index, 0, string->size-1);

    String result;
    result.capacity = end_index - start_index + 2;
    result.characters = new char[result.capacity];
    result.size = end_index - start_index+1;
    memory_copy(result.characters, &string->characters[start_index], result.size);
    result.characters[result.size] = 0;
    return result;
}

bool string_equals(String* s1, String* s2)
{
    if (s1->size != s2->size) { return false; }
    return memcmp(s1->characters, s2->characters, s1->size) == 0;
}

String string_create_static(const char* content)
{
    String result;
    result.characters = const_cast<char*>(content);
    result.size = (int)strlen(content);
    result.capacity = result.size + 1;
    return result;
}

String string_create_empty(int capacity) {
    String result;
    result.characters = new char[capacity];
    result.characters[0] = 0;
    result.size = 0;
    result.capacity = capacity;
    return result;
}

String string_create_from_string_with_extra_capacity(String* other, int extra_capacity) {
    String result;
    result.capacity = other->size + 1 + extra_capacity;
    result.characters = new char[result.capacity];
    strcpy_s(result.characters, result.capacity, other->characters);
    result.size = other->size;
    return result;
}

String string_create(const char* content) {
    String result;
    result.size = (int)strlen(content);
    result.characters = new char[result.size+1];
    result.capacity = result.size + 1;
    strcpy_s(result.characters, result.size+1, content);
    return result;
}

void string_create_from_filepath_to_path_and_filename(String* path, String* filename, const char* filepath)
{
    // 1. Search last / or \
    // Create new strings before and after that
    String buffer = string_create(filepath); // Make editable buffer from const char
    int last_slash = -1;
    for (int i = 0; i < buffer.size; i++) {
        // Replace all \ with /
        if (buffer.characters[i] == '\\') {
            buffer.characters[i] = '/';
            last_slash = i;
        }
    }

    if (last_slash == -1) {
        // No path exists in filepath
        *filename = buffer;
        *path = string_create("./");
    }
    else {
        // Filepath contains a path and a filename, needs to be split
        *path = buffer;
        path->size = last_slash+1;
        *filename = string_create(path->characters + path->size);
        path->characters[path->size] = 0; // Add 0 terminator to path
    }
}

void string_destroy(String* string) {
    if (string->characters != 0) {
        delete[] string->characters;
    }
}

void string_reserve(String* string, int new_capacity) {
    if (string->capacity >= new_capacity) {
        return;
    }
    char* resized_buffer = new char[new_capacity];
    strcpy_s(resized_buffer, new_capacity, string->characters);
    delete[] string->characters;
    string->characters = resized_buffer;
    string->capacity = new_capacity;
}

void string_append(String* string, const char* appendix) {
    int appendix_length = (int)strlen(appendix);
    int required_capacity = string->size + appendix_length + 1;
    if (string->capacity < required_capacity) {
        string_reserve(string, required_capacity);
    }
    strcpy_s(string->characters + string->size, appendix_length+1, appendix);
    string->size += appendix_length;
}

void string_replace_character(String* string, char to_replace, char replace_with)
{
    for (int i = 0; i < string->size; i++) {
        if (string->characters[i] == to_replace) {
            string->characters[i] = replace_with;
        }
    }
}

void string_truncate(String* string, int vector_length) {
    if (vector_length < string->size) {
        string->size = vector_length;
        string->characters[vector_length] = 0;
    }
}

Optional<int> string_find_character_index_reverse(String* string, char character, int startpos)
{
    Optional<int> result;
    if (startpos >= string->size || startpos < 0) {
        result.available = false;
        return result;
    }

    for (int i = startpos; i >= 0; i--) {
        if (string->characters[i] == character) {
            result.available = true;
            result.value = i;
            return result;
        }
    }

    result.available = false;
    return result;
}

bool string_ends_with(const char* string, const char* ending) {
    int ending_length = (int) strlen(ending);
    int string_length = (int) strlen(string);
    if (ending_length > string_length) return false;
    return strcmp(ending, &(string[string_length - ending_length])) == 0;
}

String string_create_formated(const char* format, ...) 
{
    va_list args;
    va_start(args, format);

    // Allocate buffer
    String result;
    result.size = vsnprintf(0, 0, format, args);
    result.capacity = result.size+1;
    result.characters = new char[result.capacity];

    // Fill buffer
    vsnprintf(result.characters, result.capacity, format, args);
    va_end(args);

    return result;
}

void string_append_formated(String* string, const char* format, ...) 
{
    va_list args;
    va_start(args, format);
    int message_length = vsnprintf(0, 0, format, args);
    string_reserve(string, string->size + message_length + 1);
    int ret_val = vsnprintf(string->characters + string->size, string->capacity - string->size, format, args);
    if (ret_val < 0) {
        logg("Shit?\n");
    }
    string->size = string->size + message_length;
    if (string->size != strlen(string->characters)) {
        logg("shit??\n");
    }
    va_end(args);
}

void string_append_character(String* string, char c)
{
    if (string->size + 2 > string->capacity) {
        string_reserve(string, string->capacity * 2);
    }
    string->characters[string->size] = c; // Overwrite 0 terminator
    string->characters[string->size+1] = 0; // Set 0 terminator again
    string->size += 1;
}

void string_remove_substring(String* string, int start_index, int end_index) 
{
    if (string->size == 0 || end_index <= start_index) {
        return;
    }
    start_index = math_clamp(start_index, 0, string->size-1);
    end_index = math_clamp(end_index, 0, string->size);
    int length = end_index - start_index;
    for (int i = start_index; i < string->size - length; i++) {
        string->characters[i] = string->characters[i + length];
    }
    string->size = string->size - length;
    string->characters[string->size] = 0;
}

void string_reset(String* string)
{
    string->size = 0;
    string->characters[0] = 0;
}

void string_append_string(String* string, String* appendix)
{
    string_reserve(string, string->size + appendix->size + 1);
    memory_copy(string->characters + string->size, appendix->characters, appendix->size + 1);
    string->size += appendix->size;
}

void string_remove_character(String* string, int index) 
{
    if (index >= string->size || index < 0) {
        return;
    }
    for (int i = index; i + 1 <= string->size; i++) {
        string->characters[i] = string->characters[i+1];
    }
    string->size -= 1;
}

void string_insert_character_before(String* string, byte character, int index)
{
    // Check if enough space is available
    if (string->size + 2 >= string->capacity) {
        string_reserve(string, string->capacity*2);
    }
    // Move all character after index one forward (Overwriting the 0 terminator)
    for (int i = string->size; i - 1 >= index; i--) {
        string->characters[i] = string->characters[i-1];
    }
    // Insert character
    string->characters[index] = character;
    // Update size
    string->size += 1;
    string->characters[string->size] = 0; // Null-Terminator
}

Optional<int> string_find_character_index(String* string, char c, int start_position) {
    if (start_position >= string->size) {
        return optional_make_failure<int>();
    }
    char* result = strchr(string->characters + start_position, c);
    if (result == nullptr) {
        return optional_make_failure<int>();
    }
    else {
        return optional_make_success<int>(result - string->characters);
    }
}

bool string_compare_substring(String* string, int start_index, String* other) {
    if (string->size - start_index < other->size) {
        return false;
    }
    return strncmp(string->characters + start_index, other->characters, other->size) == 0;
}

void string_insert_string(String* string, String* insertion, int position) 
{
    position = math_clamp(position, 0, string->size);
    int new_size = string->size + insertion->size;
    string_reserve(string, new_size+1);
    string->characters[new_size] = 0;
    // Create a hole in the string where we the insertion can be placed
    for (int i = new_size - 1; i-insertion->size >= position; i--) {
        string->characters[i] = string->characters[i - insertion->size];
    }
    // Fill the hole with the insertion
    memory_copy(string->characters + position, insertion->characters, insertion->size);
    string->size = new_size;
}

void string_prepend_string(String* string, String* prepension) {
    string_insert_string(string, prepension, 0);
}


bool string_contains_substring(String* string, String* substring) {
    return strstr(string->characters, substring->characters) != 0;
}

void string_clear(String* string)
{
    string->size = 0;
    string->characters[0] = 0;
}

void string_set_characters(String* string, const char* characters)
{
    string_clear(string);
    string_append(string, characters);
}

Optional<float> string_parse_float(String* string) {
    char* end_ptr;
    float result = strtof(string->characters, &end_ptr);
    if (string->characters + string->size != end_ptr) {
        return optional_make_failure<float>();
    }
    return optional_make_success(result);
}

Optional<int> string_parse_int(String* string) {
    char* end_ptr;
    int result = strtol(string->characters, &end_ptr, 10);
    if (string->characters + string->size != end_ptr) {
        return optional_make_failure<int>();
    }
    return optional_make_success(result);
}

bool string_contains_character(String string, char character) {
    for (int i = 0; i < string.size; i++) {
        if (string.characters[i] == character) {
            return true;
        }
    }
    return false;
}

void string_append_character_array(String* string, Array<char> appendix) 
{
    int appendix_length = appendix.size;
    int required_capacity = string->size + appendix_length+1;
    string_reserve(string, required_capacity);

    memory_copy(string->characters + string->size, appendix.data, appendix.size);
    string->size += appendix.size;
    string->characters[string->size] = 0;
}

bool string_contains_only_characters_in_set(String* string, String set, bool use_set_complement)
{
    for (int i = 0; i < string->size; i++)
    {
        char c = string->characters[i];
        if (!use_set_complement) {
            if (!string_contains_character(set, c)) return false;
        }
        else {
            if (string_contains_character(set, c)) return false;
        }
    }
    return true;
}

bool string_equals_cstring(String* string, const char* compare) {
    if (strcmp(string->characters, compare) == 0) {
        return true;
    }
    return false;
}

