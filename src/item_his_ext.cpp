#include "item.h"

#include "itype.h"

bool item::is_uid( UID uid ) const
{
    return uid >= UID_MIN && this->uid == uid;
}

UID item::get_uid()
{
    if( uid == UID_NONE ) {
        uid = generate_uid();
    }
    return uid;
}

item *item::find_item( UID uid )
{
    // just speed optimization
    if( uid < UID_MIN ) {
        return nullptr;
    }

    if( is_uid( uid ) ) {
        return this;
    }

    for( auto &it : contents ) {
        item *found = it.find_item( uid );
        if( found != nullptr ) {
            return found;
        }
    }

    return nullptr;
}

bool item::find_parents( UID uid, std::vector<item *> &parents )
{
    // just speed optimization
    if( uid < UID_MIN ) {
        return false;
    }

    if( is_uid( uid ) ) {
        parents.push_back( this );
        return true;
    }

    for( auto &it : contents ) {
        if( it.find_parents( uid, parents ) ) {
            parents.push_back( this );
            return true;
        }
    }

    return false;
}

bool item::is_container_liquid() const
{
    return !is_container_empty() && contents.front().type->phase == LIQUID;
}

bool item::is_container_liquid( itype_id liquid ) const
{
    return is_container_liquid() && contents.front().typeId() == liquid;
}

bool item::is_container_mixed() const
{
    if( !is_container() || is_container_empty() ) {
        return false;
    }

    const item &it = contents.front();
    if( !it.count_by_charges() && contents.size() > 1 ) {
        return true;
    }

    itype_id id = it.typeId();
    for( auto &it : contents ) {
        if( id != it.typeId() ) {
            return true;
        }
    }

    return false;
}

void item::pull_out( item *payload )
{
    container_stack stack = get_container_stack();

    for( auto iter = stack.begin(); iter != stack.end(); iter++ ) {
        //delete the item if the pointer memory addresses are the same
        item &it = *iter;
        if( payload == &it ) {
            stack.erase( iter );
            break;
        }
    }
}

void item::pull_out( item *payload, long quantity )
{
    if( payload->charges <= quantity ) {
        pull_out( payload );
    } else {
        payload->mod_charges( -quantity );
    }
}

bool item::load_with( item &it, std::string &err )
{
    if( is_container_liquid() ) {
        fill_with( it );
        return true;
    }

    if( !can_load_with( it, err ) ) {
        return false;
    }

    bool tryaddcharges = ( it.charges != -1 && it.count_by_charges() );
    if( tryaddcharges ) {
        for( auto &i : contents ) {
            if( i.merge_charges( it ) ) {
                return true;
            }
        }
    }

    put_in( it );
    return true;
}

bool item::can_load_with( item &it, std::string &err )
{
    auto remaining_capacity = get_container_remaing_capacity();
    auto item_volume = it.volume( false );
    if( remaining_capacity < item_volume ) {
        err = string_format( _( "Your %s can't hold any more %s." ), tname().c_str(), it.tname().c_str() );
        return false;
    }

    if( !it.made_of( material_id( "powder" ) ) ) {
        auto max_size_prec = get_container_max_size() * 4;
        if( max_size_prec > 0_ml ) {
            if( it.count_by_charges() ) {
                item_volume = item_volume / it.charges;
            }
            if( item_volume > max_size_prec ) {
                err = string_format( _( "The %s does not fit into your %s." ), it.tname().c_str(),
                                     tname().c_str() );
                return false;
            }
        }
    }

    return true;
}

container_stack item::get_container_stack()
{
    return container_stack( &contents, this );
}

container_stack item::get_container_stack() const
{
    // HACK: callers could modify items through this
    // TODO: a const version of content_stack is needed
    return const_cast<item*>(this)->get_container_stack();
}

units::volume item::get_container_max_size() const
{
    if( !is_container() ) {
        debugmsg( "Calling get_container_max_size() for item of type %s", type->get_id().c_str() );
        return 0;
    }

    return units::from_milliliter( type->container->max_size );
}

units::volume item::get_container_used_capacity() const
{
    units::volume used = 0_ml;
    for( auto &i : contents ) {
        used += i.volume( false );
    }

    return used;
}

units::volume item::get_container_remaing_capacity() const
{
    return get_container_capacity() - get_container_used_capacity();
}

void item::get_container_content( listcitemref &items ) const
{
    for( auto &it : contents ) {
        items.push_back( it );
    }
}

std::vector<intcitemref> item::get_container_content() const
{
    listcitemref items;
    get_container_content( items );

    items.sort( compare_tname );

    int n = 0;
    std::vector<intcitemref> ret;
    citemref previt = items.front();
    std::string prev = previt.get().tname();
    for( citemref it : items ) {
        std::string name = it.get().tname();
        if( prev != name ) {
            ret.push_back( intcitemref( n, previt ) );
            n = 0;
            prev = name;
            previt = it;
        }
        n++;
    }
    ret.push_back( intcitemref( n, previt ) );

    return ret;
}

void item::get_container_food( listitemref &items )
{
    for( item &it : contents ) {
        if( it.is_food() ) {
            items.push_back( it );
        } else if( !it.is_container_empty() ) {
            it.get_container_food( items );
        }
    }
}

vectoritemref item::get_container_food()
{
    listitemref items;
    get_container_food( items );

    items.sort( compare );
    items.unique( same );

    vectoritemref ret;
    for( item &it : items ) {
        ret.push_back( it );
    }

    return ret;
}

bool item::compare( const item &first, const item &second )
{
    if( first.display_name() != second.display_name() ) {
        return first.display_name() < second.display_name();
    }

    return first.charges < second.charges;
}

bool item::compare_tname( const item &first, const item &second )
{
    return first.tname() < second.tname();
}

bool item::same( const item &first, const item &second )
{
    return first.display_name() == second.display_name();
}

typedef invlets_bitset;

void item::add_content_invlets( invlets_bitset &invlets ) const
{
    if( is_container_mixed() ) {
        for( const item &it : contents ) {
            invlets.set( it.invlet );
            it.add_content_invlets( invlets );
        }
    }
}
