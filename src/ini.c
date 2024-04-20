
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "hashing.h"
#include "ini.h"

static void IniParse(IniFile *ini)
{
    char section[128];
    bool is_section = false;
    int read = 0;
    size_t len = 0;
    char *line = NULL;
    char *value = NULL;

    while ((read = getline(&line, &len, ini->fp)) != -1)
    {
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
        {
            continue;
        }

        line[strcspn(line, "\n")] = 0;

        if (line[0] == '[' && line[strlen(line) - 1] == ']')
        {
            sscanf(line, "[%[^]]", section);
            is_section = true;
            continue;
        }

        if (is_section)
        {
            char key[128];
            // This is why we can't have nice things.
            // Some icon themes have their Directory list 2047 characters long (Papirus)
            // This breaks many ini parser libraries and requires more memory allocations to be done
            // Icon themes should follow what KDE does with their themes by having it split into ScaledDirectories and Directories
            char *delim = strchr(line, '=');
            if (delim != NULL)
            {
                // Split key and value
                *delim = '\0';
                strcpy(key, line) ;

                size_t value_len = strlen(delim + 1);
                // No error checking, I don't care
                value = realloc(value, value_len + 1);
                strcpy(value, delim + 1);
                // Shove that data in there 
                HashMapInsertWithSection(ini->map, section, key, value);
            }
        }
    }

    free(value);
    free(line);
}

IniFile *IniFileLoad(const char *path)
{
    IniFile *ini = malloc(sizeof(*ini));
    ini->fp = fopen(path, "r");
    if (ini->fp == NULL)
    {
        fprintf(stderr, "Error opening file %s\n", path);
        free(ini);
        return NULL;
    }

    ini->map = HashMapCreate();
    IniParse(ini);

    return ini;
}

void IniDestroy(IniFile *ini)
{
    HashMapDestroy(ini->map);
    fclose(ini->fp);
    free(ini);
}

const char *IniGetString(IniFile *ini, const char *key)
{
    return HashMapGet(ini->map, key);
}

int IniGetInt(IniFile *ini, const char *key)
{
    const char *value = HashMapGet(ini->map, key);
    if (value == NULL)
        return -1;
    return (int)strtol(value, NULL, 0);
}

void IniPrintAll(IniFile *ini)
{
    HashMapPrint(ini->map);
}
