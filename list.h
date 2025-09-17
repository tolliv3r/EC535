#ifndef LIST_H
#define LIST_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t value;
    char     bin[33];   // 32 bits + NUL
    char     ascii[5];  // 4 chars + NUL
} Item;

typedef struct Node {
    Item         item;
    struct Node *next;
} Node;

void make_item(uint32_t x, Item *out);

void insert_sorted(Node **head, const Item *it);

void write_list(FILE *fout, const Node *head);

void free_list(Node *head);

#endif
