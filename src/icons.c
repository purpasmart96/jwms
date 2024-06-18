#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>

#include <bsd/string.h>

#include "bstree.h"
#include "hashing.h"
#include "darray.h"
#include "common.h"
#include "list.h"
#include "desktop_entries.h"
#include "icons.h"


static const Pair icon_types[] =
{
    {"Fixed",      Fixed      },
    {"Scaled",     Scaled     },
    {"Threshold",  Threshold  },
    {"Fallback",   Fallback   },
};

static const Pair icon_contexts[] =
{
    {"Actions",     ActionsContext     },
    {"Devices",     DevicesContext     },
    {"FileSystems", FileSystemsContext },
    {"MimeTypes",   MimeTypesContext   },
};

static size_t GetIconType(char *key)
{
    for (size_t i = 0; i < ARRAY_SIZE(icon_types); i++)
    {
        if (strcmp(icon_types[i].key, key) == 0)
            return i;
    }

    return -1;
}

static size_t GetIconContext(char *key)
{
    for (size_t i = 0; i < ARRAY_SIZE(icon_contexts); i++)
    {
        if (strcmp(icon_contexts[i].key, key) == 0)
            return i;
    }

    return -1;
}

void ParseSection(FILE *file, const char *section_name, HashMap *map)
{
    char line[512];
    char section[128];
    char key[128];
    char value[128];
    while (fgets(line, sizeof(line), file))
    {
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
        {
            continue;
        }

        // Remove newline character
        line[strcspn(line, "\n")] = 0;

        // Check if line contains a section header
        if (line[0] == '[' && line[strlen(line) - 1] == ']')
        {
            // Extract section name
            sscanf(line, "[%127[^]]", section);
            continue;
        }

        // Parse key-value pairs in the section
        if (strcmp(section, section_name) == 0)
        {
            if (sscanf(line, "%127[^=]=%s", key, value) == 2)
            {
                HashMapInsertWithSection(map, section_name, key, value);
            }
            else
            {
                break;
            }
        }
    }
}

void ParseSections(FILE *file, HashMap *map)
{
    char line[256];
    char section[128];
    char key[128];
    char value[128];
    bool in_section = false;
    while (fgets(line, sizeof(line), file))
    {
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
        {
            continue;
        }

        // Remove newline character
        line[strcspn(line, "\n")] = 0;

        // Check if line contains a section header
        if (line[0] == '[' && line[strlen(line) - 1] == ']')
        {
            // Extract section name
            sscanf(line, "[%127[^]]", section);
            in_section = true;
        }

        // Parse key-value pairs in the section
        if (in_section)
        {
            if (sscanf(line, "%127[^=]=%s", key, value) == 2)
            {
                // Should also insert the section name into a dynamic array and store for later...
                HashMapInsertWithSection(map, section, key, value);
            }
        }
    }
}

static int ParseInt(const char *str)
{
    char *end;
    int result = (int)strtol(str, &end, 10);
    if (end > str)
        return result;

    return -1;
}

static void ParseThemeIcons(DArray *icons, const char *theme)
{

    char path[512];
    const char *base = "/usr/share/icons/";

    strlcpy(path, base, sizeof(path));
    strlcat(path, theme, sizeof(path));
    strlcat(path, "/index.theme", sizeof(path));

    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error opening index.theme file in directory %s\n", path);
        return;
    }

    char line[256];
    char section[128];
    char key[128];
    char value[128];
    bool in_section = false;

    int scale = 1;
    int size = 0;
    int min_size = -1;
    int max_size = -1;
    int threshold = 2;
    IconContext context = -1;
    IconType type = Threshold;

    while (fgets(line, sizeof(line), fp))
    {
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
        {
            continue;
        }

        // Remove newline character
        line[strcspn(line, "\n")] = 0;

        // Check if line contains a section header
        if (line[0] == '[' && line[strlen(line) - 1] == ']')
        {
            sscanf(line, "[%127[^]]", section);
            if (strcmp(section, "Icon Theme") != 0)
            {
                //ListAdd(dirs, section, strlen(section) + 1);
                in_section = true;
                continue;
            }
        }

        // Parse key-value pairs in the section
        if (in_section)
        {
            if (sscanf(line, "%127[^=]=%127s", key, value) == 2)
            {
                if (strcmp(key, "Size") == 0)
                {
                    size = ParseInt(value);
                    if (min_size == -1)  // fallback value
                        min_size = size;
                    if (max_size == -1)  // fallback value
                        max_size = size;
                }
                else if (strcmp(key, "Scale") == 0)
                {
                    scale = ParseInt(value);
                }
                else if (strcmp(key, "Context") == 0)
                {
                    int icontext = GetIconContext(value);
                    if (icontext != -1)
                    {
                        context = (IconContext)icontext;
                    }
                }
                else if (strcmp(key, "Type") == 0)
                {
                    int itype = GetIconType(value);
                    if (itype != -1)
                    {
                        type = (IconType)itype;
                    }
                }
                else if (strcmp(key, "MaxSize") == 0)
                {
                    max_size = ParseInt(value);
                }
                else if (strcmp(key, "MinSize") == 0)
                {
                    min_size = ParseInt(value);
                }
                else if (strcmp(key, "Threshold") == 0)
                {
                    threshold = ParseInt(value);
                }
            }
            else
            {
                XDGIcon *icon = IconCreate(section, type, context, size, min_size, max_size, scale, threshold);
                if (icon != NULL)
                {
                    DArrayAdd(icons, icon);
                    //ListAdd(icon_paths, section, strlen(section) + 1);
                    //HashMapInsert2(icons, section, icon);
                }
                scale = 1;
                size = 0;
                min_size = -1;
                max_size = -1;
                threshold = 2;
                context = -1;
                type = Threshold;
            }
        }
    }
    fclose(fp);
}

