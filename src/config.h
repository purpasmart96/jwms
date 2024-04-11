#ifndef CONFIG_H
#define CONFIG_H

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

typedef struct
{
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

int WriteJWMConfig(DArray *entries, HashMap *icons);

#endif
