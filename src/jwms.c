#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

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
            "  -h, --help         Display this information\n"
            "  -v, --version      Display version information\n"
            "  -a, --all          Generate all JWM files\n"
            "  -A, --autostart    Generate the JWM autostart script\n"
            "  -b, --binds        Generate the JWM keybinds\n"
            "  -g, --groups       Generate the JWM program groups\n"
            "  -i, --icons        Generate the JWM icon paths\n"
            "  -j, --jwmrc        Generate the JWM rc file\n"
            "  -m, --menu         Generate the JWM rootmenu\n"
            "  -p, --prefs        Generate the JWM preference\n"
            "  -s, --styles       Generate the JWM styles\n"
            "  -t, --tray         Generate the JWM tray\n");
}

static const struct option long_opts[] =
{
    {"help",      no_argument, 0, 'h'},
    {"version",   no_argument, 0, 'v'},
    {"all",       no_argument, 0, 'a'},
    {"autostart", no_argument, 0, 'A'}, 
    {"binds",     no_argument, 0, 'b'},
    {"groups",    no_argument, 0, 'g'},
    {"icons",     no_argument, 0, 'i'},
    {"jwmrc",     no_argument, 0, 'j'},
    {"menu",      no_argument, 0, 'm'},
    {"prefs",     no_argument, 0, 'p'},
    {"styles",    no_argument, 0, 's'},
    {"tray",      no_argument, 0, 't'},
    {0, 0, 0, 0}  // terminator
};

static int LoadAllDesktopEntries(BTreeNode **entries)
{
    // These paths should not be hardcoded, but they work for now
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
    
    *icons = FindAllIcons(entries, jwm->global_preferred_icon_size, 1);
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

    if (CreateJWMRootMenu(jwm, entries, icons, NULL) != 0)
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
    {
        DestroyIconThemes();
        HashMapDestroy(icons);
    }
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

int HandleCmd(int opt, JWM *jwm, cfg_t *cfg, BTreeNode **entries, HashMap **icons)
{
    switch (opt)
    {
        case 'a': // --all
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
        case 'A': // --autostart
            return CreateJWMAutoStart(jwm, cfg);

        case 'b': // --binds
            return CreateJWMBinds(jwm, cfg);

        case 'g': // --groups
            return CreateJWMGroup(jwm);

        case 'i': // --icons
            return CreateJWMIcons(jwm);

        case 'j': // --jwmrc
            return CreateJWMRCFile(jwm);

        case 'm': // --menu
            if (*entries == NULL && LoadAllDesktopEntries(entries) != 0)
            {
                return EXIT_FAILURE;
            }
            if (*icons == NULL && LoadIcons(jwm, *entries, icons) != 0)
            {
                return EXIT_FAILURE;
            }
            return CreateJWMRootMenu(jwm, *entries, *icons, NULL);

        case 'p': // --prefs
            return CreateJWMPreferences(jwm);

        case 's': // --styles
            return CreateJWMStyles(jwm);

        case 't': // --tray
            if (*entries == NULL && LoadAllDesktopEntries(entries) != 0)
            {
                return EXIT_FAILURE;
            }
            if (*icons == NULL && LoadIcons(jwm, *entries, icons) != 0)
            {
                return EXIT_FAILURE;
            }
            return CreateJWMTray(jwm, *entries, *icons);

        default:
            return EXIT_FAILURE;
    }
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

    // Handle help and version options explicitly first
    int opt;
    int index = 0;
    while ((opt = getopt_long(argc, argv, "hvaAbgijmpst", long_opts,
            &index)) != -1)
    {
        switch (opt)
        {
            case 'h': // --help
                Help();
                return EXIT_SUCCESS;

            case 'v': // --version
                About();
                return EXIT_SUCCESS;
            case '?':
                Usage();
                Help();
                return EXIT_FAILURE;

            default:
                if (InitializeConfig(&jwm, &cfg) != 0)
                {
                    CleanUp(jwm, cfg, icons, entries);
                    return EXIT_FAILURE;
                }
    
                if (HandleCmd(opt, jwm, cfg, &entries, &icons) != 0)
                {
                    CleanUp(jwm, cfg, icons, entries);
                    return EXIT_FAILURE;
                }
        }

        if (opt == 'a')
            break;
    }

    CleanUp(jwm, cfg, icons, entries);
    return EXIT_SUCCESS;
}
