#pragma once
#ifndef ITEM_HIS_H
#define ITEM_HIS_H

#include "rng.h"
#include "units.h"

#include <vector>

class item;
class player;

typedef std::reference_wrapper<item> itemref;
typedef std::reference_wrapper<const item> citemref;
typedef std::pair<int, itemref> intitemref;
typedef std::pair<int, citemref> intcitemref;
typedef std::list<itemref> listitemref;
typedef std::list<citemref> listcitemref;
typedef std::vector<itemref> vectoritemref;
typedef std::vector<citemref> vectorcitemref;

struct root_item;

struct position_uid {
    int position = INT_MIN;     // position of root item
    UID uid = UID_NONE;         // uid of nested item

    position_uid() = default;
    position_uid(UID uid) : uid(uid) {
    }
    position_uid(int position, UID uid) : position(position), uid(uid) {
    }
    position_uid(player &u, root_item ri);

};

struct root_item {
    item *root = nullptr;   // root item
    item *it = nullptr;     // nested item

    root_item() = default;
    root_item(item *root, item *it);
    root_item(player &u, position_uid pu);

    std::vector<item *> find_parents();
};

#endif
