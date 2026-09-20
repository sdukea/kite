#ifndef KITE_COMMANDS_H
#define KITE_COMMANDS_H

#include "filesystem.h"

#define MAX_ARGS 8

int tokenize(char *line, char *argv[], int max_args);
void dispatch_command(FileSystem *fs, int argc, char *argv[], int *should_exit);

#endif
