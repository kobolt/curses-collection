#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "node.h"



diff_node_t *diff_node_new(diff_node_t *parent, char *name, diff_type_t type)
{
  int len;
  diff_node_t *new;

  new = (diff_node_t *)malloc(sizeof(diff_node_t));

  if (name == NULL) {
    new->name = NULL;
  } else {
    len = strlen(name) + 1;
    new->name = (char *)malloc(len);
    strncpy(new->name, name, len);
  }
  new->type = type;
  new->no_of_subnodes = 0;
  new->parent = parent;
  new->subnode = NULL;
  new->expanded = true;

  return new;
}



diff_node_t *diff_node_add(diff_node_t *current, char *name, diff_type_t type)
{
  diff_node_t *new;

  new = diff_node_new(current, name, type);

  if (current->subnode == NULL) {
    current->subnode = malloc(sizeof(diff_node_t *));
  } else {
    current->subnode = realloc(current->subnode, sizeof(diff_node_t *) * (current->no_of_subnodes + 1));
  }

  current->subnode[current->no_of_subnodes] = new;
  current->no_of_subnodes++;

  return new;
}



void diff_node_remove(diff_node_t *node)
{
  int i;
  free(node->name);
  for (i = 0; i < node->no_of_subnodes; i++) {
    diff_node_remove(node->subnode[i]);
  }
  if (node->subnode != NULL) {
    free(node->subnode);
  }
  free(node);
}



int diff_node_depth(diff_node_t *node)
{
  int depth;

  depth = 0;
  do {
    if (node->type == DIFF_TYPE_ROOT) {
      break;
    }
    node = node->parent;
    depth++;
  } while (node != NULL);

  return depth;
}



char *diff_node_path(diff_node_t *node, char *path, int path_len)
{
  char temp[PATH_MAX];

  strncpy(path, node->name, path_len);

  node = node->parent;
  while (node != NULL) {
    if (node->type == DIFF_TYPE_ROOT) {
      break;
    }
    strncpy(temp, path, PATH_MAX);
    snprintf(path, path_len, "%s/%s", node->name, temp);
    node = node->parent;
  }

  return path;
}



void diff_node_dump(diff_node_t *node)
{
  int i;
  int depth;

  depth = diff_node_depth(node);
  while (depth-- > 1)
    printf("  ");

  switch (node->type) {
  case DIFF_TYPE_FILE_EQUAL:
  case DIFF_TYPE_DIR_EQUAL:
    printf("=");
    break;

  case DIFF_TYPE_FILE_DIFFERS:
  case DIFF_TYPE_DIR_DIFFERS:
    printf("*");
    break;

  case DIFF_TYPE_FILE_ADDED:
  case DIFF_TYPE_DIR_ADDED:
    printf("+");
    break;

  case DIFF_TYPE_FILE_MISSING:
  case DIFF_TYPE_DIR_MISSING:
    printf("-");
    break;

  default:
    break;
  }

  printf("%s", node->name);

  switch (node->type) {
  case DIFF_TYPE_DIR_EQUAL:
  case DIFF_TYPE_DIR_DIFFERS:
  case DIFF_TYPE_DIR_ADDED:
  case DIFF_TYPE_DIR_MISSING:
    printf("/");
    break;

  default:
    break;
  }

  printf("\n");

  for (i = 0; i < node->no_of_subnodes; i++) {
    diff_node_dump(node->subnode[i]);
  }
}



void diff_node_parents_differ(diff_node_t *node)
{
  do {
    if (node->type == DIFF_TYPE_ROOT) {
      break;
    } else if (node->type == DIFF_TYPE_DIR_EQUAL) {
      node->type = DIFF_TYPE_DIR_DIFFERS;
    }
    
    node = node->parent;
  } while (node != NULL);
}



static int diff_node_compare(const void *p1, const void *p2)
{
  diff_node_t *p1p, *p2p;
  p1p = *((diff_node_t **)p1);
  p2p = *((diff_node_t **)p2);
  return strcmp(((diff_node_t *)p1p)->name, ((diff_node_t *)p2p)->name);
}



void diff_node_sort(diff_node_t *node)
{
  int i;
  qsort(node->subnode, node->no_of_subnodes, sizeof(diff_node_t *), diff_node_compare);
  for (i = 0; i < node->no_of_subnodes; i++) {
    diff_node_sort(node->subnode[i]);
  }
}



void diff_node_unexpand_all(diff_node_t *node)
{
  int i;

  switch (node->type) {
  case DIFF_TYPE_DIR_EQUAL:
  case DIFF_TYPE_DIR_DIFFERS:
  case DIFF_TYPE_DIR_ADDED:
  case DIFF_TYPE_DIR_MISSING:
    if (node->no_of_subnodes > 0) {
      node->expanded = false;
    }
    break;

  default:
    break;
  }

  for (i = 0; i < node->no_of_subnodes; i++) {
    diff_node_unexpand_all(node->subnode[i]);
  }
}



