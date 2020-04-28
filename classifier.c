#define _GNU_SOURCE // memmem

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "classifier.h"

static void classifier_cleanup(Classifier* classifier);
static int scrub(const char* str, int slen, char* buf, int blen);

Classifier* classifier_build(void) {
    Classifier* classifier = (Classifier*) malloc(sizeof(Classifier));
    memset(classifier, 0, sizeof(Classifier));
    return classifier;
}

void classifier_destroy(Classifier* classifier) {
    classifier_cleanup(classifier);
    free((void*) classifier);
}

int classifier_add_pattern(Classifier* classifier, const char* name, const char* pattern) {
    if (classifier->next >= classifier->size) {
        int s = classifier->size > 0 ? 2 * classifier->size : 16;

        Matcher** m = realloc(classifier->matcher, s * sizeof(Matcher*));
        if (!m) {
            // FUCK!
        }
        classifier->matcher = m;

        const char** n = realloc(classifier->name, s * sizeof(char*));
        if (!n) {
            // FUCK!
        }
        classifier->name = n;

        classifier->size = s;
    }

    Matcher* m = matcher_build();
    matcher_set_pattern(m, pattern);
    classifier->matcher[classifier->next] = m;
    classifier->name[classifier->next] = strdup(name);
    return classifier->next++;
}

int classifier_add_patterns_from_file(Classifier* classifier, const char* file_name) {
    char name[1024];
    char pattern[1024*10];
    int ppos = 0;
    int count = 0;
    FILE* fp = fopen(file_name, "r");
    while (1) {
        char buf[1024];
        if (!fgets(buf, 1024, fp)) {
            break;
        }
        ++count;
        int last = 0;
        for (int j = 2; buf[j] != '\0'; ++j) {
            if (!isspace(buf[j])) {
                last = j;
            }
        }
        switch (buf[0]) {
            case 'N':
                if (ppos > 0) {
                    // printf("PATTERN [%s]\n", pattern);
                    classifier_add_pattern(classifier, name, pattern);
                    ppos = 0;
                }
                memcpy(name, buf + 2, last - 1);
                name[last - 1] = '\0';
                // printf("NAME [%s]\n", name);
                break;
            case 'P':
                if (ppos > 0) {
                    // printf("PATTERN [%s]\n", pattern);
                    classifier_add_pattern(classifier, name, pattern);
                    ppos = 0;
                }
                memcpy(pattern + ppos, buf + 2, last - 1);
                pattern[ppos + last - 1] = '\0';
                ppos += last - 1;
                break;
            case 'C':
                memcpy(pattern + ppos, buf + 2, last - 1);
                pattern[ppos + last - 1] = '\0';
                ppos += last - 1;
                break;
        }
    }
    if (ppos > 0) {
        // printf("PATTERN [%s]\n", pattern);
        classifier_add_pattern(classifier, name, pattern);
        ppos = 0;
    }
    fclose(fp);
    return count;
}

const char* classifier_match(Classifier* classifier, const char* str, int len) {
    if (!classifier->matcher || !classifier->name) {
        return 0;
    }
    char tmp[1024];
    scrub(str, len, tmp, 1024);

    for (int j = 0; j < classifier->next; ++j) {
        int rc = matcher_match(classifier->matcher[j], tmp, len);
        if (rc > 0) {
            continue;
        }
        return classifier->name[j];
    }
    return 0;
}

static void classifier_cleanup(Classifier* classifier) {
    if (classifier->matcher) {
        for (int j = 0; j < classifier->next; ++j) {
            matcher_destroy(classifier->matcher[j]);
        }
        free((void*) classifier->matcher);
        classifier->matcher = 0;
    }

    if (classifier->name) {
        for (int j = 0; j < classifier->next; ++j) {
            free((void*) classifier->name[j]);
        }
        free((void*) classifier->name);
        classifier->name = 0;
    }

    classifier->size = classifier->next = 0;
}

static int scrub(const char* str, int slen, char* buf, int blen) {
    if (slen <= 0) {
        slen = strlen(str);
    }
    int len = slen;
    memcpy(buf, str, len);
    buf[len] = '\0';
    // s/\%([[:xdigit:]][[:xdigit:]])/chr(hex($1))/ge;
    // NEXT s/(;)? ([ ])? WUID=$HEX; ([ ])? WTB=$digits(;)?//xg;
    // s/[ ]APCPMS=\^[^\^]+\^;//xg;
    // DONE s/;;+/;/g;
    // DONE s|&#47;|/|g;
    static struct {
        const char* from;
        const char* to;
    } scrub[] = {
        { "&#47;", "/" },
        { ";;", ";" },
    };
    for (unsigned j = 0; j < sizeof(scrub) / sizeof(scrub[0]); ++j) {
        int f = strlen(scrub[j].from);
        int t = strlen(scrub[j].to);
        while (1) {
            char* p = memmem(buf, len, scrub[j].from, f);
            if (!p) {
                break;
            }
            memcpy(p, scrub[j].to, t);
            memcpy(p + t, p + f, len - f);
            len -= f - t;
        }
    }
    return len;
}
