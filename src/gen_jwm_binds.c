#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <bsd/string.h>
#include <confuse.h>

#include "common.h"
#include "bstree.h"
#include "list.h"
#include "hashing.h"
#include "darray.h"
#include "icons.h"
#include "desktop_entries.h"
#include "config.h"

static const struct
{
    char *in;
    char *out;
} key_mods[] = {
    {"Alt",     "A"},
    {"Control", "C"},
    {"Shift",   "S"},
    {"mod1",    "1"},
    {"mod2",    "2"},
    {"mod3",    "3"},
    {"mod4",    "4"},
    {"mod5",    "5"}
};

static char *GetJWMKeyMod(char *in)
{
    for (size_t i = 0; i < ARRAY_SIZE(key_mods); i++)
    {
        if (strcmp(key_mods[i].in, in) == 0)
            return key_mods[i].out;
    }
    return NULL;
}

int CreateJWMBinds(JWM *jwm, cfg_t *cfg)
{
    char path[512];
    char keymods[64];
    const char *fname = "binds";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    printf("Generating JWM bindings!\n");

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the binds xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");

    int n = cfg_size(cfg, "keybind");
	DEBUG_LOG("\nFound %d keybinds:\n", n);

	for (int i = 0; i < n; i++)
    {
		cfg_t *keybind = cfg_getnsec(cfg, "keybind", i);

        int num_mods = cfg_size(keybind, "mods");
        const char *title = cfg_title(keybind);
        DEBUG_LOG("keybind %u: %s\n", i + 1, title);
        const char *key = cfg_getstr(keybind, "key");
        const char *cmd = cfg_getstr(keybind, "command");

        if (key == NULL)
        {
            printf("key for keybind %s was NULL! Skipping keybind...\n", title);
            continue;
        }

        if (cmd == NULL)
        {
            printf("Command for keybind %s was NULL! Skipping keybind...\n", title);
            continue;
        }

        DEBUG_LOG("key: %s\n", key);

        for (int j = 0; j < num_mods; j++)
        {
            char *in_keymod = cfg_getnstr(keybind,"mods", j);
            DEBUG_LOG("keymod %d: %s\n", j, in_keymod);
            char *out_keymod = GetJWMKeyMod(in_keymod);

            if (out_keymod == NULL)
            {
                fclose(fp);
                return -1;
            }

            strlcat(keymods, out_keymod, sizeof(keymods));
        }

        DEBUG_LOG("command: %s\n\n", cmd);

        if (!num_mods)
        {
            WRITE_CFG("    <Key key=\"%s\">%s</Key>\n", key, cmd);
        }
        else
        {
            WRITE_CFG("    <Key mask=\"%s\" key=\"%s\">%s</Key>\n", keymods, key, cmd);
        }
        keymods[0] = '\0';
	}

    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}
