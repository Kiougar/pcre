#define _GNU_SOURCE // memmem

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "classifier.h"

static void classifier_cleanup(Classifier* classifier);
static int scrub(const char* str, int slen, char* buf, int bmax);

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
    len = scrub(str, len, tmp, 1024);

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

static int scrub(const char* str, int slen, char* buf, int bmax) {
    (void) bmax;
    if (slen <= 0) {
        slen = strlen(str);
    }
    // TODO s/\%([[:xdigit:]][[:xdigit:]])/chr(hex($1))/ge;
    // DONE s/(;)? ([ ])? WUID=$HEX; ([ ])? WTB=$digits(;)?//xg;
    // DONE s/[ ]APCPMS=\^[^\^]+\^;//xg;
    // DONE s/;;+/;/g;
    // DONE s|&#47;|/|g;
    int show = 0;
    int len = 0;
    for (int k = 0; k < slen; ) {
        int found = 0;
        switch (str[k]) {
            case '&':
                if (memcmp(str + k, "&#47;", 5) == 0) {
                    memcpy(buf + len, "/", 1);
                    len += 1;
                    k += 5;
                    found = 1;
                }
                break;

            case ';':
                if (memcmp(str + k, ";;", 2) == 0) {
                    // we "fake" it: move one character ahead, don't write
                    // anything to the buffer and hope to catch any real
                    // pattern starting with ';' on the next interation
                    k += 1;
                    found = 1;
                }
                if (memcmp(str + k, "; WUID=", 7) == 0) {
                    int match = 0;
                    int l = 0;
                    for (l = k + 7; l < slen; ++l) {
                        int valid = isxdigit(str[l]);
                        if (!valid) {
                            match = 1;
                            --l; // backtrack one character
                            break;
                        }
                    }
                    if (match) {
                        k = l + 1;
                        found = 1;
                    }
                }
                if (memcmp(str + k, "; WTB=", 6) == 0) {
                    int match = 0;
                    int l = 0;
                    for (l = k + 6; l < slen; ++l) {
                        int valid = isdigit(str[l]);
                        if (!valid) {
                            match = 1;
                            --l; // backtrack one character
                            break;
                        }
                    }
                    if (match) {
                        k = l + 1;
                        found = 1;
                    }
                }
                if (memcmp(str + k, "; APCPMS=^", 10) == 0) {
                    int match = 0;
                    int l = 0;
                    for (l = k + 10; l < slen; ++l) {
                        int valid = str[l] != '^';
                        if (!valid) {
                            match = 1;
                            break;
                        }
                    }
                    if (match) {
                        k = l + 1;
                        found = 1;
                    }
                }
                break;

            default:
                break;
        }
        if (!found) {
            buf[len++] = str[k++];
        }
    }
    buf[len] = '\0';
    if (show) {
        fprintf(stderr, "SCRUBBED\n%s%s", str, buf);
    }
    return len;
}
