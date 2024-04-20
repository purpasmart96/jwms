
#include <libgen.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include <dirent.h>
#include <errno.h>

#include <bsd/string.h>
#include <string.h>

#include "common.h"
#include "darray.h"
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

static const Pair xdg_extra_categories[] =
{
    {"WebBrowser",       WebBrowser      },
    {"FileManager",      FileManager     },
    {"TerminalEmulator", TerminalEmulator},
    {"TextEditor",       TextEditor      },
};

static const char *bad_args[] =
{
    "%U",
    "%F",
    "%u",
    "%f",
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

static bool StringToBool(char *input)
{
    const int max_length = 6;
    int length = strlen(input);

    if (input == NULL || length > max_length)
        return false;

    char temp[6];
    for (size_t i = 0; input[i]; i++)
    {
        temp[i] = tolower(input[i]);
    }

    temp[length] = '\0';

    return strcmp(temp, "true") == 0 || strcmp(temp, "1") == 0;
}

static void StripTrailingWSpace(char *str)
{
    if (str == NULL)
        return;

    // Remove trailing whitespace
    int length = strlen(str);
    while (length > 0 && isspace(str[length - 1]))
    {
        length--;
    }

    str[length] = '\0';
}

static void RemoveSubStrNoOverlap(char *str, const char *substr)
{
    char *found = strstr(str, substr);
    if (!found)
        return;

    size_t sub_length = strlen(substr);
    size_t end_length = strlen(found + sub_length) + 1;
    memcpy(found, found + sub_length, end_length);
}

static XDGDesktopEntry *CreateEmptyEntry()
{
    XDGDesktopEntry *entry = malloc(sizeof(*entry));

    if (entry == NULL)
        return NULL;

    entry->category = Invalid;
    entry->extra_category = IgnoredOrInvalid;
    entry->category_name = NULL;
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
    free(uentry->category_name);
    free(uentry->extra_category_name);
    free(uentry->name);
    free(uentry->exec);
    free(uentry->icon);
    free(entry);
}

static void EntryPrint(void *ptr)
{
    XDGDesktopEntry *entry = ptr;
    printf("Program             : %s\n", entry->name);
    printf("Category            : %s\n", entry->category_name);
    printf("Extra Category      : %s\n", entry->extra_category_name);
    printf("CMD                 : %s\n", entry->exec);
    printf("Icon                : %s\n", entry->icon);
    printf("Terminal required   : %d\n", entry->terminal_required);
}

void EntriesPrint(DArray *entries)
{
    DArrayPrint(entries, EntryPrint);
}

void EntriesDestroy(DArray *entries)
{
    DArrayDestroy(entries, DestroyEntry);
}

static int ProgramCmp(void *a, void *b)
{
    XDGDesktopEntry *entry_a = a;
    XDGDesktopEntry *entry_b = b;
    if ((strcmp(entry_a->exec, entry_b->exec) == 0) && entry_a->extra_category == entry_b->extra_category)
        return 0;
    return -1;
}

static int BrowserCmp(void *a, void *b)
{
    XDGDesktopEntry *entry_a = a;
    XDGDesktopEntry *entry_b = b;
    if ((strcmp(entry_a->exec, entry_b->exec) == 0) && entry_a->extra_category == entry_b->extra_category)
        return 0;
    return -1;
}

static int ExecCmp(const void *a, const void *b)
{
    const XDGDesktopEntry *entry_a = a;
    const char *exec = b;
    return strcmp(entry_a->exec, exec);
}

void *EntriesBinarySearchExec(DArray *entries, const void *target)
{
    return DArrayBinarySearch(entries, target, ExecCmp);
}

static int EntrySortCmp(const void *a, const void *b )
{
    const XDGDesktopEntry *entry_a = *(XDGDesktopEntry**)a;
    const XDGDesktopEntry *entry_b = *(XDGDesktopEntry**)b;

    int val = strcmp(entry_a->exec, entry_b->exec);

    return val;
}

void EntriesSort(DArray *entries)
{
    DArraySort(entries, EntrySortCmp);
}

XDGDesktopEntry *EntriesSearchExec(DArray *entries, const char *key)
{
    return EntriesBinarySearchExec(entries, key);
}

bool EntryExecExists(DArray *entries, const char *key)
{
    return DArrayContains(entries, key, EntrySortCmp, ExecCmp);
}

XDGDesktopEntry *GetCoreProgram(DArray *entries, XDGAdditionalCategories extra_category, const char *name)
{
    char *base_name = basename(strdup(name)); 
    XDGDesktopEntry *entry = EntriesBinarySearchExec(entries, base_name);
    if (entry != NULL)
        if (entry->extra_category == extra_category)
        {
            free(base_name);
            return entry;
        }

    printf("Couldn't find %s. Finding another one\n", base_name);
    // Couldn't find it, let's search for the first valid one
    for (unsigned int i = 0; i < entries->size; i++)
    {
        XDGDesktopEntry *entry = entries->data[i];

        if (entry->extra_category == extra_category)
        {
            free(base_name);
            // Found one, let's use it
            return entry;
        }
    }

    free(base_name);
    return NULL;
}

static void ParseExec(XDGDesktopEntry *entry, const char *exec)
{
    entry->exec = strdup(exec);
    for (size_t i = 0; i < ARRAY_SIZE(bad_args); i++)
    {
        if (strstr(exec, bad_args[i]) != NULL)
        {
            RemoveSubStrNoOverlap(entry->exec, bad_args[i]);
        }
    }
    StripTrailingWSpace(entry->exec);
}

static void ParseCategories(XDGDesktopEntry *entry, char *categories)
{
    //printf("Listed categories: %s\n", categories);

    char *reserved;
    char *token = strtok_r(categories, ";", &reserved);
    while (token != NULL)
    {
        XDGMainCategories main = GetXDGMainCategoryType(token);
        XDGAdditionalCategories extra = GetXDGAdditionalCategoryType(token);

        if (main != Invalid && entry->category_name == NULL)
        {

            entry->category_name = strdup(token);
            entry->category = main;
        }

        if (extra != IgnoredOrInvalid && entry->extra_category_name == NULL)
        {
            entry->extra_category_name = strdup(token);
            entry->extra_category = extra;
        }

        // We reached the end, exit the loop
        if (main != Invalid && extra != IgnoredOrInvalid)
            break;

        token = strtok_r(NULL, ";", &reserved);
    }
}

static bool ParseDesktopEntry(XDGDesktopEntry *entry, int key_type, char *value, bool *application, bool *icon_exists, bool *has_exec)
{
    switch (key_type)
    {
        case Type:
        {
             // Ignore links and directories for now...
            if (strcmp(value, "Application") == 0)
                *application = true;
            break;
        }

        case Name:
        {
            if (entry->name != NULL)
            {
                printf("Name already set! %s -> %s\n", entry->name, value);
            }
            entry->name = strdup(value);
            break;
        }
        case GenericName:
        break;
        case Comment:
        break;
        case Icon:
        {
            if (entry->icon != NULL)
            {
                printf("Icon already set! %s -> %s\n", entry->icon, value);
            }
            *icon_exists = true;
            entry->icon = strdup(value);
            break;
        }
        case Hidden:
        break;
        case TryExec:
        {
            //printf("Found TryExec: %s\n", value);
            break;
        }
        case Exec:
        {
            //const char *stripped_exec = basename(value);
            ParseExec(entry, value);
            *has_exec = true;
            break;
        }
        case Path:
        {
            //printf("Found Path: %s\n", value);
            break;
        }
        case Terminal:
        {
            entry->terminal_required = StringToBool(value);
            break;
        }
        case MimeType:
        {
            //printf("Found MimeType: %s\n", value);
            break;
        }
        case Categories:
        {
            ParseCategories(entry, value);
            break;
        }
        case URL:
        break;

        default:
            //printf("Invalid or ingored category \"%s\" contains \"%s\"\n", key, value);
        break;
    }
    return false;
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

    bool application = false;
    bool icon_exists = false;
    bool has_exec = false;
    XDGDesktopEntry *entry = CreateEmptyEntry();
    bool is_desktop_entry = false;
    // Read line by line
    while ((read = getline(&line, &len, fp)) != -1)
    {
        // Skip blank lines and comments
        if (line[0] == '\n' || line[0] == '#')
            continue;

        // Simple trailing newline removal
        line[strcspn(line, "\n")] = 0;

        if (line[0] == '[' && line[strlen(line) - 1] == ']')
        {
            // Detect nested entries in one desktop entry file (eg:Steam)
            if (strcmp(line, "[Desktop Entry]") == 0)
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
            char *value = strtok_r(NULL, "=", &reserved);

            if (value == NULL)
                continue; // Not a key-value pair

            for (unsigned int i = 0; i < ARRAY_SIZE(xdg_keys); i++)
            {
                if (strcmp(key, xdg_keys[i].key) == 0)
                {
                    ParseDesktopEntry(entry, i, value, &application, &icon_exists, &has_exec);
                    break;
                }
            }
        }
    }

    fclose(fp);
    free(line);

    if (icon_exists && application && has_exec)
        return entry;

    DestroyEntry(entry);
    return NULL;
}

int LoadDesktopEntries(DArray *entries)
{
    char buffer[512];
    const char *path = "/usr/share/applications/";

    DIR *dentry_dir = opendir(path);

    if (dentry_dir == NULL)
        return -1;

    struct dirent *dirp;

    while ((dirp = readdir(dentry_dir)) != NULL)
    {
        char *ext = strrchr(dirp->d_name, '.');
        if (ext && (strcmp(ext, ".desktop") == 0))
        {
            strlcpy(buffer, path, sizeof(buffer));
            strlcat(buffer, dirp->d_name, sizeof(buffer));

            XDGDesktopEntry *entry = ReadDesktopEntry(buffer);

            if (entry != NULL)
            {
                DArrayAdd(entries, entry);
            }
            else
            {
                printf("Failed to parse %s\n", buffer);
            }
        }
    }

    return closedir(dentry_dir);
}
