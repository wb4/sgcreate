
#include "control_point.h"

#include <stdio.h>

void control_point_dump(const control_point_t *point) {
  printf("Value: %f;  Link: %f;  X: %f::%f;  Y: %ld::%ld\n", point->x, point->other_x, point->left_x, point->right_x, point->left_y, point->right_y);
}

void control_point_reflect(control_point_t *point, float axis) {
  float tmp_x;
  size_t tmp_y;

  point->x = axis + (axis - point->x);
  point->other_x = axis + (axis - point->other_x);

  point->left_x = 1.0f - point->left_x;
  point->right_x = 1.0f - point->right_x;

  tmp_x = point->left_x;
  point->left_x = point->right_x;
  point->right_x = tmp_x;

  if (point->left_x == 0.0f) point->left_x = 1.0f;
  if (point->right_x == 1.0f) point->right_x = 0.0f;

  point->left_y = -point->left_y;
  point->right_y = -point->right_y;

  tmp_y = point->left_y;
  point->left_y = point->right_y;
  point->right_y = tmp_y;
}
