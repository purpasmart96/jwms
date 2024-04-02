
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

#include "hashing.h"
#include "ini.h"
#include "jwms.h"

static KeyValuePair xdg_keys[] =
{
    {"Type",                 Type                },
    {"Version",              Version             },
    {"Name",                 Name                },
    {"GenericName",          GenericName         },
    {"NoDisplay",            NoDisplay           },
    {"Comment",              Comment             },
    {"Icon",                 Icon                },
    {"Hidden",               Hidden              },
    {"OnlyShowIn",           OnlyShowIn          },
    {"NotShowIn",            NotShowIn           },
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

static KeyValuePair xdg_main_categories[] =
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
    {"Settings",    Settings   },
    {"System",      System     },
    {"Utility",     Utility    }
};

static KeyValuePair xdg_extra_categories[] =
{
    {"WebBrowser",       WebBrowser      },
    {"FileManager",      FileManager     },
    {"TerminalEmulator", TerminalEmulator},
    {"TextEditor",       TextEditor      },
};

static KeyValuePair *CreateCategory(char *key, int value)
{
    KeyValuePair *category = malloc(sizeof(KeyValuePair));
    category->key = strdup(key);
    category->value = value;

    return category;
}

static void DestroyCategory(KeyValuePair *category)
{
    free(category->key);
    free(category);
}

static XDGDesktopEntry *CreateEntry(KeyValuePair *category, KeyValuePair *extra_category, char *name, char *exec,
                                    char *icon, bool terminal_required)
{
    XDGDesktopEntry *entry = malloc(sizeof(XDGDesktopEntry));

    if (entry == NULL)
        return NULL;

    if (!category)
        entry->category = CreateCategory("", -1);
    else
        entry->category = category;

    if (!extra_category)
        entry->extra_category = CreateCategory("", -1);
    else
        entry->extra_category = extra_category;

    entry->name = strdup(name);
    entry->exec = strdup(exec);
    entry->icon = strdup(icon);
    entry->desc = NULL;
    entry->terminal_required = terminal_required;
    return entry;
}

static void DestroyEntry(XDGDesktopEntry *entry)
{
    DestroyCategory(entry->category);
    DestroyCategory(entry->extra_category);
    free(entry->name);
    free(entry->exec);
    //free(entry->desc);
    free(entry->icon);

    free(entry);
}

static XDGDesktopEntries *EntriesCreateArray(size_t capacity)
{
    XDGDesktopEntries *entries = malloc(sizeof(*entries));
    entries->capacity = capacity;
    entries->data = malloc(sizeof(XDGDesktopEntry) * entries->capacity);
    entries->size = 0;

    return entries;
}

static void EntriesPrint(XDGDesktopEntries *entries)
{
    for (size_t i = 0; i < entries->size; i++)
    {
        printf("\n");
        printf("Program             : %s\n", entries->data[i]->name);
        printf("Category            : %s\n", entries->data[i]->category->key);
        printf("Extra Category      : %s\n", entries->data[i]->extra_category->key);
        printf("CMD                 : %s\n", entries->data[i]->exec);
        printf("Icon                : %s\n", entries->data[i]->icon);
        printf("Terminal required   : %d\n", entries->data[i]->terminal_required);
    }
}

static XDGDesktopEntry *EntriesSearchExec(XDGDesktopEntries *entries, const char *key)
{
    for (size_t i = 0; i < entries->size; i++)
    {
        //if (strcmp(entries->data[i]->exec, key) == 0)
        if (strstr(entries->data[i]->exec, key) != NULL)
            return entries->data[i];
    }

    return NULL;
}


static XDGDesktopEntry **EntriesResizeArray(XDGDesktopEntries *entries, size_t capacity)
{
    entries->capacity = capacity;
    XDGDesktopEntry **temp = realloc(entries->data, sizeof(XDGDesktopEntry) * entries->capacity);

    if (!temp)
    {
        return NULL;
    }

    entries->data = temp;

    return temp;
}

static void EntriesDestroy(XDGDesktopEntries *entries)
{
    for (size_t i = 0; i < entries->size; i++)
    {
        DestroyEntry(entries->data[i]);
    }

    free(entries->data);
    free(entries);
}

bool EntriesEmpty(XDGDesktopEntries *entries)
{
    return !entries->size;
}

bool EntriesFull(XDGDesktopEntries *entries)
{
    return entries->size == entries->capacity;
}

static int EntriesAdd(XDGDesktopEntries *entries, XDGDesktopEntry *element)
{
    if (EntriesFull(entries))
    {
        if (!EntriesResizeArray(entries, entries->capacity *= 2))
        {
            return -1;
        }
    }
    // Push an element on the top of it and increase its size by one
    entries->data[entries->size++] = element;

    return 0;
}

