#include <stddef.h>
#include <stdbool.h>

#include "darray.h"
#include "hashing.h"

#include "config.h"
#include "themes.h"


int UseTheme(JWM *jwm, ColorTheme theme)
{
    switch (theme)
    {
        case Adwaita:
        {
            break;
        }
        case BreezeDark:
        {
            //@define-color theme_bg_color_breeze #2a2e32;
            //@define-color theme_fg_color_breeze #fcfcfc;

            //@define-color theme_unfocused_base_color_breeze #1b1e20;
            //@define-color theme_unfocused_bg_color_breeze #2a2e32;
            //@define-color theme_unfocused_fg_color_breeze #fcfcfc;
            jwm->global_bg_color_active = "#2a2e32";
            jwm->global_bg_color_inactive = "#2a2e32";

            jwm->global_fg_color_active = "#fcfcfc";
            jwm->global_fg_color_inactive = "#fcfcfc";
            break;
        }
        default:
            break;
    }

    return 0;
}
