
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "list.h"
#include "image.h"
#include "heightmap.h"
#include "util.h"


/* My eye separation is about 280 pixels for my display.
   Let's use these values for our default settings.
   A good maximum stereo separation in the image is half the eye separation.
   That puts the image at the same distance as the viewer's reflection.
   280 / 2 is 140

   We don't want the minimum separation to be half of the max.  That could cause
   misconvergence for the viewer.  Let's try 0.65
*/

#define EYE_SEPARATION_DEFAULT (280.0f)

#define MIN_MAX_SEPARATION_RATIO (0.65f)

#define SEPARATION_MAX_DEFAULT (0.7f * EYE_SEPARATION_DEFAULT)
#define SEPARATION_MIN_DEFAULT ((float) (MIN_MAX_SEPARATION_RATIO * SEPARATION_MAX_DEFAULT))

#define EDGE_ECHO_OFFSET_RATIO (0.1f)  /* This value times max separation = how many rows down to go in the texture image to prevent echo */


int ascii_to_ssize_t(const char *ascii, ssize_t *result) {
  char *end;
  long temp;

  temp = strtol(ascii, &end, 10);

  if (*end) {
    return -1;
  }

  if (errno == ERANGE) {
    return -1;
  }

  *result = (ssize_t) temp;
  if (*result != temp) {
    errno = ERANGE;
    return -1;
  }

  return 0;
}


int ascii_to_size_t(const char *ascii, size_t *result) {
  char *end;
  unsigned long temp;

  temp = strtoul(ascii, &end, 10);

  if (*end) {
    return -1;
  }

  if (errno == ERANGE) {
    return -1;
  }

  *result = (size_t) temp;
  if (*result != temp) {
    errno = ERANGE;
    return -1;
  }

  return 0;
}


int ascii_to_float(const char *ascii, float *result) {
  char *end;
  float temp;

  errno = 0;

  temp = strtof(ascii, &end);

  if (*end) {
    return -1;
  }

  if ((temp == HUGE_VALF || temp == -HUGE_VALF || temp == 0.0f) && errno == ERANGE) {
    return -1;
  }

  *result = temp;

  return 0;
}


image_t *create_texture(size_t width, size_t height, pattern_t type) {
  image_t *texture;

  if ((texture = image_create_random(width, height, type)) == NULL) {
    perror("image_create_random()");
  }

  return texture;
}


image_t *get_texture(const char *filename, size_t width, size_t height, pattern_t type) {
  image_t *image;

  if (filename == NULL) {
    image = create_texture(width, height, type);
  } else {
    if ((image = image_read(filename)) == NULL) {
      perror("image_read()");
    }
  }

  return image;
}


/* Scale the texture vertically such that, if we were to scale it horizontally to the
   specified width, the aspect ratio would be preserved. */
int scale_texture_height(image_t *texture, float width) {
  size_t height;

  height = (size_t) roundf((width / image_get_width(texture)) * image_get_height(texture));

  return image_scale(texture, image_get_width(texture), height);
}


int get_separation(float *separation, const heightmap_t *heightmap, size_t row, float x, float eye_sep, float sep_min, float sep_max) {
  float c;
  float d;

  c = heightmap_get(heightmap, x, row);

  d = (c * sep_min) / (eye_sep - sep_min) + ((1.0f-c) * sep_max) / (eye_sep - sep_max);

  *separation = (d * eye_sep) / (d + 1.0f);

  return 0;
}


/* Returns a normalized value representing the fraction into a tiled texture row of the given width the x value would be. */
float x_to_texture(float x, float width) {
  return fmodf(x, width) / width;
}


int insert_wraparound_control_point(list_t *points, control_point_t *point) {
  control_point_t other_point;

  other_point.x = points->last->point.x + (point->x - points->last->point.x) * (1.0f - points->last->point.right_x) / (1.0f + point->left_x - points->last->point.right_x);
  other_point.other_x = -1.0f;
  other_point.left_x = 1.0f;
  other_point.left_y = point->left_y;
  other_point.right_x = 0.0f;
  other_point.right_y = point->left_y;

  if (list_add(points, &other_point, points->last) == -1) {
    perror("list_add()");
    return -1;
  }

  return 0;
}


ssize_t find_inserted_texture_shift(float width, float x) {
  ssize_t column;
  ssize_t shift;

  column = (ssize_t) (x / width);

  shift = (column / 2) + 1;

  if (column % 2) {
    shift = -shift;
  }

  return shift;
}


