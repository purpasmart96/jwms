
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

// Ugly global, should be removed
XDGDesktopEntry *g_terminal = NULL;

static void AddTraySpacing(FILE *fp, Tray *tray)
{
    if (tray->position < Left)
    {
        WRITE_CFG("       <Spacer width=\"%d\"/>\n", tray->spacing);
    }
    else
    {
        WRITE_CFG("       <Spacer height=\"%d\"/>\n", tray->spacing);
    }
}

static void AddProgramToTray(BTreeNode *entries, FILE *fp, HashMap *icons, Tray *tray, const char *exec)
{
    //XDGDesktopEntry *program = EntriesSearch(entries, exec);
    XDGDesktopEntry *program = GetProgram(entries, exec);
    if (program != NULL)
    {
        const char *icon = HashMapGet(icons, program->icon);
        if (icon == NULL)
        {
            icon = program->icon;
        }

        WRITE_CFG("       <TrayButton popup=\"%s\" icon=\"%s\">exec:%s</TrayButton>\n", program->name, icon, program->exec);
        AddTraySpacing(fp, tray);
    }
    else
    {
        printf("Couldn't find %s!\n", exec);
    }
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
    // Start of the tray xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");

    for (int i = 0; i < jwm->num_trays; i++)
    {
        Tray *tray = &jwm->trays[i];
        const char *auto_hide = tray->autohide ? "on" : "off";
        
        if (tray->position == Bottom)
        {
            WRITE_CFG("   <Tray x=\"0\" y=\"-1\" height=\"%d\" autohide=\"%s\" delay=\"%d\">\n", tray->thickness, auto_hide, tray->autohide_delay);
        }
        else if (tray->position == Top)
        {
            WRITE_CFG("   <Tray x=\"0\" y=\"0\" height=\"%d\" autohide=\"%s\" delay=\"%d\">\n", tray->thickness, auto_hide, tray->autohide_delay);
        }
        else if (tray->position == Left)
        {
            WRITE_CFG("   <Tray x=\"0\" y=\"0\" width=\"%d\" layout=\"vertical\" autohide=\"%s\" delay=\"%d\">\n", tray->thickness, auto_hide, tray->autohide_delay);
        }
        else
        {
            WRITE_CFG("   <Tray x=\"-1\" y=\"0\" width=\"%d\" layout=\"vertical\" autohide=\"%s\" delay=\"%d\">\n", tray->thickness, auto_hide, tray->autohide_delay);
        }
        
        if (tray->menu_button_enabled)
        {
            if (!jwm->tray_use_menu_icon)
            {
                // Write configuration without the icon
                WRITE_CFG("       <TrayButton label=\"%s\">root:1</TrayButton>\n", jwm->tray_menu_text);
            }
            else
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
        }

        char *show_desktop_icon = FindIcon("desktop", jwm->global_preferred_icon_size, 1);
        AddTraySpacing(fp, tray);

        if (tray->num_programs > 0)
        {
            for (int j = 0; j < tray->num_programs; j++)
            {
                AddProgramToTray(entries, fp, icons, tray, tray->programs[j]);
            }
        }

        if (tray->tasklist_enabled)
        {
            WRITE_CFG("       <TaskList labeled=\"%s\" labelpos=\"%s\" maxwidth=\"%d\"/>\n", tray->tasklist_labeled ? "true" : "false", tray->tasklist_label_position, 256);
            //WRITE_CFG("       <Spacer height=\"%d\"/>\n", 4);
            AddTraySpacing(fp, tray);
            //WRITE_CFG("       <Spacer/>\n");
        }
    
        WRITE_CFG("       <TrayButton popup=\"Show Desktop\" icon=\"%s\">showdesktop</TrayButton>\n", show_desktop_icon);
        AddTraySpacing(fp, tray);

        if (tray->pager_enabled)
        {
            WRITE_CFG("       <Pager labeled=\"%s\"/>\n", jwm->pager_labled ? "true" : "false");
            AddTraySpacing(fp, tray);
        }

        if (tray->systray_enabled)
        {
            WRITE_CFG("       <Dock spacing=\"%d\" width=\"%d\"/>\n", jwm->tray_systray_spacing, jwm->tray_systray_size);
            AddTraySpacing(fp, tray);
        }

        if (tray->clock_enabled)
        {
            WRITE_CFG("       <Clock format=\"%%l:%%M %%p\"><Button mask=\"123\">exec:xclock</Button></Clock>\n");
            AddTraySpacing(fp, tray);
        }

        WRITE_CFG("   </Tray>\n");
    
        if (i != jwm->num_trays && jwm->num_trays > 1)
            WRITE_CFG("\n");

        free(show_desktop_icon);
    }

    WRITE_CFG("</JWM>\n");
    fclose(fp);
    return 0;
}
