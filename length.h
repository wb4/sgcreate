#ifndef LENGTH_H
#define LENGTH_H

#include <stddef.h>

typedef struct {
  float meters;
} length_t;

length_t length_from_meters(float meters);
length_t length_from_centimeters(float centimeters);
length_t length_from_millimeters(float centimeters);
length_t length_from_inches(float inches);

// length_from_string() creates a length_t instance from a string.
//
// String format is: <number> [optional whitespace] <unit>
//
// <unit> can be any of the following:
//  - meters, meter, m
//  - centimeters, centimeter, cm
//  - millimeters, millimeter, mm
//  - inches, inch, in, "
//
//  The following examples are all valid and equivalent:
//  43.8 meters
//  43.8meters
//  43.8m
//  4380cm
//  43800 mm
//  1724.409 inches
//  1724.409"
//
//  Returns 0 on succes, -1 on parse error or unrecognized unit.
int length_from_string(length_t *dest, const char *string);

float length_meters(length_t length);
float length_centimeters(length_t length);
float length_millimeters(length_t length);
float length_inches(length_t length);

void length_fmt_centimeters(length_t length, char *dest, size_t dest_len);
void length_fmt_millimeters(length_t length, char *dest, size_t dest_len);

length_t length_scale(length_t length, float scale);

int length_cmp(length_t a, length_t b);

#endif
