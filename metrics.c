#include "metrics.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const float METERS_PER_CENTIMETER = 0.01;
const float METERS_PER_MILLIMETER = 0.001;
const float METERS_PER_INCH = 0.0254;

length_t length_from_meters(float meters) {
  length_t result = { .meters = meters };
  return result;
}

length_t length_from_centimeters(float centimeters) {
  return length_from_meters(centimeters * METERS_PER_CENTIMETER);
}

length_t length_from_millimeters(float millimeters) {
  return length_from_meters(millimeters * METERS_PER_MILLIMETER);
}

length_t length_from_inches(float inches) {
  return length_from_meters(inches * METERS_PER_INCH);
}

int length_from_string(length_t *dest, const char *string) {
  // Extract the scalar (number).
  char *endptr;
  float scalar = strtof(string, &endptr);
  if (errno == ERANGE || endptr == string) {
    return -1;
  }

  // Skip any whitespace between the scalar and the unit name.
  while (*endptr && isspace(*endptr)) endptr++;
  if (!*endptr) return -1;

  // Identify the units.
  const char *unit = endptr;

  const struct {
    const char *unit;
    length_t (*length_from_scalar)(float scalar);
  } unit_funcs[] = {
    { "meters", length_from_meters },
    { "meter", length_from_meters },
    { "m", length_from_meters },

    { "centimeters", length_from_centimeters },
    { "centimeter", length_from_centimeters },
    { "cm", length_from_centimeters },

    { "millimeters", length_from_millimeters },
    { "millimeter", length_from_millimeters },
    { "mm", length_from_millimeters },

    { "inches", length_from_inches },
    { "inch", length_from_inches },
    { "in", length_from_inches },
    { "\"", length_from_inches }
  };
  const size_t unit_func_count = sizeof(unit_funcs) / sizeof(unit_funcs[0]);

  size_t unit_index;
  for (unit_index = 0;  unit_index < unit_func_count;  unit_index++) {
    if (!strcmp(unit, unit_funcs[unit_index].unit)) {
      *dest = unit_funcs[unit_index].length_from_scalar(scalar);
      return 0;
    }
  }

  return -1;
}

float length_meters(length_t length) {
  return length.meters;
}

float length_centimeters(length_t length) {
  return length.meters / METERS_PER_CENTIMETER;
}

float length_millimeters(length_t length) {
  return length.meters / METERS_PER_MILLIMETER;
}

float length_inches(length_t length) {
  return length.meters / METERS_PER_INCH;
}

static void fmt_unit(length_t length, char *dest, size_t dest_len, const char *units, float (*scalar_func)(length_t)) {
  snprintf(dest, dest_len, "%g%s", scalar_func(length), units);
}

void length_fmt_centimeters(length_t length, char *dest, size_t dest_len) {
  fmt_unit(length, dest, dest_len, "cm", length_centimeters);
}

void length_fmt_millimeters(length_t length, char *dest, size_t dest_len) {
  fmt_unit(length, dest, dest_len, "mm", length_millimeters);
}

length_t length_scale(length_t length, float scale) {
  length_t result = { .meters = scale * length.meters };
  return result;
}

int length_cmp(length_t a, length_t b) {
  if (a.meters < b.meters) {
    return -1;
  } else if (a.meters > b.meters) {
    return 1;
  } else {
    return 0;
  }
}

linear_density_t linear_density(unsigned count, length_t length) {
  linear_density_t result = { .count = count, .length = length };
  return result;
}

float count_per_length(linear_density_t linear_density, length_t length) {
  return length.meters / linear_density.length.meters * linear_density.count;
}

length_t length_for_count(linear_density_t linear_density, float count) {
  return length_from_meters(length_meters(linear_density.length) * count / linear_density.count);
}
