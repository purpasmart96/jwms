#ifndef DESKTOP_ENTRIES_H
#define DESKTOP_ENTRIES_H

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
    Invalid = -1
} XDGMainCategories;

typedef enum
{
    WebBrowser,
    FileManager,
    TerminalEmulator,
    TextEditor,
    IgnoredOrInvalid = -1
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

// /usr/share/applications
typedef struct
{
    XDGMainCategories category;
    XDGAdditionalCategories extra_category;
    char *category_name;
    char *extra_category_name;
    char *name;
    char *exec;
    char *desc;
    char *icon;
    bool terminal_required;
} XDGDesktopEntry;

void EntriesPrint(DArray *entries);
void EntriesDestroy(DArray *entries);
void BinaryInsertionSort(DArray *entries);
void EntriesSort(DArray *entries);
XDGDesktopEntry *EntriesSearchExec(DArray *entries, const char *key);
XDGDesktopEntry *GetCoreProgram(DArray *entries, XDGAdditionalCategories extra_category, const char *name);
int LoadDesktopEntries(DArray *entries);

#endif
