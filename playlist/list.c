#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* strcasecmp() */
#include <time.h>
#include <limits.h>
#include "play.h"
#include "list.h"

/* The original set. */
static char **list_original = NULL;
static int list_original_size = 0;

/* The filtered set. */
static char **list_filtered = NULL;
static int list_filtered_size = 0;

/* The abstraction. */
static char **list = NULL;
static int list_size = 0;
static int list_current = -1;

int list_import_file(char *path)
{
  char line[PATH_MAX];
  char *p;
  FILE *fh;

  fh = fopen(path, "r");
  if (fh == NULL)
    return -1;

  list_original = (char **)malloc(sizeof(char *));
  if (list_original == NULL) {
    fclose(fh);
    return -1;
  }
  list_original_size = 0;

  while (fgets(line, PATH_MAX, fh) != NULL) {
    for (p = line; *p != '\0'; p++) {
      if ((*p == '\r') || (*p == '\n')) {
        *p = '\0';
        break;
      }
    }

    list_original[list_original_size] = (char *)malloc(sizeof(char) * 
      (strlen(line) + 1));
    if (list_original[list_original_size] == NULL) {
      fclose(fh);
      return -1;
    }
    strncpy(list_original[list_original_size], line, strlen(line) + 1);

    list_original_size++;
    list_original = (char **)realloc(list_original, sizeof(char *) * 
      (list_original_size + 1));
    if (list_original == NULL) {
      fclose(fh);
      return -1;
    }
    list_original[list_original_size] = NULL;
  }

  fclose(fh);

  /* Set original list as the one to use right after loading the file. */
  list = list_original;
  list_size = list_original_size;

  return 0;
}

void list_request(int no)
{
  if (no >= 0 && no < list_size) {
    play_cancel();
    list_current = no;
    play_execute(list[list_current]);
  }
}

void list_request_next(void)
{
  if (list_current >= (list_size - 1)) {
    /* If triggered by last in list, show that nothing is currently playing. */
    list_current = -1;
    return;
  }
  play_cancel(); /* Will most likely be ignored, since called after done. */
  list_current++;
  play_execute(list[list_current]);
}

char *list_get(int no, int *playing)
{
  char *basename;

  if (no == list_current)
    *playing = 1;
  else
    *playing = 0;

  if (no >= 0 && no < list_size) {
    basename = rindex(list[no], '/');
    if (basename == NULL) {
      return list[no];
    } else {
      return basename + 1;
    }
  } else {
    return NULL;
  }
}

int list_size_get(void)
{
  return list_size;
}

int list_swap(int no_a, int no_b)
{
  char *temp;

  if (no_a < 0 || 
      no_b < 0 ||
      no_a >= list_size ||
      no_b >= list_size)
    return -1;

  play_block();

  temp = list[no_b];
  list[no_b] = list[no_a];
  list[no_a] = temp;

  if (no_a == list_current)
    list_current = no_b;
  else if (no_b == list_current)
    list_current = no_a;

  play_unblock();

  return 0;
}

void list_shuffle(void)
{
  char *temp;
  int i, r;

  srandom(time(NULL));

  play_block();

  for (i = 0; i < list_size; i++) {
    r = random() % list_size;

    temp = list[i];
    list[i] = list[r];
    list[r] = temp;

    if (r == list_current)
      list_current = i;
    else if (i == list_current)
      list_current = r;
  }

  play_unblock();
}

static int list_cmp(char *filter, char *original)
{
  char *basename;

  basename = rindex(original, '/');
  if (basename == NULL)
    basename = original;
  else
    basename++; /* Remove leading slash. */

  while (*basename != '\0') {
    if (strncasecmp(filter, basename, strlen(filter)) == 0)
      return 0;
    basename++;
  }
  return -1;
}

static int list_filter_produces_size(char *filter)
{
  int i, size;

  size = 0;
  for (i = 0; i < list_original_size; i++) {
    if (list_cmp(filter, list_original[i]) == 0)
      size++;
  }

  return size;
}

int list_filter_apply(char *filter)
{
  int i;

  /* Just ignore everything if an invalid filter is given. */
  if (list_filter_produces_size(filter) == 0)
    return -1;

  play_block(); /* Entering critical section. */

  /* Free old filtered list first, if it exists. */
  if (list_filtered != NULL) {
    for (i = 0; i < list_filtered_size; i++) {
      free(list_filtered[i]);
    }
    free(list_filtered);
    list_filtered = NULL;
  }

  /* Use the empty string to disable the filter. */
  if (strlen(filter) == 0) {
    list = list_original;
    list_size = list_original_size;
    goto list_filter_apply_done;
  }

  list_filtered = (char **)malloc(sizeof(char *));
  if (list_filtered == NULL)
    return -1;

  list_filtered_size = 0;
  for (i = 0; i < list_original_size; i++) {
    if (list_cmp(filter, list_original[i]) == 0) {
      list_filtered[list_filtered_size] = (char *)malloc(sizeof(char) * 
        (strlen(list_original[i]) + 1));
      if (list_filtered[list_filtered_size] == NULL)
        return -1;
      strncpy(list_filtered[list_filtered_size], 
        list_original[i], strlen(list_original[i]) + 1);

      list_filtered_size++;
      list_filtered = (char **)realloc(list_filtered, sizeof(char *) *
        (list_filtered_size + 1));
      if (list_filtered == NULL)
        return -1;
    }
  }

  /* This should not happen, as size is checked earlier, but just in case... */
  if (list_filtered_size == 0) {
    free(list_filtered);
    list_filtered = NULL;

    list = list_original;
    list_size = list_original_size;
    goto list_filter_apply_done;
  }

  list = list_filtered;
  list_size = list_filtered_size;

list_filter_apply_done:
  play_unblock(); /* Leaving critical section. */
  play_cancel();
  list_current = 0;
  play_execute(list[list_current]);
  return 0;
}

void list_destroy(void)
{
  int i;

  if (list_original != NULL) {
    for (i = 0; i < list_original_size; i++) {
      free(list_original[i]);
    }
    free(list_original);
    list_original = NULL;
  }

  if (list_filtered != NULL) {
    for (i = 0; i < list_filtered_size; i++) {
      free(list_filtered[i]);
    }
    free(list_filtered);
    list_filtered = NULL;
  }
}

