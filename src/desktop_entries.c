#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <dirent.h>
#include <errno.h>

#include <bsd/string.h>

#include "common.h"
#include "bstree.h"
#include "list.h"

#include "desktop_entries.h"

static const Pair xdg_keys[] =
{
    {"Type",                 Type                 },
    {"Version",              Version              },
    {"Name",                 Name                 },
    {"GenericName",          GenericName          },
    {"NoDisplay",            NoDisplay            },
    {"Comment",              Comment              },
    {"Icon",                 Icon                 },
    {"Hidden",               Hidden               },
    {"OnlyShowIn",           OnlyShowIn           },
    {"NotShowIn",            NotShowIn            },
    {"DBusActivatable",      DBusActivatable     },
    {"TryExec",              TryExec             },
    {"Exec",                 Exec                },
    {"Path",                 Path                },
    {"Terminal",             Terminal            },
    {"Actions",              Actions             },
    {"MimeType",             MimeType            },
    {"Categories",           Categories          },
    {"Implements",           Implements          },
    {"Keywords",             Keywords            },
    {"StartupNotify",        StartupNotify       },
    {"StartupWMClass",       StartupWMClass      },
    {"URL",                  URL                 },
    {"PrefersNonDefaultGPU", PrefersNonDefaultGPU},
    {"SingleMainWindow",     SingleMainWindow    }
};

static const Pair xdg_main_categories[] =
{
    {"AudioVideo",  AudioVideo },
    {"Audio",       Audio      },
    {"Video",       Video      },
    {"Development", Development},
    {"Education",   Education  },
    {"Game",        Game       },
    {"Graphics",    Graphics   },
    {"Network",     Network    },
    {"Office",      Office     },
    {"Science",     Science    },
    {"Settings",    Settings  },
    {"System",      System    },
    {"Utility",     Utility   }
};

static int xdg_main_category_tracker[13];

static const Pair xdg_extra_categories[] =
{
    {"WebBrowser",       WebBrowser      },
    {"FileManager",      FileManager     },
    {"TerminalEmulator", TerminalEmulator},
    {"TextEditor",       TextEditor      },
};

static XDGMainCategories GetXDGMainCategoryType(const char *category)
{
    for (size_t i = 0; i < ARRAY_SIZE(xdg_main_categories); i++)
    {
        if (strcmp(category, xdg_main_categories[i].key) == 0)
        {
            return xdg_main_categories[i].value;
        }
    }

    return Invalid;
}

static XDGAdditionalCategories GetXDGAdditionalCategoryType(const char *category)
{
    for (size_t i = 0; i < ARRAY_SIZE(xdg_extra_categories); i++)
    {
        if (strcmp(category, xdg_extra_categories[i].key) == 0)
        {
            return xdg_extra_categories[i].value;
        }
    }

    return IgnoredOrInvalid;
}

static XDGDesktopEntry *CreateEmptyEntry()
{
    XDGDesktopEntry *entry = malloc(sizeof(*entry));

    if (entry == NULL)
        return NULL;

    entry->categories = ListCreate();
    //entry->category = Invalid;
    entry->extra_category = IgnoredOrInvalid;
    //entry->category_name = NULL;
    entry->extra_category_name = NULL;
    entry->name = NULL;
    entry->exec = NULL;
    entry->icon = NULL;
    entry->terminal_required = false;
    return entry;
}

static void DestroyEntry(void *entry)
{
    XDGDesktopEntry *uentry = (XDGDesktopEntry*)entry;
    //free(uentry->category_name);
    free(uentry->extra_category_name);
    free(uentry->name);
    free(uentry->exec);
    free(uentry->icon);
    ListDestroy(uentry->categories);
    free(entry);
}

static void CategoryPrint(void *ptr)
{
    printf("%s ", (char*)ptr);
}

