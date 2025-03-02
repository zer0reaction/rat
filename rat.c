#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

// TODO: check if files exist

typedef unsigned char char8_t;

typedef struct {
    size_t used;
    size_t capacity;
    void *data;
} Region;

Region region_alloc_alloc(size_t capacity)
{
    Region r = {
        .used = 0,
        .capacity = capacity,
        .data = malloc(capacity)
    };
    return r;
}

void *region_alloc(Region *r, size_t bytes)
{
    assert(r->used + bytes <= r->capacity);

    void *ptr = r->data + r->used;
    r->used += bytes;
    return ptr;
}

void region_free(Region *r)
{
    r->used = 0;
    r->capacity = 0;
    free(r->data);
}

size_t get_file_size(const char *path)
{
    FILE *file = fopen(path, "r");

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    fclose(file);
    return file_size;
}

void archive_files(const char *output_path, char **input_paths, size_t number_of_input_paths)
{
    FILE *output_file = fopen(output_path, "a");
    assert(output_file);

    fwrite(&number_of_input_paths, sizeof(number_of_input_paths), 1, output_file);

    for (size_t i = 0; i < number_of_input_paths; i++) {
        char *input_path = input_paths[i];
        FILE *input_file = fopen(input_path, "rb");
        assert(input_file);

        size_t input_path_size = strlen(input_path);
        size_t input_file_size = get_file_size(input_path);

        fwrite(&input_path_size, sizeof(input_path_size), 1, output_file);
        fwrite(input_path, input_path_size, 1, output_file);
        fwrite(&input_file_size, sizeof(input_file_size), 1, output_file);

        for (size_t i = 0; i < input_file_size; i++) {
            char8_t c = fgetc(input_file);
            fwrite(&c, 1, 1, output_file);
        }

        fclose(input_file);
    }

    fclose(output_file);
}

void extract_files(char *input_path)
{
    // [number of files: 64] [length of path: 64] [path: 64] [file size: 64] [contents: ?]
    Region r = region_alloc_alloc(1024 * 1024);

    FILE *input_file = fopen(input_path, "r");
    assert(input_file);

    size_t number_of_files;
    fread(&number_of_files, sizeof(number_of_files), 1, input_file);

    for (size_t i = 0; i < number_of_files; i++) {
        size_t length_of_path;
        fread(&length_of_path, sizeof(length_of_path), 1, input_file);

        char *path = region_alloc(&r, length_of_path + 1);
        fread(path, length_of_path, 1, input_file);
        path[length_of_path] = 0;

        size_t file_size;
        fread(&file_size, sizeof(file_size), 1, input_file);

        FILE *output_file = fopen(path, "a");
        assert(output_file);

        for (int i = 0; i < file_size; i++) {
            fputc(fgetc(input_file), output_file);
        }

        fclose(output_file);
    }

    fclose(input_file);
    region_free(&r);
}

int main(int argc, char **argv)
{
    if (strcmp("-a", argv[1]) == 0) {
        archive_files(argv[2], &(argv[3]), argc - 3);
    } else if (strcmp("-e", argv[1]) == 0) {
        extract_files(argv[2]);
    }

    return 0;
}
