#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "tree.h"
#include "node.h"
#include "navi.h"



int main(int argc, char *argv[])
{
  diff_node_t *root;

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <directory 1> <directory 2>\n", argv[0]);
    return 1;
  }

  root = diff_node_new(NULL, NULL, DIFF_TYPE_ROOT);

  diff_tree_compare_dir(argv[1], argv[2], root);
  diff_node_sort(root);
  diff_node_unexpand_all(root);

  if (isatty(STDOUT_FILENO)) {
    diff_navi_loop(root, argv[1], argv[2]); /* Curses interface. */
  } else {
    diff_node_dump(root); /* Use text-dump when being piped. */
  }

  diff_node_remove(root);

  return 0;
}



