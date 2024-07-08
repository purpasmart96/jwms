
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <bsd/string.h>
#include <confuse.h>

#include "bstree.h"
#include "list.h"
#include "hashing.h"
#include "darray.h"
#include "icons.h"
#include "desktop_entries.h"
#include "config.h"

XDGDesktopEntry *g_terminal = NULL;

static void AddTraySpacing(JWM *jwm, FILE *fp, int spacing)
{
    if (jwm->tray_pos < Left)
    {
        WRITE_CFG("       <Spacer width=\"%d\"/>\n", spacing);
    }
    else
    {
        WRITE_CFG("       <Spacer height=\"%d\"/>\n", spacing);
    }
}

static void WriteJWMTray(JWM *jwm, BTreeNode *entries, FILE *fp, HashMap *icons)
{
    // Get Terminal
    g_terminal = GetCoreProgram(entries, TerminalEmulator, jwm->terminal_name);
    if (g_terminal != NULL)
    {
        const char *icon = HashMapGet(icons, g_terminal->icon);
        if (icon == NULL)
        {
            icon = g_terminal->icon;
        }

        WRITE_CFG("       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", g_terminal->name, icon, g_terminal->exec);
    }
    else
    {
        WRITE_CFG("       <TrayButton popup=\"Terminal\" icon=\"terminal\">exec:x-terminal-emulator</TrayButton>\n");
    }

    AddTraySpacing(jwm, fp, jwm->tray_icon_spacing);

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

    AddTraySpacing(jwm, fp, jwm->tray_icon_spacing);

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

    AddTraySpacing(jwm, fp, jwm->tray_icon_spacing);
}

int CreateJWMTray(JWM *jwm, BTreeNode *entries, HashMap *icons)
{
    char path[512];
    const char *fname = "tray";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    const char *auto_hide = jwm->tray_auto_hide ? "on" : "off";
    // Start of the tray xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");

    if (jwm->tray_pos == Bottom)
    {
        WRITE_CFG("   <Tray x=\"0\" y=\"-1\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
    }
    else if (jwm->tray_pos == Top)
    {
        WRITE_CFG("   <Tray x=\"0\" y=\"0\" height=\"%d\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
    }
    else if (jwm->tray_pos == Left)
    {
        WRITE_CFG("   <Tray x=\"0\" y=\"0\" width=\"%d\" layout=\"vertical\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
    }
    else
    {
        WRITE_CFG("   <Tray x=\"-1\" y=\"0\" width=\"%d\" layout=\"vertical\" autohide=\"%s\" delay=\"1000\">\n", jwm->tray_height, auto_hide);
    }

    if (jwm->tray_use_menu_icon)
    {
        char *resolved_icon = FindIcon(jwm->tray_menu_icon, jwm->global_preferred_icon_size, 1);

        // Use the new icon if found, otherwise use the provided icon
        const char *icon_path = (resolved_icon != NULL) ? resolved_icon : jwm->tray_menu_icon;

        // Write configuration with the icon
        WRITE_CFG("       <TrayButton icon=\"%s\">root:1</TrayButton>\n", icon_path);

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

    char *show_desktop_icon = FindIcon("desktop", jwm->global_preferred_icon_size, 1);
    AddTraySpacing(jwm, fp, jwm->tray_icon_spacing);
    WriteJWMTray(jwm, entries, fp, icons);
    WRITE_CFG("       <TaskList labeled=\"%s\" maxwidth=\"%d\"/>\n", "true", 256);
    //WRITE_CFG("       <Spacer height=\"%d\"/>\n", 4);
    AddTraySpacing(jwm, fp, 4);
    //WRITE_CFG("       <Spacer/>\n");
    WRITE_CFG("       <TrayButton popup=\"Show Desktop\" icon=\"%s\">showdesktop</TrayButton>\n", show_desktop_icon);
    AddTraySpacing(jwm, fp, jwm->tray_icon_spacing);
    WRITE_CFG("       <Pager labeled=\"%s\"/>\n", jwm->pager_labled ? "true" : "false");
    AddTraySpacing(jwm, fp, jwm->tray_icon_spacing);

    if (jwm->tray_use_systray)
    {
        WRITE_CFG("       <Dock spacing=\"%d\" width=\"%d\"/>\n", jwm->tray_systray_spacing, jwm->tray_systray_size);
        AddTraySpacing(jwm, fp, jwm->tray_icon_spacing);
    }

    WRITE_CFG("       <Clock format=\"%%l:%%M %%p\"><Button mask=\"123\">exec:xclock</Button></Clock>\n");
    AddTraySpacing(jwm, fp, jwm->tray_icon_spacing);
    WRITE_CFG("   </Tray>\n");
    WRITE_CFG("</JWM>");

    free(show_desktop_icon);
    fclose(fp);
    return 0;
}
