#ifndef COMMON_H
#define COMMON_H

#define ARRAY_SIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define CLAMP(X, MIN, MAX)  ((X) < (MIN) ? (MIN) : ((X) > (MAX) ? (MAX) : (X)))

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

typedef struct {
    char *key;
    int value;
} Pair;

#endif