// Not Tested
XDGDesktopEntry **EntriesRemove(XDGDesktopEntries *entries, size_t index)
{
    // Allocate an array with a size 1 less than the current one
    XDGDesktopEntry **temp = malloc((entries->size - 1) * sizeof(XDGDesktopEntry));

    if (index != 0) // copy everything BEFORE the index
        memcpy(temp, entries->data, (index - 1) * sizeof(XDGDesktopEntry));

    if (index != (entries->size - 1)) // copy everything AFTER the index
        memcpy(temp + index, entries->data + index + 1, (entries->size - index - 1) * sizeof(XDGDesktopEntry));

    free(entries->data);
    return temp;
}

static XDGDesktopEntry *GetCoreProgram(XDGDesktopEntries *entries, int extra_category, const char *name)
{
    for (unsigned int i = 0; i < entries->size; i++)
    {
        XDGDesktopEntry *entry = entries->data[i];
        if ((strcmp(entry->exec, name) == 0) && entry->extra_category->value == extra_category)
            return entry;
    }

    printf("Couldn't find %s. Finding another one\n", name);
    // Couldn't find it, let's search for the first valid one
    for (unsigned int i = 0; i < entries->size; i++)
    {
        KeyValuePair *extra = entries->data[i]->extra_category;

        if (extra->value == extra_category)
        {
            // Found one, let's use it
            return entries->data[i];
        }
    }

    return NULL;
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

static const char *bad_args[] =
{
    "%U",
    "%F",
    "%u",
    "%f",
    "--player-operation-mode",
    "--pause"
};

static void RemoveSubStrAndShiftL(char *str, const char *substr)
{
    char *found = strstr(str, substr);
    if (!found)
        return;

    size_t sub_length = strlen(substr);
    size_t end_length = strlen(found + sub_length) + 1;
    memcpy(found, found + sub_length, end_length);

    // Remove trailing whitespace
    size_t length = strlen(str);
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

static void RemoveAllSubStrsNoOverlap(char *str, const char *substr)
{
    char *found = strstr(str, substr);
    if (!found)
        return;

    size_t sub_length = strlen(substr);
    while (found != NULL)
    {
        size_t end_length = strlen(found + sub_length) + 1;
        memcpy(found, found + sub_length, end_length);
        found = strstr(str, substr);
    }
}

static char *ParseExec(char *exec)
{
    char *temp = exec;
    for (unsigned int i = 0; i < ARRAY_SIZE(bad_args); i++)
    {
        if (strstr(temp, bad_args[i]) != NULL)
        {
            RemoveSubStrNoOverlap(temp, bad_args[i]);
        }
    }

    StripTrailingWSpace(temp);

    return temp;
}

static KeyValuePair *ParseCategories(char *categories)
{
    printf("Listed categories: %s\n", categories);

    char *reserved;
    char *token = strtok_r(categories, ";", &reserved);
    while (token != NULL)
    {
        for (unsigned int i = 0; i < ARRAY_SIZE(xdg_main_categories); i++)
        {
            KeyValuePair *category = &xdg_main_categories[i];
            if (strcmp(token, category->key) == 0)
            {
                printf("Found valid category: %s\n", token);
                //return category;
                return CreateCategory(category->key, category->value);
            }
        }

        token = strtok_r(NULL, ";", &reserved);
    }

    printf("No valid category found in list: %s\n", categories);

    //return CreateCategory("", -1);
    return NULL;
}

static KeyValuePair *ParseAdditionalCategories(char *categories)
{
    printf("Listed additional categories: %s\n", categories);

    char *reserved;
    char *token = strtok_r(categories, ";", &reserved);
    while (token != NULL)
    {
        for (unsigned int i = 0; i < ARRAY_SIZE(xdg_extra_categories); i++)
        {
            KeyValuePair *category = &xdg_extra_categories[i];
            if (strcmp(token, category->key) == 0)
            {
                printf("Found valid category: %s\n", token);
                //return category;
                return CreateCategory(category->key, category->value);
            }
        }

        token = strtok_r(NULL, ";", &reserved);
    }

    //return CreateCategory("", -1);
    return NULL;
}

static XDGDesktopEntry *ReadDesktopEntry(const char *path)
{
    FILE *fp;
    fp = fopen(path, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return NULL;
    }

    int read = 0;
    size_t len = 0;
    char *line = NULL;

    KeyValuePair *main_category = NULL;
    KeyValuePair *extra_category = NULL;
    bool application = false;
    bool icon_exists = false;
    //bool category_exists = false;
    //bool extra_category_exists = false;
    //char type[64];
    char name[128];
    char exec[128];
    //char desc[256];
    char icon[128];
    bool terminal_required = false;

    // Read line by line
    while ((read = getline(&line, &len, fp)) != -1)
    {
        // Skip blank lines and comments
        if (line[0] == '\n' || line[0] == '#')
            continue;

        // Simple trailing newline removal
        line[strcspn(line, "\n")] = 0;

        char *reserved;
        char *key = strtok_r(line, "=", &reserved);
        char *value = strtok_r(NULL, "=", &reserved);

        if (value == NULL)
            continue; // Not a key-value pair

        for (unsigned int i = 0; i < ARRAY_SIZE(xdg_keys); i++)
        {
            if (strcmp(key, xdg_keys[i].key) == 0)
            {
                switch (xdg_keys[i].value)
                {
                    case Type:
                    {
                        // Ignore links and directories for now...
                        if (strcmp(value, "Application") == 0)
                            application = true;
                        break;
                    }

                    case Name:
                        strcpy(name, value);
                    break;
                    case GenericName:
                    break;
                    case Comment:
                    break;
                    case Icon:
                    {
                        icon_exists = true;
                        strcpy(icon, value);
                        break;
                    }

                    case Hidden:
                    break;
                    case TryExec:
                    break;
                    case Exec:
                        strcpy(exec, ParseExec(value));
                    break;
                    case Path:
                    break;
                    case Terminal:
                        terminal_required = StringToBool(value);
                    break;
                    case MimeType:
                    break;
                    case Categories:
                    {
                        // This is needed since strtok_r is destructive
                        char temp[64];
                        strcpy(temp, value);
                        main_category = ParseCategories(temp);
                        extra_category = ParseAdditionalCategories(value);
                        break;
                    }

                    case URL:
                    break;

                    default:
                        printf("Invalid or ingored category \"%s\" contains \"%s\"\n", xdg_keys[i].key, value);
                    break;
                }
            }
        }
    }

    fclose(fp);
    free(line);

    if (!icon_exists)
        strcpy(icon, "\0");
    // Create a empty category
    //if (!category_exists)
        //main_category = CreateCategory("", -1);
    if (application)
        return CreateEntry(main_category, extra_category, name, exec, icon, terminal_required);

    return NULL;
}

static int LoadDesktopEntries(XDGDesktopEntries *entries)
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
                EntriesAdd(entries, entry);
            }
            else
            {
                printf("Failed to parse %s\n", buffer);
            }

            //printf("%s\n", dirp->d_name);
        }
    }

    return closedir(dentry_dir);
}

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

