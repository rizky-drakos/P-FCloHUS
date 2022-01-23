#pragma once
#include <unordered_map>
#include <vector>
#include <iostream>
#include <memory>

class Item {
    public:
        int id;
        float utility;

        Item(int id);
};

class Sequence {
    public:
        unsigned int size;
        float utility;
        std::vector<std::shared_ptr<Item>> items;

        Sequence();
};

class ItemInstance {
    public:
        float utility;
        float rem;
        unsigned int position;

        ItemInstance(float utility, float rem, unsigned int position);
};

class Pattern {
    public:
        std::string name;
        int lastItem;
        bool isSExt;
        std::unordered_map<unsigned int, std::vector<std::shared_ptr<ItemInstance>>> siduls;
        bool isMaximal;
        bool do_ext;
        bool do_s_ext;
        unsigned int size;
        /*
            Reserve parent information for later use
        */
        bool isParentSExt;
        int parentLastItem;
        /*
            To speed up, these metrics are cached in a pattern when the pattern is constructed
        */
        float RBU;
        float umin;
        unsigned int SE;
        unsigned int SLIP;
        Pattern();
};