int generate_left_eye_cannot_see_control_points(list_t *points, float sep_max, float center, int *last_invalid, control_point_t *point) {
  /* This point links to a place to the left of where a previous point linked to.
     This means that the left eye can't see what the right eye sees here.
     So, the range to the left of this point needs to map to some other row in
     the texture image to prevent edge echo.

     * @     .     *
     *  @     .     *
     *   @     .     *
     *    @     .     *
     *  @        .        *   <- current point

               |
               V

     * @     .     *
     *  @     .     *
     *   @     .     *
     *    @     .     *#
     *           .        #   <- current point

   */

  points->last->point.right_x = x_to_texture(points->last->point.x, sep_max);
  points->last->point.right_y = points->last->point.left_y;

  if (!*last_invalid) {
    points->last->point.right_y = find_inserted_texture_shift(sep_max, point->x - center);
  }

  point->left_x = x_to_texture(point->x, sep_max);
  point->left_y = points->last->point.right_y;

  /* If this control point just happens to map to the edge of the texture, then the left
     side of it should map to 1, not 0, and the right side should be 0, not 1. */
  if (point->left_x == 0.0f) point->left_x = 1.0f;
  if (point->right_x == 1.0f) point->right_x = 0.0f;

  if (points->last->point.right_x >= point->left_x) {
    /* The range to the left wraps around the edge of the texture image, so we need to
       insert another control point to represent the edge. */
    if (insert_wraparound_control_point(points, point) == -1) {
      return -1;
    }
  }

  *last_invalid = 1;

  return 0;
}


int generate_right_eye_cannot_see_control_points(list_t *points, float *greatest_other_x, node_t **start, int *last_invalid, control_point_t *point) {
  /* This point passes through the screen to the left of where at least one previous point
     passed through.  This means that the right eye can't see what the left eye sees for
     those other points.  So, we need to remove those points to the right of this one
     and adjust the left range of this point.

     * !       .       *
     *  @       .       *
     *   @       .       *
     *    @       .       *
     *        @    .    @   <- current point

                |
                V

     * !       .       *
     *          .        
     *           .        
     *            .        
     *        @    .   !@    <- current point

   */
  control_point_t other_point;

  other_point.x = other_point.left_x = 0;  /* shut up the compiler warnings */

  while (point->x <= points->last->point.x) {
    other_point = points->last->point;
    if (*start == points->last) {
      *start = (*start)->prev;
    }
    list_remove_last(points);
  }

  /* Interpolate the texture position based on this point's position between the point immediately to it's left and the point that was removed immediately to its right. */
  point->left_x = points->last->point.right_x + (other_point.left_x - points->last->point.right_x) * (point->x - points->last->point.x) / (other_point.x - points->last->point.x);
  point->left_y = points->last->point.right_y;

  *greatest_other_x = point->other_x;

  *last_invalid = 0;

  return 0;
}


