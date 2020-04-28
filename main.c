#include <stdio.h>
#include "matcher.h"
#include "classifier.h"

// #define TEST_BASIC_PCRE 1

#define PATTERNS_FILE "./patterns.dat"

#define DEFAULT_USER_AGENTS_FILE "./ua_20200424.txt"

#if defined(TEST_BASIC_PCRE)
static void test_pcre(void) {
    const char* tests[] = {
        "Windows NT 11-40.g",
        "Windows 98-deprecated",
    };
    const char* pattern = "(?:(?<os_type>Windows[ ]NT)[ ](?<os_version>[-0-9._a-zA-Z]+))|(?<os_type>Windows[ ]XP|Windows[ ]98|Win98)";
    printf("Regex to use: %s\n", pattern);

    Matcher* matcher = matcher_build();
    do {
        int rc;

        rc = matcher_set_pattern(matcher, pattern);
        if (rc != 0) {
            printf("ERROR: could not set pattern for matcher %p\n", matcher);
            break;
        }

        for (unsigned t = 0; t < sizeof(tests) / sizeof(tests[0]); ++t) {
            const char* test = tests[t];
            printf("\n");
            printf("String: %s\n", test);
            printf("        %s\n", "0123456789012345678901234567890123456789");
            printf("        %s\n", "0         1         2         3");

            rc = matcher_match(matcher, test, 0);
            if (rc != 0) {
                printf("ERROR: could not match string '%s' for matcher %p\n", test, matcher);
            }
        }

    } while(0);

    matcher_destroy(matcher);
}
#endif

static void test_file(const char* file_name) {
    FILE* fp = fopen(file_name, "r");
    if (fp == 0) {
        fprintf(stderr, "Cannot read file [%s]\n", file_name);
        return;
    }

    Classifier* classifier = classifier_build();
    int count = classifier_add_patterns_from_file(classifier, PATTERNS_FILE);
    fprintf(stderr, "Added %d patterns from file [%s]\n", count, PATTERNS_FILE);

    count = 0;
    while (1) {
        char buf[1024];
        if (!fgets(buf, 1024, fp)) {
            break;
        }
        const char* match = classifier_match(classifier, buf, 0);
        printf("%s=> [%s]\n", buf, match ? match : "UNDEF");
        ++count;
    }
    fclose(fp);
    fprintf(stderr, "Matched %d strings from file [%s]\n", count, file_name);

    classifier_destroy(classifier);
}

int main(int argc, char *argv[]) {
#if defined(TEST_BASIC_PCRE)
    (void) argc;
    (void) argv;
    test_pcre();
#endif

    if (argc <= 1) {
        test_file(DEFAULT_USER_AGENTS_FILE);
    } else {
        for (int j = 1; j < argc; ++j) {
            test_file(argv[j]);
        }
    }
    return 0;
}
