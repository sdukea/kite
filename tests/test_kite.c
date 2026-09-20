#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/filesystem.h"
#include "../src/persistence.h"

static void test_root_creation(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs.root != NULL);
    assert(fs.root == fs.cwd);
    assert(fs.root->type == DIRECTORY_NODE);
    assert(fs.root->parent == NULL);

    fs_destroy(&fs);
    printf("test_root_creation passed\n");
}

static void test_directory_and_file_creation(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_mkdir(&fs, "projects") == FS_OK);
    assert(fs_touch(&fs, "README.txt") == FS_OK);

    Node *n;
    assert(fs_resolve(&fs, "projects", &n) == FS_OK);
    assert(n->type == DIRECTORY_NODE);
    assert(fs_resolve(&fs, "README.txt", &n) == FS_OK);
    assert(n->type == FILE_NODE);

    fs_destroy(&fs);
    printf("test_directory_and_file_creation passed\n");
}

static void test_duplicate_creation(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_mkdir(&fs, "dsa") == FS_OK);
    assert(fs_mkdir(&fs, "dsa") == FS_ERR_ALREADY_EXISTS);
    assert(fs_touch(&fs, "dsa") == FS_ERR_ALREADY_EXISTS);

    fs_destroy(&fs);
    printf("test_duplicate_creation passed\n");
}

static void test_absolute_and_relative_navigation(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_mkdir(&fs, "/projects") == FS_OK);
    assert(fs_mkdir(&fs, "/projects/dsa") == FS_OK);
    assert(fs_cd(&fs, "projects") == FS_OK);
    assert(fs_cd(&fs, "dsa") == FS_OK);

    char buf[MAX_PATH_LEN];
    fs_pwd(&fs, buf, sizeof(buf));
    assert(strcmp(buf, "/projects/dsa") == 0);

    assert(fs_cd(&fs, "/") == FS_OK);
    fs_pwd(&fs, buf, sizeof(buf));
    assert(strcmp(buf, "/") == 0);

    fs_destroy(&fs);
    printf("test_absolute_and_relative_navigation passed\n");
}

static void test_parent_navigation(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_mkdir(&fs, "/a") == FS_OK);
    assert(fs_mkdir(&fs, "/a/b") == FS_OK);
    assert(fs_cd(&fs, "/a/b") == FS_OK);
    assert(fs_cd(&fs, "..") == FS_OK);

    char buf[MAX_PATH_LEN];
    fs_pwd(&fs, buf, sizeof(buf));
    assert(strcmp(buf, "/a") == 0);

    assert(fs_cd(&fs, "../..") == FS_OK);
    fs_pwd(&fs, buf, sizeof(buf));
    assert(strcmp(buf, "/") == 0);

    fs_destroy(&fs);
    printf("test_parent_navigation passed\n");
}

static void test_file_write_and_read(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_touch(&fs, "notes.txt") == FS_OK);

    const char *content = NULL;
    assert(fs_read(&fs, "notes.txt", &content) == FS_OK);
    assert(strcmp(content, "") == 0);

    assert(fs_write(&fs, "notes.txt", "hello world") == FS_OK);
    assert(fs_read(&fs, "notes.txt", &content) == FS_OK);
    assert(strcmp(content, "hello world") == 0);

    assert(fs_write(&fs, "notes.txt", "overwritten") == FS_OK);
    assert(fs_read(&fs, "notes.txt", &content) == FS_OK);
    assert(strcmp(content, "overwritten") == 0);

    fs_destroy(&fs);
    printf("test_file_write_and_read passed\n");
}

static void test_deletion(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_mkdir(&fs, "empty") == FS_OK);
    assert(fs_rm(&fs, "empty") == FS_OK);

    Node *n;
    assert(fs_resolve(&fs, "empty", &n) == FS_ERR_NOT_FOUND);

    assert(fs_mkdir(&fs, "full") == FS_OK);
    assert(fs_touch(&fs, "full/file.txt") == FS_OK);
    assert(fs_rm(&fs, "full") == FS_ERR_NOT_EMPTY);
    assert(fs_rm(&fs, "full/file.txt") == FS_OK);

    assert(fs_cd(&fs, "full") == FS_OK);
    assert(fs_rm(&fs, "/full") == FS_ERR_IS_CWD);
    assert(fs_cd(&fs, "/") == FS_OK);
    assert(fs_rm(&fs, "full") == FS_OK);

    assert(fs_rm(&fs, "/") == FS_ERR_ROOT);

    fs_destroy(&fs);
    printf("test_deletion passed\n");
}

static void test_rename(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_mkdir(&fs, "old") == FS_OK);
    assert(fs_rename(&fs, "old", "newname") == FS_OK);

    Node *n;
    assert(fs_resolve(&fs, "old", &n) == FS_ERR_NOT_FOUND);
    assert(fs_resolve(&fs, "newname", &n) == FS_OK);

    assert(fs_touch(&fs, "taken") == FS_OK);
    assert(fs_rename(&fs, "newname", "taken") == FS_ERR_ALREADY_EXISTS);

    fs_destroy(&fs);
    printf("test_rename passed\n");
}