int generate_both_eyes_can_see_control_points(list_t *points, float *greatest_other_x, float sep_max, node_t **start, int *last_invalid, control_point_t *point) {
  /* This control point is well-behaved.  It falls to the right of all previous control points,
     and the point it links to also is to the right of all previous links.

     * @     .     *
     *  @     .     *
     *   @     .     *
     *    @     .     *
     *     @     .     *   <- current point

   */
  float bound_x;
  node_t *end;

  if (*last_invalid) {
    bound_x = point->other_x;
  } else {
    bound_x = points->last->point.other_x;
  }

  list_find_range(points, start, &end, bound_x, point->other_x, *start);

  /* Sanity check.  The range should never be null. */
  if (*start == NULL) {
    fputs("*start is NULL after list_find_range()\n", stderr);
    fprintf(stderr, "bound_x is %f, point->other_x is %f\n", bound_x, point->other_x);
    exit(1);
  }
  if (end == NULL) {
    fputs("end is NULL after list_find_range()\n", stderr);
    fprintf(stderr, "bound_x is %f, point->other_x is %f\n", bound_x, point->other_x);
    exit(1);
  }

  if (*start == end) {
    /* The previous point was invalid, AND this point links to a point that exactly coincides
       with a previous control point. */
    point->left_x = (*start)->point.left_x;
    point->left_y = (*start)->point.left_y;

    point->right_x = (*start)->point.right_x;
    point->right_y = (*start)->point.right_y;
  } else {
    /* Calculate where in the texture image this point maps to, by linearly interpolating between the two control points
       that the point it links to falls between. */
    point->right_x = end->prev->point.right_x + (point->other_x - end->prev->point.x) * (end->point.left_x - end->prev->point.right_x) / (end->point.x - end->prev->point.x);
    point->right_y = end->prev->point.right_y;

    point->left_x = point->right_x;
    point->left_y = point->right_y;

    /* If the place this point links to falls exactly on another control point, then we need
       to be careful about where the right side of this point links to.  For example, that other
       control point could represent where the texture image wraps around, or the left and
       right sides could map to two different rows in the texture image. */
    if (point->other_x == end->point.x) {
      point->right_x = end->point.right_x;
      point->right_y = end->point.right_y;
    }
  }

  if (*last_invalid) {
    /* The previous point didn't link to anything.  Therefore, the left range should map
       to some other part of the texture image.
     */
    points->last->point.right_x = x_to_texture(points->last->point.x, sep_max);  /* Insert some texture. */
    points->last->point.right_y = points->last->point.left_y;  /* The range left from that point was inserted, so use the same row here. */

    point->left_x = x_to_texture(point->x, sep_max);  /* Match inserted left texture with the previous point's right texture. */
    point->left_y = points->last->point.right_y;
  } else {
    if (*start != end) {
      /* Copy any enclosed control points. */
      node_t *node;

      for (node = (*start)->next;  node != end;  node = node->next) {
        control_point_t other_point;

        other_point = node->point;
        other_point.other_x = other_point.x;
        other_point.x = points->last->point.x + (other_point.other_x - points->last->point.other_x) * (point->x - points->last->point.x) / (point->other_x - points->last->point.other_x);
        /* Don't copy it if the copy would exactly coincide with this control point. */
        if (other_point.x != point->x) {
          if (list_add(points, &other_point, points->last) == -1) {
            perror("list_add()");
            return -1;
          }
        }
      }
    }
  }

  *greatest_other_x = point->other_x;

  *last_invalid = 0;

  return 0;
}


int generate_h_place_control_points(list_t *points, size_t row, heightmap_t *heightmap, float eye_separation, float separation_min, float separation_max, float h_place, float *greatest_other_x, node_t **start, int *last_invalid) {
  control_point_t point;
  float sep;
  float half_sep;
  float center;

  if (get_separation(&sep, heightmap, row, h_place, eye_separation, separation_min, separation_max) == -1) {
    return -1;
  }

  half_sep = 0.5f * sep;

  center = 0.5f * heightmap_get_width(heightmap);

  point.x = h_place + half_sep;
  point.other_x = h_place - half_sep;

  point.left_x = -1;
  point.right_x = 0;

  point.left_y = 0;
  point.right_y = 0;

  if (point.other_x <= *greatest_other_x) {
    /*
     * @     .     *
     *  @     .     *
     *   @     .     *
     *    @     .     *
     *  @        .        *   <- current point
     */
    if (generate_left_eye_cannot_see_control_points(points, separation_max, center, last_invalid, &point) == -1) {
      return -1;
    }
  } else if (point.x <= points->last->point.x) {
    /*
     * @       .       *
     *  @       .       *
     *   @       .       *
     *    @       .       *
     *        @    .    *   <- current point
     */
    if (generate_right_eye_cannot_see_control_points(points, greatest_other_x, start, last_invalid, &point) == -1) {
      return -1;
    }
  } else {
    /*
     * @     .     *
     *  @     .     *
     *   @     .     *
     *    @     .     *
     *     @     .     *   <- current point
     */
    if (generate_both_eyes_can_see_control_points(points, greatest_other_x, separation_max, start, last_invalid, &point) == -1) {
      return -1;
    }
  }

  /* Finally, add this control point to the list. */
  if (list_add(points, &point, points->last) == -1) {
    perror("list_add()");
    return -1;
  }

  return 0;
}


