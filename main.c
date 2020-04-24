#include <stdio.h>
#include "matcher.h"

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;
    const char* tests[] = {
        "Windows NT 11-40.g",
        "Windows 98-deprecated",
    };
    const char* pattern = "(?:(?<os_type>Windows[ ]NT)[ ](?<os_version>[-0-9._a-zA-Z]+))|(?<os_type>Windows[ ]XP|Windows[ ]98|Win98)(-[a-zA-Z]+)?";
    printf("Regex to use: %s\n", pattern);

    Matcher* matcher = matcher_build();
    matcher_set_pattern(matcher, pattern);

    for (unsigned t = 0; t < sizeof(tests) / sizeof(tests[0]); ++t) {
        const char* test = tests[t];
        printf("\n");
        printf("String: %s\n", test);
        printf("        %s\n", "0123456789012345678901234567890123456789");
        printf("        %s\n", "0         1         2         3");

        matcher_match(matcher, test, 0);
    }

    matcher_destroy(matcher);
    return 0;
}
