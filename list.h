
#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

#include "control_point.h"

typedef struct node_tag {
  struct node_tag *prev;
  struct node_tag *next;

  control_point_t point;
} node_t;

typedef struct list_tag {
  node_t *first;
  node_t *last;
} list_t;

int list_init(list_t *list);

void list_destroy(list_t *list);

int list_add(list_t *list, const control_point_t *point, node_t *from);

/* Finds the last node that has an x value less than x. */
node_t *list_find(list_t *list, float x, node_t *from);

/* Finds the largest sublist of nodes contained in the range x1..x2, prepended by the first node
   to the left of the range. */
void list_find_range(list_t *list, node_t **start, node_t **end, float x1, float x2, node_t *from);

void list_remove_first(list_t *list);
void list_remove_last(list_t *list);

void list_dump(const list_t *list);

/* Reverses the list and reflects all its control point values around the specified axis. */
void list_reflect(list_t *list, float axis);

#endif
