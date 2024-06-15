#ifndef LIST_H
#define LIST_H

typedef struct Node
{
    void *data;
    struct Node *next;
} Node;

typedef struct List
{
    Node *head;
    size_t size;
} List;

List *ListCreate(void);
bool ListContains(List *list, const void *item,  int (*CmpFunc)(const void*, const void*));
void ListAdd(List *list, void *item, size_t data_size);
void ListInsert(List *list, void* item);
void ListPrint(List *list, void (*PrintFunc)(void*));
void ListDestroy(List *list);

#endif
