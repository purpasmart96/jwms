
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
//#include "ini.h"
#include "icons.h"
#include "desktop_entries.h"
#include "list.h"
#include "config.h"


int main()
{
    DArray *entries = DArrayCreate(100, sizeof(XDGDesktopEntry*));
    LoadDesktopEntries(entries);
    EntriesSort(entries);

    List *icons_input = ListCreate();
    for (size_t i = 0; i < entries->size; i++)
    {
        XDGDesktopEntry *current_entry = entries->data[i];
        ListAdd(icons_input, current_entry->icon, strlen(current_entry->icon) + 1);
    }

    //List *icons_output = FindAllIcons(icons_input, 32, 1);
    ListPrint(icons_input);
    //ListDestroy(icons_output);
    HashMap *icons_output = FindAllIcons(icons_input, 32, 1);
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
    ListDestroy(icons_input);

    //EntriesPrint(entries);
    // Test
    XDGDesktopEntry *entry = EntriesSearchExec(entries, "firefox-esr");
    if (entry != NULL)
        printf("Search found:\n%s\n%s\n%s\n%s\n", entry->name, entry->category_name, entry->exec, entry->icon);

    EntriesDestroy(entries);

    return 0;
}
