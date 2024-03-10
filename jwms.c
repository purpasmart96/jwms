
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
//#include <pwd.h>

#include <bsd/string.h>
#include <confuse.h>

#define ARRAY_SIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

// Freedesktop.org miniumum required categories
typedef enum
{
    AudioVideo,
    Audio,
    Video,
    Development,
    Education,
    Game,
    Graphics,
    Network,
    Office,
    Science,
    Settings,
    System,
    Utility
} XDGMainCategories;

static const char *xdg_categories[] =
{
    "AudioVideo",
    "Audio",
    "Video",
    "Development",
    "Education",
    "Game",
    "Graphics",
    "Network",
    "Office",
    "Science",
    "Settings",
    "System",
    "Utility"
};

// Can't and most likely wont support these
static const char *xdg_parse_blacklist[] =
{
    "MimeType",
    "TryExec",
    "StartupNotify",
    "Name[ast]",
    "Name[bg]",
    "Name[bs]",
    "Name[ca]",
    "Name[ca@valencia]]",
    "Name[cs]",
    "Name[da]",
    "Name[de]",
    "Name[dz]",
    "Name[el]",
    "Name[en_CA]",
    "Name[en_GB]",
    "Name[eo]",
    "Name[es]",
    "Name[et]",
    "Name[eu]",
    "Name[fi]",
    "Name[fr]",
    "Name[ga]",
    "Name[gl]",
    "Name[gu]",
    "Name[he]",
    "Name[hr]",
    "Name[hu]",
    "Name[id]",
    "Name[it]",
    "Name[ja]",
    "Name[kk]",
    "Name[ko]",
    "Name[lt]",
    "Name[lv]",
    "Name[mk]",
    "Name[ml]",
    "Name[mr]",
    "Name[ms]",
    "Name[nb]",
    "Name[ne]",
    "Name[nl]",
    "Name[nn]",
    "Name[oc]",
    "Name[pl]",
    "Name[pt]",
    "Name[pt_BR]",
    "Name[ro]",
    "Name[ru]",
    "Name[sk]",
    "Name[sl]",
    "Name[sr]",
    "Name[sr@latin]",
    "Name[sv]",
    "Name[tr]",
    "Name[uk]",
    "Name[vi]",
    "Name[zh_CN]",
    "Name[zh_HK]",
    "Name[zh_TW]",

    "Comment[ast]",
    "Comment[bg]",
    "Comment[bs]",
    "Comment[ca]",
    "Comment[ca@valencia]]",
    "Comment[cs]",
    "Comment[da]",
    "Comment[de]",
    "Comment[dz]",
    "Comment[el]",
    "Comment[en_CA]",
    "Comment[en_GB]",
    "Comment[eo]",
    "Comment[es]",
    "Comment[et]",
    "Comment[eu]",
    "Comment[fi]",
    "Comment[fr]",
    "Comment[ga]",
    "Comment[gl]",
    "Comment[gu]",
    "Comment[he]",
    "Comment[hr]"
};

// /usr/share/applications
typedef struct
{
    //XDGMainCategories type;
    char *name;
    char *category;
    char *exec;
    char *desc;
    char *icon;
    bool terminal;
} XDGDesktopEntry;


typedef struct
{
    size_t size;
    size_t capacity;
    XDGDesktopEntry **data;
} XDGDesktopEntries;


typedef struct {
    int height;
} JWMRootMenu;

typedef enum
{
    Bottom,
    Top,
    Left,
    Right
} TrayPositions;

typedef struct {
    TrayPositions tpos;
    int height;
    bool auto_hide;
    int icon_spacing;
} JWMTray;


static XDGDesktopEntry *CreateEntry(char *category, char *name, char *exec, char *icon, bool terminal)
{
    XDGDesktopEntry *entry = malloc(sizeof(XDGDesktopEntry));

    if (entry == NULL)
        return NULL;

    entry->category = strdup(category);
    entry->name = strdup(name);
    entry->exec = strdup(exec);
    entry->icon = strdup(icon);
    entry->desc = NULL;
    entry->terminal = terminal;

    return entry;
}

