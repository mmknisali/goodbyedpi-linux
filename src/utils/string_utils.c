#include "../include/goodbyedpi.h"
#include "../include/logging.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Convert string to lowercase
void string_to_lower(char *str)
{
    if (!str) return;
    
    for (size_t i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

// Convert string to uppercase
void string_to_upper(char *str)
{
    if (!str) return;
    
    for (size_t i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

// Mix case of string (alternating case for evasion)
void mix_case(char *str, size_t len)
{
    if (!str || len == 0) return;
    
    for (size_t i = 0; i < len && str[i]; i++) {
        if (i % 2 == 0) {
            str[i] = tolower((unsigned char)str[i]);
        } else {
            str[i] = toupper((unsigned char)str[i]);
        }
    }
}

// Random mix case (random character case changes)
void mix_case_random(char *str, size_t len)
{
    if (!str || len == 0) return;
    
    for (size_t i = 0; i < len && str[i]; i++) {
        if (rand() % 2) {
            str[i] = toupper((unsigned char)str[i]);
        } else {
            str[i] = tolower((unsigned char)str[i]);
        }
    }
}

// Check if string contains only whitespace
bool is_whitespace(const char *str)
{
    if (!str) return true;
    
    while (*str) {
        if (!isspace((unsigned char)*str)) {
            return false;
        }
        str++;
    }
    return true;
}

// Trim whitespace from beginning of string
char *trim_left(char *str)
{
    if (!str) return NULL;
    
    while (isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

// Trim whitespace from end of string
char *trim_right(char *str)
{
    if (!str) return NULL;
    
    char *end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return str;
}

// Trim whitespace from both ends
char *trim(char *str)
{
    if (!str) return NULL;
    return trim_left(trim_right(str));
}

// Safe string copy with null termination
void safe_string_copy(char *dest, const char *src, size_t dest_size)
{
    if (!dest || !src || dest_size == 0) return;
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

// Check if string ends with suffix
bool ends_with(const char *str, const char *suffix)
{
    if (!str || !suffix) return false;
    
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > str_len) return false;
    
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

// Check if string starts with prefix
bool starts_with(const char *str, const char *prefix)
{
    if (!str || !prefix) return false;
    
    size_t prefix_len = strlen(prefix);
    if (prefix_len > strlen(str)) return false;
    
    return strncmp(str, prefix, prefix_len) == 0;
}

// Find substring in string (case insensitive)
char *stristr(const char *haystack, const char *needle)
{
    if (!haystack || !needle) return NULL;
    
    if (*needle == '\0') return (char *)haystack;
    
    for (; *haystack; haystack++) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            const char *h = haystack;
            const char *n = needle;
            
            while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
                h++;
                n++;
            }
            
            if (*n == '\0') {
                return (char *)haystack;
            }
        }
    }
    
    return NULL;
}

// Replace all occurrences of substring
int string_replace_all(char *str, size_t str_size, const char *find, const char *replace)
{
    if (!str || !find || !replace || str_size == 0) return -1;
    
    size_t find_len = strlen(find);
    size_t replace_len = strlen(replace);
    int count = 0;
    
    if (find_len == 0) return 0;
    
    char *pos = strstr(str, find);
    while (pos && (pos + replace_len < str + str_size)) {
        // Make room for replacement
        if (replace_len != find_len) {
            size_t remaining = strlen(pos) - find_len + 1;
            if (strlen(str) + replace_len - find_len >= str_size) {
                break; // Not enough space
            }
            memmove(pos + replace_len, pos + find_len, remaining);
        }
        
        // Copy replacement
        memcpy(pos, replace, replace_len);
        count++;
        
        // Find next occurrence
        pos = strstr(pos + replace_len, find);
    }
    
    return count;
}

// Count occurrences of substring
int string_count_occurrences(const char *str, const char *find)
{
    if (!str || !find) return 0;
    
    int count = 0;
    size_t find_len = strlen(find);
    
    if (find_len == 0) return 0;
    
    const char *pos = str;
    while ((pos = strstr(pos, find)) != NULL) {
        count++;
        pos += find_len;
    }
    
    return count;
}