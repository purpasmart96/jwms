#ifndef COMMON_H
#define COMMON_H

#define ARRAY_SIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

typedef struct {
    char *key;
    int value;
} Pair;

#endif
