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
typedef struct
{
    uint32_t themeKey;
    bool supportsSvg;

    char *userTheme;
    char *systemTheme;
    DArray *iconDirs;
    HashMap *themeList;
} IconLoader;

typedef struct 
{
    char *contentDir;
    DArray keyList;
    DArray parents;
    bool valid;
} IconTheme;
*/

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

typedef enum
{
    NotIndexed,
    PartiallyIndexed,
    FullyIndexed
} IndexedState;

typedef struct
{
    HashMap *icons;

    char *path;
    int size;
    int scale;
    IconContext context;
    IconType type;
    int min_size;
    int max_size;
    int threshold;

    IndexedState index_state;
} XDGIconDir;

typedef struct
{
    DArray *icon_dirs;
    char *name;
    DArray *parents;
    //bool valid;
    //char **gtk_caches;
} IconTheme;

XDGIconDir *IconCreate(const char *path, IconType type, IconContext context, int size, int min_size, int max_size, int scale, int threshold);

int GetCurrentGTKIconThemeName(char theme_name[]);
//char *LookupIcon(char *icon_name, int size, int scale, char *theme);
char *LookupIcon2(IconTheme *theme, const char *icon_name, int size, int scale);
char *LookupIcon(IconTheme *theme, const char *icon_name, int size, int scale);
char *FindIcon(const char *icon, int size, int scale);
HashMap *FindAllIcons(List *icons, int size, int scale);
HashMap *FindAllIcons2(BTreeNode *entries, int size, int scale);

int PreloadIconThemes(const char *theme);
int PreloadIconThemesFast(const char *theme);
void DestroyIconThemes(void);
char *SearchIconInThemes(const char *icon, int size, int scale, int max_theme_depth);
char *SearchIconInThemes2(const char *icon, int size, int scale, int max_theme_depth);
char *SearchIconInTheme(const char *theme_name, const char *icon, int size, int scale);
//XDGIcon *LookupIconHelper(XDGIcon *icon_dir_info, char *icon_name, char *theme);

#endif
