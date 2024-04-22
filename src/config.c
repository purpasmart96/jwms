
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
#include "darray.h"
//#include "list.h"
//#include "hashing.h"
//#include "ini.h"
//#include "icons.h"
#include "desktop_entries.h"
#include "hashing.h"
#include "config.h"


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

// WIP
static void GenJWMIcons(JWM *jwm, DArray *entries)
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
    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<JWM>\n");
    fprintf(fp, "   <IconPath>/usr/share/pixmaps/</IconPath>\n");
    fprintf(fp, "</JWM>");

    fclose(fp);
}

static void WriteJWMStyle(JWM *jwm, FILE *fp, Styles style)
{
    fprintf(fp, "    <%s", style_types[style].key);

    switch (style)
    {
        case WindowStyle:
        {
            fprintf(fp, " decorations=\"%s\">\n", "flat");
            fprintf(fp, "        <Font align=\"center\">Sans-10</Font>\n");
            fprintf(fp, "        <Height>26</Height>\n");
            fprintf(fp, "        <Width>4</Width>\n");
            fprintf(fp, "        <Corner>1</Corner>\n");
            fprintf(fp, "        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            fprintf(fp, "        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            fprintf(fp, "        <Opacity>0.9</Opacity>\n");
            fprintf(fp, "        <Outline>#FFFFFF</Outline>\n");
            fprintf(fp, "        <Active>\n");
            fprintf(fp, "            <Foreground>%s</Foreground>\n", jwm->global_fg_color_active);
            fprintf(fp, "            <Background>%s</Background>\n", jwm->global_bg_color_active);
            fprintf(fp, "            <Opacity>1.0</Opacity>\n");
            fprintf(fp, "            <Outline>#FFFFFF</Outline>\n");
            fprintf(fp, "        </Active>\n");
            break;
        }
        case ClockStyle:
        {
            fprintf(fp, ">\n");
            fprintf(fp, "        <Font align=\"center\">Sans-10</Font>\n");
            fprintf(fp, "        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            fprintf(fp, "        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            break;
        }
        case TrayStyle:
        {
            fprintf(fp, " decorations=\"%s\">\n", "flat");
            fprintf(fp, "        <Font align=\"center\">Sans-10</Font>\n");
            fprintf(fp, "        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            fprintf(fp, "        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            fprintf(fp, "        <Outline>#FFFFFF</Outline>\n");
            fprintf(fp, "        <Active>\n");
            fprintf(fp, "            <Foreground>%s</Foreground>\n", jwm->global_fg_color_active);
            fprintf(fp, "            <Background>%s</Background>\n", jwm->global_bg_color_active);
            fprintf(fp, "            <Opacity>0.5</Opacity>\n");
            fprintf(fp, "        </Active>\n");
            fprintf(fp, "        <Opacity>0.8</Opacity>\n");
            break;
        }
        case TaskListStyle:
        {
            fprintf(fp, " list=\"all\" group=\"true\">\n");
            fprintf(fp, "        <Font align=\"center\">Sans-10</Font>\n");
            fprintf(fp, "        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            fprintf(fp, "        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            fprintf(fp, "        <Outline>#FFFFFF</Outline>\n");
            fprintf(fp, "        <Active>\n");
            fprintf(fp, "            <Foreground>%s</Foreground>\n", jwm->global_fg_color_active);
            fprintf(fp, "            <Background>%s</Background>\n", jwm->global_bg_color_active);
            fprintf(fp, "            <Outline>#FFFFFF</Outline>\n");
            fprintf(fp, "        </Active>\n");
            break;
        }
        case PagerStyle:
        {
            fprintf(fp, ">\n");
            fprintf(fp, "        <Outline>#FFFFFF</Outline>\n");
            fprintf(fp, "        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            fprintf(fp, "        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            fprintf(fp, "        <Font align=\"center\">Sans-10</Font>\n");
            fprintf(fp, "        <Text>#FFFFFF</Text>\n");
            fprintf(fp, "        <Active>\n");
            fprintf(fp, "            <Foreground>%s</Foreground>\n", jwm->global_fg_color_active);
            fprintf(fp, "            <Background>%s</Background>\n", jwm->global_bg_color_active);
            fprintf(fp, "        </Active>\n");
            break;
        }
        case MenuStyle:
        {
            fprintf(fp, " decorations=\"%s\">\n", "flat");
            fprintf(fp, "        <Font align=\"center\">Sans-10</Font>\n");
            fprintf(fp, "        <Height>26</Height>\n");
            fprintf(fp, "        <Width>4</Width>\n");
            fprintf(fp, "        <Corner>1</Corner>\n");
            fprintf(fp, "        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            fprintf(fp, "        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            fprintf(fp, "        <Opacity>0.9</Opacity>\n");
            fprintf(fp, "        <Outline>#FFFFFF</Outline>\n");
            fprintf(fp, "        <Active>\n");
            fprintf(fp, "            <Foreground>%s</Foreground>\n", jwm->global_fg_color_active);
            fprintf(fp, "            <Background>%s</Background>\n", jwm->global_bg_color_active);
            fprintf(fp, "            <Opacity>1.0</Opacity>\n");
            fprintf(fp, "            <Outline>#FFFFFF</Outline>\n");
            fprintf(fp, "        </Active>\n");
            break;
        }
        case PopupStyle:
        {
            fprintf(fp, " list=\"all\" group=\"true\">\n");
            fprintf(fp, "        <Font align=\"center\">Sans-10</Font>\n");
            fprintf(fp, "        <Outline>#FFFFFF</Outline>\n");
            fprintf(fp, "        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            fprintf(fp, "        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            break;
        }
        default:
            break;
    }

    fprintf(fp, "    </%s>\n", style_types[style].key);
}

static void GenJWMStyles(JWM *jwm, DArray *entries)
{
    char path[512];
    const char *fname = "styles_test";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return;
    }

    printf("Writing to %s\n", path);

    // Start of the styles/theme xml file
    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<JWM>\n");

    WriteJWMStyle(jwm, fp, WindowStyle);
    WriteJWMStyle(jwm, fp, ClockStyle);
    WriteJWMStyle(jwm, fp, TrayStyle);
    WriteJWMStyle(jwm, fp, TaskListStyle);
    WriteJWMStyle(jwm, fp, PagerStyle);
    WriteJWMStyle(jwm, fp, MenuStyle);
    WriteJWMStyle(jwm, fp, PopupStyle);

    fprintf(fp, "</JWM>\n");
    fclose(fp);
}

static void WriteJWMTray(JWM *jwm, DArray *entries, FILE *fp)
{
    // Get Terminal
    XDGDesktopEntry *terminal = GetCoreProgram(entries, TerminalEmulator, jwm->terminal_name);
    if (terminal != NULL)
    {
        fprintf(fp, "       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", terminal->name, terminal->icon, terminal->exec);
    }
    else
    {
        fprintf(fp, "       <TrayButton popup=\"Terminal\" icon=\"terminal\">exec:x-terminal-emulator</TrayButton>\n");
    }

    fprintf(fp, "       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    // Get FileManager
    XDGDesktopEntry *filemanager = GetCoreProgram(entries, FileManager, jwm->filemanager_name);

    if (filemanager != NULL)
    {
        fprintf(fp, "       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", filemanager->name, filemanager->icon, filemanager->exec);
    }
    else
    {
        fprintf(fp, "       <TrayButton popup=\"File Manager\" icon=\"system-file-manager\">exec:spacefm</TrayButton>\n");
    }

    fprintf(fp, "       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);

    // Get WebBrowser
    XDGDesktopEntry *browser = GetCoreProgram(entries, WebBrowser, jwm->browser_name);
    if (browser != NULL)
    {
        fprintf(fp, "       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", browser->name, browser->icon, browser->exec);
    }
    else
    {
        fprintf(fp, "       <TrayButton popup=\"Web browser\" icon=\"firefox\">exec:firefox</TrayButton>\n");
    }

    fprintf(fp, "       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
}

static void GenJWMTray(JWM *jwm, DArray *entries)
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

    const char *auto_hide = (jwm->tray_auto_hide == true) ? "on" : "off";
    // Start of the tray xml file
    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<JWM>\n");

    // This can be improved...
    if (jwm->tray_pos == Bottom)
    {
        fprintf(fp, "   <Tray x=\"0\" y=\"-1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
    }
    else if (jwm->tray_pos == Top)
    {
        fprintf(fp, "   <Tray x=\"0\" y=\"+1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
    }
    else
    {
        fprintf(fp, "   <Tray x=\"0\" y=\"-1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
        printf("Error! Tray Position Not Implemented Yet!\n");
    }

    fprintf(fp, "       <TrayButton icon=\"/usr/share/jwm/jwm-blue.svg\">root:1</TrayButton>\n");
    fprintf(fp, "       <Spacer width=\"4\"/>\n");
    WriteJWMTray(jwm, entries, fp);
    fprintf(fp, "       <TaskList maxwidth=\"200\"/>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    fprintf(fp, "       <TrayButton popup=\"Show Desktop\" icon=\"desktop\">showdesktop</TrayButton>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    fprintf(fp, "       <Pager labeled=\"true\"/>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n",jwm->tray_icon_spacing);
    fprintf(fp, "       <Dock/>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    fprintf(fp, "       <Clock format=\"%%l:%%M %%p\"><Button mask=\"123\">exec:xclock</Button></Clock>\n");
    fprintf(fp, "       <Spacer width=\"4\"/>\n");
    fprintf(fp, "   </Tray>\n");
    fprintf(fp, "</JWM>");

    fclose(fp);
}

static void WriteJWMRootMenuCategoryList(DArray *entries, HashMap *icons, FILE *fp, XDGMainCategories category, const char *category_name, const char *overide_category_name)
{
    const char *menu_name = category_name;
    if (overide_category_name != NULL)
        menu_name = overide_category_name;

    fprintf(fp, "       <Menu icon=\"%s\" label=\"%s\">\n", category_name, menu_name);

    for (size_t i = 0; i < entries->size; i++)
    {
        XDGDesktopEntry *entry = entries->data[i];
        
        if (entry->category == category)
        {
            const char *icon = HashMapGet(icons, entry->icon);
            if (icon == NULL)
            {
                icon = entry->icon;
            }

            if (!entry->terminal_required)
            {
                fprintf(fp, "           <Program icon=\"%s\" label=\"%s\">%s</Program>\n",
                        icon, entry->name, entry->exec);
            }
            else
            {
                fprintf(fp, "           <Program icon=\"%s\" label=\"%s\">x-terminal-emulator -e %s</Program>\n",
                        icon, entry->name, entry->exec);
            }
        }
    }

    fprintf(fp, "       </Menu>\n");
}

static void GenJWMRootMenu(JWM *jwm, DArray *entries, HashMap *icons)
{
    FILE *fp;
    char path[512];
    const char *fname = "root_menutest";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    fp = fopen(path, "w");
    printf("Writing to %s\n", path);

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return;
    }

    // Start of the root menu xml file
    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<JWM>\n");
    fprintf(fp, "   <RootMenu height=\"%d\" onroot=\"12\">\n", jwm->root_menu_height);

    WriteJWMRootMenuCategoryList(entries, icons, fp, Network,"Network", "Internet");
    WriteJWMRootMenuCategoryList(entries, icons, fp, AudioVideo,"AudioVideo", "Multimedia");
    WriteJWMRootMenuCategoryList(entries, icons, fp, Development,"Development", NULL);
    WriteJWMRootMenuCategoryList(entries, icons, fp, Office,"Office", NULL);
    WriteJWMRootMenuCategoryList(entries, icons, fp, Graphics,"Graphics", NULL);
    //WriteJWMRootMenuCategoryList(entries, icons, fp, Video,"Video", "Multimedia");
    WriteJWMRootMenuCategoryList(entries, icons, fp, Settings,"Settings", NULL);
    WriteJWMRootMenuCategoryList(entries, icons, fp, System,"System", NULL);
    WriteJWMRootMenuCategoryList(entries, icons, fp, Utility,"Utility", NULL);

    fprintf(fp, "   </RootMenu>\n");
    fprintf(fp, "</JWM>");

    fclose(fp);
}

int WriteJWMConfig(DArray *entries, HashMap *icons)
{
    cfg_opt_t opts[] =
    {
        CFG_STR("global_webbrowser", "firefox", CFGF_NONE),
        CFG_STR("global_terminal", "x-terminal-emulator", CFGF_NONE),
        CFG_STR("global_filemanager", "spacefm", CFGF_NONE),
        CFG_STR("global_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("global_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("global_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("global_fg_color_inactive", "#CCCCCC", CFGF_NONE),

        CFG_STR("window_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("window_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("window_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("window_fg_color_inactive", "#CCCCCC", CFGF_NONE),

        CFG_STR("menu_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("menu_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("menu_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("menu_fg_color_inactive", "#CCCCCC", CFGF_NONE),

        CFG_INT("rootmenu_height", 26, CFGF_NONE),
        CFG_STR("tray_terminal", "xterm", CFGF_NONE),
        CFG_STR("tray_filemanager", "spacefm", CFGF_NONE),
        CFG_STR("tray_browser", "firefox", CFGF_NONE),
        CFG_STR("tray_position", "bottom", CFGF_NONE),
        CFG_BOOL("tray_autohide", false, CFGF_NONE),
        CFG_INT("tray_height", 32, CFGF_NONE),
        CFG_INT("tray_icon_spacing", 4, CFGF_NONE),
        CFG_END()
    };

    cfg_t *cfg;
    cfg = cfg_init(opts, CFGF_NONE);

    if (cfg_parse(cfg, "jwms.conf") == CFG_PARSE_ERROR)
        return -1;

    printf("rootmenu_height=%ld\n", cfg_getint(cfg, "rootmenu_height"));

    printf("tray_browser=%s\n", cfg_getstr(cfg, "tray_browser"));
    printf("tray_position=%s\n", cfg_getstr(cfg, "tray_position"));
    printf("tray_autohide=%d\n", cfg_getbool(cfg, "tray_autohide"));
    printf("tray_height=%ld\n", cfg_getint(cfg, "tray_height"));
    printf("tray_icon_spacing=%ld\n", cfg_getint(cfg, "tray_icon_spacing"));

    JWM *jwm = calloc(1, sizeof(*jwm));

    const char *tpos = cfg_getstr(cfg, "tray_position");

    jwm->tray_pos = GetTrayPosition(tpos);
    // No input checking here yet...
    jwm->global_bg_color_active = cfg_getstr(cfg, "global_bg_color_active");
    jwm->global_bg_color_inactive = cfg_getstr(cfg, "global_bg_color_inactive");
    jwm->global_fg_color_active = cfg_getstr(cfg, "global_fg_color_active");
    jwm->global_fg_color_inactive = cfg_getstr(cfg, "global_fg_color_inactive");


    jwm->tray_auto_hide = cfg_getbool(cfg, "tray_autohide");
    jwm->tray_height = cfg_getint(cfg, "tray_height");
    jwm->tray_icon_spacing = cfg_getint(cfg, "tray_icon_spacing");

    jwm->terminal_name = cfg_getstr(cfg, "tray_terminal");
    jwm->filemanager_name = cfg_getstr(cfg, "tray_filemanager");
    jwm->browser_name = cfg_getstr(cfg, "tray_browser");

    jwm->root_menu_height = cfg_getint(cfg, "rootmenu_height");

    if (CreateJWMFolder(jwm) != 0)
        goto failure;

    GenJWMRootMenu(jwm, entries, icons);
    GenJWMTray(jwm, entries);
    GenJWMStyles(jwm, entries);
    GenJWMIcons(jwm, entries);

    free(jwm->autogen_config_path);
    free(jwm);
    cfg_free(cfg);

    return 0;

failure:
    free(jwm);
    cfg_free(cfg);
    return -1;
}

