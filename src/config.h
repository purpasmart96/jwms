#ifndef CONFIG_H
#define CONFIG_H

typedef enum
{
    Bottom,
    Top,
    Left,
    Right
} TrayPositions;

typedef struct
{
    TrayPositions tray_pos;
    int tray_height;
    bool tray_auto_hide;
    int tray_icon_spacing;
    int root_menu_height;

    char *autogen_config_path;
    char *browser_name;
    char *terminal_name;
    char *filemanager_name;
} JWM;

int WriteJWMConfig(DArray *entries, HashMap *icons);

#endif
