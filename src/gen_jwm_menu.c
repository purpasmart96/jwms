#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <bsd/string.h>
#include <confuse.h>

#include "common.h"
#include "bstree.h"
#include "list.h"
#include "hashing.h"
#include "darray.h"
#include "icons.h"
#include "desktop_entries.h"
#include "config.h"

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


static int CategoryCmp(const void *a, const void *b)
{
    const char *name1 = a;
    const char *name2 = b;
    return strcmp(name1, name2) == 0;
}

typedef struct
{
    char *real_name;
    char *menu_name;
    int value;
} MenuCategory;

typedef struct
{
    HashMap *icons;
    FILE *fp;
    MenuCategory *category;
} CategoryArgs;

static void WriteMenuCategory(void *entry_ptr, void *args_ptr)
{
    XDGDesktopEntry *entry = entry_ptr;
    CategoryArgs *args = args_ptr;
    FILE *fp = args->fp;

    if (ListContains(entry->categories, args->category->real_name, CategoryCmp))
    {
        const char *icon = HashMapGet(args->icons, entry->icon);
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
            WRITE_CFG("            <Program icon=\"%s\" label=\"%s\">%s -e %s</Program>\n",
                        icon, entry->name, g_terminal->exec, entry->exec);
        }
    }
}

static void WriteJWMRootMenuCategoryList(BTreeNode *entries, HashMap *icons, FILE *fp, MenuCategory *category)
{
    char *category_icon = FindIcon(category_icons[category->value], 32, 1);
    WRITE_CFG("       <Menu icon=\"%s\" label=\"%s\">\n", category_icon, category->menu_name);
    free(category_icon);
 
    CategoryArgs args =
    {
        .icons = icons,
        .fp = fp,
        .category = category,     
    };

    // Recursively traverse the BST in-order and write each entry to the file
    BSTInOrderTraverse(entries, WriteMenuCategory, &args);

    WRITE_CFG("       </Menu>\n");
}

typedef struct
{
    MenuCategory category;
    bool found;
} Args;

static void CountCategories(void *ptr, void *args_ptr)
{
    XDGDesktopEntry *entry = ptr;
    Args *args = args_ptr;
    
    for (int i = 0; i < 11; ++i)
    {
        if (ListContains(entry->categories, args[i].category.real_name, CategoryCmp))
        {
            args[i].found = true;
        }
    }
}

int CreateJWMRootMenu(JWM *jwm, BTreeNode *entries, HashMap *icons)
{
    char path[512];
    const char *fname = "menu";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the root menu xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("    <RootMenu height=\"%d\" onroot=\"12\">\n", jwm->root_menu_height);

    // Clean up duplicate code, should refactor this imo
    MenuCategory categories[] =
    {
        {"Development", "Development", Development },
        {"Education",   "Education",   Education   },
        {"Game",        "Games",       Game        },
        {"Graphics",    "Graphics",    Graphics    },
        //{"Audio",       "Multimedia",  Audio       },
        //{"Video",       "Multimedia",  Video       },
        {"AudioVideo",  "Multimedia",  AudioVideo  },
        {"Network",     "Internet",    Network     },
        {"Office",      "Office",      Office      },
        {"Science",     "Science",     Science     },
        {"Settings",    "Settings",    Settings    },
        {"System",      "System",      System      },
        {"Utility",     "Utilities",   Utility     }
    };

    const int num_categories = 11;
    Args args[num_categories];

    for (int i = 0; i < num_categories; i++)
    {
        args[i].category = categories[i];
        args[i].found = false;
    }

    BSTInOrderTraverse(entries, CountCategories, args);

    for (int i = 0; i < num_categories; i++)
    {
        if (args[i].found)
        {
            WriteJWMRootMenuCategoryList(entries, icons, fp, &args[i].category);
        }
    }

    WRITE_CFG("        <Restart label=\"Refresh\" icon=\"view-refresh\"/>\n");
    WRITE_CFG("        <Exit label=\"Logout\" icon=\"system-log-out\"/>\n");
    // Let's assume were using systemd for now
    //WRITE_CFG("        <Program icon=\"system-reboot\" label=\"Restart\">systemctl reboot</Program>\n");
    //WRITE_CFG("        <Program icon=\"system-shutdown\" label=\"Shutdown\">systemctl poweroff</Program>\n");
    WRITE_CFG("    </RootMenu>\n");
    WRITE_CFG("</JWM>");

    fclose(fp);
    return 0;
}
