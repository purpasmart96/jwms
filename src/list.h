#ifndef LIST_H
#define LIST_H

typedef struct Node
{
    void *data;
    struct Node *next;
} Node;


typedef struct List
{
    size_t size;
    Node *head;
} List;

List *ListCreate(void);
void ListAdd(List *list, void *item, size_t data_size);
void ListInsert(List *list, void* item);
void ListPrint(List *list);
void ListDestroy(List *list);

#endif
