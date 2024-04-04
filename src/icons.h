#ifndef ICONS_H
#define ICONS_H

#include "darray.h"
typedef enum
{
    Fixed,
    Scaled,
    Threshold,
    Fallback
} IconType;

typedef struct
{
    char *path;
    char *context;
    IconType type;
    int size;
    int min_size;
    int max_size;
    int scale;
    int threshold;
} XDGIcon;

/*
typedef struct
{
    size_t capacity;
    size_t size;
    XDGIcon **data;
} XDGIcons;
*/

typedef struct
{
    char *name;
    DArray *icons;
    //HashMap *map;
    //char  **parents;
    //bool valid;
    //char **gtk_caches;
} IconTheme;

XDGIcon *IconCreate(const char *path, IconType type, int size, int min_size, int max_size, int scale, int threshold);

char *GetCurrentGTKIconThemeName();
//char *LookupIcon(char *icon_name, int size, int scale, char *theme);
char *LookupIcon(IconTheme *theme, const char *icon_name, int size, int scale);
char *FindIcon(const char *icon, int size, int scale);
//void TestPrintSections(HashMap *map);
//XDGIcon *LookupIconHelper(XDGIcon *icon_dir_info, char *icon_name, char *theme);

#endif
