
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <bsd/string.h>
#include <confuse.h>

#include "darray.h"
#include "hashing.h"
#include "ini.h"
#include "icons.h"
#include "desktop_entries.h"
#include "jwms.h"


static TrayPositions GetTrayPosition(const char *tpos)
{
    if (strcmp(tpos, "bottom") == 0)
        return Bottom;
    if (strcmp(tpos, "top") == 0)
        return Top;
    if (strcmp(tpos, "left") == 0)
        return Left;
    if (strcmp(tpos, "right") == 0)
        return Right;

    printf("Invalid Tray Position! Using Default Position (bottom)\n");
    return Bottom;
}

static void WriteJWMTray(JWMTray *tray, DArray *entries, FILE *fp)
{

    // Get Terminal
    XDGDesktopEntry *terminal = GetCoreProgram(entries, TerminalEmulator, tray->terminal_name);
    if (terminal != NULL)
    {
        fprintf(fp, "       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", terminal->name, terminal->icon, terminal->exec);
    }
    else
    {
        fprintf(fp, "       <TrayButton popup=\"Terminal\" icon=\"terminal\">exec:x-terminal-emulator</TrayButton>\n");
    }

    fprintf(fp, "       <Spacer width=\"%d\"/>\n", tray->icon_spacing);
    // Get FileManager
    XDGDesktopEntry *filemanager = GetCoreProgram(entries, FileManager, tray->filemanager_name);
    if (filemanager != NULL)
    {
        fprintf(fp, "       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", filemanager->name, filemanager->icon, filemanager->exec);
    }
    else
    {
        fprintf(fp, "       <TrayButton popup=\"File Manager\" icon=\"system-file-manager\">exec:spacefm</TrayButton>\n");
    }

    fprintf(fp, "       <Spacer width=\"%d\"/>\n", tray->icon_spacing);
    // Get WebBrowser
    XDGDesktopEntry *browser = GetCoreProgram(entries, WebBrowser, tray->browser_name);
    if (browser != NULL)
    {
        fprintf(fp, "       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", browser->name, browser->icon, browser->exec);
    }
    else
    {
        fprintf(fp, "       <TrayButton popup=\"Web browser\" icon=\"firefox\">exec:firefox</TrayButton>\n");
    }
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", tray->icon_spacing);
}

static void GenJWMTray(JWMTray *tray, DArray *entries)
{
    FILE *fp;
    char path[512];

    const char *home = getenv("HOME");
    const char *dir = "/.config/jwm/";
    const char *fname = "traytest";

    strlcpy(path, home, sizeof(path));
    strlcat(path, dir, sizeof(path));

    // Check if directory exists
    if (access(path, F_OK) != 0)
    {
        // Create it
        if (mkdir(path, 0777) != 0)
        {
            fprintf(stderr, "Failed to create directory\n");
            return;
        }
        printf("Directory %s/.config/jwm/ Created\n", home);
    }

    strlcat(path, fname, sizeof(path));

    printf("Writing to %s\n", path);

    const char *auto_hide = (tray->auto_hide == true) ? "on" : "off";

    fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return;
    }

    // Start of the tray xml file
    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<JWM>\n");

    // This can be improved...
    if (tray->tpos == Bottom)
    {
        fprintf(fp, "   <Tray x=\"0\" y=\"-1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", tray->height, auto_hide);
    }
    else if (tray->tpos == Top)
    {
        fprintf(fp, "   <Tray x=\"0\" y=\"+1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", tray->height, auto_hide);
    }
    else
    {
        fprintf(fp, "   <Tray x=\"0\" y=\"-1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", tray->height, auto_hide);
        printf("Error! Tray Position Not Implemented Yet!\n");
    }

    fprintf(fp, "       <TrayButton icon=\"/usr/share/jwm/jwm-blue.svg\">root:1</TrayButton>\n");
    fprintf(fp, "       <Spacer width=\"4\"/>\n");
    WriteJWMTray(tray, entries, fp);
    fprintf(fp, "       <TaskList maxwidth=\"200\"/>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", tray->icon_spacing);
    fprintf(fp, "       <TrayButton popup=\"Show Desktop\" icon=\"desktop\">showdesktop</TrayButton>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", tray->icon_spacing);
    fprintf(fp, "       <Pager labeled=\"true\"/>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n",tray->icon_spacing);
    fprintf(fp, "       <Dock/>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", tray->icon_spacing);
    fprintf(fp, "       <Clock format=\"%%l:%%M %%p\"><Button mask=\"123\">exec:xclock</Button></Clock>\n");
    fprintf(fp, "       <Spacer width=\"4\"/>\n");
    fprintf(fp, "   </Tray>\n");
    fprintf(fp, "</JWM>");

    fclose(fp);
}