int generate_right_half_control_points(list_t *points, size_t row, heightmap_t *heightmap, float eye_separation, float separation_min, float separation_max) {
  float width;  /* width of the heightmap */
  float h_place;  /* current horizontal position in this row of the heightmap */
  float greatest_other_x;  /* right-most left x-position that has been linked to.  This is used to determine eye visibility. */

  node_t *start;  /* We keep a 'bookmark' into the linked list of points to speed up insertions. */

  int last_invalid;  /* Whether the previous control point didn't link with anything. */

  width = (float) heightmap_get_width(heightmap);

  /* Go from the center to the right side of the screen. */
  last_invalid = 0;
  start = points->last->prev;
  greatest_other_x = points->last->point.other_x;

  /* We initialize h_place one pixel to the right of the midpoint because the calling code has already generated
     the initial two control points from the midpoint. */
  for (h_place = 0.5f * width + 1.0f;  h_place < width;  h_place += 1.0f) {
    if (generate_h_place_control_points(points, row, heightmap, eye_separation, separation_min, separation_max, h_place, &greatest_other_x, &start, &last_invalid) == -1) {
      return -1;
    }
  }

  return 0;
}


int generate_middle_control_points(list_t *points, size_t row, heightmap_t *heightmap, float eye_separation, float separation_min, float separation_max, float width) {
  float h_place;

  control_point_t point;

  float sep;
  float half_sep;


  /* Make the initial two control points in the middle. */
  h_place = 0.5f * width;
  if (get_separation(&sep, heightmap, row, h_place, eye_separation, separation_min, separation_max) == -1) {
    return -1;
  }

  half_sep = 0.5f * sep;

  point.x = h_place - half_sep;
  point.other_x = h_place + half_sep;
  point.left_x = 1.0f;
  point.left_y = 0;
  point.right_x = 0.0f;
  point.right_y = 0;
  if (list_add(points, &point, NULL) == -1) {
    perror("list_add()");
    return -1;
  }

  point.other_x = point.x;
  point.x = h_place + half_sep;
  if (list_add(points, &point, NULL) == -1) {
    perror("list_add()");
    return -1;
  }

  return 0;
}


int generate_control_points(list_t *points, size_t row, heightmap_t *heightmap, float eye_separation, float separation_min, float separation_max) {
  float width;


  width = (float) heightmap_get_width(heightmap);

  /* We're doing the left side first, so reflect the heightmap. */
  heightmap_set_reflected(heightmap, 1);

  /* Make the initial two control points. */
  if (generate_middle_control_points(points, row, heightmap, eye_separation, separation_min, separation_max, width) == -1) {
    return -1;
  }

  /* Go from the center to the left side of the screen. */
  if (generate_right_half_control_points(points, row, heightmap, eye_separation, separation_min, separation_max) == -1) {
    return -1;
  }

  /* Unreflect the heightmap, and reflect the list. */
  heightmap_set_reflected(heightmap, 0);
  list_reflect(points, 0.5f * width);

  /* Go from the center to the right side of the screen. */
  if (generate_right_half_control_points(points, row, heightmap, eye_separation, separation_min, separation_max) == -1) {
    return -1;
  }

  return 0;
}


int add_color_for_range(image_t *texture, float left, float right, size_t row, float scale, float *accum) {
  float r;
  float g;
  float b;
  float a;

  float tmp_right;

  float width;

  float length;

  float pixel[4];

  if (scale <= 0.0f || scale > 1.0f) {
    fprintf(stderr, "Warning: add_color_for_range(): scale is %f\n", scale);
  }

  r = g = b = a = 0.0f;

  /* Sanity check. */
  if (left < 0.0f || left > 1.0f) {
    fprintf(stderr, "left (%f) is outside the range (0..1]\n", left);
    exit(1);
  }
  if (right < 0.0f || right > 1.0f) {
    fprintf(stderr, "right (%f) is outside the range (0..1]\n", right);
    exit(1);
  }

  /* Map the left..right range from 0..1 to 0..<texture width> */
  width = (float) image_get_width(texture);
  left *= width;
  right *= width;

  length = right - left;

  while (right - floorf(left) > 1.0f) {
    /* We stradle the border between pixels. */
    tmp_right = floorf(left) + 1.0f;

    image_get_pixel(texture, pixel, (size_t) floorf(left), row);

    r += pixel[0] * (tmp_right - left);
    g += pixel[1] * (tmp_right - left);
    b += pixel[2] * (tmp_right - left);
    a += pixel[3] * (tmp_right - left);

    left = tmp_right;
  }

  /* Now we're fully contained within a single pixel. */
  image_get_pixel(texture, pixel, (size_t) floorf(left), row);

  r += pixel[0] * (right - left);
  g += pixel[1] * (right - left);
  b += pixel[2] * (right - left);
  a += pixel[3] * (right - left);

  scale /= length;

  accum[0] += r * scale;
  accum[1] += g * scale;
  accum[2] += b * scale;
  accum[3] += a * scale;

  return 0;
}


