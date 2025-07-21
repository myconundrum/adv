#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <stdbool.h>

// Menu options
typedef enum {
    MENU_OPTION_NEW_GAME = 0,
    MENU_OPTION_LOAD_GAME,
    MENU_OPTION_QUIT,
    MENU_OPTION_COUNT
} MenuOption;

// Main menu state
typedef struct {
    MenuOption selected_option;
    bool option_selected;
} MainMenu;

// Main menu functions
void main_menu_init(MainMenu *menu);
void main_menu_cleanup(MainMenu *menu);

// Input handling
void main_menu_handle_input(MainMenu *menu, int key);

// Rendering
void main_menu_render(MainMenu *menu);

// Get selection
MenuOption main_menu_get_selection(const MainMenu *menu);
bool main_menu_has_selection(const MainMenu *menu);

#endif
