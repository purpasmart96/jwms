#ifndef THEMES_H
#define THEMES_H

typedef enum
{
    Adwaita,
    AdwaitaDark,
    Breeze,
    BreezeDark,
    Custom
} ColorTheme;

int UseTheme(JWM *jwm, ColorTheme theme);

#endif
