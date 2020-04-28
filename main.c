#include <stdio.h>
#include "matcher.h"
#include "classifier.h"

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

static void test_file(void) {
    Classifier* classifier = classifier_build();
    classifier_add_patterns_from_file(classifier, "./patterns.dat");
    // FILE* fp = fopen("./ua_small.txt", "r");
    FILE* fp = fopen("./ua_20200424.txt", "r");
    while (1) {
        char buf[1024];
        if (!fgets(buf, 1024, fp)) {
            break;
        }
#if 1
        const char* match = classifier_match(classifier, buf, 0);
        printf("%s=> [%s]\n", buf, match ? match : "UNDEF");
#endif
    }
    fclose(fp);
    classifier_destroy(classifier);
}

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    // test_pcre();
    test_file();
    return 0;
}
