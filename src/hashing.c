#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "hashing.h"

#define LOAD_FACTOR 0.75
#define CAPACITY_START 32

// 64 bit and 32 bit fnv1-a
static size_t Hash(const char *key)
{
#ifdef __x86_64__
    const size_t fnv_prime = 1099511628211;
    size_t hash = 14695981039346656037u; // FNV offset basis
#else
    const size_t fnv_prime = 16777619;
    size_t hash = 2166136261u; // FNV offset basis
#endif

    for (size_t i = 0; key[i] != '\0'; i++)
    {
        hash ^= (unsigned char)key[i];
        hash *= fnv_prime;
    }

    return hash;
}

HashMap *HashMapCreate(void)
{
    HashMap *map = malloc(sizeof(*map));
    map->entries = malloc(sizeof(Key *) * CAPACITY_START);
    for (size_t i = 0; i < CAPACITY_START; i++)
    {
        map->entries[i] = NULL;
    }
    map->size = 0;
    map->capacity = CAPACITY_START;

    return map;
}

HashMap2 *HashMapCreate2(size_t data_size, void (*DestroyCallback)(void*), void (*PrintCallback)(void*))
{
    HashMap2 *map = malloc(sizeof(*map));
    map->entries = malloc(sizeof(Key2*) * CAPACITY_START);

    for (size_t i = 0; i < CAPACITY_START; i++)
    {
        map->entries[i] = NULL;
    }
    map->size = 0;
    map->data_size = data_size;
    map->capacity = CAPACITY_START;
    map->DestroyCallback = DestroyCallback;
    map->PrintCallback = PrintCallback;

    return map;
}

void HashMapResize(HashMap *map)
{
    size_t new_capacity = map->capacity * 2;
    Key **new_entries = malloc(sizeof(Key *) * new_capacity);

    for (size_t i = 0; i < new_capacity; i++)
    {
        new_entries[i] = NULL;
    }

    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i] != NULL)
        {
            size_t index = map->entries[i]->hash & (new_capacity - 1);
            while (new_entries[index] != NULL)
            {
                index = (index + 1) & (new_capacity - 1);
            }

            new_entries[index] = map->entries[i];
        }
    }

    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
}

void HashMapResize2(HashMap2 *map)
{
    size_t new_capacity = map->capacity * 2;
    Key2 **new_entries = malloc(sizeof(Key2*) * new_capacity);

    for (size_t i = 0; i < new_capacity; i++)
    {
        new_entries[i] = NULL;
    }

    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i] != NULL)
        {
            size_t index = map->entries[i]->hash & (new_capacity - 1);
            while (new_entries[index] != NULL)
            {
                index = (index + 1) & (new_capacity - 1);
            }

            new_entries[index] = map->entries[i];
        }
    }

    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
}

void HashMapInsert(HashMap *map, const char *key, const char *value)
{
    if ((double)map->size / map->capacity > LOAD_FACTOR)
    {
        HashMapResize(map);
    }

    size_t hash = Hash(key);
    char *temp_value = strdup(value);

    size_t index = hash & (map->capacity - 1);

    while (map->entries[index] != NULL)
    {
        // Compare precomputed hashes
        if (map->entries[index]->hash == hash)
        {
            // If hashes match, compare keys
            if (strcmp(map->entries[index]->key, key) == 0)
            {
                // Dupe found, update value and finish
                free(map->entries[index]->value);
                map->entries[index]->value = temp_value;
                return;
            }
        }
        // Handle collision using open addressing
        index = (index + 1) & (map->capacity - 1);
    }

    Key *new_entry = malloc(sizeof(Key));
    new_entry->key = strdup(key);
    new_entry->value = temp_value;
    new_entry->hash = hash;

    map->entries[index] = new_entry;
    map->size++;
}

void HashMapInsert2(HashMap2 *map, const char *key, void *value)
{
    if ((double)map->size / map->capacity > LOAD_FACTOR)
    {
        HashMapResize2(map);
    }

    size_t hash = Hash(key);

    size_t index = hash & (map->capacity - 1);

    while (map->entries[index] != NULL)
    {
        // Compare precomputed hashes
        if (map->entries[index]->hash == hash)
        {
            // If hashes match, compare keys
            if (strcmp(map->entries[index]->key, key) == 0)
            {
                // Dupe found, update value and finish
                map->DestroyCallback(map->entries[index]->value);
                map->entries[index]->value = value;
                return;
            }
        }
        // Handle collision using open addressing
        index = (index + 1) & (map->capacity - 1);
    }

    Key2 *new_entry = malloc(sizeof(Key2));
    new_entry->key = strdup(key);
    new_entry->value = value;
    new_entry->hash = hash;

    map->entries[index] = new_entry;
    map->size++;
}

