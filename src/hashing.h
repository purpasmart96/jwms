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
    void *value;
    size_t hash;
} Key2;

typedef struct
{
    size_t size;
    size_t capacity;
    Key **entries;
} HashMap;

typedef struct
{
    size_t data_size;
    size_t size;
    size_t capacity;
    Key2 **entries;
    void (*DestroyCallback)(void*);
    void (*PrintCallback)(void*);
} HashMap2;

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

HashMap2 *HashMapCreate2(size_t data_size, void (*DestroyCallback)(void*), void (*PrintCallback)(void*));
void HashMapResize2(HashMap2 *map);
void HashMapInsert2(HashMap2 *map, const char *key, void *value);
void *HashMapGet2(HashMap2 *map, const char *key);
void HashMapPrint2(HashMap2 *map);
void HashMapDestroy2(HashMap2 *map);

#endif
