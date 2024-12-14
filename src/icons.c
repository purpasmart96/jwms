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

// If enabled, all nested children icon themes get searched
// If not it will only search two themes, the parent theme, and the default "hicolor" theme
#define SEARCH_INHERITED_ICON_THEMES

// Max files to index per icon theme sub directory.
// If you have a really slow hard drive you may want to lower this
#define MAX_FILES_TO_INDEX_PER_DIR 250

#define MULTIPHASE_ICON_SEARCH
//#define HYBRID_ICON_SEARCH

static HashMap2 *themes_map = NULL;
// Keys for fast searching of themes_map
static DArray *themes_names = NULL;

static const char *extra_icons[] =
{
    "applications-multimedia",
    "applications-development",
    "applications-education",
    "applications-games",
    "applications-graphics",
    "applications-internet",
    "applications-office", 
    "applications-science",
    "applications-system", 
    "applications-utilities",

    "applications-multimedia-symbolic",
    "applications-development-symbolic",
    "applications-education-symbolic",
    "applications-games-symbolic",
    "applications-graphics-symbolic",
    "applications-internet-symbolic",
    "applications-office-symbolic", 
    "applications-science-symbolic",
    "applications-system-symbolic", 
    "applications-utilities-symbolic",

    "settings-configure-symbolic",
    "preferences-desktop", 
    "system-search",
    "view-refresh",
    "system-log-out",
    "system-shutdown",
    "system-reboot"
};

static const Pair common_icon_sizes[] =
{
    {"48",  48  },
    {"32",  32  },
    {"24",  24  },
    {"22",  22  },
    {"64",  64  },
    {"16",  16  },
    {"96",  96  },
    {"128", 128 },
    {"256", 256 }
};

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

static void ParseInheritedThemes(IconTheme *theme, const char *inherits)
{
#ifndef SEARCH_INHERITED_ICON_THEMES
    return;
#endif

    char *str_copy = strdup(inherits);

    char *reserved;
    char *theme_name = strtok_r(str_copy, ",", &reserved);
    while (theme_name != NULL)
    {
        DEBUG_LOG("Found inherited theme: %s\n", theme_name);
        DArrayAdd(theme->parents, strdup(theme_name));
        theme_name = strtok_r(NULL, ",", &reserved);
    }
    free(str_copy);
}

static void ParseThemeIcons(IconTheme *theme)
{
    char path[512];
    const char *base = "/usr/share/icons/";

    strlcpy(path, base, sizeof(path));
    strlcat(path, theme->name, sizeof(path));
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
            //if (strcmp(section, "Icon Theme") != 0)
            //{
                in_section = true;
                continue;
            //}
        }

        // Parse key-value pairs in the section
        if (in_section)
        {
            if (sscanf(line, "%127[^=]=%127s", key, value) == 2)
            {
                if (strcmp(key, "Inherits") == 0)
                {
                    ParseInheritedThemes(theme, value);
                }
                else if (strcmp(key, "Size") == 0)
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
                    DArrayAdd(theme->icon_dirs, icon);
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
    icon_dir->index_state = NotIndexed;

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

static int IconDirCmpSubStr(const void *a, const void *b)
{
    const XDGIconDir *icon_a = a;
    const char *icon_path = b;

    return strstr(icon_a->path, icon_path) != NULL;
}

static void IndexSingleIconDir(XDGIconDir *icon_dir, const char *theme_path)
{
    char directory_path[512];
    snprintf(directory_path, sizeof(directory_path), "%s/%s", theme_path, icon_dir->path);

    DIR *dir = opendir(directory_path);
    if (dir == NULL)
    {
        return; // Skip if the directory cannot be opened
    }

    struct dirent *entry;
    size_t file_count = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        // Stop indexing if the limit is reached
        if (file_count > MAX_FILES_TO_INDEX_PER_DIR)
        {
            DEBUG_LOG("Max files reached per directory! Finishing %s early!\n", directory_path);
            icon_dir->index_state = PartiallyIndexed;
            break;
        }

        char *ext = strrchr(entry->d_name, '.');
        if (ext)
        {
            bool valid_ext = false;
            switch (ext[1])
            {
                case 'p':
                    valid_ext = true; //strcmp(ext, ".png") == 0;
                    break;
                case 's':
                    valid_ext = true; //strcmp(ext, ".svg") == 0;
                    break;
                case 'x':
                    valid_ext = true; //strcmp(ext, ".xpm") == 0;
                    break;
            }

            if (valid_ext)
            {
                char full_path[768];
                char name[128];
                strlcpy(name, entry->d_name, sizeof(name));
                snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, name);
                char *pos = strrchr(name, '.');
                *pos = '\0';
                HashMapInsert(icon_dir->icons, name, full_path);
                file_count++;
            }
        }
    }

    closedir(dir);

    // Mark the directory as indexed
    if (icon_dir->index_state == NotIndexed)
        icon_dir->index_state = FullyIndexed;
}

