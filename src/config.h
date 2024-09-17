#ifndef CONFIG_H
#define CONFIG_H

#define WRITE_CFG(...) \
    fprintf(fp, __VA_ARGS__)

#define GetBGColor(type, state) \
    jwm->type##_use_global_colors ? jwm->global_bg_color_##state : jwm->type##_bg_color_##state

#define GetFGColor(type, state) \
    jwm->type##_use_global_colors ? jwm->global_fg_color_##state : jwm->type##_fg_color_##state

#define GetFontName(type) \
    jwm->type##_use_global_font ? jwm->global_font : jwm->type##_font

#define GetFontAlignment(type) \
    jwm->type##_use_global_font ? jwm->global_font_alignment : jwm->type##_font_alignment

#define GetFontSize(type) \
    jwm->type##_use_global_font ? jwm->global_font_size : jwm->type##_font_size

#define GetDecorationsStyle(type) \
    jwm->type##_use_global_decorations_style ? jwm->global_decorations_style : jwm->type##_decorations_style

#define GetOutlineColor(type, state) \
    jwm->type##_use_global_colors ? jwm->global_outline_color : jwm->type##_outline_color_##state

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

#define MAX_TRAYS 6
#define MAX_TRAY_PROGRAMS 48

typedef struct
{
    TrayPositions position;
    bool autohide;
    int autohide_delay;
    int thickness;
    int spacing;

    bool menu_button_enabled;

    char *programs[MAX_TRAY_PROGRAMS];
    int num_programs;

    bool tasklist_enabled;
    bool tasklist_labeled;
    char *tasklist_label_position;

    bool pager_enabled;
    bool systray_enabled;
    bool clock_enabled;
} Tray;

typedef struct
{
    char *global_decorations_style;
    char *global_bg_color_active;
    char *global_bg_color_inactive;
    char *global_fg_color_active;
    char *global_fg_color_inactive;
    char *global_outline_color;
    int global_preferred_icon_size;
    char *global_font;
    char *global_font_alignment;
    int global_font_size;

    bool tray_use_global_decorations_style;
    bool tray_use_global_colors;
    bool tray_use_global_font;
    bool tray_use_menu_icon;
    int tray_systray_size;
    int tray_systray_spacing;
    char *tray_menu_icon;
    char *tray_menu_text;
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

    Tray trays[MAX_TRAYS];
    int num_trays;

    bool tasklist_use_global_decorations_style;
    bool tasklist_use_global_colors;
    bool tasklist_use_global_font;
    bool tasklist_outline_enabled;
    char *tasklist_decorations_style;
    char *tasklist_bg_color_active;
    char *tasklist_bg_color_inactive;
    char *tasklist_fg_color_active;
    char *tasklist_fg_color_inactive;
    char *tasklist_outline_color_active;
    char *tasklist_outline_color_inactive;
    char *tasklist_font;
    char *tasklist_font_alignment;
    int tasklist_font_size;

    int root_menu_height;

    bool window_use_global_decorations_style;
    bool window_use_global_colors;
    bool window_use_global_font;
    bool window_outline_enabled;
    bool window_use_aerosnap;
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

    int desktop_workspaces;
    char *desktop_background_type;
    char *desktop_background;

    char *autogen_config_path;
    char *browser_name;
    char *terminal_name;
    char *filemanager_name;
} JWM;

int CreateJWMFolder(JWM *jwm);
int CreateJWMStartup(JWM *jwm);
int CreateJWMGroup(JWM *jwm);
int CreateJWMPreferences(JWM *jwm);
int CreateJWMIcons(JWM *jwm);
int CreateJWMRCBackup(char *path, char *backup_path);
int CreateJWMRCFile(JWM *jwm);
int CreateJWMAutoStart(JWM *jwm, cfg_t *cfg);
int CreateJWMBinds(JWM *jwm, cfg_t *cfg);
int CreateJWMTray(JWM *jwm, BTreeNode *entries, HashMap *icons);
int CreateJWMRootMenu(JWM *jwm, BTreeNode *entries, HashMap *icons, const char *xdg_menu_path);
int CreateJWMStyles(JWM *jwm);
int LoadJWMConfig(JWM **jwm, cfg_t **cfg);


#endif
