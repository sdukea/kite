#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "persistence.h"

#define MAX_RECORD_CONTENT_LEN (16 * 1024 * 1024) /* 16 MiB */

static void write_subtree(FILE *fp, const Node *node, const char *path_prefix)
{
    for (const Node *child = node->children; child != NULL; child = child->next) {
        char path[MAX_PATH_LEN];
        snprintf(path, sizeof(path), "%s/%s", path_prefix, child->name);

        if (child->type == DIRECTORY_NODE) {
            fprintf(fp, "D %zu 0\n%s\n", strlen(path), path);
            write_subtree(fp, child, path);
        } else {
            size_t content_len = (child->content != NULL) ? strlen(child->content) : 0;
            fprintf(fp, "F %zu %zu\n%s\n", strlen(path), content_len, path);
            if (content_len > 0) {
                fwrite(child->content, 1, content_len, fp);
            }
            fprintf(fp, "\n");
        }
    }
}

PersistError save_filesystem(const FileSystem *fs, const char *db_path)
{
    FILE *fp = fopen(db_path, "w");
    if (fp == NULL) return PERSIST_ERR_IO;

    fprintf(fp, "KITE_V1\n");
    write_subtree(fp, fs->root, "");

    fclose(fp);
    return PERSIST_OK;
}

static int expect_newline(FILE *fp)
{
    return fgetc(fp) == '\n';
}

PersistError load_filesystem(FileSystem *fs, const char *db_path)
{
    FILE *fp = fopen(db_path, "r");
    if (fp == NULL) return PERSIST_ERR_IO;

    char header[16];
    if (fgets(header, sizeof(header), fp) == NULL || strcmp(header, "KITE_V1\n") != 0) {
        fclose(fp);
        return PERSIST_ERR_MALFORMED;
    }

    for (;;) {
        char type;
        long path_len, content_len;
        int scanned = fscanf(fp, " %c %ld %ld", &type, &path_len, &content_len);
        if (scanned == EOF) break;
        if (scanned != 3) { fclose(fp); return PERSIST_ERR_MALFORMED; }
        if (type != 'D' && type != 'F') { fclose(fp); return PERSIST_ERR_MALFORMED; }
        if (path_len <= 0 || path_len >= MAX_PATH_LEN) { fclose(fp); return PERSIST_ERR_MALFORMED; }
        if (content_len < 0 || content_len > MAX_RECORD_CONTENT_LEN) { fclose(fp); return PERSIST_ERR_MALFORMED; }
        if (!expect_newline(fp)) { fclose(fp); return PERSIST_ERR_MALFORMED; }

        char path[MAX_PATH_LEN];
        if (fread(path, 1, (size_t)path_len, fp) != (size_t)path_len) { fclose(fp); return PERSIST_ERR_MALFORMED; }
        path[path_len] = '\0';
        if (!expect_newline(fp)) { fclose(fp); return PERSIST_ERR_MALFORMED; }

        char *content = NULL;
        if (content_len > 0) {
            content = malloc((size_t)content_len + 1);
            if (content == NULL) { fclose(fp); return PERSIST_ERR_MEMORY; }
            if (fread(content, 1, (size_t)content_len, fp) != (size_t)content_len) {
                free(content);
                fclose(fp);
                return PERSIST_ERR_MALFORMED;
            }
            content[content_len] = '\0';
        }

        if (type == 'F') {
            if (!expect_newline(fp)) { free(content); fclose(fp); return PERSIST_ERR_MALFORMED; }
        }

        FsError err;
        if (type == 'D') {
            err = fs_mkdir(fs, path);
        } else {
            err = fs_touch(fs, path);
            if (err == FS_OK && content_len > 0) {
                err = fs_write(fs, path, content);
            }
        }
        free(content);

        if (err != FS_OK) { fclose(fp); return PERSIST_ERR_MALFORMED; }
    }

    fclose(fp);
    return PERSIST_OK;
}