static void EntryPrint(void *ptr, void *args)
{
    XDGDesktopEntry *entry = ptr;
    printf("Program             : %s\n", entry->name);
    printf("Categories          : ");
    ListPrint(entry->categories, CategoryPrint);
    printf("\n");
    printf("Extra Category      : %s\n", entry->extra_category_name);
    printf("CMD                 : %s\n", entry->exec);
    printf("Icon                : %s\n", entry->icon);
    printf("Terminal required   : %d\n", entry->terminal_required);
    printf("\n");
}

static int ExecCmp(const void *a, const void *b)
{
    const XDGDesktopEntry *entry_a = a;
    const char *exec = b;
    return strcmp(entry_a->exec, exec);
}

static int NameCmp(const void *a, const void *b)
{
    const XDGDesktopEntry *entry_a = a;
    const XDGDesktopEntry *entry_b = b;
    return strcasecmp(entry_a->name, entry_b->name);
}

static int NameCmp2(const void *a, const void *b)
{
    const XDGDesktopEntry *entry_a = a;
    const char *name = b;
    return strcasecmp(entry_a->name, name);
}

void EntriesPrint(BTreeNode *entries)
{
    BSTInOrderTraverse(entries, EntryPrint, NULL);
}

void EntryRemove(BTreeNode *entries, const char *key)
{
    BSTDestroyNode(entries, key, NameCmp2, DestroyEntry);
}

XDGDesktopEntry *EntriesSearch(BTreeNode *entries, const char *key)
{
    BTreeNode *node = BSTSearchNode(entries, key, NameCmp2);
    return node != NULL ? node->data : NULL;
}

void EntriesDestroy(BTreeNode *entries)
{
    BSTDestroy(&entries, DestroyEntry);
}

static XDGDesktopEntry *InorderTraverseProgramSearch(BTreeNode *node, XDGAdditionalCategories extra_category, const char *name, XDGDesktopEntry **fallback)
{
    if (node == NULL)
    {
        return NULL;
    }

    XDGDesktopEntry *result = InorderTraverseProgramSearch(node->left, extra_category, name, fallback);
    if (result != NULL)
    {
        return result;
    }

    XDGDesktopEntry *entry = node->data;
    if (entry->extra_category == extra_category)
    {
        char *exec_basename = basename(entry->exec);
        if (strcmp(exec_basename, name) == 0)
        {
            return entry;
        }

        if (strstr(exec_basename, name) != NULL)
        {
            return entry;
        }
        DEBUG_LOG("Setting %s as an fallback since %s does not match %s\n", entry->exec, entry->exec, name);
        *fallback = entry;
    }

    return InorderTraverseProgramSearch(node->right, extra_category, name, fallback);
}

XDGDesktopEntry *GetCoreProgram(BTreeNode *root, XDGAdditionalCategories extra_category, const char *name)
{
    XDGDesktopEntry *fallback = NULL;

    XDGDesktopEntry *result = InorderTraverseProgramSearch(root, extra_category, name, &fallback);
    
    if (result == NULL)
    {
        if (fallback == NULL)
        {
            printf("Couldn't find %s, And no fallback available! Quitting program...\n", name);
            exit(1);
        }

        printf("Couldn't find %s. Using %s as a fallback\n", name, fallback->exec);
    }

    return result ? result : fallback;
}

// DRY principle violated here, should find a better way of searching programs
static XDGDesktopEntry *InorderTraverseProgramSearch2(BTreeNode *node, const char *name)
{
    if (node == NULL)
    {
        return NULL;
    }

    XDGDesktopEntry *result = InorderTraverseProgramSearch2(node->left, name);
    if (result != NULL)
    {
        return result;
    }

    XDGDesktopEntry *entry = node->data;
    char *exec_basename = basename(entry->exec);
    if (strcmp(exec_basename, name) == 0)
    {
        return entry;
    }

    return InorderTraverseProgramSearch2(node->right, name);
}

XDGDesktopEntry *GetProgram(BTreeNode *root, const char *name)
{
    XDGDesktopEntry *result = InorderTraverseProgramSearch2(root, name);
    
    if (result == NULL)
    {
        printf("Couldn't find program %s!\n", name);
    }

    return result ? result : NULL;
}

