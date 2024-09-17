
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>

#include <bsd/string.h>
#include <confuse.h>
#include <unistd.h>

#include "common.h"
#include "bstree.h"
#include "list.h"
#include "hashing.h"
#include "darray.h"
#include "icons.h"
#include "desktop_entries.h"
#include "config.h"


int CreateJWMFolder(JWM *jwm)
{
    char path[512];
    const char *home = getenv("HOME");
    const char *dir = "/.config/jwm/";

    strlcpy(path, home, sizeof(path));
    strlcat(path, dir, sizeof(path));

    // Check if directory exists
    if (access(path, F_OK) != 0)
    {
        // Create it
        if (mkdir(path, 0755) != 0)
        {
            fprintf(stderr, "Failed to create directory\n");
            return -1;
        }
        printf("Created directory %s\n", path);
    }

    jwm->autogen_config_path = strdup(path);

    return 0;
}

int CreateJWMStartup(JWM *jwm)
{
    char path[512];
    const char *fname = "startup";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the startup xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("   <StartupCommand>~/.config/jwm/autostart</StartupCommand>\n");
    //WRITE_CFG("   <RestartCommand>~/.config/jwm/autostart</RestartCommand>\n");
    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

int CreateJWMIcons(JWM *jwm)
{
    char path[512];
    const char *fname = "icons";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the icon xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("   <IconPath>/usr/share/jwm/</IconPath>\n");
    WRITE_CFG("   <IconPath>/usr/share/pixmaps/</IconPath>\n");
    WRITE_CFG("</JWM>");

    fclose(fp);
    return 0;
}

int CreateJWMGroup(JWM *jwm)
{
    char path[512];
    const char *fname = "group";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    printf("Generating JWM groups!\n");

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the group xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("    <Group>\n");
    WRITE_CFG("        <Option>tiled</Option>\n");
    if (jwm->window_use_aerosnap)
        WRITE_CFG("        <Option>aerosnap</Option>\n");
    WRITE_CFG("    </Group>\n");
    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

int CreateJWMPreferences(JWM *jwm)
{
    char path[512];
    const char *fname = "prefs";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the prefs xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    if (jwm->trays[0].position < Left)
    {
        WRITE_CFG("   <Desktops width=\"%d\" height=\"%d\">\n", jwm->desktop_workspaces, 1);
    }
    else
    {
        WRITE_CFG("   <Desktops width=\"%d\" height=\"%d\">\n", 1, jwm->desktop_workspaces);
    }
    WRITE_CFG("       <Background type=\"%s\">%s</Background>\n", jwm->desktop_background_type, jwm->desktop_background);
    WRITE_CFG("   </Desktops>\n");
    WRITE_CFG("   <DoubleClickSpeed>%d</DoubleClickSpeed>\n", 400);
    WRITE_CFG("   <DoubleClickDelta>%d</DoubleClickDelta>\n", 2);
    WRITE_CFG("   <FocusModel>%s</FocusModel>\n", jwm->window_focus_model);
    WRITE_CFG("   <SnapMode distance=\"%d\">%s</SnapMode>\n", jwm->window_snap_distance, jwm->window_snap_mode);
    WRITE_CFG("   <MoveMode>%s</MoveMode>\n", jwm->window_move_mode);
    WRITE_CFG("   <ResizeMode>%s</ResizeMode>\n", jwm->window_resize_mode);
    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}

// There will only be one backup of the .jwmrc file
int CreateJWMRCBackup(char *path, char *backup_path)
{
    FILE *fp = fopen(path, "r");

    if (fp == NULL)
    {
        printf("%s does not exist! Skipping creation of backup!\n", path);
        return -1;
    }

    FILE *fp_bak = fopen(backup_path, "w");

    if (fp_bak == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", backup_path, strerror(errno));
        return -1;
    }

    char buffer[BUFSIZ];
    size_t bytes_read = 0;

    // Read contents from file and write the contents to a new file
    while ((bytes_read = fread(buffer, 1, BUFSIZ, fp)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, fp_bak) != bytes_read)
        {
            fprintf(stderr, "Error writing to '%s': %s\n", backup_path, strerror(errno));
            fclose(fp);
            fclose(fp_bak);
            return -1;
        }
    }

    fclose(fp);
    fclose(fp_bak);
    return 0;
}

int CreateJWMRCFile(JWM *jwm)
{
    char path[512];
    char path_bak[512];
    const char *home = getenv("HOME");
    const char *fname = ".jwmrc";
    const char *fnamebak = ".jwmrc.BAK";

    strlcpy(path, home, sizeof(path));
    strlcat(path, "/", sizeof(path));
    strlcpy(path_bak, path, sizeof(path_bak));
    strlcat(path, fname, sizeof(path));
    strlcat(path_bak, fnamebak, sizeof(path_bak));

    if (CreateJWMRCBackup(path, path_bak) == 0)
    {
        DEBUG_LOG("Succesfully created backup of %s in %s\n", fname, path_bak);
    }

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the .jwmrc file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/menu</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/startup</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/tray</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/group</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/styles</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/icons</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/prefs</Include>\n");
    WRITE_CFG("    <Include>$HOME/.config/jwm/binds</Include>\n");
    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}