XDGIcon *IconCreate(const char *path, IconType type, IconContext context, int size, int min_size, int max_size, int scale, int threshold)
{
    XDGIcon *icon_dir = malloc(sizeof(*icon_dir));
    icon_dir->path = strdup(path);
    icon_dir->size = size;
    icon_dir->scale = scale;
    icon_dir->context = context;
    icon_dir->type = type;
    icon_dir->max_size = max_size;
    icon_dir->min_size = min_size;
    icon_dir->threshold = threshold;

    return icon_dir;
}

void IconDestroy(void *icon_dir_ptr)
{
    XDGIcon *icon_dir = icon_dir_ptr;
    free(icon_dir->path);
    free(icon_dir);
}

void IconPrint(void *icon_dir_ptr)
{
    XDGIcon *icon_dir = icon_dir_ptr;
    printf("%s\n",icon_dir->path);
}

static int IconDirCmp(const void *a, const void *b)
{
    const XDGIcon *icon_a = *(XDGIcon**)a;
    const XDGIcon *icon_b = *(XDGIcon**)b;

    return strcasecmp(icon_a->path, icon_b->path);
}

IconTheme *LoadIconTheme(const char *theme_name)
{
    IconTheme *theme = malloc(sizeof(*theme));
    //theme->paths = ListCreate();
    theme->name = strdup(theme_name);
    theme->icons = DArrayCreate(64, sizeof(XDGIcon*));
    //theme->icons2 = HashMapCreate2(sizeof(XDGIcon*), IconDestroy, IconPrint);
    ParseThemeIcons(theme->icons, theme_name);
    // Sort not really needed
    //DArraySort(theme->icons, IconDirCmp);
    //printf("\n");
    //DArrayPrint(theme->icons, IconPrint);
    //printf("\n");
    return theme;
}

void UnLoadIconTheme(IconTheme *icon_theme)
{
    //ListDestroy(icon_theme->paths);
    //HashMapDestroy2(icon_theme->icons2);
    DArrayDestroy(icon_theme->icons, IconDestroy);
    //IconsDestroy(icon_theme->icons);
    free(icon_theme->name);
    free(icon_theme);
}

// This could be a general utility function for all forms of parsing ini like key value pairs
static int ParseGtkThemeValue(const char *line, char *value)
{
    char *equalsym = strchr(line, '=');

    if (equalsym == NULL)
        return -1;

    // Start of the value after '='
    char *start = equalsym + 1;

    // skip whitespace
    while (*start == ' ')
        start++;

    if (*start == '"')
    {
        // Skip opening quote
        start++;
        // Find end of quoted value
        size_t len = strcspn(start, "\"\n");
        strlcpy(value, start, len + 1);
    }
    else
    {
        // Find end of quoted value
        size_t len = strcspn(start, "\n");
        strlcpy(value, start, len + 1);
    }

    return 0;
}

