#ifndef _LIST_H
#define _LIST_H

int list_import_file(char *path);
void list_request(int no);
void list_request_next(void);
char *list_get(int no, int *playing);
int list_swap(int no_a, int no_b);
void list_shuffle(void);
int list_size_get(void);
int list_filter_apply(char *filter);
void list_destroy(void);

#endif /* _LIST_H */
