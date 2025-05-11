#ifndef SQLI_FILTER_H
#define SQLI_FILTER_H

#include <stdbool.h>

char *normalize_input(const char *input);

bool detect_sqli(const char *normalized);

#endif