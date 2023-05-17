
#ifndef UTIL_H
#define UTIL_H


#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>


#define rand_float(max) rand_in_range(0, max)


float rand_in_range(float min, float max);
float rand_r_in_range(unsigned int *seed, float min, float max);
int rand_in_range_int(int min, int max);
float rand_normal(void);
size_t rand_index(size_t length);
int rand_bool(void);

float cap_float(float value, float min, float max);
float wrap_float(float value, float min, float max);

float jitter(float value, float max_jitter);
float jitter_with_cap(float value, float max_jitter, float min, float max);
float jitter_with_wrap(float value, float max_jitter, float min, float max);

#define PERROR(message) fprintf(stderr, "%s(), %s, line %u: %s: %s\n", __FUNCTION__, __FILE__, __LINE__, message, strerror(errno))


#endif
