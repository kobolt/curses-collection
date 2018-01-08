#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include "tree.h"
#include "node.h"



static void diff_tree_added_dir(char *path, diff_node_t *current)
{
  DIR *dh;
  struct dirent *entry;
  struct stat st;
  char fullpath[PATH_MAX];
  diff_node_t *subnode;

  dh = opendir(path);
  if (dh == NULL) {
    fprintf(stderr, "Warning: Unable to open directory: %s\n", path);
    return;
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
      subnode = diff_node_add(current, entry->d_name, DIFF_TYPE_DIR_ADDED);
      diff_tree_added_dir(fullpath, subnode);
    } else if (S_ISREG(st.st_mode)) {
      diff_node_add(current, entry->d_name, DIFF_TYPE_FILE_ADDED);
    }
  }
  closedir(dh);
}



static void diff_tree_missing_dir(char *path, diff_node_t *current)
{
  DIR *dh;
  struct dirent *entry;
  struct stat st;
  char fullpath[PATH_MAX];
  diff_node_t *subnode;

  dh = opendir(path);
  if (dh == NULL) {
    fprintf(stderr, "Warning: Unable to open directory: %s\n", path);
    return;
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
      subnode = diff_node_add(current, entry->d_name, DIFF_TYPE_DIR_MISSING);
      diff_tree_missing_dir(fullpath, subnode);
    } else if (S_ISREG(st.st_mode)) {
      diff_node_add(current, entry->d_name, DIFF_TYPE_FILE_MISSING);
    }
  }
  closedir(dh);
}



static int diff_tree_compare_file(char *path1, char *path2)
{
  FILE *fh1, *fh2;
  int c1, c2;

  fh1 = fopen(path1, "rb");
  if (fh1 == NULL) {
    fprintf(stderr, "Warning: Unable to open file: %s\n", path1);
    return 0;
  }

  fh2 = fopen(path2, "rb");
  if (fh2 == NULL) {
    fprintf(stderr, "Warning: Unable to open file: %s\n", path2);
    return 0;
  }

  while ((c1 = fgetc(fh1)) != EOF) {
    c2 = fgetc(fh2);
    if (c1 != c2) {
      fclose(fh1);
      fclose(fh2);
      return 1;
    }
  }

  fclose(fh1);
  fclose(fh2);
  return 0;
}



void diff_tree_compare_dir(char *path1, char *path2, diff_node_t *current)
{
  DIR *dh;
  struct dirent *entry;
  struct stat st1, st2;
  char fullpath1[PATH_MAX], fullpath2[PATH_MAX];
  diff_node_t *subnode;

  dh = opendir(path1);
  if (dh == NULL) {
    fprintf(stderr, "Warning: Unable to open directory: %s\n", path1);
    return;
  }

  while ((entry = readdir(dh))) {
    if (entry->d_name[0] == '.')
      continue; /* Ignore files with leading dot. */

    snprintf(fullpath1, PATH_MAX, "%s/%s", path1, entry->d_name);
    snprintf(fullpath2, PATH_MAX, "%s/%s", path2, entry->d_name);

    if (stat(fullpath1, &st1) == -1) {
      fprintf(stderr, "Warning: Unable to stat() path: %s\n", fullpath1);
      continue;
    }

    if (S_ISDIR(st1.st_mode)) {
      if (stat(fullpath2, &st2) == -1) {
        subnode = diff_node_add(current, entry->d_name, DIFF_TYPE_DIR_ADDED);
        diff_node_parents_differ(current);
        diff_tree_added_dir(fullpath1, subnode);
      } else {
        if (S_ISDIR(st2.st_mode)) {
          subnode = diff_node_add(current, entry->d_name, DIFF_TYPE_DIR_EQUAL);
          diff_tree_compare_dir(fullpath1, fullpath2, subnode);
        } else {
          subnode = diff_node_add(current, entry->d_name, DIFF_TYPE_DIR_ADDED);
          diff_node_parents_differ(current);
          diff_tree_added_dir(fullpath1, subnode);
        }
      }
      
    } else if (S_ISREG(st1.st_mode)) {
      if (stat(fullpath2, &st2) == -1) {
        diff_node_add(current, entry->d_name, DIFF_TYPE_FILE_ADDED);
        diff_node_parents_differ(current);
      } else {
        if (S_ISREG(st2.st_mode)) {
          if (st1.st_size == st2.st_size) {
            if (diff_tree_compare_file(fullpath1, fullpath2)) {
              diff_node_add(current, entry->d_name, DIFF_TYPE_FILE_DIFFERS);
              diff_node_parents_differ(current);
            } else {
              diff_node_add(current, entry->d_name, DIFF_TYPE_FILE_EQUAL);
            }
          } else {
            diff_node_add(current, entry->d_name, DIFF_TYPE_FILE_DIFFERS);
            diff_node_parents_differ(current);
          }
        } else {
          diff_node_add(current, entry->d_name, DIFF_TYPE_FILE_ADDED);
          diff_node_parents_differ(current);
        }
      }
    }
  }
  closedir(dh);

  dh = opendir(path2);
  if (dh == NULL) {
    fprintf(stderr, "Warning: Unable to open directory: %s\n", path2);
    return;
  }

  while ((entry = readdir(dh))) {
    if (entry->d_name[0] == '.')
      continue; /* Ignore files with leading dot. */

    snprintf(fullpath1, PATH_MAX, "%s/%s", path1, entry->d_name);
    snprintf(fullpath2, PATH_MAX, "%s/%s", path2, entry->d_name);

    if (stat(fullpath2, &st2) == -1) {
      fprintf(stderr, "Warning: Unable to stat() path: %s\n", fullpath2);
      continue;
    }

    if (S_ISDIR(st2.st_mode)) {
      if (stat(fullpath1, &st1) == -1) {
        subnode = diff_node_add(current, entry->d_name, DIFF_TYPE_DIR_MISSING);
        diff_node_parents_differ(current);
        diff_tree_missing_dir(fullpath2, subnode);
      } else {
        if (! S_ISDIR(st1.st_mode)) {
          subnode = diff_node_add(current, entry->d_name, DIFF_TYPE_DIR_MISSING);
          diff_node_parents_differ(current);
          diff_tree_missing_dir(fullpath2, subnode);
        }
      }
      
    } else if (S_ISREG(st2.st_mode)) {
      if (stat(fullpath1, &st1) == -1) {
        diff_node_add(current, entry->d_name, DIFF_TYPE_FILE_MISSING);
        diff_node_parents_differ(current);
      } else {
        if (! S_ISREG(st1.st_mode)) {
          diff_node_add(current, entry->d_name, DIFF_TYPE_FILE_MISSING);
          diff_node_parents_differ(current);
        }
      }
    }
  }
  closedir(dh);
}



