#ifndef CONFIG_H
#define CONFIG_H

typedef enum
{
    Bottom,
    Top,
    Left,
    Right
} TrayPositions;

typedef enum
{
    WindowStyle,
    ClockStyle,
    TrayStyle,
    TaskListStyle,
    PagerStyle,
    MenuStyle,
    PopupStyle,
} Styles;

typedef struct
{
    TrayPositions tray_pos;
    int tray_height;
    bool tray_auto_hide;
    int tray_icon_spacing;
    int root_menu_height;

    char *global_bg_color_active;
    char *global_bg_color_inactive;
    char *global_fg_color_active;
    char *global_fg_color_inactive;

    char *window_bg_color_active;
    char *window_bg_color_inactive;
    char *window_fg_color_active;
    char *window_fg_color_inactive;

    char *menu_bg_color_active;
    char *menu_bg_color_inactive;
    char *menu_fg_color_active;
    char *menu_fg_color_inactive;

    char *autogen_config_path;
    char *browser_name;
    char *terminal_name;
    char *filemanager_name;
} JWM;

int WriteJWMConfig(DArray *entries, HashMap *icons);

#endif