char *GetCurrentGTKIconThemeName()
{
    char path[512];

    const char *home = getenv("HOME");
    const char *fname = ".gtkrc-2.0";

    strlcpy(path, home, sizeof(path));
    strlcat(path, "/", sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return NULL;
    }

    int read = 0;
    size_t len = 0;
    char *line = NULL;
    char *icon_theme = NULL;

    // Read line by line
    while ((read = getline(&line, &len, fp)) != -1)
    {
        // Skip blank lines and comments
        if (line[0] == '\n' || line[0] == '#')
            continue;

        char key[256];
        char value[256];

        // Use sscanf to parse the line
        if (sscanf(line, " %255[^= ]", key) == 1)
        {
            if (ParseGtkThemeValue(line, value) != -1)
            {
                if (strcmp(key, "gtk-icon-theme-name") == 0)
                {
                    //DEBUG_LOG("Icon Theme: %s\n", value);
                    icon_theme = strdup(value);
                    break;
                }
            }
            else
            {
                DEBUG_LOG("Failed to parse key value pair: %s:%s\n", key, value);
            }
        }
    }

    free(line);
    fclose(fp);

    return icon_theme;
}

static bool DirectoryMatchesSize(XDGIcon *icon, int icon_size, int icon_scale)
{
    if (icon->scale != icon_scale)
    {
        return false;
    }
    else if (icon->type == Fixed)
    {
        return icon->size == icon_size;
    }
    else if (icon->type == Scaled)
    {
        return (icon->min_size <= icon_size && icon_size <= icon->max_size);
    }
    else if (icon->type == Threshold)
    {
        return ((icon->size - icon->threshold) <= icon_size && icon_size <= (icon->size + icon->threshold));
    }

    return false;
}

// Function to calculate the Directory Size Distance
static int DirectorySizeDistance(XDGIcon *icon, int icon_size, int icon_scale)
{
    if (icon->type == Fixed)
    {
        return abs(icon->size * icon->scale - icon_size * icon_scale);
    }
    else if (icon->type == Scaled)
    {
        if (icon_size * icon_scale < icon->min_size * icon->scale)
        {
            return icon->min_size * icon->scale - icon_size * icon_scale;
        }

        if (icon_size * icon_scale > icon->max_size * icon->scale)
        {
            return icon_size * icon_scale - icon->max_size * icon->scale;
        }
    }
    else if (icon->type == Threshold)
    {
        if (icon_size * icon_scale < (icon->size - icon->threshold) * icon->scale)
        {
            return icon->min_size * icon->scale - icon_size * icon_scale;
        }

        if (icon_size * icon_size > (icon->size + icon->threshold) * icon->scale)
        {
            return icon_size * icon_size - icon->max_size * icon->scale;
        }
    }

    return 0;
}

char *LookupIcon(IconTheme *theme, const char *icon_name, int size, int scale)
{
    if (access(icon_name, F_OK) == 0)
        return strdup(icon_name);

    char icon_path[512];
    char closest_icon_path[512];
    const char *base_dir = "/usr/share/icons/";

    char theme_dir[128];
    strlcpy(theme_dir, base_dir, sizeof(theme_dir));
    strlcat(theme_dir, theme->name, sizeof(theme_dir));

    const char *icon_exts[] = {".png", ".svg", ".xpm"};
    const int num_exts = 3;

    int min_size = INT_MAX;

    for (size_t i = 0; i < theme->icons->size; i++)
    {
        XDGIcon *current_icon = theme->icons->data[i];

        for (int j = 0; j < num_exts; j++)
        {
            // Create the full path.
            // Seems like one big snprintf call is faster then mutliple strlcpy and strlcat calls
            snprintf(icon_path, sizeof(icon_path), "%s/%s/%s%s", theme_dir, current_icon->path, icon_name, icon_exts[j]);

            if (access(icon_path, F_OK) == 0)
            {
                if (DirectoryMatchesSize(current_icon, size, scale))
                {
                    return strdup(icon_path);
                }

                int size_delta = DirectorySizeDistance(current_icon, size, scale);
                if (size_delta < min_size)
                {
                    min_size = size_delta;
                    strlcpy(closest_icon_path, icon_path, sizeof(closest_icon_path));
                }
            }
        }
    }

    // Return the closest icon path found
    if (min_size != INT_MAX)
        return strdup(closest_icon_path);

    // Icon not found, return NULL
    return NULL;
}

char *LookupFallbackIcon(const char *icon)
{
    /*
    for each directory in $(basename list)
    {
        for extension in ("png", "svg", "xpm")
        {
            if exists directory/iconname.extension
                return directory/iconname.extension
        }
    }
    */
    return NULL;
}