static void WriteJWMTray(JWMTray *tray, XDGDesktopEntries *entries, FILE *fp)
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

static void GenJWMTray(JWMTray *tray, XDGDesktopEntries *entries)
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

static void WriteJWMRootMenuCategoryList(XDGDesktopEntries *entries, FILE *fp, const char *category, const char *overide_category_name)
{
    const char *menu_name = category;
    if (overide_category_name != NULL)
        menu_name = overide_category_name;

    fprintf(fp, "       <Menu icon=\"%s\" label=\"%s\">\n", category, menu_name);

    for (size_t i = 0; i < entries->size; i++)
    {
        XDGDesktopEntry *entry = entries->data[i];
        if (strcmp(entry->category->key, category) == 0)
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

static void GenJWMRootMenu(JWMRootMenu *root_menu, XDGDesktopEntries *entries)
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

    WriteJWMRootMenuCategoryList(entries, fp, "Network", "Internet");
    WriteJWMRootMenuCategoryList(entries, fp, "AudioVideo", "Multimedia");
    WriteJWMRootMenuCategoryList(entries, fp, "Development", NULL);
    WriteJWMRootMenuCategoryList(entries, fp, "Office", NULL);
    WriteJWMRootMenuCategoryList(entries, fp, "Graphics", NULL);
    //WriteJWMRootMenuCategoryList(entries, fp, "Video", NULL);
    WriteJWMRootMenuCategoryList(entries, fp, "Settings", NULL);
    WriteJWMRootMenuCategoryList(entries, fp, "System", NULL);
    WriteJWMRootMenuCategoryList(entries, fp, "Utility", NULL);

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

    XDGDesktopEntries *entries = EntriesCreateArray(100);
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
        printf("Search found:\n%s\n%s\n%s\n%s\n", entry->name, entry->category->key, entry->exec, entry->icon);

    free(root_menu);
    free(tray);

    EntriesDestroy(entries);
    cfg_free(cfg);

    return 0;
}
