#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "bstree.h"

// Function to create a new node with a given value/data
BTreeNode *BSTCreateNode(void *data)
{
    BTreeNode *temp = malloc(sizeof(*temp));
    temp->data = data;
    temp->left = temp->right = NULL;
    return temp;
}

// Function to search for a node with a specific key in the tree
BTreeNode *BSTSearchNode(BTreeNode *root, const void *key, int (*CompareCallback)(const void*, const void*))
{
    if (root == NULL)
    {
        return NULL;
    }

    int cmp = CompareCallback(root->data, key);

    if (cmp == 0)
    {
        return root;
    }
    else if (cmp > 0)
    {
        return BSTSearchNode(root->left, key, CompareCallback);
    }
    else
    {
        return BSTSearchNode(root->right, key, CompareCallback);
    }
}

// Function to insert a node with a specific value in the tree
BTreeNode *BSTInsertNode(BTreeNode *node, void *value, int (*CompareCallback)(const void*, const void*))
{
    if (node == NULL)
    {
        return BSTCreateNode(value);
    }

    int cmp = CompareCallback(value, node->data);

    if (cmp < 0)
    {
        node->left = BSTInsertNode(node->left, value, CompareCallback);
    }
    else if (cmp > 0)
    {
        node->right = BSTInsertNode(node->right, value, CompareCallback);
    }

    return node;
}

// Function to perform post-order traversal
void BSTPostOrderTraverse(BTreeNode *root, void (*Func)(void*, void*), void *args)
{
    if (root != NULL)
    {
        BSTPostOrderTraverse(root->left, Func, args);
        BSTPostOrderTraverse(root->right, Func, args);
        // User defined function, can be whatever
        Func(root->data, args);
    }
}
 
// Function to perform in-order traversal
void BSTInOrderTraverse(BTreeNode *root, void (*Func)(void*, void*), void *args)
{
    if (root != NULL)
    {
        BSTPostOrderTraverse(root->left, Func, args);
        // User defined function, can be whatever
        Func(root->data, args);
        BSTPostOrderTraverse(root->right, Func, args);
    }
}
 
// Function to perform pre-order traversal
void BSTPreOrderTraverse(BTreeNode *root, void (*Func)(void*, void*), void *args)
{
    if (root != NULL)
    {
        // User defined function, can be whatever
        Func(root->data, args);
        BSTPostOrderTraverse(root->left, Func, args);
        BSTPostOrderTraverse(root->right, Func, args);
    }
}

// Function to find the minimum value
BTreeNode *BSTFindMin(BTreeNode *node)
{
    while (node && node->left != NULL)
    {
        node = node->left;
    }
    return node;
}

int BSTGetHeight(BTreeNode *root)
{
    if (root == NULL)
    {
        return 0;
    }
    return MAX(BSTGetHeight(root->left), BSTGetHeight(root->right)) + 1;
}

// Function to delete a key from the BST
BTreeNode *BSTDestroyNode(BTreeNode *root, const void *key, int (*CompareCallback)(const void*, const void*), void (*DestroyCallback)(void*))
{
    if (root == NULL)
    {
        return NULL;
    }

    int cmp = CompareCallback(root->data, key);

    if (cmp > 0)
    {
        root->left = BSTDestroyNode(root->left, key, CompareCallback, DestroyCallback);
    } 
    else if (cmp < 0)
    {
        root->right = BSTDestroyNode(root->right, key, CompareCallback, DestroyCallback);
    } 
    else
    {
        if (root->left == NULL)
        {
            printf("Destroying Node %p\n", root->data);
            BTreeNode *temp = root->right;
            DestroyCallback(root->data);
            free(root);
            return temp;
        }
        else if (root->right == NULL)
        {
            printf("Destroying Node %p\n", root->data);
            BTreeNode *temp = root->left;
            DestroyCallback(root->data);
            free(root);
            return temp;
        }

        BTreeNode *temp = BSTFindMin(root->right);
        DestroyCallback(root->data);
        root->data = temp->data;
        root->right = BSTDestroyNode(root->right, temp->data, CompareCallback, DestroyCallback);
    }

    return root;
}


// This function traverses tree in post order to delete each and every node of the tree
static void _destroy_tree(BTreeNode *root, void (*DestroyCallback)(void*)) 
{
    if (root == NULL)
        return;

    // first delete both subtrees
    _destroy_tree(root->left, DestroyCallback);
    _destroy_tree(root->right, DestroyCallback);
  
    // then delete the node
    if (root->data != NULL)
        DestroyCallback(root->data);

    free(root);
} 

void BSTDestroy(BTreeNode **root, void (*DestroyCallback)(void*))
{
    _destroy_tree(*root, DestroyCallback);
    *root = NULL;
}
