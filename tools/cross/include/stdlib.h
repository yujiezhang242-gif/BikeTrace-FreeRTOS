#ifndef TEST_STDLIB_H
#define TEST_STDLIB_H
#include <stddef.h>
double strtod(const char *,char **);
void *malloc(size_t);
void free(void *);
void abort(void);
#endif
