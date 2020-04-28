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
    int ca_count; // capture count
    int nc_count; // name count
    int nc_size; // name size
    const char* nc_data; // nametable
    const char** nc_arr; // name array for easier access
    int ov_len; // output vector length
    int* ov_subs; // output vector positions
} Matcher;

Matcher* matcher_build(void);
void matcher_destroy(Matcher* matcher);
int matcher_set_pattern(Matcher* matcher, const char* pattern);
int matcher_match(Matcher* matcher, const char* str, int len);

#endif
