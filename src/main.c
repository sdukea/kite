#include <stdio.h>
#include <string.h>

#include "filesystem.h"
#include "commands.h"
#include "persistence.h"

#define DB_PATH "data/kite.db"
#define MAX_LINE_LEN 1024

int main(void)
{
    FileSystem fs;
    fs_init(&fs);

    FileSystem loaded;
    fs_init(&loaded);
    PersistError perr = load_filesystem(&loaded, DB_PATH);
    if (perr == PERSIST_OK) {
        fs_destroy(&fs);
        fs = loaded;
    } else {
        fs_destroy(&loaded);
        if (perr != PERSIST_ERR_IO) {
            fprintf(stderr, "kite: warning: could not load %s, starting with an empty filesystem\n", DB_PATH);
        }
    }

    char line[MAX_LINE_LEN];
    int should_exit = 0;

    while (!should_exit) {
        char pwd_buf[MAX_PATH_LEN];
        fs_pwd(&fs, pwd_buf, sizeof(pwd_buf));
        printf("kite:%s $ ", pwd_buf);
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) break;

        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';

        char *argv[MAX_ARGS];
        int argc = tokenize(line, argv, MAX_ARGS);
        if (argc <= 0) continue;

        dispatch_command(&fs, argc, argv, &should_exit);
    }

    save_filesystem(&fs, DB_PATH);
    fs_destroy(&fs);
    return 0;
}
