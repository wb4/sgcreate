
#ifndef CONTROL_POINT_H
#define CONTROL_POINT_H

#include <stdlib.h>

typedef struct control_point_tag {
  float x;
  float other_x;

  float left_x;
  ssize_t left_y;

  float right_x;
  ssize_t right_y;
} control_point_t;

void control_point_dump(const control_point_t *point);

void control_point_reflect(control_point_t *point, float axis);

#endif
