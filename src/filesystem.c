#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filesystem.h"

void fs_init(FileSystem *fs)
{
    Node *root = calloc(1, sizeof(Node));
    if (root == NULL) {
        fprintf(stderr, "kite: fatal: out of memory while initializing filesystem\n");
        exit(1);
    }

    strncpy(root->name, "/", MAX_NAME_LEN);
    root->name[MAX_NAME_LEN] = '\0';
    root->type = DIRECTORY_NODE;
    root->content = NULL;
    root->parent = NULL;
    root->children = NULL;
    root->next = NULL;

    fs->root = root;
    fs->cwd = root;
}

void fs_destroy(FileSystem *fs)
{
    if (fs == NULL || fs->root == NULL) return;

    Node *child = fs->root->children;
    while (child != NULL) {
        Node *next = child->next;
        free_tree(child);
        child = next;
    }

    free(fs->root);
    fs->root = NULL;
    fs->cwd = NULL;
}

static FsError resolve_from(Node *start, const char *path, Node **out)
{
    char buf[MAX_PATH_LEN];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    Node *current = start;
    char *saveptr = NULL;
    char *token = strtok_r(buf, "/", &saveptr);

    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            /* stay in place */
        } else if (strcmp(token, "..") == 0) {
            if (current->parent != NULL) current = current->parent;
        } else {
            if (current->type != DIRECTORY_NODE) return FS_ERR_NOT_DIR;
            Node *next = find_child(current, token);
            if (next == NULL) return FS_ERR_NOT_FOUND;
            current = next;
        }
        token = strtok_r(NULL, "/", &saveptr);
    }

    *out = current;
    return FS_OK;
}

FsError fs_resolve(const FileSystem *fs, const char *path, Node **out)
{
    if (fs == NULL || path == NULL || out == NULL) return FS_ERR_INVALID_PATH;
    if (path[0] == '\0') return FS_ERR_INVALID_PATH;

    Node *start = (path[0] == '/') ? fs->root : fs->cwd;
    return resolve_from(start, path, out);
}

FsError fs_resolve_parent(const FileSystem *fs, const char *path,
                           Node **out_parent, char *out_name, size_t name_size)
{
    if (fs == NULL || path == NULL || out_parent == NULL || out_name == NULL) {
        return FS_ERR_INVALID_PATH;
    }
    if (path[0] == '\0') return FS_ERR_INVALID_PATH;

    char buf[MAX_PATH_LEN];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *last_slash = strrchr(buf, '/');
    const char *name;
    Node *parent;

    if (last_slash == NULL) {
        parent = fs->cwd;
        name = buf;
    } else {
        name = last_slash + 1;
        if (name[0] == '\0') return FS_ERR_INVALID_PATH;
        *last_slash = '\0';

        if (buf[0] == '\0') {
            parent = fs->root;
        } else {
            FsError err = fs_resolve(fs, buf, &parent);
            if (err != FS_OK) return err;
        }
    }

    if (!is_valid_name(name)) return FS_ERR_INVALID_NAME;
    if (parent->type != DIRECTORY_NODE) return FS_ERR_NOT_DIR;

    strncpy(out_name, name, name_size - 1);
    out_name[name_size - 1] = '\0';
    *out_parent = parent;
    return FS_OK;
}

static FsError create_entry(FileSystem *fs, const char *path, NodeType type)
{
    Node *parent;
    char name[MAX_NAME_LEN + 1];

    FsError err = fs_resolve_parent(fs, path, &parent, name, sizeof(name));
    if (err != FS_OK) return err;
    if (find_child(parent, name) != NULL) return FS_ERR_ALREADY_EXISTS;

    Node *node = create_node(name, type);
    if (node == NULL) return FS_ERR_MEMORY;

    if (!add_child(parent, node)) {
        free_tree(node);
        return FS_ERR_ALREADY_EXISTS;
    }
    return FS_OK;
}

FsError fs_mkdir(FileSystem *fs, const char *path) { return create_entry(fs, path, DIRECTORY_NODE); }
FsError fs_touch(FileSystem *fs, const char *path) { return create_entry(fs, path, FILE_NODE); }

FsError fs_rm(FileSystem *fs, const char *path)
{
    Node *target;
    FsError err = fs_resolve(fs, path, &target);
    if (err != FS_OK) return err;

    if (target == fs->root) return FS_ERR_ROOT;
    if (target->type == DIRECTORY_NODE && target->children != NULL) return FS_ERR_NOT_EMPTY;
    if (target == fs->cwd) return FS_ERR_IS_CWD;

    if (!remove_child(target->parent, target)) return FS_ERR_NOT_FOUND;
    free_tree(target);
    return FS_OK;
}

