#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "matcher.h"

static void matcher_cleanup(Matcher* matcher);

Matcher* matcher_build(void) {
    Matcher* matcher = (Matcher*) malloc(sizeof(Matcher));
    memset(matcher, 0, sizeof(Matcher));
    return matcher;
}

int matcher_set_pattern(Matcher* matcher, const char* pattern) {
    matcher_cleanup(matcher);

    do {
        matcher->pattern = strdup(pattern);
        if (!matcher->pattern) {
            printf("ERROR: could not duplicate '%s'\n", pattern);
            break;
        }

        int options = 0;
        const char *error = 0;
        int offset = 0;
        const unsigned char *tableptr = 0;

        options = PCRE_DUPNAMES;
        matcher->code = pcre_compile(pattern, options, &error, &offset, tableptr);
        if (!matcher->code) {
            printf("ERROR: could not compile '%s': %s\n", pattern, error);
            break;
        }

        options = 0;
        matcher->extra = pcre_study(matcher->code, options, &error);
        if (error) {
            printf("ERROR: could not study '%s': %s\n", pattern, error);
            break;
        }

        int rc = 0;

        rc = pcre_fullinfo(matcher->code, matcher->extra, PCRE_INFO_NAMECOUNT, &matcher->nc_count);
        if (rc != 0) {
            printf("ERROR: could not get info count for '%s': %d\n", pattern, rc);
            break;
        }

        rc = pcre_fullinfo(matcher->code, matcher->extra, PCRE_INFO_NAMEENTRYSIZE, &matcher->nc_size);
        if (rc != 0) {
            printf("ERROR: could not get info size for '%s': %d\n", pattern, rc);
            break;
        }

        rc = pcre_fullinfo(matcher->code, matcher->extra, PCRE_INFO_NAMETABLE, &matcher->nc_data);
        if (rc != 0) {
            printf("ERROR: could not get info table for '%s': %d\n", pattern, rc);
            break;
        }

        matcher->nc_len = 99; // TODO: get this value from somewhere else
        matcher->nc_arr = (const char**) calloc(matcher->nc_len, sizeof(char*));
        matcher->nc_subs = (int*) calloc(2 * matcher->nc_len, sizeof(int));

        int ok = 1;
        for (int j = 0; j < matcher->nc_count; ++j) {
            const char* p = matcher->nc_data + j * matcher->nc_size;
            int pos = (p[0] << 8) | p[1];
            if (pos >= matcher->nc_len) {
                printf("ERROR: named capture #%d is outside range 0-%d for pattern '%s'\n",
                       pos, matcher->nc_len, pattern);
                ok = 0;
                break;
            }
            matcher->nc_arr[pos] = &p[2];
        }
        if (!ok) {
            break;
        }

        // ALL GOOD -- return
        return 0;
    } while (0);

    // ERROR -- clean up
    matcher_cleanup(matcher);
    return 1;
}

void matcher_destroy(Matcher* matcher) {
    matcher_cleanup(matcher);
    free((void*) matcher);
}

int matcher_match(Matcher* matcher, const char* str, int len) {
    int start = 0;
    int options = 0;
    if (len <= 0) {
        len = strlen(str);
    }
    memset(matcher->nc_subs, 0, 2 * matcher->nc_len * sizeof(int));
    int rc = pcre_exec(matcher->code, matcher->extra, str, len, start, options, matcher->nc_subs, 2 * matcher->nc_len);
    if (rc < 0) {
        switch (rc) {
            case PCRE_ERROR_NOMATCH      : printf("String did not match pattern\n");   break;
            case PCRE_ERROR_NULL         : printf("Something was null\n");             break;
            case PCRE_ERROR_BADOPTION    : printf("A bad option was passed\n");        break;
            case PCRE_ERROR_BADMAGIC     : printf("Magic number bad (re corrupt?)\n"); break;
            case PCRE_ERROR_UNKNOWN_NODE : printf("Something kooky in compiled re\n"); break;
            case PCRE_ERROR_NOMEMORY     : printf("Ran out of memory\n");              break;
            default                      : printf("Unknown error\n");                  break;
        }
        return 1;
    }
    if (rc == 0) {
        // yes, this is recommended in the man page
        // but it should never happen because we resized subs according to the pattern
        rc = 2 * matcher->nc_len / 3;
        printf("Too many substrings were found to fit, adjusted to %d\n", rc);
    }

    const char *match;
    for (int j = 0; j < rc; j++) {
        int f = matcher->nc_subs[2*j+0];
        int t = matcher->nc_subs[2*j+1];
        if (f < 0 || t < 0) {
            continue;
        }

        const char* nc_name = j == 0 ? "*FULL*" : "*NONE*";
        if (j < matcher->nc_len && matcher->nc_arr[j]) {
            nc_name = matcher->nc_arr[j];
        }
        pcre_get_substring(str, matcher->nc_subs, rc, j, &match);
        printf("Match(%2d): (%2d,%2d): '%s' [%s]\n", j, f, t, match, nc_name);
        pcre_free_substring(match);
    }
    return 0;
}

static void matcher_cleanup(Matcher* matcher) {
    if (matcher->extra) {
        pcre_free_study(matcher->extra);
        matcher->extra = 0;
    }
    if (matcher->code) {
        pcre_free(matcher->code);
        matcher->code = 0;
    }
    if (matcher->pattern) {
        free((void*) matcher->pattern);
        matcher->pattern = 0;
    }
    if (matcher->nc_arr) {
        free((void*) matcher->nc_arr);
        matcher->nc_arr = 0;
    }
    if (matcher->nc_subs) {
        free((void*) matcher->nc_subs);
        matcher->nc_subs = 0;
    }
    matcher->nc_count = matcher->nc_size = matcher->nc_len = 0;
    matcher->nc_data = 0;
}
