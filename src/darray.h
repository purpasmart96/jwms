#ifndef DARRAY_H
#define DARRAY_H

typedef struct
{
    void **data;
    void (*DestroyCallback)(void*);
    int (*SortCompareCallback)(const void*, const void*);
    int (*SearchCompareCallback)(const void*, const void*);
    size_t size;
    size_t capacity;
    bool dirty;
} DArray;

//void DArrayDestroyElement(void *element, void (*GenericDestroy)(void*));
DArray *DArrayCreate(size_t capacity, void (*DestroyCallback)(void *),
                    int (*SearchCompareCallback)(const void*, const void*),
                    int (*SortCompareCallback)(const void*, const void*));

void *DArrayLinearSearch(DArray *darray, const void *target);
void *DArrayBinarySearch(DArray *darray, const void *target);
void *DArraySearch(DArray *darray, const void *target);
void DArraySort(DArray *darray);
bool DArrayContains(DArray *darray, const void *target);
void DArrayRemoveDupes(DArray *darray);
void DArrayPrint(DArray *darray, void (*PrintCallback)(void*));
void **DArrayResize(DArray *darray, size_t capacity);
void DArrayDestroy(DArray *darray);
bool DArrayEmpty(DArray *darray);
bool DArrayFull(DArray *entries);
int DArrayAdd(DArray *darray, void *element);
int DArrayRemove(DArray *darray, size_t index);

#endif
