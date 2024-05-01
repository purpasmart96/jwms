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
    bool tray_outline_enabled;

    int root_menu_height;

    char *global_bg_color_active;
    char *global_bg_color_inactive;
    char *global_fg_color_active;
    char *global_fg_color_inactive;
    char *global_outline_color;

    bool window_use_global_colors;
    int window_height;
    int window_width;
    int window_corner_rounding;
    bool window_outline_enabled;
    char *window_bg_color_active;
    char *window_bg_color_inactive;
    char *window_fg_color_active;
    char *window_fg_color_inactive;
    char *window_outline_color_active;
    char *window_outline_color_inactive;
    float window_opacity_active;
    float window_opacity_inactive;

    bool menu_use_global_colors;
    char *menu_bg_color_active;
    char *menu_bg_color_inactive;
    char *menu_fg_color_active;
    char *menu_fg_color_inactive;
    bool menu_outline_enabled;
    char *menu_outline_color;
    float menu_opacity;

    char *autogen_config_path;
    char *browser_name;
    char *terminal_name;
    char *filemanager_name;
} JWM;

int WriteJWMConfig(DArray *entries, HashMap *icons);

#endif
