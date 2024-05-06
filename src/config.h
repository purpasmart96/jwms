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
    bool tray_use_menu_icon;
    char tray_menu_text;
    bool tray_auto_hide;
    int tray_icon_spacing;
    bool tray_outline_enabled;

    int root_menu_height;

    char *global_bg_color_active;
    char *global_bg_color_inactive;
    char *global_fg_color_active;
    char *global_fg_color_inactive;
    char *global_outline_color;
    char *global_font;
    char *global_font_alignment;
    int global_font_size;

    bool window_use_global_colors;
    bool window_use_global_font;
    bool window_outline_enabled;
    int window_height;
    int window_width;
    int window_corner_rounding;
    char *window_bg_color_active;
    char *window_bg_color_inactive;
    char *window_fg_color_active;
    char *window_fg_color_inactive;
    char *window_outline_color_active;
    char *window_outline_color_inactive;
    float window_opacity_active;
    float window_opacity_inactive;
    char *window_font;
    char *window_font_alignment;
    int window_font_size;

    bool menu_use_global_colors;
    bool menu_use_global_font;
    bool menu_outline_enabled;
    char *menu_bg_color_active;
    char *menu_bg_color_inactive;
    char *menu_fg_color_active;
    char *menu_fg_color_inactive;
    char *menu_outline_color;
    float menu_opacity;
    char *menu_font;
    char *menu_font_alignment;
    int menu_font_size;

    char *autogen_config_path;
    char *browser_name;
    char *terminal_name;
    char *filemanager_name;
} JWM;

int WriteJWMConfig(DArray *entries, HashMap *icons);

#endif
