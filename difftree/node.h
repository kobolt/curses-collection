#ifndef _NODE_H
#define _NODE_H

#include <stdbool.h>

typedef enum {
  DIFF_TYPE_ROOT,
  DIFF_TYPE_FILE_EQUAL,
  DIFF_TYPE_FILE_DIFFERS,
  DIFF_TYPE_FILE_ADDED,
  DIFF_TYPE_FILE_MISSING,
  DIFF_TYPE_DIR_EQUAL,
  DIFF_TYPE_DIR_DIFFERS,
  DIFF_TYPE_DIR_ADDED,
  DIFF_TYPE_DIR_MISSING,
} diff_type_t;

typedef struct diff_node_s {
  bool expanded; /* For visualization purposes. */
  diff_type_t type;
  char *name;
  struct diff_node_s *parent;
  unsigned int no_of_subnodes;
  struct diff_node_s **subnode;
} diff_node_t;

diff_node_t *diff_node_new(diff_node_t *parent, char *name, diff_type_t type);
diff_node_t *diff_node_add(diff_node_t *current, char *name, diff_type_t type);
void diff_node_remove(diff_node_t *node);
int diff_node_depth(diff_node_t *node);
char *diff_node_path(diff_node_t *node, char *path, int path_len);
void diff_node_dump(diff_node_t *node);
void diff_node_parents_differ(diff_node_t *node);
void diff_node_sort(diff_node_t *node);
void diff_node_unexpand_all(diff_node_t *node);

#endif /* _NODE_H */
