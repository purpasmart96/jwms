#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <bsd/string.h>
#include <confuse.h>

#include "common.h"
#include "bstree.h"
#include "darray.h"
#include "hashing.h"
#include "list.h"
#include "icons.h"
#include "desktop_entries.h"

#include "config.h"

static TrayPositions GetTrayPosition(const char *tpos)
{
    if (strcmp(tpos, "bottom") == 0)
        return Bottom;
    else if (strcmp(tpos, "top") == 0)
        return Top;
    else if (strcmp(tpos, "left") == 0)
        return Left;
    else if (strcmp(tpos, "right") == 0)
        return Right;

    printf("Invalid Tray Position! Using Default Position (bottom)\n");
    return Bottom;
}

static void ParseTrays(JWM *jwm, cfg_t *cfg)
{
    int n = cfg_size(cfg, "tray");
	printf("\nFound %d tray(s):\n", n);

    if (n > MAX_TRAYS)
    {
        printf("Too many trays! Ignoring extra trays!\n");
        n = MAX_TRAYS;
    }

    jwm->num_trays = n;

	for (int i = 0; i < n; i++)
    {
        Tray *curr_tray = &jwm->trays[i];
		cfg_t *tray = cfg_getnsec(cfg, "tray", i);

        const char *title = cfg_title(tray);
        printf("tray %u: %s\n", i + 1, title);

        const char *tpos = cfg_getstr(tray, "position");
        curr_tray->position = GetTrayPosition(tpos);
        curr_tray->autohide = cfg_getbool(tray, "autohide");
        curr_tray->autohide_delay = cfg_getint(tray, "autohide_delay");
        curr_tray->thickness = cfg_getint(tray, "thickness");
        curr_tray->spacing = cfg_getint(tray, "spacing");
        curr_tray->menu_button_enabled = cfg_getbool(tray, "menu_button_enabled");

        curr_tray->num_programs = cfg_size(tray, "programs");
        curr_tray->tasklist_enabled = cfg_getbool(tray, "tasklist_enabled");
        curr_tray->tasklist_labeled = cfg_getbool(tray, "tasklist_labeled");
        curr_tray->tasklist_label_position = cfg_getstr(tray, "tasklist_label_position");
        curr_tray->pager_enabled = cfg_getbool(tray, "pager_enabled");
        curr_tray->systray_enabled = cfg_getbool(tray, "systray_enabled");
        curr_tray->clock_enabled = cfg_getbool(tray, "clock_enabled");

        for (int j = 0; j < curr_tray->num_programs; j++)
        {
            curr_tray->programs[j] = cfg_getnstr(tray,"programs", j);
            printf("program %d: %s\n", j, curr_tray->programs[j]);
        }
	}
}

static float GetValidOpacity(float opacity)
{
    return CLAMP(opacity, 0.0, 1.0);
}

static int GetValidDefaultIconSize(int icon_size)
{
    if (!MultiplesOf8(icon_size))
    {
        printf("global_preferred_icon_size is not mutliple of 8!\nUsing the default value of 32\n");
        icon_size = 32;
    }

    return icon_size;
}

static int ReadConfigFile(cfg_t *cfg, const char *filename, const char *user_dir, const char *system_dir)
{
    int ret = -1;
    char user_path[512];

    ExpandPath(user_path, user_dir, sizeof(user_path));
    strlcat(user_path, filename, sizeof(user_path));

	ret = cfg_parse(cfg, user_path);
    if (ret != CFG_SUCCESS)
    {
        if (ret == CFG_FILE_ERROR)
        {
			printf("Error opening %s\nUsing system config instead\n", user_path);
        }
        else
        {
        	printf("Error parsing %s\nUsing system config instead\n", user_path);
        }
	}
    else
    {
        return ret;
    }

    char system_path[512];

    ExpandPath(system_path, system_dir, sizeof(system_path));
    strlcat(system_path, filename, sizeof(system_path));

	ret = cfg_parse(cfg, system_path);
	if (ret != CFG_SUCCESS)
    {
        if (ret == CFG_FILE_ERROR)
        {
			//printf("Failed opening %s\n Using default values\n", system_cfg);
            printf("Failed opening %s\n", system_path);
            return ret;
        } 
        else
        {
        	printf("Failed parsing %s\n", system_path);
            return ret;
        }
	}

    return ret;
}

