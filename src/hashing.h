#ifndef HASHING_H
#define HASHING_H

typedef struct
{
    char *key;
    char *value;
    size_t hash;
} Key;

typedef struct NodeSet
{
    // value is the key
    char *key;
    struct NodeSet *next;
} NodeSet;

typedef struct
{
    char *key;
    int value;
    unsigned int hash;
} Key2;

typedef struct
{
    size_t size;
    size_t capacity;
    Key **entries;
} HashMap;

typedef struct {
    size_t size;
    size_t capacity;
    NodeSet **entries;
} HashSet;

void HashMapResize(HashMap *map);
void HashMapInsert(HashMap *map, const char *key, const char *value);
void HashMapInsertWithSection(HashMap *map, const char *section, const char *key, const char *value);
const char *HashMapGet(HashMap *map, const char *key);
HashMap *HashMapCreate(void);
void HashMapPrint(HashMap *map);
void HashMapDestroy(HashMap *map);

#endif
