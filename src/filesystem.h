#ifndef KITE_FILESYSTEM_H
#define KITE_FILESYSTEM_H

#include <stddef.h>

#include "tree.h"

#define MAX_PATH_LEN 4096

typedef enum {
    FS_OK = 0,
    FS_ERR_NOT_FOUND,
    FS_ERR_NOT_DIR,
    FS_ERR_IS_DIR,
    FS_ERR_IS_CWD,
    FS_ERR_ALREADY_EXISTS,
    FS_ERR_NOT_EMPTY,
    FS_ERR_ROOT,
    FS_ERR_MEMORY,
    FS_ERR_INVALID_NAME,
    FS_ERR_INVALID_PATH
} FsError;

typedef struct {
    Node *root;
    Node *cwd;
} FileSystem;

void fs_init(FileSystem *fs);
void fs_destroy(FileSystem *fs);

FsError fs_resolve(const FileSystem *fs, const char *path, Node **out);
FsError fs_resolve_parent(const FileSystem *fs, const char *path,
                           Node **out_parent, char *out_name, size_t name_size);

FsError fs_mkdir(FileSystem *fs, const char *path);
FsError fs_touch(FileSystem *fs, const char *path);
FsError fs_rm(FileSystem *fs, const char *path);
FsError fs_cd(FileSystem *fs, const char *path);
FsError fs_write(FileSystem *fs, const char *path, const char *content);
FsError fs_read(const FileSystem *fs, const char *path, const char **out_content);
FsError fs_rename(FileSystem *fs, const char *path, const char *new_name);
FsError fs_find(const FileSystem *fs, const char *name, int *out_count);

void fs_ls(const FileSystem *fs);
char *fs_pwd(const FileSystem *fs, char *buf, size_t buf_size);
void fs_tree(const FileSystem *fs);

const char *fs_error_message(FsError err);

#endif
