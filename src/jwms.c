
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <bsd/string.h>
#include <confuse.h>

#include "common.h"
#include "darray.h"
#include "list.h"
#include "hashing.h"
#include "bstree.h"
#include "icons.h"
#include "desktop_entries.h"
#include "list.h"
#include "config.h"

static int LoadAllDesktopEntries(BTreeNode **entries)
{
    const char *default_app_dir = "/usr/share/applications/";
    const char *user_app_dir = "~/.local/share/applications/";

    int success = LoadDesktopEntries(entries, default_app_dir);

    if (success != 0)
    {
        printf("Failed to load desktop entries from the default path %s!\n", default_app_dir);
        return -1;
    }

    char user_app_dir_buffer[512];
    ExpandPath(user_app_dir_buffer, user_app_dir, sizeof(user_app_dir_buffer));
    success = LoadDesktopEntries(entries, user_app_dir_buffer);

    if (success != 0)
    {
        printf("Failed to load desktop entries from the user %s! Skipping...\n", user_app_dir_buffer);
    }

    return 0;
}

static int LoadIcons(JWM *jwm, BTreeNode *entries, HashMap **icons)
{
    printf("Loading icons...\n");
    *icons = FindAllIcons2(entries, jwm->global_preferred_icon_size, 1);
    if (*icons == NULL)
    {
        printf("Failed to load icons!\n");
        return -1;
    }

    printf("Finished loading icons\n");
    return 0;
    //HashMapPrint(icons_output);
}

static int GenerateFiles(JWM *jwm, cfg_t *cfg, BTreeNode *entries, HashMap *icons)
{
    if (CreateJWMFolder(jwm) != 0)
        return -1;

    if (CreateJWMStartup(jwm) != 0)
        return -1;

    if (CreateJWMGroup(jwm) != 0)
        return -1;

    if (CreateJWMTray(jwm, entries, icons) != 0)
        return -1;

    if (CreateJWMRootMenu(jwm, entries, icons) !=0)
        return -1;

    if (CreateJWMStyles(jwm) != 0)
        return -1;

    if (CreateJWMPreferences(jwm) != 0)
        return -1;

    if (CreateJWMIcons(jwm) != 0)
        return -1;

    if (CreateJWMBinds(jwm, cfg) != 0)
        return -1;

    if (CreateJWMAutoStart(jwm, cfg) != 0)
        return -1;

    if (CreateJWMRCFile(jwm) != 0)
        return -1;

    return 0;
}


int main()
{
    JWM *jwm;
    cfg_t *cfg;
    BTreeNode *entries = NULL;
    HashMap *icons = NULL;

    // Load the config first
    if (LoadJWMConfig(&jwm, &cfg) != 0)
    {
        printf("Failed to properly load the jwms.conf file! Aborting!\n");
        return EXIT_FAILURE;
    }

    if (LoadAllDesktopEntries(&entries) != 0)
        return EXIT_FAILURE;


    if (LoadIcons(jwm, entries, &icons) != 0)
        return EXIT_FAILURE;

    // Test for node deletion code
    //EntryRemove(entries, "Okular");

    if (GenerateFiles(jwm, cfg, entries, icons) != 0)
    {
        return EXIT_FAILURE;
    }

    // Hashmap test
    //IniFile *icon_theme = IniFileLoad("/usr/share/icons/hicolor/index.theme");
    //if (icon_theme != NULL)
    //{
        //IniPrintAll(icon_theme);
    //    printf("Icon Directories: %s\n", IniGetString(icon_theme, "Icon Theme:Directories"));
    //    IniDestroy(icon_theme);
    //}

    HashMapDestroy(icons);

    //EntriesPrint(entries);
    // Test
    //XDGDesktopEntry *entry = EntriesSearch(entries, "Firefox Web Browser");
    //if (entry != NULL)
    //    printf("Search found:\n%s\n%s\n%s\n%s\n", entry->name, entry->category_name, entry->exec, entry->icon);

    EntriesDestroy(entries);

    free(jwm->autogen_config_path);
    free(jwm);
    cfg_free(cfg);

    return 0;
}
