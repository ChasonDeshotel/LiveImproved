#ifndef TYPES_H
#define TYPES_H

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

struct Plugin {
    int number;
    std::string name;
    std::string type;
    std::string uri;
};

#endif
