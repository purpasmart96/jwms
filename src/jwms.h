#ifndef JWMS_H
#define JWMS_H

#define ARRAY_SIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

typedef struct {
    int height;
} JWMRootMenu;

typedef enum
{
    Bottom,
    Top,
    Left,
    Right
} TrayPositions;

typedef struct {
    TrayPositions tpos;
    int height;
    bool auto_hide;
    int icon_spacing;
    char *browser_name;
    char *terminal_name;
    char *filemanager_name;
} JWMTray;

typedef struct
{
    JWMRootMenu *root_menu;
    JWMTray *tray;
    char *browser_name;
    char *terminal_name;
    char *filemanager_name;
} JWM;

#endif
