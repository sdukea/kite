#ifndef KITE_PERSISTENCE_H
#define KITE_PERSISTENCE_H

#include "filesystem.h"

typedef enum {
    PERSIST_OK = 0,
    PERSIST_ERR_IO,
    PERSIST_ERR_MALFORMED,
    PERSIST_ERR_MEMORY
} PersistError;

PersistError save_filesystem(const FileSystem *fs, const char *db_path);
PersistError load_filesystem(FileSystem *fs, const char *db_path);

#endif
