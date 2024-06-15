#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "list.h"

List *ListCreate(void)
{
    List *list = malloc(sizeof(*list));
    if (list == NULL)
        return NULL;

    list->head = NULL;
    list->size = 0;

    return list;
}

void ListDestroy(List *list)
{
    if (!list)
    {
        return;
    }

    Node *current = list->head;
    while (current != NULL)
    {
        Node *next = current->next;
        free(current->data);
        free(current);
        current = next;
    }

    free(list);
}

bool ListContains(List *list, const void *item,  int (*CmpFunc)(const void*, const void*))
{
    Node *current = list->head;
    while (current != NULL)
    {
        if (CmpFunc(current->data, item))
        {
            return true;
        }
        current = current->next;
    }
    return false;
}

void ListAdd(List *list, void *item, size_t data_size)
{
    Node *new_node = malloc(sizeof(*new_node));
    if (new_node == NULL)
        return;

    new_node->data = malloc(data_size);
    new_node->next = list->head;

    memcpy(new_node->data, item, data_size);
    list->head = new_node;
    list->size++;
}

void ListPrint(List *list, void (*PrintFunc)(void*))
{
    Node *current = list->head;
    while (current != NULL)
    {
        PrintFunc(current->data);
        //printf("%s\n", (char*)current->data);
        current = current->next;
    }
}
