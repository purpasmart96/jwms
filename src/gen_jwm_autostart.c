
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>

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

static void WriteAutostartProgram(FILE *fp, int sleep_time, bool kill, bool fork, const char *program, const char *args)
{
    DEBUG_LOG("program: %s\n", program);
    if (kill)
        WRITE_CFG("pkill %s\n", program);

    if (args != NULL)
    {
        if (fork && sleep_time > 0)
        {
            WRITE_CFG("sleep %d && %s %s &\n", sleep_time, program, args);
        }
        else if (sleep_time > 0)
        {
            WRITE_CFG("sleep %d && %s %s\n", sleep_time, program, args);
        }
        else if (fork)
        {
            WRITE_CFG("%s %s &\n", program, args);
        }
        else
        {
            WRITE_CFG("%s %s\n", program, args);
        }
    }
    else
    {
        if (fork && sleep_time > 0)
        {
            WRITE_CFG("sleep %d && %s &\n", sleep_time, program);
        }
        else if (sleep_time > 0)
        {
            WRITE_CFG("sleep %d && %s\n", sleep_time, program);
        }
        else if (fork)
        {
            WRITE_CFG("%s &\n", program);
        }
        else
        {
            WRITE_CFG("%s\n", program);
        }
    }
}

int CreateJWMAutoStart(JWM *jwm, cfg_t *cfg)
{
    char path[512];
    const char *fname = "autostart";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    printf("Generating autostart script!\n");

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the bash autostart script
    WRITE_CFG("#!/bin/bash\n\n");

    int n = cfg_size(cfg, "autostart");
	DEBUG_LOG("\nFound %d autostart tasks:\n", n);

	for (int i = 0; i < n; i++)
    {
		cfg_t *autostart = cfg_getnsec(cfg, "autostart", i);

        const char *title = cfg_title(autostart);
        DEBUG_LOG("autostart %u: %s\n", i + 1, title);

        int sleep_time = cfg_getint(autostart, "sleep_time");
        bool fork = cfg_getbool(autostart, "fork_needed");
        bool kill = cfg_getbool(autostart, "restart_kill");
        char *program = cfg_getstr(autostart, "program");
        char *args = cfg_getstr(autostart, "args");

        if (program == NULL)
        {
            printf("Program name was NULL for autostart %s !\n", title);
            fclose(fp);
            return -1;
        }
        
        WriteAutostartProgram(fp, sleep_time, kill, fork, program, args);
	}

    chmod(path, 0755);

    fclose(fp);
    return 0;
}