char *FindIconHelper(const char *icon, int size, int scale, const char *theme)
{
    IconTheme *icon_theme = LoadIconTheme(theme);

    if (icon_theme == NULL)
        return  NULL;

    char *filename = LookupIcon(icon_theme, icon, size, scale);
    if (filename != NULL)
    {
        UnLoadIconTheme(icon_theme);
        return filename;
    }

    /*
    if theme has parents
        parents = theme.parents

    for (parent in parents)
    {
        filename = FindIconHelper (icon, size, scale, parent)
        if filename != none
            return filename
    }
    */

    UnLoadIconTheme(icon_theme);
    return NULL;
}

char *FindIcon(const char *icon, int size, int scale)
{
    char *theme = GetCurrentGTKIconThemeName();
    char *filename = FindIconHelper(icon, size, scale, theme);
    if (filename != NULL)
    {
        goto success;
    }

    filename = FindIconHelper(icon, size, scale, "hicolor");
    if (filename != NULL)
    {
        goto success;
    }

    //return LookupFallbackIcon(icon);
    free(theme);
    return NULL;

success:
    free(theme);
    return filename;
}

HashMap *FindAllIcons(List *icons, int size, int scale)
{
    char *theme = GetCurrentGTKIconThemeName();
    if (theme == NULL)
    {
        printf("Failed to get GTK icon theme name!\n");
        return NULL;
    }

    IconTheme *icon_theme = LoadIconTheme(theme);
    IconTheme *default_theme = LoadIconTheme("hicolor");
    HashMap *valid_icons = HashMapCreate();
    Node *current = icons->head;
    while (current != NULL)
    {
        const char *icon = (char*)current->data;
        char *filename = LookupIcon(icon_theme, icon, size, scale);
        if (filename != NULL)
        {
            HashMapInsert(valid_icons, icon, filename);
            //ListAdd(valid_icons, filename, strlen(filename) + 1);
        }
        else
        {
            filename = LookupIcon(default_theme, icon, size, scale);
            if (filename != NULL)
            {
                HashMapInsert(valid_icons, icon, filename);
                //ListAdd(valid_icons, filename, strlen(filename) + 1);
            }
        }
        free(filename);
        current = current->next;
    }

    free(theme);
    UnLoadIconTheme(icon_theme);
    UnLoadIconTheme(default_theme);
    //return LookupFallbackIcon(icon);
    return valid_icons;
}

typedef struct
{
    IconTheme *icon_theme;
    IconTheme *default_theme;
    HashMap *valid_icons;
    int size;
    int scale;
} Args;

static void SearchAndStoreIcon(void *entry_ptr, void *args_ptr)
{
    XDGDesktopEntry *entry = entry_ptr;
    Args *args = args_ptr;
    const char *icon = entry->icon;
    
    char *filename = LookupIcon(args->icon_theme, icon, args->size, args->scale);
    if (filename != NULL)
    {
        HashMapInsert(args->valid_icons, icon, filename);
    }
    else
    {
        filename = LookupIcon(args->default_theme, icon, args->size, args->scale);
        if (filename != NULL)
        {
            HashMapInsert(args->valid_icons, icon, filename);
        }
    }

    free(filename);
}

HashMap *FindAllIcons2(BTreeNode *entries, int size, int scale)
{
    char *theme = GetCurrentGTKIconThemeName();
    if (theme == NULL)
    {
        printf("Failed to get GTK icon theme name!\n");
        return NULL;
    }

    IconTheme *icon_theme = LoadIconTheme(theme);
    if (icon_theme == NULL)
    {
        printf("Failed to load current GTK icon theme!\n");
        return NULL;
    }

    IconTheme *default_theme = LoadIconTheme("hicolor");
    if (default_theme == NULL)
    {
        printf("Failed to load default GTK icon theme!\n");
        return NULL;
    }

    HashMap *valid_icons = HashMapCreate();

    Args args =
    {
        .icon_theme = icon_theme,
        .default_theme = default_theme,
        .valid_icons = valid_icons,
        .size = size,
        .scale = scale        
    };

    BSTInOrderTraverse(entries, SearchAndStoreIcon, &args);

    free(theme);
    UnLoadIconTheme(icon_theme);
    UnLoadIconTheme(default_theme);
    //return LookupFallbackIcon(icon);
    return valid_icons;
}
