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


#define VERSION "v0.1"

static void About(void)
{
    printf("jwm-helper " VERSION " by Matt W\n");
}

static void Usage(void)
{
    About();
    printf("Usage: jwms [option...]\n\n");
}

static void Help(void)
{
    printf("Options:\n"
            "  --all          Generate all JWM files\n"
            "  --autostart    Generate the JWM autostart script\n"
            "  --binds        Generate the JWM keybinds\n"
            "  --groups       Generate the JWM program groups\n"
            "  --icons        Generate the JWM icon paths\n"
            "  --jwmrc        Generate the JWM rc file\n"
            "  --menu         Generate the JWM rootmenu\n"
            "  --prefs        Generate the JWM preference\n"
            "  --styles       Generate the JWM styles\n"
            "  --tray         Generate the JWM tray\n"
            "  --help         Display this information\n"
            "  --version      Display version information\n");
}

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

static int GenerateAll(JWM *jwm, cfg_t *cfg, BTreeNode *entries, HashMap *icons)
{
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

static int InitializeConfig(JWM **jwm, cfg_t **cfg)
{
    if (*cfg != NULL && *jwm != NULL)
    {
        return 0;
    }

    if (LoadJWMConfig(jwm, cfg) != 0)
    {
        printf("Failed to properly load the jwms.conf file! Aborting!\n");
        return -1;
    }

    if (CreateJWMFolder(*jwm) != 0)
    {
        return -1;
    }

    return 0;
}

static void CleanUp(JWM *jwm, cfg_t *cfg, HashMap *icons, BTreeNode *entries)
{
    if (icons)
        HashMapDestroy(icons);
    if (entries)
        EntriesDestroy(entries);
    if (jwm)
    {
        free(jwm->autogen_config_path);
        free(jwm);
    }
    if (cfg)
        cfg_free(cfg);
}

int HandleCmd(const char *cmd, JWM *jwm, cfg_t *cfg, BTreeNode **entries, HashMap **icons)
{
    if (strcmp(cmd, "--all") == 0)
    {
        if (*entries == NULL && LoadAllDesktopEntries(entries) != 0)
        {
            return EXIT_FAILURE;
        }
        if (*icons == NULL && LoadIcons(jwm, *entries, icons) != 0)
        {
            return EXIT_FAILURE;
        }
        return GenerateAll(jwm, cfg, *entries, *icons);
    }
    else if (strcmp(cmd, "--autostart") == 0)
    {
        return CreateJWMAutoStart(jwm, cfg);
    }
    else if (strcmp(cmd, "--binds") == 0)
    {
        return CreateJWMBinds(jwm, cfg);
    }
    else if (strcmp(cmd, "--groups") == 0)
    {
        return CreateJWMGroup(jwm);
    }
    else if (strcmp(cmd, "--icons") == 0)
    {
        return CreateJWMIcons(jwm);
    }
    else if (strcmp(cmd, "--jwmrc") == 0)
    {
        return CreateJWMRCFile(jwm);
    }
    else if (strcmp(cmd, "--menu") == 0)
    {
        if (*entries == NULL && LoadAllDesktopEntries(entries) != 0)
        {
            return EXIT_FAILURE;
        }
        if (*icons == NULL && LoadIcons(jwm, *entries, icons) != 0)
        {
            return EXIT_FAILURE;
        }
        return CreateJWMRootMenu(jwm, *entries, *icons);
    }
    else if (strcmp(cmd, "--prefs") == 0)
    {
        return CreateJWMPreferences(jwm);
    }
    else if (strcmp(cmd, "--styles") == 0)
    {
        return CreateJWMStyles(jwm);
    }
    else if (strcmp(cmd, "--tray") == 0)
    {
        if (*entries == NULL && LoadAllDesktopEntries(entries) != 0)
        {
            return EXIT_FAILURE;
        }
        if (*icons == NULL && LoadIcons(jwm, *entries, icons) != 0)
        {
            return EXIT_FAILURE;
        }
        return CreateJWMTray(jwm, *entries, *icons);
    }

    printf("Unknown option: %s.\n", cmd);
    Usage();
    Help();

    return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    JWM *jwm = NULL;
    cfg_t *cfg = NULL;
    BTreeNode *entries = NULL;
    HashMap *icons = NULL;

    if (argc < 2)
    {
        Usage();
        Help();
        return EXIT_SUCCESS;
    }

    for (int i = 1; i < argc; i++)
    {
        char *cmd = argv[i];

        if (strcmp(cmd, "--help") == 0)
        {
            Help();
            return EXIT_SUCCESS;
        }

        if (strcmp(cmd, "--version") == 0)
        {
            About();
            return EXIT_SUCCESS;
        }

        if (InitializeConfig(&jwm, &cfg) != 0)
        {
            CleanUp(jwm, cfg, icons, entries);
            return EXIT_FAILURE;
        }

        if (HandleCmd(cmd, jwm, cfg, &entries, &icons) != 0)
        {
            CleanUp(jwm, cfg, icons, entries);
            return EXIT_FAILURE;
        }

        // Exit when --all is finished
        if (strcmp(cmd, "--all") == 0)
        {
            break;
        }
    }

    CleanUp(jwm, cfg, icons, entries);
    return EXIT_SUCCESS;
}
