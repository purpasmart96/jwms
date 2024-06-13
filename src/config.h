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
    char *global_decorations_style;
    char *global_bg_color_active;
    char *global_bg_color_inactive;
    char *global_fg_color_active;
    char *global_fg_color_inactive;
    char *global_outline_color;
    char *global_font;
    char *global_font_alignment;
    int global_font_size;

    bool tray_use_global_decorations_style;
    bool tray_use_global_colors;
    bool tray_use_global_font;
    bool tray_use_menu_icon;
    TrayPositions tray_pos;
    int tray_height;
    char *tray_menu_icon;
    char *tray_menu_text;
    bool tray_auto_hide;
    int tray_icon_spacing;
    bool tray_outline_enabled;
    char *tray_decorations_style;
    char *tray_bg_color_active;
    char *tray_bg_color_inactive;
    char *tray_fg_color_active;
    char *tray_fg_color_inactive;
    char *tray_outline_color_active;
    char *tray_outline_color_inactive;
    float tray_opacity;
    char *tray_font;
    char *tray_font_alignment;
    int tray_font_size;

    int root_menu_height;

    bool window_use_global_decorations_style;
    bool window_use_global_colors;
    bool window_use_global_font;
    bool window_outline_enabled;
    int window_height;
    int window_width;
    int window_corner_rounding;
    char *window_decorations_style;
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
    char *window_focus_model;
    char *window_snap_mode;
    int window_snap_distance;
    char *window_move_mode;
    char *window_resize_mode;

    bool clock_use_global_colors;
    bool clock_use_global_font;
    char *clock_bg_color_inactive;
    char *clock_fg_color_inactive;
    char *clock_font;
    char *clock_font_alignment;
    int clock_font_size;

    bool pager_use_global_colors;
    bool pager_use_global_font;
    bool pager_outline_enabled;
    bool pager_labled;
    char *pager_bg_color_active;
    char *pager_bg_color_inactive;
    char *pager_fg_color_active;
    char *pager_fg_color_inactive;
    char *pager_outline_color_inactive;
    char *pager_text_color;
    char *pager_font;
    char *pager_font_alignment;
    int pager_font_size;

    bool menu_use_global_decorations_style;
    bool menu_use_global_colors;
    bool menu_use_global_font;
    bool menu_outline_enabled;
    char *menu_decorations_style;
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

int WriteJWMConfig(BTreeNode *entries, HashMap *icons);

#endif
