#ifndef INI_H
#define INI_H

typedef struct
{
    FILE *fp;
    HashMap *map;
} IniFile;

IniFile *IniFileLoad(const char *path);
void IniDestroy(IniFile *ini);
int IniGetInt(IniFile *ini, const char *key);
const char *IniGetString(IniFile *ini, const char *key);
void IniPrintAll(IniFile *ini);

#endif