IconTheme *LoadIconTheme(const char *theme_name)
{
    IconTheme *theme = malloc(sizeof(*theme));
    theme->name = strdup(theme_name);
    theme->icon_dirs = DArrayCreate(64, (void*)IconDestroy, NULL, IconDirCmp);
    theme->parents = DArrayCreate(8, free, NULL, NULL);

    ParseThemeIcons(theme);
    DArraySort(theme->icon_dirs);

    //printf("\n");
    //DArrayPrint(theme->icons, IconPrint);
    //printf("\n");
    return theme;
}

void UnLoadIconTheme(IconTheme *icon_theme)
{
    //HashMapDestroy(icon_theme->icon_index);
    //if (icon_theme->parents != NULL)
    DArrayDestroy(icon_theme->parents);
    DArrayDestroy(icon_theme->icon_dirs);

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

// TODO: This is not very good, should parse the gtk3 icon theme instead since that's more common now
// Should also redo the parsing, it's not very good and throws warnings on older versions of gcc
int GetCurrentGTKIconThemeName(char *theme_name)
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

static bool LookupIconBackup(XDGIconDir *icon_dir, const char *icon_name, const char *theme_path)
{
    char icon_path[512];
    const char *icon_exts[] = {".png", ".svg", ".xpm"};
    const int num_exts = 3;
    for (int i = 0; i < num_exts; i++)
    {
        // Create the full path.
        // Seems like one big snprintf call is faster then mutliple strlcpy and strlcat calls
        snprintf(icon_path, sizeof(icon_path), "%s/%s/%s%s", theme_path, icon_dir->path, icon_name, icon_exts[i]);

        if (access(icon_path, F_OK) == 0)
        {
            HashMapInsert(icon_dir->icons, icon_name, icon_path);
            return true;
        }
    }

    return false;
}

static char *LookupIconExactSize(IconTheme *theme, const char *icon_name, int size, int scale)
{
    // Icon size substr
    char icon_size[4];
    snprintf(icon_size, sizeof(icon_size), "%d", size);

    size_t found = 0;

    int index_array[theme->icon_dirs->size];

    for (size_t i = 0; i < theme->icon_dirs->size; i++)
    {
        if (IconDirCmpSubStr(theme->icon_dirs->data[i], icon_size))
        {
            index_array[found++] = i;
        }
    }

    // No valid subdirs were found
    if (!found)
    {
        return NULL;
    }

    // Build partial path
    const char *base_dir = "/usr/share/icons";
    char theme_dir[256];
    snprintf(theme_dir, sizeof(theme_dir), "%s/%s", base_dir, theme->name);

    // Now look for the correct icon in those valid subdirs 
    for (size_t i = 0; i < found; i++)
    {
        int curr_index = index_array[i];
        XDGIconDir *curr_icon_dir = theme->icon_dirs->data[curr_index];
        // Check if the directory is indexed, if not, try to index it
        if (curr_icon_dir->index_state == NotIndexed)
        {
            IndexSingleIconDir(curr_icon_dir, theme_dir);
        }
        // If partially indexed, fallback to using the access syscall
        if (curr_icon_dir->index_state == PartiallyIndexed)
        {
            // If valid, add icon to the hashmap
            if (!LookupIconBackup(curr_icon_dir, icon_name, theme_dir))
                continue;
        }
        // Now, search for the icon in the indexed hash map
        const char *icon_path = HashMapGet(curr_icon_dir->icons, icon_name);
        if (icon_path == NULL)
            continue;
        if (DirectoryMatchesSize(curr_icon_dir, size, scale))
        {
            return strdup(icon_path); // Exact match
        }
    }

    return NULL;
}

static char *LookupIconScaled(IconTheme *theme, const char *icon_name)
{
    size_t found = 0;
    int index_array[theme->icon_dirs->size];

    for (size_t i = 0; i < theme->icon_dirs->size; i++)
    {
        if (IconDirCmpSubStr(theme->icon_dirs->data[i], "scalable"))
        {
            index_array[found++] = i;
        }
    }

    // No valid subdirs were found
    if (!found)
    {
        return NULL;
    }

    // Build partial path
    const char *base_dir = "/usr/share/icons";
    char theme_dir[256];
    snprintf(theme_dir, sizeof(theme_dir), "%s/%s", base_dir, theme->name);

    // Now look for the correct icon in those valid subdirs 
    for (size_t i = 0; i < found; i++)
    {
        int curr_index = index_array[i];
        XDGIconDir *curr_icon_dir = theme->icon_dirs->data[curr_index];
        // Check if the directory is indexed, if not, index it
        if (curr_icon_dir->index_state == NotIndexed)
        {
            IndexSingleIconDir(curr_icon_dir, theme_dir);
        }
        // If partially indexed, fallback to using the access syscall
        if (curr_icon_dir->index_state == PartiallyIndexed)
        {
            // If valid, add icon to the hashmap
            if (!LookupIconBackup(curr_icon_dir, icon_name, theme_dir))
                continue;
        }
        // Now, search for the icon in the indexed hash map
        const char *icon_path = HashMapGet(curr_icon_dir->icons, icon_name);
        if (icon_path == NULL)
            continue;
        return strdup(icon_path); // Exact match
    }

    return NULL;
}

static char *LookupIconMultiPhase(IconTheme *theme, const char *icon_name, int size, int scale)
{
    if (access(icon_name, F_OK) == 0)
        return strdup(icon_name);

    char *found_icon = NULL;

    found_icon = LookupIconExactSize(theme, icon_name, size, scale);
    if (found_icon != NULL)
        return found_icon;

    if (strcmp(theme->name, "hicolor") == 0)
    {
        found_icon = LookupIconScaled(theme, icon_name);
        if (found_icon != NULL)
            return found_icon;
    }

    for (size_t i = 0; i < ARRAY_SIZE(common_icon_sizes); i++)
    {
        if (common_icon_sizes[i].value == size)
            continue;
    
        found_icon = LookupIconExactSize(theme, icon_name, common_icon_sizes[i].value, scale);
        if (found_icon != NULL)
            return found_icon;
    }

    return NULL;
}

static char *LookupIconHybrid(IconTheme *theme, const char *icon_name, int size, int scale)
{
    if (access(icon_name, F_OK) == 0)
        return strdup(icon_name);

    char *found_icon = LookupIconExactSize(theme, icon_name, size, scale);
    if (found_icon != NULL)
        return found_icon;

    if (strcmp(theme->name, "hicolor") == 0)
    {
        found_icon = LookupIconScaled(theme, icon_name);
        if (found_icon != NULL)
            return found_icon;
    }

    char closest_icon_path[512];
    int min_size = INT_MAX;
    char icon_path[512];
    const char *base_dir = "/usr/share/icons/";

    char theme_dir[128];
    strlcpy(theme_dir, base_dir, sizeof(theme_dir));
    strlcat(theme_dir, theme->name, sizeof(theme_dir));

    const char *icon_exts[] = {".png", ".svg", ".xpm"};
    const int num_exts = 3;

    for (size_t i = 0; i < theme->icon_dirs->size; i++)
    {
        XDGIconDir *curr_icon_dir = theme->icon_dirs->data[i];

        // Skip already searched paths
        if (curr_icon_dir->index_state == FullyIndexed)
        {
            continue;
        }
    
        for (int j = 0; j < num_exts; j++)
        {
            // Create the full path.
            // Seems like one big snprintf call is faster then mutliple strlcpy and strlcat calls
            snprintf(icon_path, sizeof(icon_path), "%s/%s/%s%s", theme_dir, curr_icon_dir->path, icon_name, icon_exts[j]);

            if (access(icon_path, F_OK) == 0)
            {
                int size_delta = DirectorySizeDistance(curr_icon_dir, size, scale);
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
    {
        return strdup(closest_icon_path);
    }

    return NULL;
}

static char *LookupIconLinear(IconTheme *theme, const char *icon_name, int size, int scale)
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

char *LookupIcon(IconTheme *theme, const char *icon_name, int size, int scale)
{
#ifdef HYBRID_ICON_SEARCH
    return LookupIconHybrid(theme, icon_name, size, scale);
#elif defined(MULTIPHASE_ICON_SEARCH)
    return LookupIconMultiPhase(theme, icon_name, size, scale);
#else
    return LookupIconLinear(theme, icon_name, size, scale);
#endif
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

static int SearchThemeNameCmp1(const void *a, const void *b)
{
    const IconTheme *theme = a;
    const char *theme_name = b;

    return strcmp(theme->name, theme_name) == 0;
}

static int SearchThemeNameCmp2(const void *a, const void *b)
{
    const char *theme_name_a = a;
    const char *theme_name_b = b;

    return strcmp(theme_name_a, theme_name_b) == 0;
}

int PreloadIconThemesOld(const char *theme)
{
    IconTheme *icon_theme = LoadIconTheme(theme);

    if (icon_theme == NULL)
        return -1;

    if (themes_map == NULL)
    {
        themes_map = HashMapCreate2((void*)UnLoadIconTheme, NULL);
        themes_names = DArrayCreate(8, free, SearchThemeNameCmp2, NULL);
    }

    HashMapInsert2(themes_map, theme, icon_theme);
    DArrayAdd(themes_names, strdup(theme));

    if (icon_theme->parents->size != 0)
    {
        for (int i = 0; i < icon_theme->parents->size; i++)
        {
            char *parent = icon_theme->parents->data[i];
            if (HashMapGet2(themes_map, parent) != NULL)
            {
                continue;
            }

            if (PreloadIconThemes(parent) != 0)
                continue;
        }
    }

    return 0;
}

static int LoadInheritedTheme(const char *theme)
{
    IconTheme *icon_theme = LoadIconTheme(theme);

    if (icon_theme == NULL)
        return -1;

    HashMapInsert2(themes_map, theme, icon_theme);
    DArrayAdd(themes_names, strdup(theme));

    if (icon_theme->parents->size != 0)
    {
        for (int i = 0; i < icon_theme->parents->size; i++)
        {
            char *parent = icon_theme->parents->data[i];
            if (HashMapGet2(themes_map, parent) != NULL)
            {
                continue;
            }

            if (LoadInheritedTheme(parent) != 0)
                continue;
        }
    }

    return 0;
}

int PreloadIconThemes(const char *theme)
{
    IconTheme *icon_theme = LoadIconTheme(theme);

    if (icon_theme == NULL)
        return -1;

    if (themes_map == NULL)
    {
        themes_map = HashMapCreate2((void*)UnLoadIconTheme, NULL);
        themes_names = DArrayCreate(8, free, SearchThemeNameCmp2, NULL);
    }

    HashMapInsert2(themes_map, theme, icon_theme);
    DArrayAdd(themes_names, strdup(theme));

    if (icon_theme->parents->size)
    {
        for (int i = 0; i < icon_theme->parents->size; i++)
        {
            char *parent = icon_theme->parents->data[i];
            if (HashMapGet2(themes_map, parent) != NULL)
                continue;
            if (LoadInheritedTheme(parent) != 0)
                continue;
        }
    }

    const char *default_theme_name = "hicolor";
    if (!DArrayContains(themes_names, default_theme_name))
    {
        IconTheme *default_theme = LoadIconTheme(default_theme_name);
        if (default_theme == NULL)
            return -1;
    
        HashMapInsert2(themes_map, default_theme_name, default_theme);
        DArrayAdd(themes_names, strdup(default_theme_name));
    }

    return 0;
}

// Ignore nested themes and only do the top level theme and hicolor
int PreloadIconThemesFast(const char *theme)
{
    IconTheme *icon_theme = LoadIconTheme(theme);

    if (icon_theme == NULL)
        return -1;

    const char *default_theme_name = "hicolor";

    IconTheme *default_theme = LoadIconTheme(default_theme_name);

    if (default_theme == NULL)
        return -1;

    themes_map = HashMapCreate2((void*)UnLoadIconTheme, NULL);
    themes_names = DArrayCreate(8, free, SearchThemeNameCmp2, NULL);

    HashMapInsert2(themes_map, theme, icon_theme);
    DArrayAdd(themes_names, strdup(theme));

    HashMapInsert2(themes_map, default_theme_name, default_theme);
    DArrayAdd(themes_names, strdup(default_theme_name));

    return 0;
}

void DestroyIconThemes(void)
{
    if (themes_map == NULL)
        return;

    HashMapDestroy2(themes_map);
    DArrayDestroy(themes_names);
}

char *SearchIconInThemes(const char *icon, int size, int scale, int max_theme_depth)
{
    int themes_to_search = themes_names->size;
    if (max_theme_depth != 0)
        themes_to_search = MIN(themes_to_search, max_theme_depth);

    for (int i = 0; i < themes_to_search; i++)
    {
        char *theme_name = themes_names->data[i];
    
        IconTheme *theme = HashMapGet2(themes_map, theme_name);
        if (theme == NULL)
            return NULL;

        char *filename = LookupIcon(theme, icon, size, scale);
        if (filename != NULL)
        {
            return filename;
        }
    }

    return NULL;
}

char *SearchIconInTheme(const char *theme_name, const char *icon, int size, int scale)
{
    const char *found_theme_name = DArrayLinearSearch(themes_names, theme_name);

    if (!found_theme_name)
        return NULL;

    IconTheme *theme = HashMapGet2(themes_map, found_theme_name);
    if (theme != NULL)
    {
        return LookupIcon(theme, icon, size, scale);
    }

    return NULL;
}

char *FindIconHelper(const char *icon, int size, int scale, const char *theme)
{
    IconTheme *icon_theme = LoadIconTheme(theme);

    if (icon_theme == NULL)
        return NULL;

    char *filename = LookupIcon(icon_theme, icon, size, scale);
    if (filename != NULL)
    {
        UnLoadIconTheme(icon_theme);
        return filename;
    }

    if (icon_theme->parents->size != 0)
    {
        for (int i = 0; i < icon_theme->parents->size; i++)
        {
            char *parent = icon_theme->parents->data[i];
            filename = FindIconHelper(icon, size, scale, parent);
            if (filename != NULL)
                return filename;
        }
    }


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

    //return LookupFallbackIcon(icon);
    //free(theme);
    return NULL;

success:
    //free(theme);
    return filename;
}

static void SearchAndStoreIconHelper(HashMap *icons, const char *icon, int size, int scale)
{
    // TEST
    //char *filename = SearchIconInTheme("hicolor", icon, size, scale);

    char *filename = SearchIconInThemes(icon, size, scale, 3);

    if (filename != NULL)
    {
        HashMapInsert(icons, icon, filename);
        free(filename);
    }
}

typedef struct
{
    HashMap *valid_icons;
    int size;
    int scale;
} Args;

static void SearchAndStoreIcon(void *entry_ptr, void *args_ptr)
{
    XDGDesktopEntry *entry = entry_ptr;
    Args *args = args_ptr;
    const char *icon = entry->icon;

    SearchAndStoreIconHelper(args->valid_icons, icon, args->size, args->scale);
}

HashMap *FindAllIcons(BTreeNode *entries, int size, int scale)
{
    char theme[256];
    int found = GetCurrentGTKIconThemeName(theme);
    //char *theme = GetCurrentGTKIconThemeName();
    //char *theme = "Papirus";
    //char *theme = "breeze-dark";
    //char *theme = "Qogir";
    //int found = 0;
    if (found != 0 || theme[0] == '\0')
    {
        printf("Failed to get GTK icon theme name!\n");
        return NULL;
    }

#ifdef SEARCH_INHERITED_ICON_THEMES
    if (PreloadIconThemes(theme) != 0)
    {
        printf("Failed to load current GTK icon theme!\n");
        return NULL;
    }
#else
    if (PreloadIconThemesFast(theme) != 0)
    {
        printf("Failed to load current GTK icon theme!\n");
        return NULL;
    }
#endif

    HashMap *valid_icons = HashMapCreate();

    Args args =
    {
        .valid_icons = valid_icons,
        .size = size,
        .scale = scale        
    };

    // Desktop entry icons
    BSTInOrderTraverse(entries, SearchAndStoreIcon, &args);

    // Extra icons that are needed, should put this somewhere else
    for (size_t i = 0; i < ARRAY_SIZE(extra_icons); i++)
    {
        SearchAndStoreIconHelper(valid_icons, extra_icons[i], size, scale);
    }

    return valid_icons;
}
