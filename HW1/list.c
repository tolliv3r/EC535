#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "list.h"

static void u32_to_bin(uint32_t x, char out[33]) {
    for (int i = 31; i >= 0; --i) {
        out[31 - i] = ((x >> i) & 1u) ? '1' : '0';
    }
    out[32] = '\0';
}

static void u32_to_ascii4(uint32_t x, char out[5])
{
    unsigned char b3 = (unsigned char)((x >> 24) & 0xFFu);
    unsigned char b2 = (unsigned char)((x >> 16) & 0xFFu);
    unsigned char b1 = (unsigned char)((x >>  8) & 0xFFu);
    unsigned char b0 = (unsigned char)((x >>  0) & 0xFFu);

    unsigned char bytes[4] = { b3, b2, b1, b0 };
    for (int i = 0; i < 4; ++i) {
        unsigned char c = bytes[i];
        out[i] = (c >= 32 && c <= 126) ? (char)c : '.';
    }
    out[4] = '\0';
}

void make_item(uint32_t x, Item *out) {
    if (!out) return;
    out->value = x;
    u32_to_bin(x, out->bin);
    u32_to_ascii4(x, out->ascii);
}

void insert_sorted(Node **head, const Item *it)
{
    if (!head || !it) return;

    Node *n = (Node*)malloc(sizeof(Node));
    if (!n) {
        fprintf(stderr, "No more memory!!!!\n");
        exit(EXIT_FAILURE);
    }
    n->item = *it;
    n->next = NULL;

    if (*head == NULL || strcmp(it->ascii, (*head)->item.ascii) < 0) {
        n->next = *head;
        *head = n;
        return;
    }

    Node *cur = *head;
    while (cur->next && strcmp(cur->next->item.ascii, it->ascii) <= 0) {
        cur = cur->next;
    }
    n->next = cur->next;
    cur->next = n;
}

void write_list(FILE *fout, const Node *head) {
    for (const Node *p = head; p != NULL; p = p->next) {
        fprintf(fout, "%s %u %s\n", p->item.ascii, p->item.value, p->item.bin);
    }
}

void free_list(Node *head) {
    while (head) {
        Node *next = head->next;
        free(head);
        head = next;
    }
}

