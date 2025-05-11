#include "sqli_filter.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

char *normalize_input(const char *input) {
    size_t len = strlen(input);
    char *buf = malloc(len + 1);
    char *dst = buf;
    const char *src = input;
    while (*src) {
        if (*src == '%'
            && isxdigit((unsigned char)src[1])
            && isxdigit((unsigned char)src[2])) {
            char hex[3] = { src[1], src[2], '\0' };
            *dst++ = (char) strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else if (*src != '\0') {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
    for (char *p = buf; *p; p++)
        *p = (char)tolower((unsigned char)*p);
    return buf;
}

static const char *sqli_patterns[] = {
    "select ", "union ", "insert ", "update ", "delete ",
    "drop ", "truncate ", "replace ", "alter ",
    // Логические
    " or 1=1", " or '1'='1'", " or \"1\"=\"1\"", 
    " and 1=1", 
    // Комменты
    "--", "#", "/*", "*/",
    // Обходы задержек
    "sleep(", "benchmark(", 
    // Чтение/записи файлов
    "load_file(", "into outfile", "into dumpfile",
    // Процедуры и расширения
    "xp_cmdshell", "exec(", "sp_", 
    // Информация о схеме
    "information_schema", "table_schema", 
    // Обфускации через конкатенацию/CHAR
    "concat(", "char(", "0x", 
    // OR/AND с обрамлением
    "' or 'a'='a'", "\" or \"a\"=\"a\"",
    NULL
};

bool detect_sqli(const char *normalized) {
    for (const char **pat = sqli_patterns; *pat; pat++) {
        if (strstr(normalized, *pat))
            return true;
    }
    return false;
}