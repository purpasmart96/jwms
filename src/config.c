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
//#include "ini.h"
#include "icons.h"
#include "desktop_entries.h"

#include "config.h"

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
    jwm->tray_use_global_colors ? jwm->global_outline_color : jwm->type##_outline_color_##state

static const char *category_icons[] =
{
    "applications-multimedia",
    "applications-multimedia",
    "applications-multimedia",
    "applications-development",
    "applications-education",
    "applications-games",
    "applications-graphics",
    "applications-internet",
    "applications-office",
    "applications-science",
    "preferences-desktop",
    "applications-system",
    "applications-utilities",
};

static const Pair style_types[] =
{
    {"WindowStyle",   WindowStyle  },
    {"ClockStyle",    ClockStyle   },
    {"TrayStyle",     TrayStyle    },
    {"TaskListStyle", TaskListStyle},
    {"PagerStyle",    PagerStyle   },
    {"MenuStyle",     MenuStyle    },
    {"PopupStyle",    PopupStyle   },
};

static int GetStyleType(char *key)
{
    for (int i = 0; i < ARRAY_SIZE(style_types); i++)
    {
        if (strcmp(style_types[i].key, key) == 0)
            return i;
    }
    return -1;
}

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

static float GetValidOpacity(float opacity)
{
    return CLAMP(opacity, 0.0, 1.0);
}

static int ReadConfigFile(cfg_t *cfg)
{
    const char *system_cfg = "/etc/jwms/jwms.conf";
    const char *user_cfg = "jwms.conf";
    int ret = -1;

	ret = cfg_parse(cfg, user_cfg);
    if (ret != CFG_SUCCESS)
    {
        if (ret == CFG_FILE_ERROR)
        {
			printf("Error opening %s\nUsing system config instead\n", user_cfg);
        }
        else
        {
        	printf("Error parsing %s\nUsing system config instead\n", user_cfg);
        }
	}
    else
    {
        return ret;
    }

	ret = cfg_parse(cfg, system_cfg);
	if (ret != CFG_SUCCESS)
    {
        if (ret == CFG_FILE_ERROR)
        {
			//printf("Failed opening %s\n Using default values\n", system_cfg);
            printf("Failed opening %s\n", system_cfg);
            return ret;
        } 
        else
        {
        	printf("Failed parsing %s\n", system_cfg);
            return ret;
        }
	}

    return ret;
}

static int CreateJWMFolder(JWM *jwm)
{
    char path[512];
    const char *home = getenv("HOME");
    const char *dir = "/.config/jwm/";

    strlcpy(path, home, sizeof(path));
    strlcat(path, dir, sizeof(path));

    // Check if directory exists
    if (access(path, F_OK) != 0)
    {
        // Create it
        if (mkdir(path, 0755) != 0)
        {
            fprintf(stderr, "Failed to create directory\n");
            return -1;
        }
        printf("Created directory %s\n", path);
    }

    jwm->autogen_config_path = strdup(path);

    return 0;
}

static int GenJWMStartup(JWM *jwm)
{
    char path[512];
    const char *fname = "startup_test";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the startup xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("   <StartupCommand>~/.config/jwm/autostart</StartupCommand>\n");
    WRITE_CFG("   <RestartCommand>~/.config/jwm/autostart</RestartCommand>\n");
    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

static void GenJWMIcons(JWM *jwm)
{
    char path[512];
    const char *fname = "icons_test";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return;
    }

    printf("Writing to %s\n", path);

    // Start of the icon xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("   <IconPath>/usr/share/jwm/</IconPath>\n");
    WRITE_CFG("   <IconPath>/usr/share/pixmaps/</IconPath>\n");
    WRITE_CFG("</JWM>");

    fclose(fp);
}

