
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "darray.h"

DArray *DArrayCreate(size_t capacity, size_t data_size)
{
    DArray *darray = malloc(sizeof(*darray));
    darray->capacity = capacity;
    darray->data_size = data_size;
    darray->data = malloc(sizeof(void*) * darray->capacity);
    darray->size = 0;
    darray->dirty = true;

    return darray;
}

void DArrayPrint(DArray *darray, void (*PrintCallback)(void*))
{
    printf("\n");
    for (size_t i = 0; i < darray->size; i++)
    {
        PrintCallback(darray->data[i]);
    }
    printf("\n");
}

void **DArrayResize(DArray *darray, size_t capacity)
{
    darray->capacity = capacity;
    void **temp = realloc(darray->data, sizeof(void*) * darray->capacity);

    if (!temp)
    {
        return NULL;
    }

    darray->data = temp;

    return temp;
}

void DArrayDestroy(DArray *darray, void (*DestroyCallback)(void*))
{
    for (size_t i = 0; i < darray->size; i++)
    {
        DestroyCallback(darray->data[i]);
    }

    free(darray->data);
    free(darray);
}

bool DArrayEmpty(DArray *darray)
{
    return !darray->size;
}

bool DArrayFull(DArray *entries)
{
    return entries->size == entries->capacity;
}

int DArrayAdd(DArray *darray, void *element)
{
    if (DArrayFull(darray))
    {
        if (!DArrayResize(darray, darray->capacity *= 2))
        {
            return -1;
        }
    }

    // Push an element on the top of it and increase its size by one
    darray->data[darray->size++] = element;
    darray->dirty = true;
    return 0;
}

void *DArrayBinarySearch(DArray *darray, const void *target, int (*CompareCallback)(void*, const void*))
{
    if (darray->dirty)
        return NULL;

    size_t left = 0;
    size_t right = darray->size - 1;
    while (left <= right)
    {
        size_t mid = left + (right - left) / 2;
        void *index = darray->data[mid];

        int cmp = CompareCallback(index, target);
        if (cmp == 0)
        {
            return index;
        }
        else if (cmp < 0)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }

    }
    return NULL;
}

// Only sorts if the array is unsorted
void DArraySort(DArray *darray, int (*CompareCallback)(const void*, const void*))
{
    if (darray->dirty)
    {
        qsort(darray->data, darray->size, sizeof(void*), CompareCallback);
        darray->dirty = false;
    }
}

// Not Tested
void **DArrayRemove(DArray *darray, size_t index)
{
    // Allocate an array with a size 1 less than the current one
    void **temp = malloc((darray->size - 1) * darray->data_size);

    if (index != 0) // copy everything BEFORE the index
        memcpy(temp, darray->data, (index - 1) * darray->data_size);

    if (index != (darray->size - 1)) // copy everything AFTER the index
        memcpy(temp + index, darray->data + index + 1, (darray->size - index - 1) * darray->data_size);

    free(darray->data);
    return temp;
}
