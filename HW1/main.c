#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "bits.h"
#include "list.h"

// problem 1 main.c
static int run_bits(const char *in_path, const char *out_path)
{
    FILE *fin = fopen(in_path, "r");
    if (!fin) {
        perror("Couldn't open input file");
        return EXIT_FAILURE;
    }

    FILE *fout = fopen(out_path, "w");
    if (!fout) {
        perror("failed to open output file");
        fclose(fin);
        return EXIT_FAILURE;
    }

    unsigned int temp;
    while (fscanf(fin, "%u", &temp) == 1) {
        uint32_t x = (uint32_t)temp;

        uint32_t mirror = BinaryMirror(x);
        uint32_t seq010 = CountSequence(x);

        fprintf(fout, "%" PRIu32 " %" PRIu32 "\n", mirror, seq010);
    }

    if (ferror(fin)) {
        perror("couldn't read input file");
        fclose(fin);
        fclose(fout);
        return EXIT_FAILURE;
    }

    fclose(fin);
    fclose(fout);
    return EXIT_SUCCESS;
}

// problem 2 main.c
static int run_list(const char *in_path, const char *out_path)
{
    FILE *fin = fopen(in_path, "r");
    if (!fin) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }
    FILE *fout = fopen(out_path, "w");
    if (!fout) {
        perror("Failed to open output file");
        fclose(fin);
        return EXIT_FAILURE;
    }

    Node *head = NULL;

    unsigned int tmp;
    while (fscanf(fin, "%u", &tmp) == 1) {
        Item it;
        make_item((uint32_t)tmp, &it);
        insert_sorted(&head, &it);
    }

    if (ferror(fin)) {
        perror("Error readng input file");
        free_list(head);
        fclose(fin);
        fclose(fout);
        return EXIT_FAILURE;
    }

    write_list(fout, head);

    free_list(head);
    fclose(fin);
    fclose(fout);
    return EXIT_SUCCESS;
}

// combined main function, for list mode add "-l"
int main(int argc, char *argv[])
{
    if (argc == 3) 
    {
        return run_bits(argv[1], argv[2]);
    }
    else if (argc == 4 && (strcmp(argv[1], "-l") == 0)) 
    {
        return run_list(argv[2], argv[3]);
    }
}