void HashMapInsertWithSection(HashMap *map, const char *section, const char *key, const char *value)
{
    char combined_key[256];
    snprintf(combined_key, sizeof(combined_key), "%s:%s", section, key);
    HashMapInsert(map, combined_key, value);
}

const char *HashMapGet(HashMap *map, const char *key)
{
    size_t hash = Hash(key);
    size_t index = hash & (map->capacity - 1);

    while (map->entries[index] != NULL)
    {
        if (map->entries[index]->hash == hash && strcmp(map->entries[index]->key, key) == 0)
        {
            return map->entries[index]->value;
        }
        // Handle collision using open addressing
        index = (index + 1) & (map->capacity - 1);
    }

    return NULL;
}

void *HashMapGet2(HashMap2 *map, const char *key)
{
    size_t hash = Hash(key);
    //size_t index = hash % map->capacity;
    size_t index = hash & (map->capacity - 1);

    while (map->entries[index] != NULL)
    {
        if (map->entries[index]->hash == hash && strcmp(map->entries[index]->key, key) == 0)
        {
            return map->entries[index]->value;
        }
        // Handle collision using open addressing
        index = (index + 1) & (map->capacity - 1);
    }

    return NULL;
}

void HashMapPrint(HashMap *map)
{
    printf("\n");
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i] != NULL)
        {
            printf("Key      : %s\n", map->entries[i]->key);
            printf("Key Hash : 0x%08lX\n", map->entries[i]->hash);
            printf("Value    : %s\n\n", map->entries[i]->value);
        }
    }

    printf("\n");
}

void HashMapPrint2(HashMap2 *map)
{
    printf("\n");
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i] != NULL)
        {
            printf("Key      : %s\n", map->entries[i]->key);
            printf("Key Hash : 0x%08lX\n", map->entries[i]->hash);
            //printf("Value    : %s\n\n", map->entries[i]->value);
            map->PrintCallback(map->entries[i]->value);
        }
    }

    printf("\n");
}

void HashMapDestroy(HashMap *map)
{
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i] != NULL)
        {
            free(map->entries[i]->key);
            free(map->entries[i]->value);
            free(map->entries[i]);
        }
    }

    free(map->entries);
    free(map);
}

void HashMapDestroy2(HashMap2 *map)
{
    for (size_t i = 0; i < map->capacity; i++)
    {
        if (map->entries[i] != NULL)
        {
            free(map->entries[i]->key);
            //free(map->entries[i]->value);
            map->DestroyCallback(map->entries[i]->value);
            free(map->entries[i]);
        }
    }

    free(map->entries);
    free(map);
}

HashSet *HashSetCreate(size_t capacity)
{
    HashSet *set = malloc(sizeof(HashSet));
    if (!set)
    {
        return NULL;
    }

    set->capacity = capacity;
    set->size = 0;

    set->entries = calloc(capacity, sizeof(NodeSet *));
    if (!set->entries)
    {
        free(set);
        return NULL;
    }

    return set;
}

void HashSetDestroy(HashSet *set)
{
    if (!set)
    {
        return;
    }

    for (size_t i = 0; i < set->capacity; i++)
    {
        NodeSet *current = set->entries[i];
        while (current != NULL)
        {
            NodeSet *next = current->next;
            free(current->key);
            free(current);
            current = next;
        }
    }

    free(set->entries);
    free(set);
}

bool HashSetContains(HashSet *set, const char *key)
{
    size_t hash = Hash(key) % set->capacity;
    NodeSet *current = set->entries[hash];
    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            return true;
        }
        current = current->next;
    }
    return false;
}

void HashSetResize(HashSet *set)
{
    // Resize the hashset
    size_t new_capacity = set->capacity * 2;
    NodeSet **new_entries = calloc(new_capacity, sizeof(NodeSet *));

    // Rehash existing elements
    for (size_t i = 0; i < set->capacity; i++)
    {
        NodeSet *current = set->entries[i];
        while (current != NULL)
        {
            NodeSet *next = current->next;
            size_t new_index = Hash(current->key) % new_capacity;
            current->next = new_entries[new_index];
            new_entries[new_index] = current;
            current = next;
        }
    }

    // Free the old buckets array
    free(set->entries);

    // Update the capacity and buckets array
    set->capacity = new_capacity;
    set->entries = new_entries;
}

void HashSetInsert(HashSet *set, const char *key)
{
    if (HashSetContains(set, key))
    {
        return;
    }

    if ((double)set->size / set->capacity >= LOAD_FACTOR)
    {
        // Resize the hashset
        HashSetResize(set);
    }

    // Insert the new key into the hashset
    size_t index = Hash(key) % set->capacity;
    NodeSet *new_node = malloc(sizeof(NodeSet));
    new_node->key = strdup(key);
    new_node->next = set->entries[index];
    set->entries[index] = new_node;
    set->size++;
}