static void ParseExec(XDGDesktopEntry *entry, const char *exec)
{
    char final[1024] = {"\0"};

    char *save_ptr;
    char *str_copy = strdup(exec);

    char *token = strtok_r(str_copy, " ", &save_ptr);

    // Parse environment variables if they exist
    if (strcmp(token, "env") == 0)
    {
        strlcpy(final, "env ", sizeof(final));
        while ((token = strtok_r(NULL, " ", &save_ptr)) != NULL && strchr(token, '=') != NULL)
        {
            char *reserved;
            char *name = strtok_r(token, "=", &reserved);
            char *value = strtok_r(NULL, "=", &reserved);
            if (name != NULL && value != NULL)
            {
                strlcat(final, name, sizeof(final));
                strlcat(final, "=", sizeof(final));
                strlcat(final, value, sizeof(final));
                strlcat(final, " ", sizeof(final));
            }
        }
    }

    // Parse program and arguments
    if (token != NULL)
    {
        strlcat(final, token, sizeof(final));
        strlcat(final, " ", sizeof(final));
        while ((token = strtok_r(NULL, " ", &save_ptr)) != NULL)
        {
            // Skip args that contain the % character
            if ((strchr(token, '%')) != NULL)
            {
                continue;
            }
            
            strlcat(final, token, sizeof(final));
            strlcat(final, " ", sizeof(final));
        }
    }

    StripTrailingWSpace(final);
    entry->exec = strdup(final);
    free(str_copy);
}

static void ParseCategories(XDGDesktopEntry *entry, char *categories, XDGMainCategories *main_category)
{
    DEBUG_LOG("Listed categories: %s\n", categories);
    *main_category = Invalid;

    char *reserved;
    char *token = strtok_r(categories, ";", &reserved);
    while (token != NULL)
    {
        XDGMainCategories main = GetXDGMainCategoryType(token);
        XDGAdditionalCategories extra = GetXDGAdditionalCategoryType(token);

        if (main != Invalid)
        {
            *main_category = main; 
            ListAdd(entry->categories, token, strlen(token) + 1);
            /*
            if (entry->category_name == NULL)
            {
                entry->category_name = strdup(token);
                entry->category = main;
            }
            */
        }

        if (extra != IgnoredOrInvalid && entry->extra_category_name == NULL)
        {
            entry->extra_category_name = strdup(token);
            entry->extra_category = extra;
        }

        token = strtok_r(NULL, ";", &reserved);
    }
}

static void ParseDesktopEntry(XDGDesktopEntry *entry, int key_type, char *key, char *value, ParsedInfo *info)
{
    switch (key_type)
    {
        case Type:
        {
            DEBUG_LOG("Found Type: %s\n", value);
            // Ignore links and directories for now...
            if (strcmp(value, "Application") == 0)
                info->application = true;
            break;
        }

        case Version:
        break;

        case Name:
        {
            DEBUG_LOG("Found Name: %s\n", value);
            if (entry->name != NULL)
            {
                printf("Name already set! %s -> %s\n", entry->name, value);
            }
            entry->name = strdup(value);
            break;
        }

        case GenericName:
        break;

        case NoDisplay:
        {
            DEBUG_LOG("NoDiplay: %s\n", value);
            info->no_display = true;
            break;
        }
        case Comment:
        break;
        case Icon:
        {
            DEBUG_LOG("Found Icon: %s\n", value);
            if (entry->icon != NULL)
            {
                printf("Icon already set! %s -> %s\n", entry->icon, value);
            }
            info->icon_exists = true;
            entry->icon = strdup(value);
            break;
        }
        case Hidden:
        {
            DEBUG_LOG("Found Hidden: %s\n", value);
            break;
        }
        case TryExec:
        {
            DEBUG_LOG("Found TryExec: %s\n", value);
            break;
        }
        case Exec:
        {
            DEBUG_LOG("Found Exec: %s\n", value);
            ParseExec(entry, value);
            info->has_exec = true;
            break;
        }
        case Path:
        {
            DEBUG_LOG("Found Path: %s\n", value);
            break;
        }
        case Terminal:
        {
            entry->terminal_required = StringToBool(value);
            break;
        }
        case MimeType:
        {
            DEBUG_LOG("Found MimeType: %s\n", value);
            break;
        }
        case Categories:
        {
            ParseCategories(entry, value, &info->main_category);
            break;
        }
        case Keywords:
            DEBUG_LOG("Found Keywords: %s\n", value);
        break;
        case URL:
            DEBUG_LOG("Found URL: %s\n", value);
        break;

        default:
            DEBUG_LOG("Invalid or ingored key: \"%s\" contains \"%s\"\n", key, value);
        break;
    }
}

