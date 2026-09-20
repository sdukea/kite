#include <stdio.h>
#include <string.h>

#include "commands.h"

typedef void (*CommandHandler)(FileSystem *fs, int argc, char *argv[], int *should_exit);

typedef struct {
    const char *name;
    CommandHandler handler;
} Command;

int tokenize(char *line, char *argv[], int max_args)
{
    int argc = 0;
    char *p = line;

    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        if (argc >= max_args) {
            fprintf(stderr, "kite: too many arguments\n");
            return -1;
        }

        if (*p == '"') {
            p++; /* consume opening quote */
            argv[argc++] = p;
            char *write_ptr = p;
            while (*p != '"' && *p != '\0') { *write_ptr++ = *p++; }
            if (*p == '\0') {
                fprintf(stderr, "kite: unterminated quoted string\n");
                return -1;
            }
            *write_ptr = '\0';
            p++; /* consume closing quote */
        } else {
            argv[argc++] = p;
            while (*p != '\0' && *p != ' ' && *p != '\t') p++;
            if (*p != '\0') { *p = '\0'; p++; }
        }
    }
    return argc;
}

static void cmd_mkdir(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit;
    if (argc != 2) { fprintf(stderr, "kite: mkdir: usage: mkdir <path>\n"); return; }
    FsError err = fs_mkdir(fs, argv[1]);
    if (err != FS_OK) fprintf(stderr, "kite: mkdir: %s: %s\n", argv[1], fs_error_message(err));
}

static void cmd_touch(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit;
    if (argc != 2) { fprintf(stderr, "kite: touch: usage: touch <path>\n"); return; }
    FsError err = fs_touch(fs, argv[1]);
    if (err != FS_OK) fprintf(stderr, "kite: touch: %s: %s\n", argv[1], fs_error_message(err));
}

static void cmd_ls(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit; (void)argv;
    if (argc != 1) { fprintf(stderr, "kite: ls: usage: ls\n"); return; }
    fs_ls(fs);
}

static void cmd_cd(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit;
    if (argc != 2) { fprintf(stderr, "kite: cd: usage: cd <path>\n"); return; }
    FsError err = fs_cd(fs, argv[1]);
    if (err != FS_OK) fprintf(stderr, "kite: cd: %s: %s\n", argv[1], fs_error_message(err));
}

static void cmd_pwd(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit; (void)argv;
    if (argc != 1) { fprintf(stderr, "kite: pwd: usage: pwd\n"); return; }
    char buf[MAX_PATH_LEN];
    fs_pwd(fs, buf, sizeof(buf));
    printf("%s\n", buf);
}

static void cmd_tree(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit; (void)argv;
    if (argc != 1) { fprintf(stderr, "kite: tree: usage: tree\n"); return; }
    fs_tree(fs);
}

static void cmd_rm(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit;
    if (argc != 2) { fprintf(stderr, "kite: rm: usage: rm <path>\n"); return; }
    FsError err = fs_rm(fs, argv[1]);
    if (err != FS_OK) fprintf(stderr, "kite: rm: %s: %s\n", argv[1], fs_error_message(err));
}

static void cmd_write(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit;
    if (argc != 3) { fprintf(stderr, "kite: write: usage: write <path> <content>\n"); return; }
    FsError err = fs_write(fs, argv[1], argv[2]);
    if (err != FS_OK) fprintf(stderr, "kite: write: %s: %s\n", argv[1], fs_error_message(err));
}

static void cmd_cat(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit;
    if (argc != 2) { fprintf(stderr, "kite: cat: usage: cat <path>\n"); return; }
    const char *content = NULL;
    FsError err = fs_read(fs, argv[1], &content);
    if (err != FS_OK) { fprintf(stderr, "kite: cat: %s: %s\n", argv[1], fs_error_message(err)); return; }
    printf("%s", content);
    if (content[0] != '\0') printf("\n");
}

static void cmd_rename(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit;
    if (argc != 3) { fprintf(stderr, "kite: rename: usage: rename <path> <new-name>\n"); return; }
    FsError err = fs_rename(fs, argv[1], argv[2]);
    if (err != FS_OK) fprintf(stderr, "kite: rename: %s: %s\n", argv[1], fs_error_message(err));
}

static void cmd_find(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)should_exit;
    if (argc != 2) { fprintf(stderr, "kite: find: usage: find <name>\n"); return; }
    int count = 0;
    fs_find(fs, argv[1], &count);
    if (count == 0) fprintf(stderr, "kite: find: %s: not found\n", argv[1]);
}

static void cmd_exit(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)fs; (void)argc; (void)argv;
    *should_exit = 1;
}

static void cmd_help(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    (void)fs; (void)argc; (void)argv; (void)should_exit;
    printf("Commands: mkdir touch ls cd pwd tree rm write cat rename find help exit\n");
}

static const Command COMMANDS[] = {
    { "mkdir",  cmd_mkdir  },
    { "touch",  cmd_touch  },
    { "ls",     cmd_ls     },
    { "cd",     cmd_cd     },
    { "pwd",    cmd_pwd    },
    { "tree",   cmd_tree   },
    { "rm",     cmd_rm     },
    { "write",  cmd_write  },
    { "cat",    cmd_cat    },
    { "rename", cmd_rename },
    { "find",   cmd_find   },
    { "help",   cmd_help   },
    { "exit",   cmd_exit   },
    { "quit",   cmd_exit   },
};

#define NUM_COMMANDS (int)(sizeof(COMMANDS) / sizeof(COMMANDS[0]))

void dispatch_command(FileSystem *fs, int argc, char *argv[], int *should_exit)
{
    if (argc <= 0) return;

    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(argv[0], COMMANDS[i].name) == 0) {
            COMMANDS[i].handler(fs, argc, argv, should_exit);
            return;
        }
    }
    fprintf(stderr, "kite: %s: command not found\n", argv[0]);
}
