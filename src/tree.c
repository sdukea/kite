#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"

int is_valid_name(const char *name)
{
    if (name == NULL || name[0] == '\0') return 0;
    if (strlen(name) > MAX_NAME_LEN) return 0;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return 0;
    if (strchr(name, '/') != NULL) return 0;
    return 1;
}

Node *create_node(const char *name, NodeType type)
{
    if (!is_valid_name(name)) {
        return NULL;
    }

    Node *node = calloc(1, sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "kite: error: out of memory while creating node\n");
        return NULL;
    }

    strncpy(node->name, name, MAX_NAME_LEN);
    node->name[MAX_NAME_LEN] = '\0';

    node->type = type;
    node->content = NULL;
    node->parent = NULL;
    node->children = NULL;
    node->next = NULL;

    return node;
}

int add_child(Node *parent, Node *child)
{
    if (parent == NULL || child == NULL) return 0;
    if (parent->type != DIRECTORY_NODE) return 0;
    if (find_child(parent, child->name) != NULL) return 0; /* duplicate */

    child->parent = parent;
    child->next = NULL;

    if (parent->children == NULL) {
        parent->children = child;
        return 1;
    }

    Node *cursor = parent->children;
    while (cursor->next != NULL) {
        cursor = cursor->next;
    }
    cursor->next = child;
    return 1;
}

Node *find_child(Node *parent, const char *name)
{
    if (parent == NULL || name == NULL) return NULL;

    for (Node *cursor = parent->children; cursor != NULL; cursor = cursor->next) {
        if (strcmp(cursor->name, name) == 0) {
            return cursor;
        }
    }
    return NULL;
}

int remove_child(Node *parent, Node *child)
{
    if (parent == NULL || child == NULL) return 0;

    if (parent->children == child) {
        parent->children = child->next;
        child->next = NULL;
        child->parent = NULL;
        return 1;
    }

    Node *cursor = parent->children;
    while (cursor != NULL && cursor->next != child) cursor = cursor->next;
    if (cursor == NULL) return 0;

    cursor->next = child->next;
    child->next = NULL;
    child->parent = NULL;
    return 1;
}

void free_tree(Node *node)
{
    if (node == NULL) return;

    Node *child = node->children;
    while (child != NULL) {
        Node *next = child->next; /* save before recursing: child is about to be freed */
        free_tree(child);
        child = next;
    }

    free(node->content);
    free(node);
}

void print_tree(const Node *node, const char *prefix)
{
    Node *child = node->children;
    while (child != NULL) {
        int is_last = (child->next == NULL);
        const char *connector = is_last ? "└── " : "├── ";

        printf("%s%s%s%s\n", prefix, connector, child->name,
               child->type == DIRECTORY_NODE ? "/" : "");

        if (child->type == DIRECTORY_NODE) {
            char child_prefix[4096];
            snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix,
                     is_last ? "    " : "│   ");
            print_tree(child, child_prefix);
        }
        child = child->next;
    }
}

int find_in_tree(Node *node, const char *name, NodeVisitor visit, void *ctx)
{
    int count = 0;
    for (Node *child = node->children; child != NULL; child = child->next) {
        if (strcmp(child->name, name) == 0) {
            if (visit != NULL) visit(child, ctx);
            count++;
        }
        if (child->type == DIRECTORY_NODE) {
            count += find_in_tree(child, name, visit, ctx);
        }
    }
    return count;
}
