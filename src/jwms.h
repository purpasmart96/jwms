#ifndef JWMS_H
#define JWMS_H

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
    Utility,
    Invalid
} XDGMainCategories;

typedef enum
{
    WebBrowser,
    FileManager,
    TerminalEmulator,
    TextEditor,
} XDGAdditionalCategories;

typedef enum
{
    Type,
    Version,
    Name,
    GenericName,
    NoDisplay,
    Comment,
    Icon,
    Hidden,
    OnlyShowIn,
    NotShowIn,
    DBusActivatable,
    TryExec,
    Exec,
    Path,
    Terminal,
    Actions,
    MimeType,
    Categories,
    Implements,
    Keywords,
    StartupNotify,
    StartupWMClass,
    URL,
    PrefersNonDefaultGPU,
    SingleMainWindow
} XDGKeyType;

// Generic key value struct
typedef struct
{
    char *key;
    int value;
} KeyValuePair;

// String key value struct
typedef struct
{
    char *key;
    char *value;
} KeyValuePairStr;

// /usr/share/applications
typedef struct
{
    KeyValuePair *category;
    KeyValuePair *extra_category;
    char *name;
    char *exec;
    char *desc;
    char *icon;
    bool terminal_required;
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
    char *browser_name;
    char *terminal_name;
    char *filemanager_name;
} JWMTray;

typedef struct
{
    JWMRootMenu *root_menu;
    JWMTray *tray;
    char *browser_name;
    char *terminal_name;
    char *filemanager_name;
} JWM;

#endif
