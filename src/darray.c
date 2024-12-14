
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "darray.h"

DArray *DArrayCreate(size_t capacity, void (*DestroyCallback)(void*),
                    int (*SearchCompareCallback)(const void *, const void *),
                    int (*SortCompareCallback)(const void *, const void *))
{
    DArray *darray = malloc(sizeof(*darray));
    darray->capacity = capacity;
    darray->data = malloc(sizeof(void*) * darray->capacity);
    darray->DestroyCallback = DestroyCallback;
    darray->SearchCompareCallback = SearchCompareCallback;
    darray->SortCompareCallback = SortCompareCallback;
    darray->size = 0;
    darray->dirty = true;

    return darray;
}

void DArrayPrint(DArray *darray, void (*PrintCallback)(void*))
{
    for (size_t i = 0; i < darray->size; i++)
    {
        PrintCallback(darray->data[i]);
    }
}

void **DArrayResize(DArray *darray, size_t capacity)
{
    void **new_array = realloc(darray->data, sizeof(void*) * capacity);

    if (!new_array)
    {
        return NULL;
    }

    darray->capacity = capacity;
    darray->data = new_array;

    return new_array;
}

void DArrayDestroy(DArray *darray)
{
    for (size_t i = 0; i < darray->size; i++)
    {
        darray->DestroyCallback(darray->data[i]);
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

void *DArrayLinearSearch(DArray *darray, const void *target)
{
    for (size_t i = 0; i < darray->size; i++)
    {
        if (darray->SearchCompareCallback(darray->data[i], target))
        {
            return darray->data[i];
        }
    }

    return NULL;
}

void *DArrayBinarySearch(DArray *darray, const void *target)
{
    if (darray->dirty)
        return NULL;

    size_t left = 0;
    size_t right = darray->size - 1;
    while (left <= right)
    {
        size_t mid = left + (right - left) / 2;
        void *index = darray->data[mid];

        int cmp = darray->SearchCompareCallback(index, target);
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

void *DArraySearch(DArray *darray, const void *target)
{
    // Linear searches are faster when N is smaller
    if (darray->dirty || darray->size < 128)
        return DArrayLinearSearch(darray, target);
    return DArrayBinarySearch(darray, target);
}

void DArraySort(DArray *darray)
{
    qsort(darray->data, darray->size, sizeof(void*), darray->SortCompareCallback);
    darray->dirty = false;
}

bool DArrayContains(DArray *darray, const void *target)
{
    return DArraySearch(darray, target) != NULL ? true : false;
}

// Unsorted: O(n log n) + O(n)
// Sorted: O(n)
void DArrayRemoveDupes(DArray *darray)
{
    DArraySort(darray);

    size_t j = 0;
    for (size_t i = 1; i < darray->size; i++)
    {
        if (darray->SearchCompareCallback(darray->data[i], darray->data[j]))
        {
            j++;
            darray->data[j] = darray->data[i];
        }
        else
        {
            darray->DestroyCallback(darray->data[i]);
        }
    }

    darray->size = j + 1;
}

int DArrayRemove(DArray *darray, size_t index)
{
    if (index >= darray->size)
    {
        return -1;  // Invalid index
    }

    // Call the destroy callback on the element to be removed
    darray->DestroyCallback(darray->data[index]);

    // Shift elements down if the removed element is not the last one
    if (index < darray->size - 1)
    {
        memmove(&darray->data[index], &darray->data[index + 1], 
                (darray->size - index - 1) * sizeof(void*));
    }

    // Decrease size
    darray->size--;

    // Mark as dirty to indicate sorting is needed
    darray->dirty = true;

    return 0;
}
