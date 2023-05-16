
#include "list.h"

#include <string.h>
#include <stdio.h>

int list_init(list_t *list) {
  list->first = NULL;
  list->last = NULL;

  return 0;
}

void list_destroy(list_t *list) {
  node_t *node;
  node_t *next;

  for (node = list->first;  node;  node = next) {
    next = node->next;
    free(node);
  }

  list->first = NULL;
  list->last = NULL;
}

int list_add(list_t *list, const control_point_t *point, node_t *from) {
  node_t *node;
  node_t *prev;

  if ((node = malloc(sizeof(*node))) == NULL) {
    return -1;
  }
  memset(node, 0, sizeof(*node));

  node->point = *point;

  if (list->first == NULL) {
    /* List is empty. */
    list->first = node;
    list->last = node;
  } else {
    /* We have to find the correct insertion point. */
    prev = list_find(list, point->x, from);
    node->prev = prev;
    if (prev == NULL) {
      /* The new node goes at the head of the list. */
      node->next = list->first;
      list->first = node;
    } else {
      node->next = prev->next;
      prev->next = node;
    }

    if (node->next) {
      node->next->prev = node;
    } else {
      list->last = node;
    }
  }

  return 0;
}

void list_remove_first(list_t *list) {
  node_t *node;

  node = list->first;

  if (node == NULL) {
    return;
  }

  if (node->next == NULL) {
    list->last = NULL;
  } else {
    node->next->prev = NULL;
  }

  list->first = node->next;

  free(node);
}

void list_remove_last(list_t *list) {
  node_t *node;

  node = list->last;

  if (node == NULL) {
    return;
  }

  if (node->prev == NULL) {
    list->first = NULL;
  } else {
    node->prev->next = NULL;
  }

  list->last = node->prev;

  free(node);
}

node_t *list_find(list_t *list, float x, node_t *from) {
  node_t *node;

  if (list->first == NULL) {
    return NULL;
  }

  if (from == NULL) {
    from = list->first;
  }

  node = from;

  while (node && node->point.x > x) {
    node = node->prev;
  }

  if (node == NULL) {
    return NULL;
  }

  while (node->next && node->next->point.x <= x) {
    node = node->next;
  }

  return node;
}

void list_find_range(list_t *list, node_t **start, node_t **end, float x1, float x2, node_t *from) {
  node_t *node;

  if (x1 > x2) {
    fprintf(stderr, "Warning: list_find_range() called with %f .. %f\n", x1, x2);
  }

  if (list->first == NULL) {
    *start = *end = NULL;
    return;
  }

  if (from == NULL) {
    from = list->first;
  }

  node = from;

  /* Step right until our right neighbor is to the right of the beginning of the range. */
  while (node->next && node->next->point.x <= x1) {
    node = node->next;
  }

  /* Now our right neighbor is to the right of the beginning of the range.  Let's step left until
     we're outside the range. */
  while (node && node->point.x > x1) {
    node = node->prev;
  }

  *start = node;

  /* Finally, step right until we're outside the range. */
  while (node && node->point.x < x2) {
    node = node->next;
  }

  *end = node;
}

void list_dump(const list_t *list) {
  const node_t *node;

  for (node = list->first;  node;  node = node->next) {
    control_point_dump(&node->point);
  }
}

void list_reflect(list_t *list, float axis) {
  node_t *node;
  node_t *tmp;

  for (node = list->first;  node;  node = node->prev) {
    control_point_reflect(&node->point, axis);

    tmp = node->prev;
    node->prev = node->next;
    node->next = tmp;
  }

  tmp = list->first;
  list->first = list->last;
  list->last = tmp;
}