FsError fs_cd(FileSystem *fs, const char *path)
{
    Node *target;
    FsError err = fs_resolve(fs, path, &target);
    if (err != FS_OK) return err;
    if (target->type != DIRECTORY_NODE) return FS_ERR_NOT_DIR;

    fs->cwd = target;
    return FS_OK;
}

FsError fs_write(FileSystem *fs, const char *path, const char *content)
{
    Node *target;
    FsError err = fs_resolve(fs, path, &target);
    if (err != FS_OK) return err;
    if (target->type != FILE_NODE) return FS_ERR_IS_DIR;

    size_t len = strlen(content != NULL ? content : "");
    char *copy = malloc(len + 1);
    if (copy == NULL) return FS_ERR_MEMORY;
    memcpy(copy, content != NULL ? content : "", len + 1);

    free(target->content); /* release whatever was there before */
    target->content = copy;
    return FS_OK;
}

FsError fs_read(const FileSystem *fs, const char *path, const char **out_content)
{
    Node *target;
    FsError err = fs_resolve(fs, path, &target);
    if (err != FS_OK) return err;
    if (target->type != FILE_NODE) return FS_ERR_IS_DIR;

    *out_content = (target->content != NULL) ? target->content : "";
    return FS_OK;
}

FsError fs_rename(FileSystem *fs, const char *path, const char *new_name)
{
    Node *target;
    FsError err = fs_resolve(fs, path, &target);
    if (err != FS_OK) return err;

    if (target == fs->root) return FS_ERR_ROOT;
    if (!is_valid_name(new_name)) return FS_ERR_INVALID_NAME;
    if (find_child(target->parent, new_name) != NULL) return FS_ERR_ALREADY_EXISTS;

    strncpy(target->name, new_name, MAX_NAME_LEN);
    target->name[MAX_NAME_LEN] = '\0';
    return FS_OK;
}

static void print_node_full_path(Node *node, void *ctx)
{
    (void)ctx;

    const Node *chain[256];
    int depth = 0;
    const Node *cur = node;

    while (cur != NULL && cur->parent != NULL) {
        if (depth < 256) chain[depth++] = cur;
        cur = cur->parent;
    }

    if (depth == 0) {
        printf("/\n");
        return;
    }

    for (int i = depth - 1; i >= 0; i--) {
        printf("/%s", chain[i]->name);
    }
    printf("%s\n", node->type == DIRECTORY_NODE ? "/" : "");
}

FsError fs_find(const FileSystem *fs, const char *name, int *out_count)
{
    if (fs == NULL || name == NULL) return FS_ERR_INVALID_PATH;

    int count = find_in_tree(fs->root, name, print_node_full_path, NULL);
    if (out_count != NULL) *out_count = count;
    return FS_OK;
}

void fs_ls(const FileSystem *fs)
{
    for (Node *child = fs->cwd->children; child != NULL; child = child->next) {
        printf("%s%s\n", child->name, child->type == DIRECTORY_NODE ? "/" : "");
    }
}

char *fs_pwd(const FileSystem *fs, char *buf, size_t buf_size)
{
    const Node *chain[256];
    int depth = 0;
    const Node *cur = fs->cwd;

    while (cur != NULL && cur != fs->root) {
        if (depth < 256) chain[depth++] = cur;
        cur = cur->parent;
    }

    if (depth == 0) {
        snprintf(buf, buf_size, "/");
        return buf;
    }

    size_t pos = 0;
    for (int i = depth - 1; i >= 0; i--) {
        int written = snprintf(buf + pos, buf_size - pos, "/%s", chain[i]->name);
        if (written < 0 || (size_t)written >= buf_size - pos) break;
        pos += (size_t)written;
    }
    return buf;
}

void fs_tree(const FileSystem *fs)
{
    printf("/\n");
    print_tree(fs->root, "");
}

const char *fs_error_message(FsError err)
{
    switch (err) {
        case FS_OK: return "success";
        case FS_ERR_NOT_FOUND: return "no such file or directory";
        case FS_ERR_NOT_DIR: return "not a directory";
        case FS_ERR_IS_DIR: return "is a directory";
        case FS_ERR_IS_CWD: return "cannot remove the current directory";
        case FS_ERR_ALREADY_EXISTS: return "already exists";
        case FS_ERR_NOT_EMPTY: return "directory not empty";
        case FS_ERR_ROOT: return "operation not permitted on root";
        case FS_ERR_MEMORY: return "out of memory";
        case FS_ERR_INVALID_NAME: return "invalid name";
        case FS_ERR_INVALID_PATH: return "invalid path";
        default: return "unknown error";
    }
}
