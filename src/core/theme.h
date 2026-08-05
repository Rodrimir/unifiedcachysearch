#ifndef THEME_H
#define THEME_H

#include <QString>

struct ThemeColors {
    QString bg1, bg2, bg3;
    QString fg1, fg2, fg3;
    QString accent, border, hoverBg;
    QString green, yellow, red;
};

inline ThemeColors darkTheme() {
    return {
        "#1e1e2e", "#181825", "#11111b",
        "#cdd6f4", "#a6adc8", "#585b70",
        "#89b4fa", "#313244", "#45475a",
        "#a6e3a1", "#f9e2af", "#f38ba8"
    };
}

inline ThemeColors lightTheme() {
    return {
        "#eff1f5", "#e6e9ef", "#dce0e8",
        "#4c4f69", "#5c5f77", "#9ca0b0",
        "#1e66f5", "#ccd0da", "#ccd0da",
        "#40a02b", "#df8e1d", "#d20f39"
    };
}

#endif
