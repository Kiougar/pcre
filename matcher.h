#ifndef MATCHER_H_
#define MATCHER_H_

/*
 * An object that knows how to match a string agains a regular expression.
 * Uses PCRE for the heavy lifting.
 * Handles named captures properly.
 *
 * TODO: create return codes that abstract PCRE
 */

#include <pcre.h>

typedef struct Matcher {
    const char* pattern;
    pcre* code;
    pcre_extra* extra;
    int nc_count;
    int nc_size;
    const char* nc_data;
    int nc_len;
    const char** nc_arr;
    int* nc_subs;
} Matcher;

Matcher* matcher_build(void);
void matcher_destroy(Matcher* matcher);
int matcher_set_pattern(Matcher* matcher, const char* pattern);
int matcher_match(Matcher* matcher, const char* str, int len);

#endif
