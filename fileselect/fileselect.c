#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curses.h>

typedef enum {
  FILE_NODE_TYPE_ROOT,
  FILE_NODE_TYPE_DIR,
  FILE_NODE_TYPE_FILE,
} file_node_type_t;

typedef struct file_node_s {
  int marked;
  char *name;
  file_node_type_t type;
  struct file_node_s *parent;
  unsigned int no_of_subnodes;
  struct file_node_s **subnode;
} file_node_t;

static int curses_scroll_offset  = 0;
static int curses_selected_entry = 0;

static file_node_t *file_node_new(file_node_t *parent, char *name, file_node_type_t type)
{
  int len;
  file_node_t *new;

  new = (file_node_t *)malloc(sizeof(file_node_t));
  if (new == NULL)
    return NULL;

  if (name == NULL) {
    new->name = NULL;
  } else {
    len = strlen(name) + 1;
    new->name = (char *)malloc(len);
    strncpy(new->name, name, len);
  }
  new->type = type;
  new->parent = parent;
  new->no_of_subnodes = 0;
  new->subnode = NULL;
  new->marked = 0;
  
  return new;
}

static file_node_t *file_node_add(file_node_t *current, char *name, file_node_type_t type)
{
  file_node_t *new;

  new = file_node_new(current, name, type);
  if (new == NULL)
    return NULL;

  if (current->subnode == NULL) {
    current->subnode = malloc(sizeof(file_node_t *));
  } else {
    current->subnode = realloc(current->subnode, sizeof(file_node_t *) * (current->no_of_subnodes + 1));
  }

  current->subnode[current->no_of_subnodes] = new;
  current->no_of_subnodes++;

  return new;
}

static void file_node_remove(file_node_t *node)
{
  int i;
  free(node->name);
  for (i = 0; i < node->no_of_subnodes; i++) {
    file_node_remove(node->subnode[i]);
  }
  if (node->subnode != NULL) {
    free(node->subnode);
  }
  free(node);
}

static int file_node_depth(file_node_t *node)
{
  int depth;

  depth = 0;
  do {
    if (node->type == FILE_NODE_TYPE_ROOT) {
      break;
    }
    node = node->parent;
    depth++;
  } while (node != NULL);

  return depth;
}

static int file_node_list_size(file_node_t *node)
{
  int i, size;

  if (node->type == FILE_NODE_TYPE_ROOT) {
    size = 0;
  } else {
    size = 1;
  }

  for (i = 0; i < node->no_of_subnodes; i++) {
    size += file_node_list_size(node->subnode[i]);
  }

  return size;
}

static char *file_node_path(file_node_t *node, char *path, int path_len)
{
  char temp[PATH_MAX];

  strncpy(path, node->name, path_len);

  node = node->parent;
  while (node != NULL) {
    if (node->type == FILE_NODE_TYPE_ROOT) {
      break;
    }
    strncpy(temp, path, PATH_MAX);
    snprintf(path, path_len, "%s/%s", node->name, temp);
    node = node->parent;
  }

  return path;
}

static int file_node_compare(const void *p1, const void *p2)
{
  file_node_t *p1p, *p2p;
  p1p = *((file_node_t **)p1);
  p2p = *((file_node_t **)p2);
  return strcmp(((file_node_t *)p1p)->name, ((file_node_t *)p2p)->name);
}

static void file_node_sort(file_node_t *node)
{
  int i;
  qsort(node->subnode, node->no_of_subnodes, sizeof(file_node_t *), file_node_compare);
  for (i = 0; i < node->no_of_subnodes; i++) {
    file_node_sort(node->subnode[i]);
  }
}

static void file_node_dump(file_node_t *node)
{
  int i, depth;

  depth = file_node_depth(node);
  while (depth-- > 1)
    printf("  ");

  if (node->type == FILE_NODE_TYPE_ROOT)
    printf("/");
  else {
    printf("%s", node->name);
    if (node->type == FILE_NODE_TYPE_DIR)
      printf("/");
  }

  printf("\n");

  for (i = 0; i < node->no_of_subnodes; i++) {
    file_node_dump(node->subnode[i]);
  }
}