int LoadJWMConfig(JWM **jwm, cfg_t **cfg)
{
	cfg_opt_t keybind_opts[] =
    {
        CFG_STR_LIST("mods", NULL, CFGF_NONE),
        CFG_STR("key", NULL, CFGF_NONE),
        CFG_STR("command", NULL, CFGF_NONE),
		CFG_END()
	};

	cfg_opt_t autostart_opts[] =
    {
        CFG_INT("sleep_time", 0, CFGF_NONE),
        CFG_BOOL("fork_needed", false, CFGF_NONE),
        CFG_BOOL("restart_kill", false, CFGF_NONE),
        CFG_STR("program", NULL, CFGF_NONE),
        CFG_STR("args", NULL, CFGF_NONE),
		CFG_END()
	};

	cfg_opt_t tray_opts[] =
    {
        CFG_STR("position", "bottom", CFGF_NONE),
        CFG_BOOL("autohide", false, CFGF_NONE),
        CFG_INT("autohide_delay", 1000, CFGF_NONE),
        CFG_INT("thickness", 32, CFGF_NONE),
        CFG_INT("spacing", 4, CFGF_NONE),
        CFG_BOOL("menu_button_enabled", false, CFGF_NONE),
        CFG_STR_LIST("programs", NULL, CFGF_NONE),
        CFG_BOOL("tasklist_enabled", false, CFGF_NONE),
        CFG_BOOL("tasklist_labeled", false, CFGF_NONE),
        CFG_STR("tasklist_label_position", "right", CFGF_NONE),
        CFG_BOOL("pager_enabled", false, CFGF_NONE),
        CFG_BOOL("systray_enabled", false, CFGF_NONE),
        CFG_BOOL("clock_enabled", false, CFGF_NONE),
		CFG_END()
	};

    cfg_opt_t opts[] =
    {
        CFG_STR("global_browser", "firefox", CFGF_NONE),
        CFG_STR("global_terminal", "xterm", CFGF_NONE),
        CFG_STR("global_filemanager", "pcmanfm", CFGF_NONE),
        CFG_STR("global_decorations_style", "flat", CFGF_NONE),
        CFG_STR("global_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("global_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("global_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("global_fg_color_inactive", "#CCCCCC", CFGF_NONE),
        CFG_STR("global_outline_color", "#FFFFFF", CFGF_NONE),
        CFG_INT("global_preferred_icon_size", 32, CFGF_NONE),
        CFG_STR("global_font", "Sans", CFGF_NONE),
        CFG_STR("global_font_alignment", "center", CFGF_NONE),
        CFG_INT("global_font_size", 10, CFGF_NONE),

        CFG_BOOL("window_use_global_decorations_style", true, CFGF_NONE),
        CFG_BOOL("window_use_global_colors", true, CFGF_NONE),
        CFG_BOOL("window_use_global_font", true, CFGF_NONE),
        CFG_BOOL("window_use_aerosnap", false, CFGF_NONE),
        CFG_BOOL("window_outline_enabled", false, CFGF_NONE),
        CFG_INT("window_height", 26, CFGF_NONE),
        CFG_INT("window_width", 4, CFGF_NONE),
        CFG_INT("window_corner_rounding", 4, CFGF_NONE),
        CFG_STR("window_decorations_style", "flat", CFGF_NONE),
        CFG_STR("window_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("window_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("window_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("window_fg_color_inactive", "#CCCCCC", CFGF_NONE),
        CFG_STR("window_outline_color_active", "#FFFFFF", CFGF_NONE),
        CFG_STR("window_outline_color_inactive", "#FFFFFF", CFGF_NONE),
        CFG_FLOAT("window_opacity_active", 1.0, CFGF_NONE),
        CFG_FLOAT("window_opacity_inactive", 1.0, CFGF_NONE),
        CFG_STR("window_font", "Sans", CFGF_NONE),
        CFG_STR("window_font_alignment", "center", CFGF_NONE),
        CFG_INT("window_font_size", 10, CFGF_NONE),
        CFG_STR("window_focus_model", "click", CFGF_NONE),
        CFG_STR("window_snap_mode", "border", CFGF_NONE),
        CFG_INT("window_snap_distance", 10, CFGF_NONE),
        CFG_STR("window_move_mode", "opaque", CFGF_NONE),
        CFG_STR("window_resize_mode", "outline", CFGF_NONE),

        CFG_BOOL("clock_use_global_colors", true, CFGF_NONE),
        CFG_BOOL("clock_use_global_font", true, CFGF_NONE),
        CFG_STR("clock_bg_color", "#111111", CFGF_NONE),
        CFG_STR("clock_fg_color", "#DDDDDD", CFGF_NONE),
        CFG_STR("clock_font", "Sans", CFGF_NONE),
        CFG_STR("clock_font_alignment", "center", CFGF_NONE),
        CFG_INT("clock_font_size", 11, CFGF_NONE),

        CFG_BOOL("pager_use_global_colors", true, CFGF_NONE),
        CFG_BOOL("pager_use_global_font", true, CFGF_NONE),
        CFG_BOOL("pager_outline_enabled", true, CFGF_NONE),
        CFG_BOOL("pager_labled", true, CFGF_NONE),
        CFG_STR("pager_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("pager_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("pager_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("pager_fg_color_inactive", "#CCCCCC", CFGF_NONE),
        CFG_STR("pager_outline_color", "#FFFFFF", CFGF_NONE),
        CFG_STR("pager_text_color", "#FFFFFF", CFGF_NONE),
        CFG_STR("pager_font", "Sans", CFGF_NONE),
        CFG_STR("pager_font_alignment", "center", CFGF_NONE),
        CFG_INT("pager_font_size", 10, CFGF_NONE),

        CFG_BOOL("menu_use_global_decorations_style", true, CFGF_NONE),
        CFG_BOOL("menu_use_global_colors", true, CFGF_NONE),
        CFG_BOOL("menu_use_global_font", true, CFGF_NONE),
        CFG_BOOL("menu_outline_enabled", false, CFGF_NONE),
        CFG_STR("menu_decorations_style", "flat", CFGF_NONE),
        CFG_STR("menu_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("menu_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("menu_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("menu_fg_color_inactive", "#CCCCCC", CFGF_NONE),
        CFG_STR("menu_outline_color", "#FFFFFF", CFGF_NONE),
        CFG_FLOAT("menu_opacity", 1.0, CFGF_NONE),
        CFG_STR("menu_font", "Sans", CFGF_NONE),
        CFG_STR("menu_font_alignment", "center", CFGF_NONE),
        CFG_INT("menu_font_size", 10, CFGF_NONE),

        CFG_INT("rootmenu_height", 26, CFGF_NONE),

        CFG_BOOL("tray_use_global_decorations_style", true, CFGF_NONE),
        CFG_BOOL("tray_use_global_colors", true, CFGF_NONE),
        CFG_BOOL("tray_use_global_font", true, CFGF_NONE),
        CFG_BOOL("tray_use_menu_icon", false, CFGF_NONE),
        CFG_BOOL("tray_outline_enabled", false, CFGF_NONE),
        CFG_INT("tray_systray_size", 24, CFGF_NONE),
        CFG_INT("tray_systray_spacing", 4, CFGF_NONE),
        CFG_STR("tray_decorations_style", "flat", CFGF_NONE),
        CFG_STR("tray_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("tray_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("tray_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("tray_fg_color_inactive", "#CCCCCC", CFGF_NONE),
        CFG_STR("tray_outline_color_active", "#FFFFFF", CFGF_NONE),
        CFG_STR("tray_outline_color_inactive", "#FFFFFF", CFGF_NONE),
        CFG_STR("tray_menu_icon", "jwm-blue", CFGF_NONE),
        CFG_STR("tray_menu_text", "Menu", CFGF_NONE),
        CFG_FLOAT("tray_opacity", 0.95, CFGF_NONE),
        CFG_STR("tray_font", "Sans", CFGF_NONE),
        CFG_STR("tray_font_alignment", "center", CFGF_NONE),
        CFG_INT("tray_font_size", 10, CFGF_NONE),
        CFG_SEC("tray", tray_opts, CFGF_MULTI | CFGF_TITLE),

        CFG_BOOL("tasklist_use_global_decorations_style", true, CFGF_NONE),
        CFG_BOOL("tasklist_use_global_colors", true, CFGF_NONE),
        CFG_BOOL("tasklist_use_global_font", true, CFGF_NONE),
        CFG_BOOL("tasklist_outline_enabled", false, CFGF_NONE),
        CFG_STR("tasklist_decorations_style", "flat", CFGF_NONE),
        CFG_STR("tasklist_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("tasklist_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("tasklist_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("tasklist_fg_color_inactive", "#CCCCCC", CFGF_NONE),
        CFG_STR("tasklist_outline_color_active", "#FFFFFF", CFGF_NONE),
        CFG_STR("tasklist_outline_color_inactive", "#FFFFFF", CFGF_NONE),
        CFG_STR("tasklist_font", "Sans", CFGF_NONE),
        CFG_STR("tasklist_font_alignment", "center", CFGF_NONE),
        CFG_INT("tasklist_font_size", 10, CFGF_NONE),

        CFG_INT("desktop_workspaces", 2, CFGF_NONE),
        CFG_STR("desktop_background_type", "solid", CFGF_NONE),
        CFG_STR("desktop_background", "#333333", CFGF_NONE),

        CFG_SEC("keybind", keybind_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_SEC("autostart", autostart_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_END()
    };

    *cfg = cfg_init(opts, CFGF_NONE);

    if (ReadConfigFile(*cfg, "jwms.conf", "~/.config/jwms/", "/etc/jwms/") != 0)
        return -1;

    *jwm = calloc(1, sizeof(**jwm));

    // No input checking here yet...
    (*jwm)->global_bg_color_active = cfg_getstr(*cfg, "global_bg_color_active");
    (*jwm)->global_decorations_style = cfg_getstr(*cfg, "global_decorations_style");
    (*jwm)->global_bg_color_inactive = cfg_getstr(*cfg, "global_bg_color_inactive");
    (*jwm)->global_fg_color_active = cfg_getstr(*cfg, "global_fg_color_active");
    (*jwm)->global_fg_color_inactive = cfg_getstr(*cfg, "global_fg_color_inactive");
    (*jwm)->global_outline_color = cfg_getstr(*cfg, "global_outline_color");
    (*jwm)->global_preferred_icon_size = GetValidDefaultIconSize(cfg_getint(*cfg, "global_preferred_icon_size"));
    (*jwm)->global_font = cfg_getstr(*cfg, "global_font");
    (*jwm)->global_font_alignment = cfg_getstr(*cfg, "global_font_alignment");
    (*jwm)->global_font_size = cfg_getint(*cfg, "global_font_size");

    (*jwm)->terminal_name = cfg_getstr(*cfg, "global_terminal");
    (*jwm)->filemanager_name = cfg_getstr(*cfg, "global_filemanager");
    (*jwm)->browser_name = cfg_getstr(*cfg, "global_browser");

    (*jwm)->window_use_global_decorations_style = cfg_getbool(*cfg, "window_use_global_decorations_style");
    (*jwm)->window_use_global_colors = cfg_getbool(*cfg, "window_use_global_colors");
    (*jwm)->window_use_global_font = cfg_getbool(*cfg, "window_use_global_font");
    (*jwm)->window_use_aerosnap = cfg_getbool(*cfg, "window_use_aerosnap");
    (*jwm)->window_height = cfg_getint(*cfg, "window_height");
    (*jwm)->window_width = cfg_getint(*cfg, "window_width");
    (*jwm)->window_corner_rounding = cfg_getint(*cfg, "window_corner_rounding");
    (*jwm)->window_decorations_style = cfg_getstr(*cfg, "window_decorations_style");
    (*jwm)->window_bg_color_active = cfg_getstr(*cfg, "window_bg_color_active");
    (*jwm)->window_bg_color_inactive = cfg_getstr(*cfg, "window_bg_color_inactive");
    (*jwm)->window_fg_color_active = cfg_getstr(*cfg, "window_fg_color_active");
    (*jwm)->window_fg_color_inactive = cfg_getstr(*cfg, "window_fg_color_inactive");
    (*jwm)->window_outline_color_active = cfg_getstr(*cfg, "window_outline_color_active");
    (*jwm)->window_outline_color_inactive = cfg_getstr(*cfg, "window_outline_color_inactive");
    (*jwm)->window_outline_enabled = cfg_getbool(*cfg, "window_outline_enabled");
    (*jwm)->window_opacity_active = GetValidOpacity(cfg_getfloat(*cfg, "window_opacity_active"));
    (*jwm)->window_opacity_inactive = GetValidOpacity(cfg_getfloat(*cfg, "window_opacity_inactive"));
    (*jwm)->window_font = cfg_getstr(*cfg, "window_font");
    (*jwm)->window_font_alignment = cfg_getstr(*cfg, "window_font_alignment");
    (*jwm)->window_font_size = cfg_getint(*cfg, "window_font_size");

    (*jwm)->window_focus_model = cfg_getstr(*cfg, "window_focus_model");
    (*jwm)->window_snap_mode = cfg_getstr(*cfg, "window_snap_mode");
    (*jwm)->window_snap_distance = cfg_getint(*cfg, "window_snap_distance");
    (*jwm)->window_move_mode = cfg_getstr(*cfg, "window_move_mode");
    (*jwm)->window_resize_mode = cfg_getstr(*cfg, "window_resize_mode");

    (*jwm)->clock_use_global_colors = cfg_getbool(*cfg, "clock_use_global_colors");
    (*jwm)->clock_use_global_font = cfg_getbool(*cfg, "clock_use_global_font");
    (*jwm)->clock_bg_color_inactive = cfg_getstr(*cfg, "clock_bg_color");
    (*jwm)->clock_fg_color_inactive = cfg_getstr(*cfg, "clock_fg_color");
    (*jwm)->clock_font = cfg_getstr(*cfg, "clock_font");
    (*jwm)->clock_font_alignment = cfg_getstr(*cfg, "clock_font_alignment");
    (*jwm)->clock_font_size = cfg_getint(*cfg, "clock_font_size");

    (*jwm)->pager_use_global_colors = cfg_getbool(*cfg, "pager_use_global_colors");
    (*jwm)->pager_use_global_font = cfg_getbool(*cfg, "pager_use_global_font");
    (*jwm)->pager_outline_enabled = cfg_getbool(*cfg, "pager_outline_enabled");
    (*jwm)->pager_labled = cfg_getbool(*cfg, "pager_labled");
    (*jwm)->pager_bg_color_active = cfg_getstr(*cfg, "pager_bg_color_active");
    (*jwm)->pager_bg_color_inactive = cfg_getstr(*cfg, "pager_bg_color_inactive");
    (*jwm)->pager_fg_color_active = cfg_getstr(*cfg, "pager_fg_color_active");
    (*jwm)->pager_fg_color_inactive = cfg_getstr(*cfg, "pager_fg_color_inactive");
    (*jwm)->pager_outline_color_inactive = cfg_getstr(*cfg, "pager_outline_color");
    (*jwm)->pager_text_color = cfg_getstr(*cfg, "pager_text_color");
    (*jwm)->pager_font = cfg_getstr(*cfg, "pager_font");
    (*jwm)->pager_font_alignment = cfg_getstr(*cfg, "pager_font_alignment");
    (*jwm)->pager_font_size = cfg_getint(*cfg, "pager_font_size");

    (*jwm)->menu_use_global_decorations_style = cfg_getbool(*cfg, "menu_use_global_decorations_style");
    (*jwm)->menu_use_global_colors = cfg_getbool(*cfg, "menu_use_global_colors");
    (*jwm)->menu_use_global_font = cfg_getbool(*cfg, "menu_use_global_font");
    (*jwm)->menu_outline_enabled = cfg_getbool(*cfg, "menu_outline_enabled");
    (*jwm)->menu_decorations_style = cfg_getstr(*cfg, "menu_decorations_style");
    (*jwm)->menu_bg_color_active = cfg_getstr(*cfg, "menu_bg_color_active");
    (*jwm)->menu_bg_color_inactive = cfg_getstr(*cfg, "menu_bg_color_inactive");
    (*jwm)->menu_fg_color_active = cfg_getstr(*cfg, "menu_fg_color_active");
    (*jwm)->menu_fg_color_inactive = cfg_getstr(*cfg, "menu_fg_color_inactive");
    (*jwm)->menu_outline_color = cfg_getstr(*cfg, "menu_outline_color");
    (*jwm)->menu_opacity = GetValidOpacity(cfg_getfloat(*cfg, "menu_opacity"));
    (*jwm)->menu_font = cfg_getstr(*cfg, "menu_font");
    (*jwm)->menu_font_alignment = cfg_getstr(*cfg, "menu_font_alignment");
    (*jwm)->menu_font_size = cfg_getint(*cfg, "menu_font_size");

    ParseTrays(*jwm, *cfg);

    (*jwm)->tray_use_global_decorations_style = cfg_getbool(*cfg, "tray_use_global_decorations_style");
    (*jwm)->tray_use_global_colors = cfg_getbool(*cfg, "tray_use_global_colors");
    (*jwm)->tray_use_global_font = cfg_getbool(*cfg, "tray_use_global_font");
    (*jwm)->tray_use_menu_icon = cfg_getbool(*cfg, "tray_use_menu_icon");
    (*jwm)->tray_outline_enabled = cfg_getbool(*cfg, "tray_outline_enabled");
    (*jwm)->tray_systray_size = cfg_getint(*cfg, "tray_systray_size");
    (*jwm)->tray_systray_spacing = cfg_getint(*cfg, "tray_systray_spacing");
    (*jwm)->tray_decorations_style = cfg_getstr(*cfg, "tray_decorations_style");
    (*jwm)->tray_bg_color_active = cfg_getstr(*cfg, "tray_bg_color_active");
    (*jwm)->tray_bg_color_inactive = cfg_getstr(*cfg, "tray_bg_color_inactive");
    (*jwm)->tray_fg_color_active = cfg_getstr(*cfg, "tray_fg_color_active");
    (*jwm)->tray_fg_color_inactive = cfg_getstr(*cfg, "tray_fg_color_inactive");
    (*jwm)->tray_outline_color_active = cfg_getstr(*cfg, "tray_outline_color_active");
    (*jwm)->tray_outline_color_inactive = cfg_getstr(*cfg, "tray_outline_color_inactive");
    (*jwm)->tray_menu_icon = cfg_getstr(*cfg, "tray_menu_icon");
    (*jwm)->tray_menu_text = cfg_getstr(*cfg, "tray_menu_text");
    (*jwm)->tray_opacity = GetValidOpacity(cfg_getfloat(*cfg, "tray_opacity"));
    (*jwm)->tray_font = cfg_getstr(*cfg, "tray_font");
    (*jwm)->tray_font_alignment = cfg_getstr(*cfg, "tray_font_alignment");
    (*jwm)->tray_font_size = cfg_getint(*cfg, "tray_font_size");

    (*jwm)->tasklist_use_global_decorations_style = cfg_getbool(*cfg, "tasklist_use_global_decorations_style");
    (*jwm)->tray_use_global_colors = cfg_getbool(*cfg, "tasklist_use_global_colors");
    (*jwm)->tasklist_use_global_font = cfg_getbool(*cfg, "tasklist_use_global_font");
    (*jwm)->tasklist_outline_enabled = cfg_getbool(*cfg, "tasklist_outline_enabled");
    (*jwm)->tasklist_decorations_style = cfg_getstr(*cfg, "tasklist_decorations_style");
    (*jwm)->tasklist_bg_color_active = cfg_getstr(*cfg, "tasklist_bg_color_active");
    (*jwm)->tasklist_bg_color_inactive = cfg_getstr(*cfg, "tasklist_bg_color_inactive");
    (*jwm)->tasklist_fg_color_active = cfg_getstr(*cfg, "tasklist_fg_color_active");
    (*jwm)->tasklist_fg_color_inactive = cfg_getstr(*cfg, "tasklist_fg_color_inactive");
    (*jwm)->tasklist_outline_color_active = cfg_getstr(*cfg, "tasklist_outline_color_active");
    (*jwm)->tasklist_outline_color_inactive = cfg_getstr(*cfg, "tasklist_outline_color_inactive");
    (*jwm)->tasklist_font = cfg_getstr(*cfg, "tasklist_font");
    (*jwm)->tasklist_font_alignment = cfg_getstr(*cfg, "tasklist_font_alignment");
    (*jwm)->tasklist_font_size = cfg_getint(*cfg, "tasklist_font_size");

    (*jwm)->desktop_workspaces = cfg_getint(*cfg, "desktop_workspaces");
    (*jwm)->desktop_background_type = cfg_getstr(*cfg, "desktop_background_type");
    (*jwm)->desktop_background = cfg_getstr(*cfg, "desktop_background");

    (*jwm)->root_menu_height = cfg_getint(*cfg, "rootmenu_height");

    return 0;
}
