#ifndef DARRAY_H
#define DARRAY_H

typedef struct
{
    size_t data_size;
    size_t size;
    size_t capacity;
    void **data;
    bool dirty;
} DArray;

//void DArrayDestroyElement(void *element, void (*GenericDestroy)(void*));
DArray *DArrayCreate(size_t capacity, size_t data_size);
void *DArrayBinarySearch(DArray *darray, const void *target, int (*CompareCallback)(const void*, const void*));
void DArraySort(DArray *darray, int (*CompareCallback)(const void*, const void*));
bool DArrayContains(DArray *darray, const void *item, int (*SortCompareCallback)(const void*, const void*), int (*SearchCompareCallback)(const void*, const void*));
void DArrayPrint(DArray *darray, void (*PrintCallback)(void*));
void **DArrayResize(DArray *darray, size_t capacity);
void DArrayDestroy(DArray *darray, void (*DestroyCallback)(void*));
bool DArrayEmpty(DArray *darray);
bool DArrayFull(DArray *entries);
int DArrayAdd(DArray *darray, void *element);
void **DArrayRemove(DArray *darray, size_t index);

#endif