static void file_node_print_marked(file_node_t *node, char *root_dir, FILE *fh)
{
  int i;
  char path[PATH_MAX];

  if (node->marked) {
    fprintf(fh, "%s/%s\n", root_dir, file_node_path(node, path, PATH_MAX));
  }

  for (i = 0; i < node->no_of_subnodes; i++) {
    file_node_print_marked(node->subnode[i], root_dir, fh);
  }
}

static int file_node_scan(file_node_t *current, char *path)
{
  DIR *dh;
  struct dirent *entry;
  struct stat st;
  char fullpath[PATH_MAX];
  file_node_t *subnode;

  dh = opendir(path);
  if (dh == NULL) {
    fprintf(stderr, "Error: Unable to open directory: %s\n", path);
    return -1;
  }

  while ((entry = readdir(dh))) {
    if (entry->d_name[0] == '.')
      continue; /* Ignore files with leading dot. */

    snprintf(fullpath, PATH_MAX, "%s/%s", path, entry->d_name);
    if (stat(fullpath, &st) == -1) {
      fprintf(stderr, "Warning: Unable to stat() path: %s\n", fullpath);
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      subnode = file_node_add(current, entry->d_name, FILE_NODE_TYPE_DIR);
      file_node_scan(subnode, fullpath);

    } else if (S_ISREG(st.st_mode)) {
      file_node_add(current, entry->d_name, FILE_NODE_TYPE_FILE);
    }
  }

  closedir(dh);
  return 0;
}

static file_node_t *file_node_get_by_node_no(file_node_t *node, int node_no, int *node_count)
{
  file_node_t *found;
  int i;

  if (node->type != FILE_NODE_TYPE_ROOT) {
    *node_count = *node_count + 1;
  }
  
  if (*node_count == node_no) {
    return node; /* Match found, return self. */
  }

  for (i = 0; i < node->no_of_subnodes; i++) {
    found = file_node_get_by_node_no(node->subnode[i], node_no, node_count);
    if (found != NULL) {
      return found;
    }
  }

  return NULL;
}

static void file_node_mark(file_node_t *node, int node_no)
{
  int node_count;
  file_node_t *found;

  node_count = 0;
  found = file_node_get_by_node_no(node, node_no, &node_count);
  if (found == NULL)
    return;

  if (found->type != FILE_NODE_TYPE_FILE)
    return; /* Only files, not directories, can be marked. */

  if (found->marked == 0) {
    found->marked = 1;
  } else {
    found->marked = 0;
  }
}

static void curses_list_draw(file_node_t *node, int line_no, int node_no, int selected)
{
  int node_count, maxy, maxx, pos, depth;
  file_node_t *found;

  node_count = 0;
  found = file_node_get_by_node_no(node, node_no, &node_count);
  if (found == NULL)
    return;

  getmaxyx(stdscr, maxy, maxx);

  if (selected)
    attron(A_REVERSE);

  if (found->name == NULL) {
    for (pos = 0; pos < maxx - 2; pos++)
      mvaddch(line_no, pos, ' ');

  } else {
    pos = 0;

    /* Depth indicator. */
    depth = file_node_depth(found);
    while (depth-- > 1) {
      mvaddch(line_no, pos++, ' ');
      mvaddch(line_no, pos++, ' ');
    }

    /* File/directory name. */
    if (found->marked)
      attron(A_BOLD);
    mvaddstr(line_no, pos, found->name);
    pos += strlen(found->name);
    if (found->marked)
      attroff(A_BOLD);

    /* Slash for directory. */
    if (found->type == FILE_NODE_TYPE_DIR)
      mvaddch(line_no, pos++, '/');

    /* Padding. */
    for (; pos < maxx - 2; pos++)
      mvaddch(line_no, pos, ' ');
  }

  if (selected)
    attroff(A_REVERSE);
}

