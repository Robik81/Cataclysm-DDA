#include "item_his.h"

#include "item.h"
#include "player.h"

position_uid::position_uid(player &u, root_item ri)
{
    assert(ri.root != nullptr);
    assert(ri.it != nullptr);

    position = u.get_item_position(ri.root);
    uid = ri.it->get_uid();
}

root_item::root_item(item *root, item *it)
{
    this->root = root;
    this->it = it;
}

root_item::root_item(player &u, position_uid pu)
{
    assert(pu.uid >= UID_MIN);

    root = &u.i_at(pu.position);
    it = root->find_item(pu.uid);
}

std::vector<item *> root_item::find_parents()
{
    assert(root != nullptr);
    assert(it != nullptr);

    std::vector<item *> parents;
    root->find_parents(it->get_uid(), parents);
    return parents;
}

std::list<item>::iterator container_stack::erase(std::list<item>::iterator it)
{
    return container->contents.erase(it);
}

void container_stack::push_back(const item &newitem)
{
    container->contents.push_back(newitem);
}

void container_stack::insert_at(std::list<item>::iterator index,
    const item &newitem)
{
    container->contents.insert(index, newitem);
}

units::volume container_stack::max_volume() const
{
    return container->get_container_capacity();
}