static void test_find_recursive(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_mkdir(&fs, "/a") == FS_OK);
    assert(fs_mkdir(&fs, "/a/b") == FS_OK);
    assert(fs_touch(&fs, "/a/b/target.txt") == FS_OK);
    assert(fs_touch(&fs, "/target.txt") == FS_OK);

    int count = 0;
    assert(fs_find(&fs, "target.txt", &count) == FS_OK);
    assert(count == 2);

    count = 0;
    assert(fs_find(&fs, "nonexistent.txt", &count) == FS_OK);
    assert(count == 0);

    fs_destroy(&fs);
    printf("test_find_recursive passed\n");
}

static void test_invalid_paths_and_names(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_mkdir(&fs, "") == FS_ERR_INVALID_PATH);
    assert(fs_mkdir(&fs, ".") == FS_ERR_INVALID_NAME);
    assert(fs_mkdir(&fs, "..") == FS_ERR_INVALID_NAME);

    Node *n;
    assert(fs_resolve(&fs, "/nowhere", &n) == FS_ERR_NOT_FOUND);

    assert(fs_touch(&fs, "file.txt") == FS_OK);
    assert(fs_cd(&fs, "file.txt") == FS_ERR_NOT_DIR);
    assert(fs_resolve(&fs, "file.txt/inside", &n) == FS_ERR_NOT_DIR);

    fs_destroy(&fs);
    printf("test_invalid_paths_and_names passed\n");
}

static void test_recursive_tree_traversal(void)
{
    FileSystem fs;
    fs_init(&fs);

    assert(fs_mkdir(&fs, "/projects") == FS_OK);
    assert(fs_mkdir(&fs, "/projects/dsa") == FS_OK);
    assert(fs_touch(&fs, "/projects/dsa/tree.c") == FS_OK);
    assert(fs_touch(&fs, "/projects/dsa/stack.c") == FS_OK);
    assert(fs_mkdir(&fs, "/projects/ml") == FS_OK);

    fs_tree(&fs); /* smoke test: must traverse the whole tree without crashing */

    fs_destroy(&fs);
    printf("test_recursive_tree_traversal passed\n");
}

static void test_persistence_round_trip(void)
{
    const char *path = "/tmp/kite_test_db.tmp";

    FileSystem fs;
    fs_init(&fs);
    assert(fs_mkdir(&fs, "/projects") == FS_OK);
    assert(fs_mkdir(&fs, "/projects/dsa") == FS_OK);
    assert(fs_touch(&fs, "/projects/dsa/tree.c") == FS_OK);
    assert(fs_write(&fs, "/projects/dsa/tree.c", "hello from test") == FS_OK);

    assert(save_filesystem(&fs, path) == PERSIST_OK);
    fs_destroy(&fs);

    FileSystem loaded;
    fs_init(&loaded);
    assert(load_filesystem(&loaded, path) == PERSIST_OK);

    Node *n;
    assert(fs_resolve(&loaded, "/projects/dsa/tree.c", &n) == FS_OK);
    const char *content = NULL;
    assert(fs_read(&loaded, "/projects/dsa/tree.c", &content) == FS_OK);
    assert(strcmp(content, "hello from test") == 0);

    fs_destroy(&loaded);
    remove(path);
    printf("test_persistence_round_trip passed\n");
}

static void test_persistence_missing_and_malformed_file(void)
{
    FileSystem fs;
    fs_init(&fs);
    assert(load_filesystem(&fs, "/tmp/kite_does_not_exist.db") == PERSIST_ERR_IO);
    fs_destroy(&fs);

    const char *bad_path = "/tmp/kite_bad_db.tmp";
    FILE *fp = fopen(bad_path, "w");
    assert(fp != NULL);
    fprintf(fp, "NOT_A_VALID_HEADER\n");
    fclose(fp);

    FileSystem fs2;
    fs_init(&fs2);
    assert(load_filesystem(&fs2, bad_path) == PERSIST_ERR_MALFORMED);
    fs_destroy(&fs2);
    remove(bad_path);

    printf("test_persistence_missing_and_malformed_file passed\n");
}

int main(void)
{
    test_root_creation();
    test_directory_and_file_creation();
    test_duplicate_creation();
    test_absolute_and_relative_navigation();
    test_parent_navigation();
    test_file_write_and_read();
    test_deletion();
    test_rename();
    test_find_recursive();
    test_invalid_paths_and_names();
    test_recursive_tree_traversal();
    test_persistence_round_trip();
    test_persistence_missing_and_malformed_file();

    printf("\nAll tests passed.\n");
    return 0;
}
