#ifndef CLASSIFIER_H_
#define CLASSIFIER_H_

/*
 * An object that keeps a list of pairs name=>pattern, and applies them in
 * order to classify a string.
 * Handles named captures properly.
 *
 * TODO: add proper support for named captures
 */

#include "matcher.h"

typedef struct Classifier {
    int size;
    int next;
    Matcher** matcher;
    const char** name;
} Classifier;

Classifier* classifier_build(void);
void classifier_destroy(Classifier* classifier);
int classifier_add_pattern(Classifier* classifier, const char* name, const char* pattern);
int classifier_add_patterns_from_file(Classifier* classifier, const char* file_name);
const char* classifier_match(Classifier* classifier, const char* str, int len);

#endif
