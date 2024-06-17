
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <bsd/string.h>
#include <confuse.h>

#include "darray.h"
#include "list.h"
#include "hashing.h"
#include "bstree.h"
//#include "ini.h"
#include "icons.h"
#include "desktop_entries.h"
#include "list.h"
#include "config.h"

int main()
{
    BTreeNode *entries = NULL;
    int success = LoadDesktopEntries(&entries);

    if (success != 0)
    {
        printf("Failed to load desktop entries!\n");
        return EXIT_FAILURE;
    }

    // Test for node deletion code
    //EntryRemove(entries, "Okular");

    printf("Loading icons...\n");
    HashMap *icons_output = FindAllIcons2(entries, 32, 1);
    if (icons_output == NULL)
    {
        printf("Failed to load icons!\n");
        return EXIT_FAILURE;
    }
    
    //HashMapPrint(icons_output);

    int result = WriteJWMConfig(entries, icons_output);
    if (result != -1)
        printf("Wrote JWM config sucessfully\n");
    else
        printf("Failed to write JWM config\n");

    // Hashmap test
    //IniFile *icon_theme = IniFileLoad("/usr/share/icons/hicolor/index.theme");
    //if (icon_theme != NULL)
    //{
        //IniPrintAll(icon_theme);
    //    printf("Icon Directories: %s\n", IniGetString(icon_theme, "Icon Theme:Directories"));
    //    IniDestroy(icon_theme);
    //}

    HashMapDestroy(icons_output);

    //EntriesPrint(entries);
    // Test
    //XDGDesktopEntry *entry = EntriesSearch(entries, "Firefox Web Browser");
    //if (entry != NULL)
    //    printf("Search found:\n%s\n%s\n%s\n%s\n", entry->name, entry->category_name, entry->exec, entry->icon);

    EntriesDestroy(entries);

    return 0;
}
