# Kite

Kite is a small program that behaves like a miniature Unix filesystem. Run it and you
get an interactive prompt (`kite:/ $`) supporting familiar commands — `mkdir`, `cd`,
`touch`, `ls`, `cat`, `write`, `rm`, `rename`, `find`, `tree`, `pwd` — to build a tree of
directories and files entirely in memory. On exit, the tree is serialized to
`data/kite.db` on real disk, and reloaded automatically the next time Kite starts.

> **Naming note:** this project was designed under the working title "TreeFS";
> [`Guide.pdf`](Guide.pdf) is the original design document and still uses that name
> (and the `treefs` binary/prompt) throughout its examples. The project itself is
> named **Kite** — the source, binary, database format, and prompt below all use
> that name.

## Building and running

```
make        # builds ./kite
./kite      # start the interactive shell
make test   # builds and runs tests/test_kite.c
make clean  # remove build artifacts
```

## Layout

| File | Responsibility |
|---|---|
| `src/tree.h` / `src/tree.c` | The `Node` struct and pure tree operations: create, link, unlink, free, print, search. Knows nothing about paths, the current directory, or commands. |
| `src/filesystem.h` / `src/filesystem.c` | The `FileSystem` struct (root + cwd), path resolution (`fs_resolve`, `fs_resolve_parent`), and every user-facing `fs_*` operation. |
| `src/commands.h` / `src/commands.c` | Tokenizing a raw input line (including quoted arguments) and dispatching it to the right `fs_*` call. |
| `src/persistence.h` / `src/persistence.c` | The on-disk, length-prefixed record format; reads and writes only through the `fs_*` functions above, never by constructing a `Node` directly. |
| `src/main.c` | The read-execute loop, plus load-at-startup and save-at-exit. |
| `tests/test_kite.c` | A standalone, assertion-based test suite linked directly against `tree.c`, `filesystem.c`, and `persistence.c`. |

See [`Guide.pdf`](Guide.pdf) for the full design write-up: data structures, C memory
management, recursion, path resolution, the persistence format, complexity analysis,
and the alternative designs that were considered and rejected.

## Example session

```
kite:/ $ mkdir projects
kite:/ $ cd projects
kite:/projects $ mkdir dsa
kite:/projects $ touch README.txt
kite:/projects $ write README.txt "hello from Kite"
kite:/projects $ cd dsa
kite:/projects/dsa $ touch tree.c
kite:/projects/dsa $ cd ..
kite:/projects $ tree
/
└── projects/
    ├── dsa/
    │   └── tree.c
    └── README.txt
kite:/projects $ exit
```
