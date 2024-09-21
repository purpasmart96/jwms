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
                XDGIconDir *icon = IconCreate(section, type, context, size, min_size, max_size, scale, threshold);
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

XDGIconDir *IconCreate(const char *path, IconType type, IconContext context, int size, int min_size, int max_size, int scale, int threshold)
{
    XDGIconDir *icon_dir = malloc(sizeof(*icon_dir));
    icon_dir->path = strdup(path);
    icon_dir->size = size;
    icon_dir->scale = scale;
    icon_dir->context = context;
    icon_dir->type = type;
    icon_dir->max_size = max_size;
    icon_dir->min_size = min_size;
    icon_dir->threshold = threshold;
    icon_dir->icons = HashMapCreate();

    return icon_dir;
}

void IconDestroy(void *icon_dir_ptr)
{
    XDGIconDir *icon_dir = icon_dir_ptr;
    HashMapDestroy(icon_dir->icons);
    free(icon_dir->path);
    free(icon_dir);
}

void IconPrint(void *icon_dir_ptr)
{
    XDGIconDir *icon_dir = icon_dir_ptr;
    printf("%s\n",icon_dir->path);
}

static int IconDirCmp(const void *a, const void *b)
{
    const XDGIconDir *icon_a = *(XDGIconDir**)a;
    const XDGIconDir *icon_b = *(XDGIconDir**)b;

    return strcasecmp(icon_a->path, icon_b->path);
}

static int IconDirCmp2(const void *a, const void *b)
{
    const XDGIconDir *icon_a = a;
    const char *icon_path = b;

    return strcasecmp(icon_a->path, icon_path);
}

static void IndexIcons(IconTheme *theme, const char *base_dir)
{
    // Iterate over all directories and icon files
    for (size_t i = 0; i < theme->icon_dirs->size; i++)
    {
        XDGIconDir *current_icon_dir = theme->icon_dirs->data[i];
        char directory_path[512];
        snprintf(directory_path, sizeof(directory_path), "%s/%s/%s", base_dir, theme->name, current_icon_dir->path);

        // Open the directory
        DIR *dir = opendir(directory_path);
        if (dir == NULL)
        {
            continue; // Skip if the directory cannot be opened
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            // Check if the entry is a regular file and has a valid extension
            //if (entry->d_type == DT_REG)
            char *ext = strrchr(entry->d_name, '.');
            if (ext)
            {
                // Check file extension
                const char *icon_exts[] = {".png", ".svg", ".xpm"};
                const int num_exts = 3;
                bool valid_ext = false;

                for (int j = 0; j < num_exts; j++)
                {
                    if ((strcmp(ext, icon_exts[j]) == 0))
                    {
                        valid_ext = true;
                        break;
                    }
                }

                if (valid_ext)
                {
                    // Create full path for the icon file
                    char full_path[768];
                    char name[128];
                    strcpy(name, entry->d_name);
                    char *pos = strrchr(name, '.');
                    *pos = '\0';
                    snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);

                    // Index the file in the hash map
                    //HashMapInsert(theme->icon_index, name, full_path);
                    HashMapInsert(current_icon_dir->icons, name, full_path);
                    theme->num_icons++;
                }
            }
        }

        closedir(dir);
    }
}

IconTheme *LoadIconTheme(const char *theme_name)
{
    IconTheme *theme = malloc(sizeof(*theme));
    theme->name = strdup(theme_name);
    theme->icon_dirs = DArrayCreate(64, sizeof(XDGIconDir*));
    //theme->icon_index = HashMapCreate();
    theme->num_icons = 0;
    ParseThemeIcons(theme->icon_dirs, theme_name);
    IndexIcons(theme, "/usr/share/icons");
    DArraySort(theme->icon_dirs, IconDirCmp);

    //printf("\n");
    //DArrayPrint(theme->icons, IconPrint);
    //printf("\n");
    return theme;
}

void UnLoadIconTheme(IconTheme *icon_theme)
{
    //HashMapDestroy(icon_theme->icon_index);
    DArrayDestroy(icon_theme->icon_dirs, IconDestroy);
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

int GetCurrentGTKIconThemeName(char theme_name[])
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
        return -1;
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
                    strcpy(theme_name, value);
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

    return 0;
}