static void curses_update_screen(file_node_t *node)
{
  int n, i, maxy, maxx;
  int scrollbar_size, scrollbar_pos;
  int list_size;
  
  list_size = file_node_list_size(node);
  
  getmaxyx(stdscr, maxy, maxx);
  erase();
  
  /* Draw text lines. */
  for (n = 0; n < maxy; n++) { 
    if ((n + curses_scroll_offset) >= list_size)
      break;
    
    if (n == (curses_selected_entry - curses_scroll_offset)) {
      curses_list_draw(node, n, n + curses_scroll_offset + 1, 1);
    } else {
      curses_list_draw(node, n, n + curses_scroll_offset + 1, 0);
    }
  }
  
  /* Draw scrollbar. */
  if (list_size <= maxy)
    scrollbar_size = maxy;
  else
    scrollbar_size = maxy / (list_size / (double)maxy);
  
  scrollbar_pos = curses_selected_entry / (double)list_size * (maxy - scrollbar_size);
  attron(A_REVERSE);
  for (i = 0; i <= scrollbar_size; i++)
    mvaddch(i + scrollbar_pos, maxx - 1, ' ');
  attroff(A_REVERSE);
  
  mvvline(0, maxx - 2, 0, maxy);
  
  /* Place cursor at end of selected line. */ 
  move(curses_selected_entry - curses_scroll_offset, maxx - 3);
}

static void curses_exit_handler(void)
{
  endwin();
}

static void curses_winch_handler(file_node_t *node)
{
  endwin(); /* To get new window limits. */
  curses_update_screen(node);
  flushinp();
  keypad(stdscr, TRUE);
}

static void file_node_curses_loop(file_node_t *node)
{
  int c, maxy, maxx, list_size;

  initscr();
  atexit(curses_exit_handler);
  noecho();
  keypad(stdscr, TRUE);

  while (1) {
    list_size = file_node_list_size(node);
    curses_update_screen(node);
    getmaxyx(stdscr, maxy, maxx);
    c = getch();

    switch (c) {
    case KEY_RESIZE:
      curses_winch_handler(node);
      break;

    case KEY_UP:
      curses_selected_entry--;
      if (curses_selected_entry < 0)
        curses_selected_entry++;
      if (curses_scroll_offset > curses_selected_entry) {
        curses_scroll_offset--;
        if (curses_scroll_offset < 0)
          curses_scroll_offset = 0;
      }
      break;

    case KEY_NPAGE:
      curses_scroll_offset += maxy / 2;
      while (maxy + curses_scroll_offset > list_size)
        curses_scroll_offset--;
      if (curses_scroll_offset < 0)
        curses_scroll_offset = 0;
      if (curses_selected_entry < curses_scroll_offset)
        curses_selected_entry = curses_scroll_offset;
      break;

    case KEY_PPAGE:
      curses_scroll_offset -= maxy / 2;
      if (curses_scroll_offset < 0)
        curses_scroll_offset = 0;
      if (curses_selected_entry > maxy + curses_scroll_offset - 1)
        curses_selected_entry = maxy + curses_scroll_offset - 1;
      break;

    case ' ':
    case KEY_IC:
      file_node_mark(node, curses_selected_entry + 1);
      /* Move cursor to next line automatically. */
    case KEY_ENTER:
    case '\n':
    case '\r':
    case KEY_DOWN:
      curses_selected_entry++;
      if (curses_selected_entry >= list_size)
        curses_selected_entry--;
      if (curses_selected_entry > maxy - 1) {
        curses_scroll_offset++;
        if (curses_scroll_offset > curses_selected_entry - maxy + 1)
          curses_scroll_offset--;
      }
      break;

    case '\e': /* Escape */
    case 'Q':
    case 'q':
      return;
    }
  }
}

int main(int argc, char *argv[])
{
  file_node_t *root;
  char *root_dir;
  FILE *fh;

  if (argc < 2) {
     fprintf(stderr, "Usage: %s <output file> [directory]\n", argv[0]);
     return 1;
  }

  fh = fopen(argv[1], "wx");
  if (fh == NULL) {
     fprintf(stderr, "Error: Cannot open file, or it exists already: %s\n", argv[1]);
     return 1;
  }

  if (argc > 2) {
    root_dir = argv[2];
  } else {
    root_dir = ".";
  }

  root = file_node_new(NULL, NULL, FILE_NODE_TYPE_ROOT);
  if (root == NULL) {
    fclose(fh);
    return 1;
  }

  if (file_node_scan(root, root_dir) != 0) {
    file_node_remove(root);
    fclose(fh);
    return 1;
  }

  file_node_sort(root);

  if (isatty(STDOUT_FILENO)) {
    file_node_curses_loop(root);
    file_node_print_marked(root, root_dir, fh);
  } else {
    /* Mostly for debugging. */
    file_node_dump(root);
  }

  file_node_remove(root);
  fclose(fh);
  return 0;
}

