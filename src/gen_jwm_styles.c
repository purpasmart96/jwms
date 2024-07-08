
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


static const Pair style_types[] =
{
    {"WindowStyle",   WindowStyle  },
    {"ClockStyle",    ClockStyle   },
    {"TrayStyle",     TrayStyle    },
    {"TaskListStyle", TaskListStyle},
    {"PagerStyle",    PagerStyle   },
    {"MenuStyle",     MenuStyle    },
    {"PopupStyle",    PopupStyle   },
};

static void WriteJWMStyle(JWM *jwm, FILE *fp, Styles style)
{
    switch (style)
    {
        case WindowStyle:
        {
            WRITE_CFG("    <%s decorations=\"%s\">\n", style_types[style].key, GetDecorationsStyle(window));
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(window), GetFontName(window), GetFontSize(window));
            WRITE_CFG("        <Height>%d</Height>\n", jwm->window_height);
            WRITE_CFG("        <Width>%d</Width>\n", jwm->window_width);
            WRITE_CFG("        <Corner>%d</Corner>\n", jwm->window_corner_rounding);
            WRITE_CFG("        <Foreground>%s</Foreground>\n", GetFGColor(window, inactive));
            WRITE_CFG("        <Background>%s</Background>\n", GetBGColor(window, inactive));
            WRITE_CFG("        <Opacity>%.2f</Opacity>\n", jwm->window_opacity_inactive);

            if (jwm->window_outline_enabled)
            {
                WRITE_CFG("        <Outline>%s</Outline>\n", jwm->window_outline_color_inactive);
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(window, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(window, active));
                WRITE_CFG("            <Opacity>%.2f</Opacity>\n", jwm->window_opacity_active);
                WRITE_CFG("            <Outline>%s</Outline>\n", GetOutlineColor(window, active));
            }
            else
            {
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(window, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(window, active));
                WRITE_CFG("            <Opacity>%.2f</Opacity>\n", jwm->window_opacity_active);
            }

            WRITE_CFG("        </Active>\n");
            break;
        }
        case ClockStyle:
        {
            WRITE_CFG("    <%s>\n", style_types[style].key);
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(clock), GetFontName(clock), GetFontSize(clock));
            WRITE_CFG("        <Foreground>%s</Foreground>\n", GetFGColor(clock, inactive));
            WRITE_CFG("        <Background>%s</Background>\n", GetBGColor(clock, inactive));
            break;
        }
        case TrayStyle:
        {
            WRITE_CFG("    <%s decorations=\"%s\">\n", style_types[style].key, GetDecorationsStyle(window));
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(tray), GetFontName(tray), GetFontSize(tray));
            WRITE_CFG("        <Foreground>%s</Foreground>\n", GetFGColor(tray, inactive));
            WRITE_CFG("        <Background>%s</Background>\n", GetBGColor(tray, inactive));

            if (jwm->tray_outline_enabled)
            {
                WRITE_CFG("        <Outline>%s</Outline>\n", GetOutlineColor(tray, inactive));
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(tray, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(tray, active));
                WRITE_CFG("            <Outline>%s</Outline>\n", GetOutlineColor(tray, active));
            }
            else
            {
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(tray, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(tray, active));
            }
    
            WRITE_CFG("        </Active>\n");
            WRITE_CFG("        <Opacity>%.2f</Opacity>\n", jwm->tray_opacity);
            break;
        }
        case TaskListStyle:
        {
            WRITE_CFG("    <%s decorations=\"%s\" group=\"%s\" list=\"%s\">\n", style_types[style].key, GetDecorationsStyle(tasklist), "true", "all");
            //WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(tasklist), GetFontName(tasklist), GetFontSize(tasklist));
            //WRITE_CFG("        <Font align=\"%s\">%d-%s</Font>\n", GetFontAlignment(tasklist), GetFontSize(tasklist), GetFontName(tasklist));
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(tasklist), GetFontName(tasklist), GetFontSize(tasklist));
            WRITE_CFG("        <Foreground>%s</Foreground>\n", GetFGColor(tasklist, inactive));
            WRITE_CFG("        <Background>%s</Background>\n", GetBGColor(tasklist, inactive));

            if (jwm->tray_outline_enabled)
            {
                WRITE_CFG("        <Outline>%s</Outline>\n", GetOutlineColor(tasklist, inactive));
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(tasklist, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(tasklist, active));
                WRITE_CFG("            <Outline>%s</Outline>\n", GetOutlineColor(tasklist, active));
            }
            else
            {
                WRITE_CFG("        <Active>\n");
                WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(tasklist, active));
                WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(tasklist, active));
            }

            WRITE_CFG("        </Active>\n");
            break;
        }
        case PagerStyle:
        {
            WRITE_CFG("    <%s>\n", style_types[style].key);
            WRITE_CFG("        <Outline>%s</Outline>\n", GetOutlineColor(pager, inactive));
            WRITE_CFG("        <Foreground>%s</Foreground>\n", GetFGColor(pager, inactive));
            WRITE_CFG("        <Background>%s</Background>\n", GetBGColor(pager, inactive));
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(pager), GetFontName(pager), GetFontSize(pager));
            WRITE_CFG("        <Text>%s</Text>\n", "#FFFFFF");
            WRITE_CFG("        <Active>\n");
            WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(pager, active));
            WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(pager, active));
            WRITE_CFG("        </Active>\n");
            break;
        }
        case MenuStyle:
        {
            WRITE_CFG("    <%s decorations=\"%s\">\n", style_types[style].key, GetDecorationsStyle(menu));
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", GetFontAlignment(menu), GetFontName(menu), GetFontSize(menu));
            WRITE_CFG("        <Foreground>%s</Foreground>\n", GetFGColor(menu, inactive));
            WRITE_CFG("        <Background>%s</Background>\n", GetBGColor(menu, inactive));
            if (jwm->menu_outline_enabled)
                WRITE_CFG("        <Outline>%s</Outline>\n", jwm->menu_use_global_colors ? jwm->global_outline_color : jwm->menu_outline_color);
            WRITE_CFG("        <Active>\n");
            WRITE_CFG("            <Foreground>%s</Foreground>\n", GetFGColor(menu, active));
            WRITE_CFG("            <Background>%s</Background>\n", GetBGColor(menu, active));
            WRITE_CFG("        </Active>\n");
            WRITE_CFG("        <Opacity>%.2f</Opacity>\n", jwm->menu_opacity);
            break;
        }
        case PopupStyle:
        {
            WRITE_CFG("    <%s enabled=\"%s\" delay=\"%d\">\n", style_types[style].key, "true", 600);
            WRITE_CFG("        <Font align=\"%s\">%s-%d</Font>\n", jwm->global_font_alignment, jwm->global_font, jwm->global_font_size);
            WRITE_CFG("        <Outline>%s</Outline>\n", jwm->global_outline_color);
            WRITE_CFG("        <Foreground>%s</Foreground>\n", jwm->global_fg_color_inactive);
            WRITE_CFG("        <Background>%s</Background>\n", jwm->global_bg_color_inactive);
            break;
        }
        default:
            break;
    }

    WRITE_CFG("    </%s>\n", style_types[style].key);
}

int CreateJWMStyles(JWM *jwm)
{
    char path[512];
    const char *fname = "styles";

    strlcpy(path, jwm->autogen_config_path, sizeof(path));
    strlcat(path, fname, sizeof(path));

    FILE *fp = fopen(path, "w");

    if (fp == NULL)
    {
        fprintf(stderr, "Error opening '%s': %s\n", path, strerror(errno));
        return -1;
    }

    printf("Writing to %s\n", path);

    // Start of the styles/theme xml file
    WRITE_CFG("<?xml version=\"1.0\"?>\n");
    WRITE_CFG("<JWM>\n");

    WriteJWMStyle(jwm, fp, WindowStyle);
    WriteJWMStyle(jwm, fp, ClockStyle);
    WriteJWMStyle(jwm, fp, TrayStyle);
    WriteJWMStyle(jwm, fp, TaskListStyle);
    WriteJWMStyle(jwm, fp, PagerStyle);
    WriteJWMStyle(jwm, fp, MenuStyle);
    WriteJWMStyle(jwm, fp, PopupStyle);

    WRITE_CFG("</JWM>\n");

    fclose(fp);
    return 0;
}