int color_row(image_t *sg, size_t row, image_t *texture, list_t *points, ssize_t edge_echo_offset) {
  node_t *node;

  float width;

  float left;
  float right;

  float tmp_right;

  float left_x;
  float right_x;

  ssize_t left_y;

  float tmp_right_x;

  float accum[4];

  ssize_t texture_height;

  size_t texture_row;
  ssize_t texture_row_used;

  ssize_t i;

  width = (float) image_get_width(sg);

  texture_row = row % image_get_height(texture);

  accum[0] = 0.0f;
  accum[1] = 0.0f;
  accum[2] = 0.0f;
  accum[3] = 0.0f;

  for (node = points->first;  node->next;  node = node->next) {
    left = node->point.x;
    right = node->next->point.x;

    left_x = node->point.right_x;
    left_y = node->point.right_y;

    right_x = node->next->point.left_x;

    /* We can't do anything if we haven't yet gotten to the left edge of the image. */
    if (right <= 0.0f) {
      continue;
    }

    /* We can stop if we've moved past the right edge of the image. */
    if (left >= width) {
      break;
    }

    texture_height = (ssize_t) image_get_height(texture);

    texture_row_used = texture_row;
    for (i = 0;  i < left_y;  ++i) {
      texture_row_used = (texture_row_used + edge_echo_offset) % texture_height;
      if (texture_row_used == texture_row) {
        --i;
      }
    }
    for (i = 0;  i > left_y;  --i) {
      texture_row_used = (texture_row_used - edge_echo_offset);
      while (texture_row_used < 0) {
        texture_row_used += texture_height;
      }
      if (texture_row_used == texture_row) {
        ++i;
      }
    }

    while (right - floorf(left) > 1.0f) {
      /* We cover more than one output image pixel. */
      tmp_right = floorf(left) + 1.0f;
      tmp_right_x = left_x + (right_x - left_x) * (tmp_right - left) / (right - left);

      if (left >= 0.0f) {
        /* We're clear of the left edge of the screen, so we're actually in a pixel. */
        if (add_color_for_range(texture, left_x, tmp_right_x, texture_row_used, tmp_right - left, accum) == -1) {
          return -1;
        }

        /* We just finished up the color for a pixel, so apply that color to the final image. */
        image_set_pixel(sg, accum, (size_t) left, row);

        /* Reset our accumulation buffer. */
        accum[0] = 0.0f;
        accum[1] = 0.0f;
        accum[2] = 0.0f;
        accum[3] = 0.0f;
      }

      left = tmp_right;
      left_x = tmp_right_x;
    }

    if (left != right) {
      /* At this point, we're fully contained inside a single pixel.  Start filling the color
         accumulation buffer for that pixel. */
      if (add_color_for_range(texture, left_x, right_x, texture_row_used, right - left, accum) == -1) {
        return -1;
      }

      /* We need to check for the special case where the right side of the range exactly coincides
         with an output image pixel boundary.  In that case, we need to write the pixel to the
         image. */
      if (floorf(right) == right) {
        /* We just finished up the color for a pixel, so apply that color to the final image. */
        image_set_pixel(sg, accum, (size_t) left, row);

        /* Reset our accumulation buffer. */
        accum[0] = 0.0f;
        accum[1] = 0.0f;
        accum[2] = 0.0f;
        accum[3] = 0.0f;
      }
    }
  }

  return 0;
}


int generate_row(image_t *sg, size_t row, heightmap_t *heightmap, image_t *texture, float eye_separation, float separation_min, float separation_max, ssize_t edge_echo_offset) {
  list_t points;

  if (list_init(&points) == -1) {
    perror("list_init()");
    return -1;
  }

  if (generate_control_points(&points, row, heightmap, eye_separation, separation_min, separation_max) == -1) {
    list_destroy(&points);
    return -1;
  }

  /* All right.  Now that we have all the control points for this row,
     it's time to color the pixels. */
  if (color_row(sg, row, texture, &points, edge_echo_offset) == -1) {
    list_destroy(&points);
    return -1;
  }

  list_destroy(&points);

  return 0;
}


