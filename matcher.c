#include <assert.h>
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

        rc = pcre_fullinfo(matcher->code, matcher->extra, PCRE_INFO_CAPTURECOUNT, &matcher->ca_count);
        if (rc != 0) {
            printf("ERROR: could not get info capture count for '%s': %d\n", pattern, rc);
            break;
        }

        rc = pcre_fullinfo(matcher->code, matcher->extra, PCRE_INFO_NAMECOUNT, &matcher->nc_count);
        if (rc != 0) {
            printf("ERROR: could not get info name count for '%s': %d\n", pattern, rc);
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

        /*
            The ov_len is set as 3 * (capture_count + 1) because
            the structure of the output vector looks like this:

                [pairs of offsets]                    = 2 * (full match + capture)
                [empty "divider" element]             = 1
                [starting positions in reverse order] = capture
                ---------------------------------------------------------
                Total size                            = 3 * (capture + 1)

            For example, for a patern that has 4 captures in total,
            the vector will look like this:

                0,18,0,10,11,18,-1,-1,-1,-1,0,-1,-1,11,0
            
            where:
                *  0,18 is the full match offsets
                *  0,10 is the 1st capture offsets
                * 11,18 is the 2nd capture offsets
                * -1,-1 is the 3rd capture offsets
                * -1,-1 is the 4th capture offsets
                *  0 is the "divider" (this is never touched)
                * -1 is the 4th capture starting offset
                * -1 is the 3rd capture starting offset
                * 11 is the 2nd capture starting offset
                *  0 is the 1st capture starting offset
        */
        matcher->ov_len = 3 * (matcher->ca_count + 1);

        // allocate memory for named array and output vector
        matcher->nc_arr = (const char**) calloc(matcher->ca_count + 1, sizeof(char*));
        matcher->ov_subs = (int*) calloc(matcher->ov_len, sizeof(int));

        int ok = 1;
        for (int j = 0; j < matcher->nc_count; ++j) {
            const char* p = matcher->nc_data + j * matcher->nc_size;
            // offset position by one to fit in the array
            int pos = (p[0] << 8) | p[1];
            if (pos > matcher->ca_count) {
                printf("ERROR: named capture #%d is outside range 0-%d for pattern '%s'\n",
                       pos, matcher->ca_count, pattern);
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

    // (re)set elements of ov_subs to -1
    memset(matcher->ov_subs, -1, matcher->ov_len * sizeof(int));
    int rc = pcre_exec(matcher->code, matcher->extra, str, len, start, options, matcher->ov_subs, matcher->ov_len);
    // TODO: deal with these error codes
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

    // No need to check for rc == 0 and adjust (as recommended in the man page)
    // because we have already made sure that the output vector can fit all matches
    // we are asserting to make sure we were indeed correct on the sizing
    // and within expected bounds of captures (substrings plus full match)
    assert(rc > 0 && rc <= matcher->ca_count + 1);

    // TODO: expose a way to allow the caller to deal with the matches / captures
    // we can use https://www.pcre.org/original/doc/html/pcre_get_named_substring.html
    const char *match;
    for (int j = 0; j < rc; j++) {
        int f = matcher->ov_subs[2*j+0];
        int t = matcher->ov_subs[2*j+1];
        if (f < 0 || t < 0) {
            continue;
        }

        const char* nc_name = j == 0 ? "*FULL*" : matcher->nc_arr[j] ? matcher->nc_arr[j] : "*NONE*";
        pcre_get_substring(str, matcher->ov_subs, rc, j, &match);
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
    if (matcher->ov_subs) {
        free((void*) matcher->ov_subs);
        matcher->ov_subs = 0;
    }
    matcher->ca_count = matcher->nc_count = matcher->nc_size = matcher->ov_len = 0;
    matcher->nc_data = 0;
}
