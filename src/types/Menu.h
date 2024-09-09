#ifndef TYPES_MENU_H
#define TYPES_MENU_H

#include <vector>
#include <string>

struct MenuCategory;

struct MenuItem {
    std::string label;
    std::string action;
    std::shared_ptr<MenuCategory> subCategory = nullptr;
};

struct MenuCategory {
    std::string name;
    std::vector<MenuItem> items;
};

#endif