static void DestroyEntry(XDGDesktopEntry *entry)
{
    free(entry->name);
    free(entry->category);
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
        printf("Program  : %s\n", entries->data[i]->name);
        printf("Category : %s\n", entries->data[i]->category);
        printf("CMD      : %s\n", entries->data[i]->exec);
        printf("Icon     : %s\n", entries->data[i]->icon);

    }
}

static XDGDesktopEntry *EntriesSearch(XDGDesktopEntries *entries, const char *key)
{
    for (size_t i = 0; i < entries->size; i++)
    {
        // Could make into one if staement
        if (strstr(entries->data[i]->name, key) != NULL)
            return entries->data[i];
        if (strstr(entries->data[i]->exec, key) != NULL)
            return entries->data[i];
        if (strstr(entries->data[i]->icon, key) != NULL)
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

XDGDesktopEntry *EntriesAdd(XDGDesktopEntries *entries, XDGDesktopEntry *element)
{
    if (EntriesFull(entries))
    {
        if (!EntriesResizeArray(entries, entries->capacity *= 2))
        {
            return NULL;
        }
    }
    // Push an element on the top of it and increase its size by one
    entries->data[entries->size++] = element;
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

/*
static int GetNumCatergories(char *categories)
{
    int count = 0;
    for (size_t i = 0; i < strlen(categories); i++)
    {
        if (categories[i] == ';')
            count++;
    }

    if (count == 0)
        return 1;

    return count;
}
*/

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

// Remove desktop entry program args like %U and %F For JWM
// TODO WIP, BROKEN
static char *ParseExec(char *exec)
{
    if (strstr(exec, "%U") != NULL)
    {
        return strtok(exec, "%U");
    }

    if (strstr(exec, "%F") != NULL)
    {
        return strtok(exec, "%F");
    }

    return exec;
}

static char *ParseCategories(char *categories)
{
    printf("Listed categories: %s\n", categories);

    char *token = strtok(categories, ";");
    while (token != NULL)
    {
        for (unsigned int i = 0; i < ARRAY_SIZE(xdg_categories); i++)
        {
            if (strcmp(token, xdg_categories[i]) == 0)
            {
                printf("Found valid category: %s\n", token);
                return token;
            }
        }

        token = strtok(NULL, ";");
    }

    printf("No valid category found in list: %s\n", categories);
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

    // Category should be converted a enum, using the raw string for now...
    //XDGMainCategories category;
    bool application = false;
    char type[64];
    char category[128];
    char name[128];
    char exec[128];
    //char desc[256];
    char icon[128];
    bool terminal = false;

    // Read line by line
    while ((read = getline(&line, &len, fp)) != -1)
    {
        // Skip blank lines and comments
        if (line[0] == '\n' || line[0] == '#')
            continue;

        // Simple trailing newline removal
        line[strcspn(line, "\n")] = 0;

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "=");

        if (value == NULL)
        {
            continue; // Not a key-value pair
        }

        // Thread safe version?
        /*
        // Returns first token
        char *token = strchr(line, '=');

        if (token == NULL)
        {
            continue;  // Not a key-value pair
        }

        // Key and value
        *token = '\0';
        char *key = line;
        char *value = token + 1;
        */

        /*
        bool skip = false;
        for (unsigned int i = 0; i < ARRAY_SIZE(xdg_parse_blacklist); i++)
        {
            if (strcmp(key, xdg_parse_blacklist[i]) == 0)
                skip = true;
        }

        if (skip)
            continue;
        */

        if (strcmp(key, "Type") == 0)
        {
            // Ignore Links and Directories for now...
            if (strcmp(value, "Application") == 0)
                application = true;
        }

        if (strcmp(key, "Name") == 0)
        {
            strcpy(name, value);
        }

        if (strcmp(key, "Exec") == 0)
        {
            strcpy(exec, value);
            // Not ready yet
            //strcpy(exec, ParseExec(exec));
        }

        if (strcmp(key, "Icon") == 0)
        {
            strcpy(icon, value);
        }

        if (strcmp(key, "Terminal") == 0)
        {
            terminal = StringToBool(value);
        }

        if (strcmp(key, "Categories") == 0)
        {
            char *temp = ParseCategories(value);
            // Check if a valid category
            if (temp)
                strcpy(category, temp);
        }
    }

    fclose(fp);

    if (application)
        return CreateEntry(category, name, exec, icon, terminal);

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

/* validates a port option (must be positive) */
/*
int conf_validate_port(cfg_t *cfg, cfg_opt_t *opt)
{
    int value = cfg_opt_getnint(opt, 0);

    if (value <= 0) {
        cfg_error(cfg, "invalid port %d in section '%s'", value, cfg_name(cfg));
        return -1;
    }
    return 0;
}*/
/* parse values for the auto-create-bookmark option */
/*
int parse_tray_position(cfg_t *cfg, cfg_opt_t *opt, const char *value)
{
    if (strcmp(value, "bottom") == 0)
        return Bottom;
    if (strcmp(value, "top") == 0)
        return Top;
    if (strcmp(value, "left") == 0)
        return Left;
    if (strcmp(value, "right") == 0)
        return Right;
    //else {
    //cfg_error(cfg, "invalid value for option '%s': %s", cfg_opt_name(opt), value);
    //    return -1;
    //}
    cfg_error(cfg, "invalid value for option '%s': %s", cfg_opt_name(opt), value);
    return Bottom;
}
*/

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
    fprintf(fp, "       <TrayButton popup=\"Terminal\" icon=\"terminal\">exec:urxvt</TrayButton>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", tray->icon_spacing);
    fprintf(fp, "       <TrayButton popup=\"File Manager\" icon=\"system-file-manager\">exec:spacefm</TrayButton>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", tray->icon_spacing);
    fprintf(fp, "       <TrayButton popup=\"Web Browser\" icon=\"firefox-esr\">exec:firefox</TrayButton>\n");
    fprintf(fp, "       <Spacer width=\"%d\"/>\n", tray->icon_spacing);
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
    //free(path);
}

static void WriteJWMRootMenuCategoryList(JWMRootMenu *root_menu, XDGDesktopEntries *entries, FILE *fp, const char *category, const char *overide_category_name)
{
    const char *menu_name = category; 
    if (overide_category_name != NULL)
        menu_name = overide_category_name;

    fprintf(fp, "       <Menu icon=\"folder\" label=\"%s\">\n", menu_name);

    for (size_t i = 0; i < entries->size; i++)
    {
        if (strcmp(entries->data[i]->category, category) == 0)
        {
            XDGDesktopEntry *entry = entries->data[i];
            fprintf(fp, "           <Program icon=\"%s\" label=\"%s\">%s</Program>\n",
                    entry->icon, entry->name, entry->exec);
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

    WriteJWMRootMenuCategoryList(root_menu, entries, fp, "Network", "Internet");
    WriteJWMRootMenuCategoryList(root_menu, entries, fp, "AudioVideo", "Multimedia");
    WriteJWMRootMenuCategoryList(root_menu, entries, fp, "Office", NULL);
    WriteJWMRootMenuCategoryList(root_menu, entries, fp, "Graphics", NULL);
    WriteJWMRootMenuCategoryList(root_menu, entries, fp, "Video", NULL);
    WriteJWMRootMenuCategoryList(root_menu, entries, fp, "Settings", NULL);
    WriteJWMRootMenuCategoryList(root_menu, entries, fp, "System", NULL);
    WriteJWMRootMenuCategoryList(root_menu, entries, fp, "Utility", NULL);

    fprintf(fp, "   </RootMenu>\n");
    fprintf(fp, "</JWM>");

    fclose(fp);
}

int main()
{

    cfg_opt_t opts[] =
    {
        CFG_INT("rootmenu_height", 26, CFGF_NONE),
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

    root_menu->height = cfg_getint(cfg, "rootmenu_height");


    XDGDesktopEntries *entries = EntriesCreateArray(100);
    LoadDesktopEntries(entries);

    GenJWMRootMenu(root_menu, entries);
    GenJWMTray(tray, entries);
    EntriesPrint(entries);

    // Test
    XDGDesktopEntry *entry = EntriesSearch(entries, "firefox");
    if (entry != NULL)
        printf("Search found:\n%s\n%s\n%s\n%s\n", entry->name, entry->category, entry->exec, entry->icon);

    free(root_menu);
    free(tray);

    EntriesDestroy(entries);
    cfg_free(cfg);

    return 0;
}