static int GenJWMGroup(JWM *jwm)
{
    char path[512];
    const char *fname = "group_test";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the group xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("    <Group>\n");
    WRITE_CFG("        <Option>tiled</Option>\n");
    //WRITE_CFG("        <Option>aerosnap</Option>\n");
    WRITE_CFG("    </Group>\n");
    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

static int GenJWMPreferences(JWM *jwm)
{
    char path[512];
    const char *fname = "prefs_test";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the prefs xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("   <Desktops width=\"%d\" height=\"%d\">\n", 2, 1);
    WRITE_CFG("       <Background type=\"%s\">%s</Background>\n", "solid", "555555");
    WRITE_CFG("   </Desktops>\n");
    WRITE_CFG("   <DoubleClickSpeed>%d</DoubleClickSpeed>\n", 400);
    WRITE_CFG("   <DoubleClickDelta>%d</DoubleClickDelta>\n", 2);
    WRITE_CFG("   <FocusModel>%s</FocusModel>\n", jwm->window_focus_model);
    WRITE_CFG("   <SnapMode distance=\"%d\">%s</SnapMode>\n", jwm->window_snap_distance, jwm->window_snap_mode);
    WRITE_CFG("   <MoveMode>%s</MoveMode>\n", jwm->window_move_mode);
    WRITE_CFG("   <ResizeMode>%s</ResizeMode>\n", jwm->window_resize_mode);
    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

typedef struct
{
    char *in;
    char *out;
} ModKeyMap;

static const ModKeyMap key_mods[] =
{
    {"Alt", "A"},
    {"Ctrl", "C"},
    {"Shift", "S"}
};

static char *GetJWMKeyMod(char *in)
{
    for (size_t i = 0; i < ARRAY_SIZE(key_mods); i++)
    {
        if (strcmp(key_mods[i].in, in) == 0)
            return key_mods[i].out;
    }
    return NULL;
}

static int GenJWMBinds(JWM *jwm, cfg_t *cfg)
{
    char path[512];
    char keymods[64];
    const char *fname = "binds_test";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the binds xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");

    int n = cfg_size(cfg, "keybind");
	printf("\nFound %d keybinds:\n", n);

	for (int i = 0; i < n; i++)
    {
		cfg_t *keybind = cfg_getnsec(cfg, "keybind", i);

        int num_mods = cfg_size(keybind, "mods");
        printf("keybind %u: %s\n", i + 1, cfg_title(keybind));
        char *cmd = cfg_getstr(keybind, "command");
        printf("command: %s\n", cmd);

        for (int j = 0; j < num_mods; j++)
        {
            char *in_keymod = cfg_getnstr(keybind,"mods", j);
            printf("keymod %d: %s\n", j, in_keymod);
            char *out_keymod = GetJWMKeyMod(in_keymod);
            if (out_keymod == NULL)
                return -1;
            strlcat(keymods, out_keymod, sizeof(keymods));
        }

        char *key = cfg_getstr(keybind, "key");
        printf("key: %s\n", key);
        printf("\n");

        if (!num_mods)
        {
            WRITE_CFG("    <Key key=\"%s\">%s</Key>\n", key, cmd);
        }
        else
        {
            WRITE_CFG("    <Key mask=\"%s\" key=\"%s\">%s</Key>\n", keymods, key, cmd);
        }
        keymods[0] = '\0';
	}

    //WRITE_CFG("    <Mouse context=\"root\" button=\"4\">ldesktop</Mouse>\n");
    //WRITE_CFG("    <Mouse context=\"root\" button=\"5\">rdesktop</Mouse>\n");

    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

static void WriteJWMStyle(JWM *jwm, FILE *fp, Styles style)
{
    WRITE_CFG("    <%s", style_types[style].key);

    switch (style)
    {
        case WindowStyle:
        {
            //WRITE_CFG("    <%s decorations=\"%s\">\n", style_types[style].key, "flat");
            WRITE_CFG(" decorations=\"%s\">\n", GetDecorationsStyle(window));
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(window), GetFontName(window), GetFontSize(window));
            WRITE_CFG("        <Height>%d</Height>\n", jwm->window_height);
            WRITE_CFG("        <Width>%d</Width>\n", jwm->window_width);
            WRITE_CFG("        <Corner>%d</Corner>\n", jwm->window_corner_rounding);
            WRITE_CFG("        <Foreground>%s</Foreground>\n", GetFGColor(window, inactive));
            WRITE_CFG("        <Background>%s</Background>\n", GetBGColor(window, inactive));
            WRITE_CFG("        <Opacity>%.2f</Opacity>\n", jwm->window_opacity_inactive);

            if (jwm->window_outline_enabled)
            {
                WRITE_CFG("        <Outline>%s</Outline>\n", jwm->window_outline_color_inactive);
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(window, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(window, active));
                WRITE_CFG("            <Opacity>%.2f</Opacity>\n", jwm->window_opacity_active);
                WRITE_CFG("            <Outline>%s</Outline>\n", GetOutlineColor(window, active));
            }
            else
            {
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(window, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(window, active));
                WRITE_CFG("            <Opacity>%.2f</Opacity>\n", jwm->window_opacity_active);
            }

            WRITE_CFG("        </Active>\n");
            break;
        }
        case ClockStyle:
        {
            WRITE_CFG(">\n");
            //WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(clock), GetFontName(clock), GetFontSize(clock));
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", jwm->global_font_alignment, jwm->global_font, jwm->global_font_size);
            WRITE_CFG("        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            break;
        }
        case TrayStyle:
        {
            WRITE_CFG(" decorations=\"%s\">\n", GetDecorationsStyle(tray));
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(tray), GetFontName(tray), GetFontSize(tray));
            WRITE_CFG("        <Foreground>%s</Foreground>\n", GetFGColor(tray, inactive));
            WRITE_CFG("        <Background>%s</Background>\n", GetBGColor(tray, inactive));

            if (jwm->tray_outline_enabled)
            {
                WRITE_CFG("        <Outline>%s</Outline>\n", GetOutlineColor(tray, inactive));
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(tray, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(tray, active));
                WRITE_CFG("            <Outline>%s</Outline>\n", GetOutlineColor(tray, active));
            }
            else
            {
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(tray, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(tray, active));
            }
    
            WRITE_CFG("        </Active>\n");
            WRITE_CFG("        <Opacity>%.2f</Opacity>\n", jwm->tray_opacity);
            break;
        }
        case TaskListStyle:
        {
            WRITE_CFG(" list=\"all\" group=\"true\">\n");
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(tray), GetFontName(tray), GetFontSize(tray));
            WRITE_CFG("        <Foreground>%s</Foreground>\n", GetFGColor(tray, inactive));
            WRITE_CFG("        <Background>%s</Background>\n", GetBGColor(tray, inactive));

            if (jwm->tray_outline_enabled)
            {
                WRITE_CFG("        <Outline>%s</Outline>\n", GetOutlineColor(tray, inactive));
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(tray, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(tray, active));
                WRITE_CFG("            <Outline>%s</Outline>\n", GetOutlineColor(tray, active));
            }
            else
            {
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(tray, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(tray, active));
            }

            WRITE_CFG("        </Active>\n");
            break;
        }
        case PagerStyle:
        {
            WRITE_CFG(">\n");
            WRITE_CFG("        <Outline>%s</Outline>\n", jwm->global_outline_color);
            WRITE_CFG("        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", jwm->global_font_alignment, jwm->global_font, jwm->global_font_size);
            WRITE_CFG("        <Text>#FFFFFF</Text>\n");
            WRITE_CFG("        <Active>\n");
            WRITE_CFG("            <Foreground>%s</Foreground>\n", jwm->global_fg_color_active);
            WRITE_CFG("            <Background>%s</Background>\n", jwm->global_bg_color_active);
            WRITE_CFG("        </Active>\n");
            break;
        }
        case MenuStyle:
        {
            char *fg_color_inactive = jwm->menu_fg_color_inactive;
            char *fg_color_active = jwm->menu_fg_color_active;
            char *bg_color_inactive = jwm->menu_bg_color_inactive;
            char *bg_color_active = jwm->menu_bg_color_active;
            char *menu_outline_color = jwm->menu_outline_color;

            if (jwm->menu_use_global_colors)
            {
                fg_color_inactive = jwm->global_fg_color_inactive;
                fg_color_active = jwm->global_fg_color_active;
                bg_color_inactive = jwm->global_bg_color_inactive;
                bg_color_active = jwm->global_bg_color_active;
                menu_outline_color = jwm->global_outline_color;
            }

            WRITE_CFG(" decorations=\"%s\">\n", GetDecorationsStyle(menu));
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(menu), GetFontName(menu), GetFontSize(menu));
            WRITE_CFG("        <Foreground>%s</Foreground>\n", fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", bg_color_inactive);
            if (jwm->menu_outline_enabled)
                WRITE_CFG("        <Outline>%s</Outline>\n", menu_outline_color);
            WRITE_CFG("        <Active>\n");
            WRITE_CFG("            <Foreground>%s</Foreground>\n", fg_color_active);
            WRITE_CFG("            <Background>%s</Background>\n", bg_color_active);
            WRITE_CFG("        </Active>\n");
            WRITE_CFG("        <Opacity>%.2f</Opacity>\n", jwm->menu_opacity);
            break;
        }
        case PopupStyle:
        {
            WRITE_CFG(" list=\"all\" group=\"true\">\n");
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", jwm->global_font_alignment, jwm->global_font, jwm->global_font_size);
            WRITE_CFG("        <Outline>%s</Outline>\n", jwm->global_outline_color);
            WRITE_CFG("        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            break;
        }
        default:
            break;
    }

    WRITE_CFG("    </%s>\n", style_types[style].key);
}

static int GenJWMStyles(JWM *jwm)
{
    char path[512];
    const char *fname = "styles_test";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the styles/theme xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");

    WriteJWMStyle(jwm, fp, WindowStyle);
    WriteJWMStyle(jwm, fp, ClockStyle);
    WriteJWMStyle(jwm, fp, TrayStyle);
    WriteJWMStyle(jwm, fp, TaskListStyle);
    WriteJWMStyle(jwm, fp, PagerStyle);
    WriteJWMStyle(jwm, fp, MenuStyle);
    WriteJWMStyle(jwm, fp, PopupStyle);

    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

static void WriteJWMTray(JWM *jwm, BTreeNode *entries, FILE *fp, HashMap *icons)
{
    // Get Terminal
    XDGDesktopEntry *terminal = GetCoreProgram(entries, TerminalEmulator, jwm->terminal_name);
    if (terminal != NULL)
    {
        const char *icon = HashMapGet(icons, terminal->icon);
        if (icon == NULL)
        {
            icon = terminal->icon;
        }

        WRITE_CFG("       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", terminal->name, icon, terminal->exec);
    }
    else
    {
        WRITE_CFG("       <TrayButton popup=\"Terminal\" icon=\"terminal\">exec:x-terminal-emulator</TrayButton>\n");
    }

    WRITE_CFG("       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    // Get FileManager
    XDGDesktopEntry *filemanager = GetCoreProgram(entries, FileManager, jwm->filemanager_name);

    if (filemanager != NULL)
    {
        const char *icon = HashMapGet(icons, filemanager->icon);
        if (icon == NULL)
        {
            icon = filemanager->icon;
        }

        WRITE_CFG("       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", filemanager->name, icon, filemanager->exec);
    }
    else
    {
        WRITE_CFG("       <TrayButton popup=\"File Manager\" icon=\"system-file-manager\">exec:spacefm</TrayButton>\n");
    }

    WRITE_CFG("       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);

    // Get WebBrowser
    XDGDesktopEntry *browser = GetCoreProgram(entries, WebBrowser, jwm->browser_name);
    if (browser != NULL)
    {
        const char *icon = HashMapGet(icons, browser->icon);
        if (icon == NULL)
        {
            icon = browser->icon;
        }
        WRITE_CFG("       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", browser->name, icon, browser->exec);
    }
    else
    {
        WRITE_CFG("       <TrayButton popup=\"Web browser\" icon=\"firefox\">exec:firefox</TrayButton>\n");
    }

    WRITE_CFG("       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
}

static void GenJWMTray(JWM *jwm, BTreeNode *entries, HashMap *icons)
{
    char path[512];
    const char *fname = "tray_test";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return;
    }

    printf("Writing to %s\n", path);

    const char *auto_hide = jwm->tray_auto_hide ? "on" : "off";
    // Start of the tray xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");

    // This can be improved...
    if (jwm->tray_pos == Bottom)
    {
        WRITE_CFG("   <Tray x=\"0\" y=\"-1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
    }
    else if (jwm->tray_pos == Top)
    {
        WRITE_CFG("   <Tray x=\"0\" y=\"+1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
    }
    else
    {
        WRITE_CFG("   <Tray x=\"0\" y=\"-1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
        printf("Error! Tray Position Not Implemented Yet!\n");
    }

    if (jwm->tray_use_menu_icon)
    {
        char *resolved_icon = FindIcon(jwm->tray_menu_icon, 32, 1);

        // Use the new icon if found, otherwise use the provided icon
        const char *icon_path = (resolved_icon != NULL) ? resolved_icon : jwm->tray_menu_icon;

        // Write configuration with the icon
        WRITE_CFG("       <TrayButton label=\"%s\" icon=\"%s\">root:1</TrayButton>\n", jwm->tray_menu_text, icon_path);

        // Free the resolved icon if it was dynamically allocated
        if (resolved_icon != NULL)
        {
            free(resolved_icon);
        }
    }
    else
    {
        // Write configuration without the icon
        WRITE_CFG("       <TrayButton label=\"%s\">root:1</TrayButton>\n", jwm->tray_menu_text);
    }

    char *show_desktop_icon = FindIcon("desktop", 32, 1);
    WRITE_CFG("       <Spacer width=\"4\"/>\n");
    WriteJWMTray(jwm, entries, fp, icons);
    WRITE_CFG("       <TaskList maxwidth=\"200\"/>\n");
    WRITE_CFG("       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    WRITE_CFG("       <TrayButton popup=\"Show Desktop\" icon=\"%s\">showdesktop</TrayButton>\n", show_desktop_icon);
    WRITE_CFG("       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    WRITE_CFG("       <Pager labeled=\"true\"/>\n");
    WRITE_CFG("       <Spacer width=\"%d\"/>\n",jwm->tray_icon_spacing);
    WRITE_CFG("       <Dock/>\n");
    WRITE_CFG("       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    WRITE_CFG("       <Clock format=\"%%l:%%M %%p\"><Button mask=\"123\">exec:xclock</Button></Clock>\n");
    WRITE_CFG("       <Spacer width=\"4\"/>\n");
    WRITE_CFG("   </Tray>\n");
    WRITE_CFG("</JWM>");

    free(show_desktop_icon);
    fclose(fp);
}

// Recursively traverse the BST in-order and write each entry to the file
static void WriteMenuCategoriesInOrder(BTreeNode *entries, HashMap *icons, FILE *fp, XDGMainCategories category)
{
    if (entries != NULL)
    {
        WriteMenuCategoriesInOrder(entries->left, icons, fp, category);

        XDGDesktopEntry *entry = entries->data;
        if (entry->category == category)
        {
            const char *icon = HashMapGet(icons, entry->icon);
            if (icon == NULL)
            {
                icon = entry->icon;
            }

            if (!entry->terminal_required)
            {
                WRITE_CFG("            <Program icon=\"%s\" label=\"%s\">%s</Program>\n",
                            icon, entry->name, entry->exec);
            }
            else
            {
                WRITE_CFG("            <Program icon=\"%s\" label=\"%s\">x-terminal-emulator -e %s</Program>\n",
                            icon, entry->name, entry->exec);
            }
        }
        WriteMenuCategoriesInOrder(entries->right, icons, fp, category);
    }
}

static void WriteJWMRootMenuCategoryList(BTreeNode *entries, HashMap *icons, FILE *fp, XDGMainCategories category, const char *category_name)
{
    char *category_icon = FindIcon(category_icons[category], 32, 1);
    WRITE_CFG("       <Menu icon=\"%s\" label=\"%s\">\n", category_icon, category_name);
    free(category_icon);
 
    WriteMenuCategoriesInOrder(entries, icons, fp, category);

    WRITE_CFG("       </Menu>\n");
}

static void GenJWMRootMenu(JWM *jwm, BTreeNode *entries, HashMap *icons)
{
    char path[512];
    const char *fname = "menu_test";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return;
    }

    printf("Writing to %s\n", path);

    // Start of the root menu xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("    <RootMenu height=\"%d\" onroot=\"12\">\n", jwm->root_menu_height);

    WriteJWMRootMenuCategoryList(entries, icons, fp, Network, "Internet");
    WriteJWMRootMenuCategoryList(entries, icons, fp, AudioVideo, "Multimedia");
    WriteJWMRootMenuCategoryList(entries, icons, fp, Development, "Development");
    WriteJWMRootMenuCategoryList(entries, icons, fp, Office, "Office");
    WriteJWMRootMenuCategoryList(entries, icons, fp, Graphics,"Graphics");
    //WriteJWMRootMenuCategoryList(entries, icons, fp, Video,"Multimedia");
    WriteJWMRootMenuCategoryList(entries, icons, fp, Settings, "Settings");
    WriteJWMRootMenuCategoryList(entries, icons, fp, System, "System");
    WriteJWMRootMenuCategoryList(entries, icons, fp, Utility,"Utilities");

    WRITE_CFG("        <Restart label=\"Refresh\" icon=\"view-refresh\"/>\n");
    WRITE_CFG("        <Exit label=\"Logout\" icon=\"system-log-out\"/>\n");
    // Let's assume were using systemd for now
    WRITE_CFG("        <Program icon=\"system-reboot\" label=\"Restart\">systemctl reboot</Program>\n");
    WRITE_CFG("        <Program icon=\"system-shutdown\" label=\"Shutdown\">systemctl poweroff</Program>\n");
    WRITE_CFG("    </RootMenu>\n");
    WRITE_CFG("</JWM>");

    fclose(fp);
}

static int GenJWMRCFile(JWM *jwm)
{
    char path[512];
    const char *home = getenv("HOME");
    const char *fname = ".jwmrc";

    strlcpy(path, home, sizeof(path));
    strlcat(path, "/", sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the .jwmrc file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/menu_test</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/startup_test</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/tray_test</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/group_test</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/styles_test</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/icons_test</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/prefs_test</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/binds_test</Include>\n");
    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

int WriteJWMConfig(BTreeNode *entries, HashMap *icons)
{
	cfg_opt_t keybind_opts[] =
    {
        CFG_STR_LIST("mods", "{Ctrl, Alt}", CFGF_NONE),
        CFG_STR("key", "T", CFGF_NONE),
        CFG_STR("command", "exec:xterm", CFGF_NONE),
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
        CFG_STR("global_font", "Sans", CFGF_NONE),
        CFG_STR("global_font_alignment", "center", CFGF_NONE),
        CFG_INT("global_font_size", 10, CFGF_NONE),

        CFG_BOOL("window_use_global_decorations_style", true, CFGF_NONE),
        CFG_BOOL("window_use_global_colors", true, CFGF_NONE),
        CFG_BOOL("window_use_global_font", true, CFGF_NONE),
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
        CFG_STR("tray_decorations_style", "flat", CFGF_NONE),
        CFG_STR("tray_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("tray_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("tray_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("tray_fg_color_inactive", "#CCCCCC", CFGF_NONE),
        CFG_STR("tray_outline_color_active", "#FFFFFF", CFGF_NONE),
        CFG_STR("tray_outline_color_inactive", "#FFFFFF", CFGF_NONE),
        CFG_STR("tray_position", "bottom", CFGF_NONE),
        CFG_BOOL("tray_autohide", false, CFGF_NONE),
        CFG_STR("tray_menu_icon", "jwm-blue", CFGF_NONE),
        CFG_STR("tray_menu_text", "Menu", CFGF_NONE),
        CFG_INT("tray_height", 32, CFGF_NONE),
        CFG_INT("tray_icon_spacing", 4, CFGF_NONE),
        CFG_FLOAT("tray_opacity", 0.95, CFGF_NONE),
        CFG_STR("tray_font", "Sans", CFGF_NONE),
        CFG_STR("tray_font_alignment", "center", CFGF_NONE),
        CFG_INT("tray_font_size", 10, CFGF_NONE),

        CFG_SEC("keybind", keybind_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_END()
    };

    cfg_t *cfg = cfg_init(opts, CFGF_NONE);

    if (ReadConfigFile(cfg) != 0)
        return -1;

    printf("rootmenu_height=%ld\n", cfg_getint(cfg, "rootmenu_height"));

    printf("global_browser=%s\n", cfg_getstr(cfg, "global_browser"));
    printf("tray_position=%s\n", cfg_getstr(cfg, "tray_position"));
    printf("tray_autohide=%d\n", cfg_getbool(cfg, "tray_autohide"));
    printf("tray_height=%ld\n", cfg_getint(cfg, "tray_height"));
    printf("tray_icon_spacing=%ld\n", cfg_getint(cfg, "tray_icon_spacing"));

    JWM *jwm = calloc(1, sizeof(*jwm));

    // No input checking here yet...
    jwm->global_decorations_style = cfg_getstr(cfg, "global_decorations_style");
    jwm->global_bg_color_active = cfg_getstr(cfg, "global_bg_color_active");
    jwm->global_bg_color_inactive = cfg_getstr(cfg, "global_bg_color_inactive");
    jwm->global_fg_color_active = cfg_getstr(cfg, "global_fg_color_active");
    jwm->global_fg_color_inactive = cfg_getstr(cfg, "global_fg_color_inactive");
    jwm->global_outline_color = cfg_getstr(cfg, "global_outline_color");
    jwm->global_font = cfg_getstr(cfg, "global_font");
    jwm->global_font_alignment = cfg_getstr(cfg, "global_font_alignment");
    jwm->global_font_size = cfg_getint(cfg, "global_font_size");

    jwm->terminal_name = cfg_getstr(cfg, "global_terminal");
    jwm->filemanager_name = cfg_getstr(cfg, "global_filemanager");
    jwm->browser_name = cfg_getstr(cfg, "global_browser");

    jwm->window_use_global_decorations_style = cfg_getbool(cfg, "window_use_global_decorations_style");
    jwm->window_use_global_colors = cfg_getbool(cfg, "window_use_global_colors");
    jwm->window_use_global_font = cfg_getbool(cfg, "window_use_global_font");
    jwm->window_height = cfg_getint(cfg, "window_height");
    jwm->window_width = cfg_getint(cfg, "window_width");
    jwm->window_corner_rounding = cfg_getint(cfg, "window_corner_rounding");
    jwm->window_decorations_style = cfg_getstr(cfg, "window_decorations_style");
    jwm->window_bg_color_active = cfg_getstr(cfg, "window_bg_color_active");
    jwm->window_bg_color_inactive = cfg_getstr(cfg, "window_bg_color_inactive");
    jwm->window_fg_color_active = cfg_getstr(cfg, "window_fg_color_active");
    jwm->window_fg_color_inactive = cfg_getstr(cfg, "window_fg_color_inactive");
    jwm->window_outline_color_active = cfg_getstr(cfg, "window_outline_color_active");
    jwm->window_outline_color_inactive = cfg_getstr(cfg, "window_outline_color_inactive");
    jwm->window_outline_enabled = cfg_getbool(cfg, "window_outline_enabled");
    jwm->window_opacity_active = GetValidOpacity(cfg_getfloat(cfg, "window_opacity_active"));
    jwm->window_opacity_inactive = GetValidOpacity(cfg_getfloat(cfg, "window_opacity_inactive"));
    jwm->window_font = cfg_getstr(cfg, "window_font");
    jwm->window_font_alignment = cfg_getstr(cfg, "window_font_alignment");
    jwm->window_font_size = cfg_getint(cfg, "window_font_size");

    jwm->window_focus_model = cfg_getstr(cfg, "window_focus_model");
    jwm->window_snap_mode = cfg_getstr(cfg, "window_snap_mode");
    jwm->window_snap_distance = cfg_getint(cfg, "window_snap_distance");
    jwm->window_move_mode = cfg_getstr(cfg, "window_move_mode");
    jwm->window_resize_mode = cfg_getstr(cfg, "window_resize_mode");

    jwm->menu_use_global_decorations_style = cfg_getbool(cfg, "menu_use_global_decorations_style");
    jwm->menu_use_global_colors = cfg_getbool(cfg, "menu_use_global_colors");
    jwm->menu_outline_enabled = cfg_getbool(cfg, "menu_outline_enabled");
    jwm->menu_decorations_style = cfg_getstr(cfg, "menu_decorations_style");
    jwm->menu_bg_color_active = cfg_getstr(cfg, "menu_bg_color_active");
    jwm->menu_bg_color_inactive = cfg_getstr(cfg, "menu_bg_color_inactive");
    jwm->menu_fg_color_active = cfg_getstr(cfg, "menu_fg_color_active");
    jwm->menu_fg_color_inactive = cfg_getstr(cfg, "menu_fg_color_inactive");
    jwm->menu_outline_color = cfg_getstr(cfg, "menu_outline_color");
    jwm->menu_opacity = GetValidOpacity(cfg_getfloat(cfg, "menu_opacity"));
    jwm->menu_font = cfg_getstr(cfg, "menu_font");
    jwm->menu_font_alignment = cfg_getstr(cfg, "menu_font_alignment");
    jwm->menu_font_size = cfg_getint(cfg, "menu_font_size");

    jwm->tray_use_global_decorations_style = cfg_getbool(cfg, "tray_use_global_decorations_style");
    jwm->tray_use_global_colors = cfg_getbool(cfg, "tray_use_global_colors");
    jwm->tray_use_global_font = cfg_getbool(cfg, "tray_use_global_font");
    jwm->tray_use_menu_icon = cfg_getbool(cfg, "tray_use_menu_icon");
    jwm->tray_outline_enabled = cfg_getbool(cfg, "tray_outline_enabled");
    jwm->tray_decorations_style = cfg_getstr(cfg, "tray_decorations_style");
    jwm->tray_bg_color_active = cfg_getstr(cfg, "tray_bg_color_active");
    jwm->tray_bg_color_inactive = cfg_getstr(cfg, "tray_bg_color_inactive");
    jwm->tray_fg_color_active = cfg_getstr(cfg, "tray_fg_color_active");
    jwm->tray_fg_color_inactive = cfg_getstr(cfg, "tray_fg_color_inactive");
    jwm->tray_outline_color_active = cfg_getstr(cfg, "tray_outline_color_active");
    jwm->tray_outline_color_inactive = cfg_getstr(cfg, "tray_outline_color_inactive");
    jwm->tray_auto_hide = cfg_getbool(cfg, "tray_autohide");
    const char *tpos = cfg_getstr(cfg, "tray_position");
    jwm->tray_pos = GetTrayPosition(tpos);
    jwm->tray_menu_icon = cfg_getstr(cfg, "tray_menu_icon");
    jwm->tray_menu_text = cfg_getstr(cfg, "tray_menu_text");
    jwm->tray_height = cfg_getint(cfg, "tray_height");
    jwm->tray_icon_spacing = cfg_getint(cfg, "tray_icon_spacing");
    jwm->tray_opacity = GetValidOpacity(cfg_getfloat(cfg, "tray_opacity"));
    jwm->tray_font = cfg_getstr(cfg, "tray_font");
    jwm->tray_font_alignment = cfg_getstr(cfg, "tray_font_alignment");
    jwm->tray_font_size = cfg_getint(cfg, "tray_font_size");

    jwm->root_menu_height = cfg_getint(cfg, "rootmenu_height");

    if (CreateJWMFolder(jwm) != 0)
        goto failure;

    GenJWMRootMenu(jwm, entries, icons);

    if (GenJWMStartup(jwm) != 0)
        goto failure;

    if (GenJWMGroup(jwm) != 0)
        goto failure;

    GenJWMTray(jwm, entries, icons);

    //UseTheme(jwm, BreezeDark);

    if (GenJWMStyles(jwm) != 0)
        goto failure;

    if (GenJWMPreferences(jwm) != 0)
        goto failure;

    GenJWMIcons(jwm);

    if (GenJWMBinds(jwm, cfg) != 0)
        goto failure;

    if (GenJWMRCFile(jwm) != 0)
        goto failure;

    free(jwm->autogen_config_path);
    free(jwm);
    cfg_free(cfg);

    return 0;

failure:
    free(jwm->autogen_config_path);
    free(jwm);
    cfg_free(cfg);
    return -1;
}

