#ifndef BSTREE_H
#define BSTREE_H

typedef struct BTreeNode
{
    void *data;
    struct BTreeNode *left, *right;
} BTreeNode;

BTreeNode *BSTCreateNode(void *value);
// Function to search for a node with a specific key in the tree
BTreeNode *BSTSearchNode(BTreeNode *root, const void *target, int (*CompareCallback)(const void*, const void*));
// Function to insert a node with a specific value in the tree
BTreeNode *BSTInsertNode(BTreeNode *node, void *value, int (*CompareCallback)(const void*, const void*));
// Function to find the minimum value
BTreeNode *BSTFindMin(BTreeNode *root);

void BSTPostOrderTraverse(BTreeNode *root, void (*Func)(void*, void*), void *args);
void BSTInOrderTraverse(BTreeNode *root, void (*Func)(void*, void*), void *args);
void BSTPreOrderTraverse(BTreeNode *root, void (*Func)(void*, void*), void *args);

// Function to delete a node from the tree
BTreeNode *BSTDestroyNode(BTreeNode *root, const void *key, int (*CompareCallback)(const void*, const void*), void (*DestroyCallback)(void*));
void BSTDestroy(BTreeNode **root, void (*DestroyCallback)(void*));

#endif
