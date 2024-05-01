
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
#include "list.h"
#include "hashing.h"
//#include "ini.h"
#include "icons.h"
#include "desktop_entries.h"
#include "hashing.h"
#include "config.h"
#include "themes.h"

#define WRITE_CFG(...) \
    fprintf(fp, __VA_ARGS__)

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
    WRITE_CFG("        <Option>aerosnap</Option>\n");
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
    WRITE_CFG("   <Desktops width=\"2\" height=\"1\">\n");
    WRITE_CFG("       <Background type=\"solid\">#111111</Background>\n");
    WRITE_CFG("   </Desktops>\n");
    WRITE_CFG("   <DoubleClickSpeed>400</DoubleClickSpeed>\n");
    WRITE_CFG("   <DoubleClickDelta>2</DoubleClickDelta>\n");
    WRITE_CFG("   <FocusModel>click</FocusModel>\n");
    WRITE_CFG("   <SnapMode distance=\"10\">border</SnapMode>\n");
    WRITE_CFG("   <MoveMode>outline</MoveMode>\n");
    WRITE_CFG("   <ResizeMode>outline</ResizeMode>\n");
    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

static int GenJWMBinds(JWM *jwm)
{
    char path[512];
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

    // Start of the prefs xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("    <Key key=\"Up\">up</Key>\n");
    WRITE_CFG("    <Key key=\"Down\">down</Key>\n");
    WRITE_CFG("    <Key key=\"Right\">right</Key>\n");
    WRITE_CFG("    <Key key=\"Left\">left</Key>\n");
    WRITE_CFG("    <Key key=\"h\">up</Key>\n");
    WRITE_CFG("    <Key key=\"j\">down</Key>\n");
    WRITE_CFG("    <Key key=\"k\">right</Key>\n");
    WRITE_CFG("    <Key key=\"l\">left</Key>\n");
    WRITE_CFG("    <Key key=\"Return\">select</Key>\n");
    WRITE_CFG("    <Key key=\"Escape\">escape</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"Tab\">nextstacked</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"F4\">close</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"#\">desktop#</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"F1\">root:1</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"F2\">window</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"F10\">maximize</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"Right\">rdesktop</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"Left\">ldesktop</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"Up\">udesktop</Key>\n");
    WRITE_CFG("    <Key mask=\"A\" key=\"Down\">ddesktop</Key>\n");

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
            char *fg_color_inactive = jwm->window_fg_color_inactive;
            char *fg_color_active = jwm->window_fg_color_active;
            char *bg_color_inactive = jwm->window_bg_color_inactive;
            char *bg_color_active = jwm->window_bg_color_active;

            if (jwm->window_use_global_colors)
            {
                fg_color_inactive = jwm->global_fg_color_inactive;
                fg_color_active = jwm->global_fg_color_active;
                bg_color_inactive = jwm->global_bg_color_inactive;
                bg_color_active = jwm->global_bg_color_active;
            }

            //WRITE_CFG("    <%s decorations=\"%s\">\n", style_types[style].key, "flat");
            WRITE_CFG(" decorations=\"%s\">\n", "flat");
            WRITE_CFG("        <Font align=\"center\">Sans-10</Font>\n");
            WRITE_CFG("        <Height>%d</Height>\n", jwm->window_height);
            WRITE_CFG("        <Width>%d</Width>\n", jwm->window_width);
            WRITE_CFG("        <Corner>%d</Corner>\n", jwm->window_corner_rounding);
            WRITE_CFG("        <Foreground>%s</Foreground>\n", fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", bg_color_inactive);
            WRITE_CFG("        <Opacity>%.2f</Opacity>\n", jwm->window_opacity_inactive);
            if (jwm->window_outline_enabled)
                WRITE_CFG("        <Outline>%s</Outline>\n", jwm->window_outline_color_inactive);
            WRITE_CFG("        <Active>\n");
            WRITE_CFG("            <Foreground>%s</Foreground>\n", fg_color_active);
            WRITE_CFG("            <Background>%s</Background>\n", bg_color_active);
            WRITE_CFG("            <Opacity>%.2f</Opacity>\n", jwm->window_opacity_active);
            if (jwm->window_outline_enabled)
                WRITE_CFG("            <Outline>%s</Outline>\n", jwm->window_outline_color_active);
            WRITE_CFG("        </Active>\n");
            break;
        }
        case ClockStyle:
        {
            WRITE_CFG(">\n");
            WRITE_CFG("        <Font align=\"center\">Sans-10</Font>\n");
            WRITE_CFG("        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            break;
        }
        case TrayStyle:
        {
            WRITE_CFG(" decorations=\"%s\">\n", "flat");
            WRITE_CFG("        <Font align=\"center\">Sans-10</Font>\n");
            WRITE_CFG("        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", jwm->global_bg_color_inactive);

            if (jwm->tray_outline_enabled)
            {
                WRITE_CFG("        <Outline>%s</Outline>\n", jwm->global_outline_color);
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", jwm->global_fg_color_active);
                WRITE_CFG("            <Background>%s</Background>\n", jwm->global_bg_color_active);
                WRITE_CFG("            <Outline>%s</Outline>\n", jwm->global_outline_color);
            }
            else
            {
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", jwm->global_fg_color_active);
                WRITE_CFG("            <Background>%s</Background>\n", jwm->global_bg_color_active);
            }
    
            WRITE_CFG("        </Active>\n");
            WRITE_CFG("        <Opacity>0.8</Opacity>\n");
            break;
        }
        case TaskListStyle:
        {
            WRITE_CFG(" list=\"all\" group=\"true\">\n");
            WRITE_CFG("        <Font align=\"center\">Sans-10</Font>\n");
            WRITE_CFG("        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            if (jwm->tray_outline_enabled)
                WRITE_CFG("        <Outline>%s</Outline>\n", jwm->global_outline_color);
            WRITE_CFG("        <Active>\n");
            WRITE_CFG("            <Foreground>%s</Foreground>\n", jwm->global_fg_color_active);
            WRITE_CFG("            <Background>%s</Background>\n", jwm->global_bg_color_active);
            if (jwm->tray_outline_enabled)
                WRITE_CFG("            <Outline>%s</Outline>\n", jwm->global_outline_color);
            WRITE_CFG("        </Active>\n");
            break;
        }
        case PagerStyle:
        {
            WRITE_CFG(">\n");
            WRITE_CFG("        <Outline>#FFFFFF</Outline>\n");
            WRITE_CFG("        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            WRITE_CFG("        <Font align=\"center\">Sans-10</Font>\n");
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

            if (jwm->menu_use_global_colors)
            {
                fg_color_inactive = jwm->global_fg_color_inactive;
                fg_color_active = jwm->global_fg_color_active;
                bg_color_inactive = jwm->global_bg_color_inactive;
                bg_color_active = jwm->global_bg_color_active;
            }

            WRITE_CFG(" decorations=\"%s\">\n", "flat");
            WRITE_CFG("        <Font align=\"center\">Sans-10</Font>\n");
            WRITE_CFG("        <Foreground>%s</Foreground>\n", fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", bg_color_inactive);
            if (jwm->menu_outline_enabled)
                WRITE_CFG("        <Outline>%s</Outline>\n", jwm->menu_outline_color);
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
            WRITE_CFG("        <Font align=\"center\">Sans-10</Font>\n");
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

static void WriteJWMTray(JWM *jwm, DArray *entries, FILE *fp, HashMap *icons)
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

static void GenJWMTray(JWM *jwm, DArray *entries, HashMap *icons)
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

    WRITE_CFG("       <TrayButton icon=\"/usr/share/jwm/jwm-blue.svg\">root:1</TrayButton>\n");
    WRITE_CFG("       <Spacer width=\"4\"/>\n");
    WriteJWMTray(jwm, entries, fp, icons);
    WRITE_CFG("       <TaskList maxwidth=\"200\"/>\n");
    WRITE_CFG("       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    WRITE_CFG("       <TrayButton popup=\"Show Desktop\" icon=\"desktop\">showdesktop</TrayButton>\n");
    WRITE_CFG("       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    WRITE_CFG("       <Pager labeled=\"true\"/>\n");
    WRITE_CFG("       <Spacer width=\"%d\"/>\n",jwm->tray_icon_spacing);
    WRITE_CFG("       <Dock/>\n");
    WRITE_CFG("       <Spacer width=\"%d\"/>\n", jwm->tray_icon_spacing);
    WRITE_CFG("       <Clock format=\"%%l:%%M %%p\"><Button mask=\"123\">exec:xclock</Button></Clock>\n");
    WRITE_CFG("       <Spacer width=\"4\"/>\n");
    WRITE_CFG("   </Tray>\n");
    WRITE_CFG("</JWM>");

    fclose(fp);
}

static void WriteJWMRootMenuCategoryList(DArray *entries, HashMap *icons, FILE *fp, XDGMainCategories category, const char *category_name)
{
    char *category_icon = FindIcon(category_icons[category], 32, 1);
    WRITE_CFG("       <Menu icon=\"%s\" label=\"%s\">\n", category_icon, category_name);
    free(category_icon);
 
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
                WRITE_CFG("            <Program icon=\"%s\" label=\"%s\">%s</Program>\n",
                            icon, entry->name, entry->exec);
            }
            else
            {
                WRITE_CFG("            <Program icon=\"%s\" label=\"%s\">x-terminal-emulator -e %s</Program>\n",
                            icon, entry->name, entry->exec);
            }
        }
    }

    WRITE_CFG("       </Menu>\n");
}

static void GenJWMRootMenu(JWM *jwm, DArray *entries, HashMap *icons)
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
    WriteJWMRootMenuCategoryList(entries, icons, fp, Utility,"Utility");

    // Let's assume were using systemD for now
    WRITE_CFG("        <Program label=\"Restart\">systemctl reboot</Program>\n");
    WRITE_CFG("        <Program label=\"Shutdown\">systemctl poweroff</Program>\n");
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
        CFG_STR("global_outline_color", "#FFFFFF", CFGF_NONE),

        CFG_BOOL("window_use_global_colors", true, CFGF_NONE),
        CFG_INT("window_height", 26, CFGF_NONE),
        CFG_INT("window_width", 4, CFGF_NONE),
        CFG_INT("window_corner_rounding", 4, CFGF_NONE),
        CFG_BOOL("window_outline_enabled", false, CFGF_NONE),
        CFG_STR("window_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("window_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("window_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("window_fg_color_inactive", "#CCCCCC", CFGF_NONE),
        CFG_STR("window_outline_color_active", "#FFFFFF", CFGF_NONE),
        CFG_STR("window_outline_color_inactive", "#FFFFFF", CFGF_NONE),
        CFG_FLOAT("window_opacity_active", 1.0, CFGF_NONE),
        CFG_FLOAT("window_opacity_inactive", 1.0, CFGF_NONE),

        CFG_BOOL("menu_use_global_colors", true, CFGF_NONE),
        CFG_STR("menu_bg_color_active", "#222222", CFGF_NONE),
        CFG_STR("menu_bg_color_inactive", "#111111", CFGF_NONE),
        CFG_STR("menu_fg_color_active", "#DDDDDD", CFGF_NONE),
        CFG_STR("menu_fg_color_inactive", "#CCCCCC", CFGF_NONE),
        CFG_BOOL("menu_outline_enabled", false, CFGF_NONE),
        CFG_STR("menu_outline_color", "#FFFFFF", CFGF_NONE),
        CFG_FLOAT("menu_opacity", 1.0, CFGF_NONE),

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
    jwm->global_outline_color = cfg_getstr(cfg, "global_outline_color");

    jwm->window_use_global_colors = cfg_getbool(cfg, "window_use_global_colors");
    jwm->window_height = cfg_getint(cfg, "window_height");
    jwm->window_width = cfg_getint(cfg, "window_width");
    jwm->window_corner_rounding = cfg_getint(cfg, "window_corner_rounding");
    jwm->window_bg_color_active = cfg_getstr(cfg, "window_bg_color_active");
    jwm->window_bg_color_inactive = cfg_getstr(cfg, "window_bg_color_inactive");
    jwm->window_fg_color_active = cfg_getstr(cfg, "window_fg_color_active");
    jwm->window_fg_color_inactive = cfg_getstr(cfg, "window_fg_color_inactive");
    jwm->window_outline_color_active = cfg_getstr(cfg, "window_outline_color_active");
    jwm->window_outline_color_inactive = cfg_getstr(cfg, "window_outline_color_inactive");
    jwm->window_outline_enabled = cfg_getbool(cfg, "window_outline_enabled");
    jwm->window_opacity_active = GetValidOpacity(cfg_getfloat(cfg, "window_opacity_active"));
    jwm->window_opacity_inactive = GetValidOpacity(cfg_getfloat(cfg, "window_opacity_inactive"));

    jwm->menu_use_global_colors = cfg_getbool(cfg, "menu_use_global_colors");
    jwm->menu_outline_color = cfg_getstr(cfg, "menu_outline_color");
    jwm->menu_outline_enabled = cfg_getbool(cfg, "menu_outline_enabled");
    jwm->menu_opacity = GetValidOpacity(cfg_getfloat(cfg, "menu_opacity"));

    jwm->tray_auto_hide = cfg_getbool(cfg, "tray_autohide");
    jwm->tray_height = cfg_getint(cfg, "tray_height");
    jwm->tray_icon_spacing = cfg_getint(cfg, "tray_icon_spacing");
    jwm->tray_outline_enabled = cfg_getbool(cfg, "tray_outline_enabled");

    jwm->terminal_name = cfg_getstr(cfg, "tray_terminal");
    jwm->filemanager_name = cfg_getstr(cfg, "tray_filemanager");
    jwm->browser_name = cfg_getstr(cfg, "tray_browser");

    jwm->root_menu_height = cfg_getint(cfg, "rootmenu_height");

    if (CreateJWMFolder(jwm) != 0)
        goto failure;

    GenJWMRootMenu(jwm, entries, icons);

    if (GenJWMGroup(jwm) != 0)
        goto failure;

    GenJWMTray(jwm, entries, icons);

    //UseTheme(jwm, BreezeDark);

    if (GenJWMStyles(jwm) != 0)
        goto failure;

    if (GenJWMPreferences(jwm) != 0)
        goto failure;

    GenJWMIcons(jwm);

    if (GenJWMBinds(jwm) != 0)
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