static void WriteJWMRootMenuCategoryList(DArray *entries, FILE *fp, XDGMainCategories category, const char *category_name, const char *overide_category_name)
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
            if (!entry->terminal_required)
            {
                fprintf(fp, "           <Program icon=\"%s\" label=\"%s\">%s</Program>\n",
                        entry->icon, entry->name, entry->exec);
            }
            else
            {
                fprintf(fp, "           <Program icon=\"%s\" label=\"%s\">x-terminal-emulator -e %s</Program>\n",
                        entry->icon, entry->name, entry->exec);
            }
        }
    }

    fprintf(fp, "       </Menu>\n");
}

static void GenJWMRootMenu(JWMRootMenu *root_menu, DArray *entries)
{
    FILE *fp;
    char path[512];

    const char *home = getenv("HOME");
    const char *dir = "/.config/jwm/";
    const char *fname = "root_menutest";

    strlcpy(path, home, sizeof(path));
    strlcat(path, dir, sizeof(path));

    // Check if directory exists
    if (access(path, F_OK) != 0)
    {
        // Create it
        if (mkdir(path, 0777) != 0)
        {
            fprintf(stderr, "Failed to create directory\n");
            return;
        }
        printf("Directory %s/.config/jwm/ Created\n", home);
    }

    strlcat(path, fname, sizeof(path));

    printf("Writing to %s\n", path);

    fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return;
    }

    // Start of the root menu xml file
    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<JWM>\n");
    fprintf(fp, "   <RootMenu height=\"%d\" onroot=\"12\">\n", root_menu->height);

    WriteJWMRootMenuCategoryList(entries, fp, Network,"Network", "Internet");
    WriteJWMRootMenuCategoryList(entries, fp, AudioVideo,"AudioVideo", "Multimedia");
    WriteJWMRootMenuCategoryList(entries, fp, Development,"Development", NULL);
    WriteJWMRootMenuCategoryList(entries, fp, Office,"Office", NULL);
    WriteJWMRootMenuCategoryList(entries, fp, Graphics,"Graphics", NULL);
    //WriteJWMRootMenuCategoryList(entries, fp, Video,"Video", "Multimedia");
    WriteJWMRootMenuCategoryList(entries, fp, Settings,"Settings", NULL);
    WriteJWMRootMenuCategoryList(entries, fp, System,"System", NULL);
    WriteJWMRootMenuCategoryList(entries, fp, Utility,"Utility", NULL);

    fprintf(fp, "   </RootMenu>\n");
    fprintf(fp, "</JWM>");

    fclose(fp);
}

int main()
{
    cfg_opt_t opts[] =
    {
        CFG_STR("global_terminal", "x-terminal-emulator", CFGF_NONE),

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
        return 1;

    printf("rootmenu_height=%ld\n", cfg_getint(cfg, "rootmenu_height"));

    printf("tray_browser=%s\n", cfg_getstr(cfg, "tray_browser"));
    printf("tray_position=%s\n", cfg_getstr(cfg, "tray_position"));
    printf("tray_autohide=%d\n", cfg_getbool(cfg, "tray_autohide"));
    printf("tray_height=%ld\n", cfg_getint(cfg, "tray_height"));
    printf("tray_icon_spacing=%ld\n", cfg_getint(cfg, "tray_icon_spacing"));

    JWMTray *tray = calloc(1, sizeof(JWMTray));
    JWMRootMenu *root_menu = calloc(1, sizeof(JWMRootMenu));

    const char *tpos = cfg_getstr(cfg, "tray_position");

    tray->tpos = GetTrayPosition(tpos);
    // No input checking here yet...
    tray->auto_hide = cfg_getbool(cfg, "tray_autohide");
    tray->height = cfg_getint(cfg, "tray_height");
    tray->icon_spacing = cfg_getint(cfg, "tray_icon_spacing");

    tray->terminal_name = cfg_getstr(cfg, "tray_terminal");
    tray->filemanager_name = cfg_getstr(cfg, "tray_filemanager");
    tray->browser_name = cfg_getstr(cfg, "tray_browser");

    root_menu->height = cfg_getint(cfg, "rootmenu_height");

    DArray *entries = DArrayCreate(100, sizeof(XDGDesktopEntry*));
    LoadDesktopEntries(entries);

    GenJWMRootMenu(root_menu, entries);
    GenJWMTray(tray, entries);
    EntriesPrint(entries);

    // Hashmap test
    IniFile *icon_theme = IniFileLoad("/usr/share/icons/hicolor/index.theme");
    if (icon_theme != NULL)
    {
        IniPrintAll(icon_theme);
        printf("Icon Directories: %s\n", IniGetString(icon_theme, "Icon Theme:Directories"));
        IniDestroy(icon_theme);
    }

    // Test
    XDGDesktopEntry *entry = EntriesSearchExec(entries, "firefox");
    if (entry != NULL)
        printf("Search found:\n%s\n%s\n%s\n%s\n", entry->name, entry->category_name, entry->exec, entry->icon);

    // Icon search test
    char *icon = FindIcon("mpv", 32, 1);
    printf("\nIcon: %s\n", icon);
    free(icon);

    free(root_menu);
    free(tray);

    EntriesDestroy(entries);
    cfg_free(cfg);

    return 0;
}