static XDGDesktopEntry *ReadDesktopEntry(const char *path)
{
    FILE *fp = fopen(path, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return NULL;
    }

    int read = 0;
    size_t len = 0;
    char *line = NULL;

    ParsedInfo info = { false };

    XDGDesktopEntry *entry = CreateEmptyEntry();
    bool is_desktop_entry = false;
    // Read line by line
    while ((read = getline(&line, &len, fp)) != -1)
    {
        // Skip blank lines and comments
        if (line[0] == '\n' || line[0] == '#')
            continue;

        // Simple trailing newline removal
        //line[strcspn(line, "\n")] = 0;

        if (line[0] == '[' && line[strlen(line) - 1] == '\n')
        {
            // Detect nested entries in one desktop entry file (eg:Steam)
            if (strcmp(line, "[Desktop Entry]\n") == 0)
            {
                is_desktop_entry = true;
            }
            else
            {
                // Stop parsing if we encounter a different section
                break;
            }
        }

        if (is_desktop_entry)
        {
            char *reserved;
            char *key = strtok_r(line, "=", &reserved);
            char *value = strtok_r(NULL, "\n", &reserved);

            if (value == NULL)
                continue; // Not a key-value pair

            for (size_t i = 0; i < ARRAY_SIZE(xdg_keys); i++)
            {
                if (strcmp(key, xdg_keys[i].key) == 0)
                {
                    ParseDesktopEntry(entry, i, key, value, &info);
                    break;
                }
            }
        }
    }

    fclose(fp);
    free(line);

    // Strict requirments for any program that wants to displayed in the menu, similar to what kde plasma's default menu/application launcher
    if (info.icon_exists && info.application && info.has_exec && !info.no_display)
    {
        // Kinda hacky, should be redone using the full xdg menu spec in the future
        //xdg_main_category_tracker[info.main_category]++;

        return entry;
    }

    DestroyEntry(entry);
    return NULL;
}

int LoadDesktopEntries(BTreeNode **entries, const char *path)
{
    char buffer[512];

    DIR *dentry_dir = opendir(path);

    if (dentry_dir == NULL)
        return -1;

    struct dirent *dirp;

    int count = 0;

    while ((dirp = readdir(dentry_dir)) != NULL)
    {
        char *ext = strrchr(dirp->d_name, '.');
        if (ext && (strcmp(ext, ".desktop") == 0))
        {
            strlcpy(buffer, path, sizeof(buffer));
            strlcat(buffer, dirp->d_name, sizeof(buffer));

            DEBUG_LOG("\nReading %s\n", buffer);
            XDGDesktopEntry *entry = ReadDesktopEntry(buffer);

            if (entry != NULL)
            {
                *entries = BSTInsertNode(*entries, entry, NameCmp);
                count++;
                DEBUG_LOG("Adding entry \"%s\" from %s to the tree\n", entry->name, buffer);
            }
            else
            {
                printf("Skipping %s\n", buffer);
            }
        }
    }

    if (count > 0)
    {
        printf("\nFinished reading %d valid desktop entries\n\n", count);
    }
    else
    {
        printf("\nNo valid desktop entries found in %s\n\n", path);
    }

    return closedir(dentry_dir);
}
