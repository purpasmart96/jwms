#include <stddef.h>
#include <stdbool.h>

#include "bstree.h"
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
            //@define-color unfocused_borders_breeze #5f6265;
            break;
        }
        case BreezeDark:
        {
            //@define-color theme_bg_color_breeze #2a2e32;
            //@define-color theme_fg_color_breeze #fcfcfc;

            //@define-color theme_unfocused_base_color_breeze #1b1e20;
            //@define-color theme_unfocused_bg_color_breeze #2a2e32;
            //@define-color theme_unfocused_fg_color_breeze #fcfcfc;
    
            //jwm->global_bg_color_active = "#2a2e32";
            // text color #fcfcfc
            // selection background #3daee9
            // view background #1b1e20
            // view background inactive #232629
            // view normal text #fcfcfc
            // view active text #3daee9
            // view inactive text #a1a9b1
            // window background #2a2e32
            // button background #31363b
            // inactive text #a1a9b1
            // decorations #3daee9

            jwm->global_bg_color_active = "#31363B";
            jwm->global_bg_color_inactive = "#2a2e32";

            jwm->global_fg_color_active = "#fcfcfc";
            jwm->global_fg_color_inactive = "#a1a9b1";
            break;
        }
        default:
            break;
    }

    return 0;
}
