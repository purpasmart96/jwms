#ifndef ICONS_H
#define ICONS_H

typedef enum
{
    Fixed,
    Scaled,
    Threshold,
    Fallback
} IconType;

typedef enum
{
    ActionsContext,
    DevicesContext,
    FileSystemsContext,
    MimeTypesContext
} IconContext;

/*
typedef enum 
{
    Size,
    Scale,
    Context,
    Type,
    MaxSize,
    MinSize,
    Threshold,
} IconDirKeyType;
*/

typedef struct
{
    char *path;
    int size;
    int scale;
    IconContext context;
    IconType type;
    int min_size;
    int max_size;
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
    HashMap2 *icons2;
    List *paths;
    //char  **parents;
    //bool valid;
    //char **gtk_caches;
} IconTheme;

XDGIcon *IconCreate(const char *path, IconType type, IconContext context, int size, int min_size, int max_size, int scale, int threshold);

char *GetCurrentGTKIconThemeName();
//char *LookupIcon(char *icon_name, int size, int scale, char *theme);
char *LookupIcon2(IconTheme *theme, const char *icon_name, int size, int scale);
char *LookupIcon(IconTheme *theme, const char *icon_name, int size, int scale);
char *FindIcon(const char *icon, int size, int scale);
//List *FindAllIcons(List *icons, int size, int scale);
HashMap *FindAllIcons(List *icons, int size, int scale);
HashMap *FindAllIcons2(BTreeNode *entries, int size, int scale);
//void TestPrintSections(HashMap *map);
//XDGIcon *LookupIconHelper(XDGIcon *icon_dir_info, char *icon_name, char *theme);

#endif
