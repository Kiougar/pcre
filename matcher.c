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

#if 0
    char *names[] = {
        "os_type",
        "os_version",
    };
    int rc = 0;

    int count = 0;
    rc = pcre_fullinfo(code, extra, PCRE_INFO_NAMECOUNT, &count);
    printf("Name count: %d (%d)\n", count, rc);

    int size = 0;
    rc = pcre_fullinfo(code, extra, PCRE_INFO_NAMEENTRYSIZE, &size);
    printf("Entry size: %d (%d)\n", size, rc);

    char* data = 0;
    rc = pcre_fullinfo(code, extra, PCRE_INFO_NAMETABLE, &data);
    printf("Table data: %p (%d)\n", data, rc);

    unsigned p = 0;
    for (int j = 0; j < count; ++j) {
        printf("%p: %02d %02d", data + p, data[p+0], data[p+1]);
        int show = 1;
        for (int k = 2; k < size; ++k) {
            int c = data[p+k];
            if (!show) {
                printf(" .");
            } else if (isprint(c)) {
                printf(" %c", c);
            } else if (c == 0) {
                printf(" |");
                show = 0;
            } else {
                printf(" .");
                show = 0;
            }
        }
        p += size;
        printf("\n");
    }

    for (unsigned t = 0; t < sizeof(names) / sizeof(names[0]); ++t) {
        const char* name = names[t];
        char* first = 0;
        char* last = 0;
        int x = pcre_get_stringtable_entries(code, name, &first, &last);
        printf("%d, %p - %p = [%s]\n", x, first, last, name);
    }
#endif

int matcher_match(Matcher* matcher, const char* str, int len) {
    int start = 0;
    int options = 0;
    // TODO: this can be gotten from the pattern table
    int subs[30];
    int size = sizeof(subs);
    if (len <= 0) {
        len = strlen(str);
    }
    int rc = pcre_exec(matcher->code, matcher->extra, str, len, start, options, subs, size);
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
        rc = size / 3;
        printf("Too many substrings were found to fit, adjusted to %d\n", rc);
    }

    printf("We have %d matches\n", rc);
    const char *match;
    for (int x = 0; x < rc; x++) {
        int f = subs[x*2+0];
        int t = subs[x*2+1];
        if (f < 0 || t < 0) {
            printf("Match(%2d): NONE\n", x);
            continue;
        }
#if 0
        char name[32] = {0};
        unsigned p = 0;
        for (int j = 0; x > 0 && j < count && name[0] == '\0'; ++j) {
            int pos = (data[p+0] << 8) | data[p+1];
            if (pos == x) {
                for (int k = 2; k < size; ++k) {
                    int c = data[p+k];
                    name[k-2] = c;
                    if (c == 0) {
                        break;
                    }
                }
            }
            p += size;
        }
#endif
        pcre_get_substring(str, subs, rc, x, &match);
        printf("Match(%2d): (%2d,%2d): '%s'\n", x, f, t, match);
        pcre_free_substring(match);
    }
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
}