static bool DirectoryMatchesSize(XDGIconDir *icon, int icon_size, int icon_scale)
{
    if (icon->scale != icon_scale)
    {
        return false;
    }

    switch (icon->type)
    {
        case Fixed:
            return icon->size == icon_size;
        case Scaled:
            return icon->min_size <= icon_size && icon_size <= icon->max_size;
        case Threshold:
            return (icon->size - icon->threshold) <= icon_size && icon_size <= (icon->size + icon->threshold);
        default:
            return false;
    }
}

// Function to calculate the Directory Size Distance
static int DirectorySizeDistance(XDGIconDir *icon, int icon_size, int icon_scale)
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

    char closest_icon_path[512]; 
    int min_size = INT_MAX;

    for (size_t i = 0; i < theme->icon_dirs->size; i++)
    {
        XDGIconDir *curr_icon_dir = theme->icon_dirs->data[i];
    
        // First check the hash map for the icon
        const char *icon_path = HashMapGet(curr_icon_dir->icons, icon_name);
        if (icon_path != NULL)
        {
            if (DirectoryMatchesSize(curr_icon_dir, size, scale))
            {
                return strdup(icon_path); // Exact match
            }

            // Calculate the size difference to find the closest match
            int size_delta = DirectorySizeDistance(curr_icon_dir, size, scale);
            if (size_delta < min_size)
            {
                min_size = size_delta;
                strlcpy(closest_icon_path, icon_path, sizeof(closest_icon_path));
            }
        }
    }

    // Return the closest icon path found
    if (min_size != INT_MAX)
        return strdup(closest_icon_path);
    return NULL;
}

char *LookupIconOld(IconTheme *theme, const char *icon_name, int size, int scale)
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

    for (size_t i = 0; i < theme->icon_dirs->size; i++)
    {
        XDGIconDir *current_icon = theme->icon_dirs->data[i];

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
    //char *theme = GetCurrentGTKIconThemeName();
    char theme[256] = "\0";
    int found = GetCurrentGTKIconThemeName(theme);
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
    //free(theme);
    return NULL;

success:
    //free(theme);
    return filename;
}

HashMap *FindAllIcons(List *icons, int size, int scale)
{
    char theme[256] = "\0";
    int found = GetCurrentGTKIconThemeName(theme);
    if (found != 0 || theme[0] == '\0')
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

    //free(theme);
    UnLoadIconTheme(icon_theme);
    UnLoadIconTheme(default_theme);
    //return LookupFallbackIcon(icon);
    return valid_icons;
}

static void SearchAndStoreIconHelper(HashMap *icons, IconTheme *icon_theme, IconTheme *default_theme, const char *icon, int size, int scale)
{
    char *filename = LookupIcon(icon_theme, icon, size, scale);
    if (filename != NULL)
    {
        HashMapInsert(icons, icon, filename);
    }
    else
    {
        filename = LookupIcon(default_theme, icon, size, scale);
        if (filename != NULL)
        {
            HashMapInsert(icons, icon, filename);
        }
    }

    free(filename);
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

    SearchAndStoreIconHelper(args->valid_icons, args->icon_theme, args->default_theme, icon, args->size, args->scale);
}

HashMap *FindAllIcons2(BTreeNode *entries, int size, int scale)
{
    char theme[256] = "\0";
    int found = GetCurrentGTKIconThemeName(theme);
    //char *theme = GetCurrentGTKIconThemeName();
    if (found != 0 || theme[0] == '\0')
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

    // Desktop entry icons
    BSTInOrderTraverse(entries, SearchAndStoreIcon, &args);

    // Extra icons, should put this in a for loop
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-multimedia", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-development", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-education", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-games", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-graphics", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-internet", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-office", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-science", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "preferences-desktop", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-system", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "applications-utilities", size, scale);

    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "system-search", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "view-refresh", size, scale);
    SearchAndStoreIconHelper(valid_icons, icon_theme, default_theme, "system-log-out", size, scale);

    DEBUG_LOG("\nStored %d icons in %s theme\n", icon_theme->num_icons, icon_theme->name);
    DEBUG_LOG("Stored %d icons in %s theme\n\n", default_theme->num_icons, default_theme->name);

    UnLoadIconTheme(icon_theme);
    UnLoadIconTheme(default_theme);
    //return LookupFallbackIcon(icon);
    return valid_icons;
}
