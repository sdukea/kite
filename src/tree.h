#ifndef KITE_TREE_H
#define KITE_TREE_H

#define MAX_NAME_LEN 127

typedef enum {
    FILE_NODE,
    DIRECTORY_NODE
} NodeType;

typedef struct Node {
    char name[MAX_NAME_LEN + 1]; /* NUL-terminated component name */
    NodeType type;
    char *content;               /* heap content; NULL for directories */

    struct Node *parent;         /* NULL only for the root */
    struct Node *children;       /* first child, or NULL */
    struct Node *next;           /* next sibling, or NULL */
} Node;

typedef void (*NodeVisitor)(Node *node, void *ctx);

int is_valid_name(const char *name);
Node *create_node(const char *name, NodeType type);
int add_child(Node *parent, Node *child);
Node *find_child(Node *parent, const char *name);
int remove_child(Node *parent, Node *child);
void free_tree(Node *node);
void print_tree(const Node *node, const char *prefix);
int find_in_tree(Node *node, const char *name, NodeVisitor visit, void *ctx);

#endif