image_t *create_stereogram(heightmap_t *heightmap, image_t *texture, float eye_separation, float separation_min, float separation_max, ssize_t edge_echo_offset) {
  image_t *sg;

  unsigned long width;
  unsigned long height;

  size_t i;

  width  = heightmap_get_width(heightmap);
  height = heightmap_get_height(heightmap);

  if ((sg = image_create(width, height)) == NULL) {
    perror("image_create() failed");
    return NULL;
  }

  for (i = 0;  i < height;  ++i) {
    if (generate_row(sg, i, heightmap, texture, eye_separation, separation_min, separation_max, edge_echo_offset) == -1 ) {
      image_destroy(sg);
      return NULL;
    }
  }

  return sg;
}


int main(int argc, char **argv) {
  heightmap_t *heightmap;
  image_t *texture;
  image_t *output;

  float eye_separation = EYE_SEPARATION_DEFAULT;

  float separation_max = SEPARATION_MAX_DEFAULT;
  float separation_min = SEPARATION_MIN_DEFAULT;

  ssize_t edge_echo_offset;

  int separation_min_specified = 0;
  int edge_echo_offset_specified = 0;

  int preserve_height = 0;

  int add_noise = 0;

  pattern_t pattern_type = PATTERN_TYPE_RANDOM;

  const char *heightmap_file = NULL;
  const char *output_file = NULL;

  const char *texture_file = NULL;

  long unsigned height;

  char *usagefmt = "\nUsage: %s [options] -i <depthmap_image> -o <output_image>\n"
                   "\n"
                   "Depthmap is an image that is one of two types:\n"
                   " * Grayscale.  Brighter pixels represent shallower depth.\n"
                   " * Rainbow.  Redder hues represent shallower depth.  This gives more depth\n"
                   "   resolution than grayscale.\n"
                   "\n"
                   "Options:\n"
                   "\n"
                   "  -f  maximum separation in pixels.  Default %.2f\n"
                   "      If -f is specified but -n (see below) is not, then -n defaults\n"
                   "      to %.2f * -f\n"
                   "  -n  minimum separation in pixels.  Default %.2f, or %.2f * -f\n"
                   "      if -f is specified and -n is not.\n"
                   "  -t  optional texture image file to use.  If this option is not provided,\n"
                   "      then a random texture will be generated.\n"
                   "  -p  preserve the height of the texture.  Use this option if, for example,\n"
                   "      your texture image is exactly the height of the stereogram and you want\n"
                   "      to keep it that way in the final image.\n"
                   "  -N  add noise to the texture image.\n"
                   "  -P  type of texture pattern to generate when the -t option is omitted.\n"
                   "      Valid values are 'perlin', 'polygons', 'ellipses', and 'dots'.  If omitted,\n"
		   "      then a random pattern type will be selected for you.\n"
                   "  -e  distance between the viewer's eyes, in pixels.  You probably want to\n"
                   "      leave this alone.  You might wish to change it if you're generating a \n"
                   "      hi-res stereogram to print as a poster.  Default %.2f\n"
                   "  -r  for inserted texture at the edges of foreground objects, how many rows\n"
                   "      to offset in the texture to prevent artifacts.  You probably don't need\n"
                   "      to mess with this, ever.  By default it is calculated based on the max\n"
                   "      separation parameter.\n"
                   "  -h  print this usage text and exit.\n";

  char usage[2048];

  int o;

  snprintf(usage, sizeof(usage), usagefmt, argv[0], SEPARATION_MAX_DEFAULT, MIN_MAX_SEPARATION_RATIO, SEPARATION_MIN_DEFAULT, MIN_MAX_SEPARATION_RATIO, EYE_SEPARATION_DEFAULT);
  usage[sizeof(usage)-1] = '\0';  /* just in case */

  while ((o = getopt(argc, argv, "i:o:f:n:t:pNP:e:r:h")) != -1) {
    switch (o) {
      case 'i':
        heightmap_file = optarg; break;
      case 'o':
        output_file = optarg; break;
      case 'f':
        if (ascii_to_float(optarg, &separation_max) == -1) {
          fputs(usage, stderr);
          fputs("\n-f requires a positive argument greater than 1.0\n\n", stderr);
          return 1;
        }
        break;
      case 'n':
        if (ascii_to_float(optarg, &separation_min) == -1) {
          fputs(usage, stderr);
          fprintf(stderr, "\n-n requires a positive argument less than maximum separation (%f)\n\n", separation_max);
          return 1;
        }
        separation_min_specified = 1;
        break;
      case 't':
        texture_file = optarg; break;
      case 'p':
        preserve_height = 1;  break;
      case 'N':
        add_noise = 1;  break;
      case 'P':
        if ((pattern_type = image_pattern_type_from_name(optarg)) == -1) {
          fputs(usage, stderr);
          fprintf(stderr, "\nInvalid pattern type for -P: %s\n\n", optarg);
          return 1;
        }
        break;
      case 'e':
        if (ascii_to_float(optarg, &eye_separation) == -1) {
          fputs(usage, stderr);
          fputs("\n-e requires a positive argument\n\n", stderr);
          return 1;
        }
        break;
      case 'r':
        if (ascii_to_ssize_t(optarg, &edge_echo_offset) == -1) {
          fputs(usage, stderr);
          fputs("\n-r requires a non-zero integer argument\n\n", stderr);
          return 1;
        }
        edge_echo_offset_specified = 1;
        break;
      case 'h':
        fputs(usage, stdout);
        fputc('\n', stdout);
        return 0;
        break;
      case '?':
      default:
        fputs(usage, stderr);
        return 1;
        break;
    }
  }

  argv += optind;
  argc -= optind;

  /* Make sure the input makes sense. */

  if (argc > 0) {
    fputs(usage, stderr);
    fputc('\n', stderr);
    return 1;
  }

  if (heightmap_file == NULL) {
    fputs(usage, stderr);
    fputs("\nMissing required parameter: -i\n\n", stderr);
    return 1;
  }

  if (output_file == NULL) {
    fputs(usage, stderr);
    fputs("\nMissing required parameter: -o\n\n", stderr);
    return 1;
  }

  if (separation_max <= 1.0f) {
    fputs(usage, stderr);
    fputs("\n-f requires a positive argument greater than 1.0\n\n", stderr);
    return 1;
  }
  if (!separation_min_specified) {
    separation_min = MIN_MAX_SEPARATION_RATIO * separation_max;
  }
  if (separation_min <= 0.0f || separation_min >= separation_max) {
    fputs(usage, stderr);
    fprintf(stderr, "\n-n requires a positive argument less than maximum separation (%f)\n\n", separation_max);
    return 1;
  }
  if (eye_separation <= 0.0f) {
    fputs(usage, stderr);
    fputs("\n-e requires a positive argument\n\n", stderr);
    return 1;
  }
  if (separation_max >= eye_separation) {
    fputs(usage, stderr);
    fputs("\neye separation must be greater than maximum separation\n\n", stderr);
    return 1;
  }
  if (!edge_echo_offset_specified) {
    edge_echo_offset = (ssize_t) (EDGE_ECHO_OFFSET_RATIO * separation_max);
  }
  if (edge_echo_offset == 0) {
    fputs(usage, stderr);
    fputs("\n-r requires a non-zero integer argument\n\n", stderr);
    return 1;
  }

  srand(time(NULL));

  image_init();  /* initialize the image library */

  if ((heightmap = heightmap_read(heightmap_file)) == NULL) {
    return -1;
  }

  height = heightmap_get_height(heightmap);

  if ((texture = get_texture(texture_file, (size_t) (0.5f + (separation_min + separation_max)), height, pattern_type)) == NULL) {
    return 1;
  }

  if (texture_file && !preserve_height) {
    /* The user is providing us with a texture to use, and has not asked us to preserve the
       height of the texture in the output.  The input texture could be any arbitrary size, but
       in the output it will be horizontally scaled to between separation_min and separation_max.
       We'd like to keep the aspect ratio of the texture for aesthetic purposes, so let's go ahead
       and scale it vertically such that it will look good in the stereogram. */
    if (scale_texture_height(texture, 0.5f * (separation_min + separation_max)) == -1) {
      return 1;
    }
  }

  if (add_noise) {
    if (image_add_noise(texture) == -1) {
      return 1;
    }
  }

  if ((output = create_stereogram(heightmap, texture, eye_separation, separation_min, separation_max, edge_echo_offset)) == NULL) {
    return -1;
  }

  if (image_write(output, output_file) == -1) {
    return -1;
  }

  image_close();  /* close the image library */

  return 0;
}
