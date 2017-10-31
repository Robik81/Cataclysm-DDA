#pragma once
#ifndef HYBRID_INV_H
#define HYBRID_INV_H

#include "cursesdef.h"
#include "enums.h"
#include "units.h"
#include "rng.h"

#include <string>
#include <array>
#include <list>
#include <vector>
#include <map>
#include <functional>

enum aim_location;
enum advanced_inv_sortby;
struct sort_case_insensitive_less;
struct advanced_inv_listitem;
class hybrid_inventory_pane;

void hybrid_inv();

/**
 * Defines the source of item stacks.
 */
struct hybrid_inv_area {
    const aim_location id;
    // Used for the small overview 3x3 grid
    int hscreenx = 0;
    int hscreeny = 0;
    // relative (to the player) position of the map point
    tripoint off;
    /** Long name, displayed, translated */
    const std::string name = "fake";
    /** Shorter name (2 letters) */
    const std::string shortname = "FK"; // FK in my coffee
    // absolute position of the map point.
    tripoint pos;
    /** Can we put items there? Only checks if location is valid, not if
        selected container in pane is. For full check use canputitems() **/
    bool canputitemsloc;
    // vehicle pointer and cargo part index
    vehicle *veh;
    int vstor;
    // description, e.g. vehicle name, label, or terrain
    std::array<std::string, 2> desc;
    // flags, e.g. FIRE, TRAP, WATER
    std::string flags;
    // total volume and weight of items currently there
    units::volume volume;
    units::mass weight;
    // maximal count / volume of items there.
    int max_size;

    hybrid_inv_area( aim_location id ) : id( id ) {}
    hybrid_inv_area( aim_location id, int hscreenx, int hscreeny, tripoint off, std::string name,
                       std::string shortname ) : id( id ), hscreenx( hscreenx ),
        hscreeny( hscreeny ), off( off ), name( name ), shortname( shortname ), pos( 0, 0, 0 ),
        canputitemsloc( false ), veh( nullptr ), vstor( -1 ), volume( 0 ), weight( 0 ),
        max_size( 0 ) {
    }

    void init();
    void init( hybrid_inventory_pane & pane );
    // if you want vehicle cargo, specify so via `in_vehicle'
    units::volume free_volume( hybrid_inventory_pane &pane, bool in_vehicle = false ) const;
    int get_item_count() const;
    // Other area is actually the same item source, e.g. dragged vehicle to the south and AIM_SOUTH
    bool is_same( const hybrid_inv_area &other ) const;
    // does _not_ check vehicle storage, do that with `can_store_in_vehicle()' below
    bool canputitems();
    bool canputitems( hybrid_inventory_pane &pane );
    // if you want vehicle cargo, specify so via `in_vehicle'
    item *get_container( hybrid_inventory_pane &pane );
    units::volume get_container_remaing_capacity( hybrid_inventory_pane &pane ) const;
    void find_parents( hybrid_inventory_pane &pane, std::vector<item *> &parents ) const;
    void set_container( hybrid_inventory_pane &pane );
    void set_container_position( hybrid_inventory_pane &pane );
    aim_location offset_to_location() const;
    bool can_store_in_vehicle() const {
        // disallow for non-valid vehicle locations
        if( id > AIM_DRAGGED || id < AIM_SOUTHWEST ) {
            return false;
        }
        return ( veh != nullptr && vstor >= 0 );
    }
};

// see item_factory.h
class item_category;

/**
 * Displayed pane, what is shown on the screen.
 */
class hybrid_inventory_pane
{
    private:
        aim_location area = NUM_AIM_LOCATIONS_HIS;
        aim_location prev_area = area;
        // pointer to the square this pane is pointing to
        bool viewing_cargo = false;
        bool prev_viewing_cargo = false;
    public:
        // set the pane's area via its square, and whether it is viewing a vehicle's cargo
        void set_area( hybrid_inv_area &square, bool in_vehicle_cargo = false ) {
            prev_area = area;
            prev_viewing_cargo = viewing_cargo;
            area = square.id;
            viewing_cargo = square.can_store_in_vehicle() && in_vehicle_cargo;
        }
        void restore_area() {
            area = prev_area;
            viewing_cargo = prev_viewing_cargo;
        }
        aim_location get_area() const {
            return area;
        }
        bool prev_in_vehicle() const {
            return prev_viewing_cargo;
        }
        bool in_vehicle() const {
            return viewing_cargo;
        }
        bool on_ground() const {
            return area > AIM_INVENTORY && area < AIM_DRAGGED;
        }
        /**
         * Index of the selected item (index of @ref items),
         */
        int index;
        advanced_inv_sortby sortby;
        catacurses::window window;
        std::vector<advanced_inv_listitem> items;
        /**
         * The current filter string.
         */
        std::string filter;
        /**
         * Whether to recalculate the content of this pane.
         * Implies @ref redraw.
         */
        bool recalc;
        /**
         * Whether to redraw this pane.
         */
        bool redraw;

        void add_items_from_area( hybrid_inv_area &square, bool vehicle_override = false );
        /**
         * Makes sure the @ref index is valid (if possible).
         */
        void fix_index();
        /**
         * @param it The item to check, oly the name member is examined.
         * @return Whether the item should be filtered (and not shown).
         */
        bool is_filtered( const advanced_inv_listitem &it ) const;
        /**
         * Same as the other, but checks the real item.
         */
        bool is_filtered( const item &it ) const;
        /**
         * Scroll @ref index, by given offset, set redraw to true,
         * @param offset Must not be 0.
         */
        void scroll_by( int offset );
        /**
         * Scroll the index in category mode by given offset.
         * @param offset Must be either +1 or -1
         */
        void scroll_category( int offset );
        /**
         * @return either null, if @ref index is invalid, or the selected
         * item in @ref items.
         */
        advanced_inv_listitem *get_cur_item_ptr();
        /**
         * Set the filter string, disables filtering when the filter string is empty.
         */
        void set_filter( const std::string &new_filter );
        /**
         * Insert additional category headers on the top of each page.
         */
        void paginate( size_t itemsPerPage );

        int container_location;
        UID container_uid;
        std::string container_desc;
        void reset_container();
        // returns either area, if not AIM_ALL, or uistate.adv_inv_last_popup_dest
        aim_location destination() const;

    private:
        /** Scroll to next non-header entry */
        void skip_category_headers( int offset );
        /** Only add offset to index, but wrap around! */
        void mod_index( int offset );

        mutable std::map<std::string, std::function<bool( const item & )>> filtercache;
};

class hybrid_inventory
{
    public:
        hybrid_inventory();
        ~hybrid_inventory();

        void display();
    private:
        /**
         * Refers to the two panes, used as index into @ref panes.
         */
        enum side {
            left  = 0,
            right = 1,
            NUM_PANES = 2
        };
        const int head_height;
        const int min_w_height;
        const int min_w_width;
        const int max_w_width;

        // swap the panes and windows via std::swap()
        void swap_panes();

        // minimap that displays things around character
        catacurses::window minimap;
        catacurses::window mm_border;
        const int minimap_width  = 3;
        const int minimap_height = 3;
        void draw_minimap();
        void refresh_minimap();
        char get_minimap_sym( side p ) const;

        bool inCategoryMode;

        int itemsPerPage;
        int w_height;
        int w_width;

        int headstart;
        int colstart;

        bool recalc;
        bool redraw;
        /**
         * Which panels is active (item moved from there).
         */
        side src;
        /**
         * Which panel is the destination (items want to go to there).
         */
        side dest;
        /**
         * True if (and only if) the filter of the active panel is currently
         * being edited.
         */
        bool filter_edit;
        /**
         * Two panels (left and right) showing the items, use a value of @ref side
         * as index.
         */
        std::array<hybrid_inventory_pane, NUM_PANES> panes;
        static const hybrid_inventory_pane null_pane;
        std::array<hybrid_inv_area, NUM_AIM_LOCATIONS_HIS> squares;

        catacurses::window head;
        catacurses::window left_window;
        catacurses::window right_window;

        bool exit;

        // store/load settings (such as index, filter, etc)
        void save_settings( bool only_panes );
        void load_settings();
        // used to return back to AIM when other activities queued are finished
        void do_return_entry();
        // returns true if currently processing a routine
        // (such as `MOVE_ALL_ITEMS' with `AIM_ALL' source)
        bool is_processing() const;

        static std::string get_sortname( advanced_inv_sortby sortby );
        bool move_all_items( bool nested_call = false );
        void print_items( hybrid_inventory_pane &pane, bool active );
        void recalc_pane( side p );
        void redraw_pane( side p );
        // Returns the x coordinate where the header started. The header is
        // displayed right of it, everything left of it is till free.
        int print_header( hybrid_inventory_pane &pane, aim_location sel );
        void init();
        /**
         * Translate an action ident from the input context to an aim_location.
         * @param action Action ident to translate
         * @param ret If the action ident referred to a location, its id is stored
         * here. Only valid when the function returns true.
         * @return true if the action did refer to an location (which has been
         * stored in ret), false otherwise.
         */
        static bool get_square( const std::string action, aim_location &ret );
        /**
         * Show the sort-by menu and change the sorting of this pane accordingly.
         * @return whether the sort order was actually changed.
         */
        bool show_sort_menu( hybrid_inventory_pane &pane );
        /**
         * Checks whether one can put items into the supplied location.
         * If the supplied location is AIM_ALL, query for the actual location
         * (stores the result in def) and check that destination.
         * @return false if one can not put items in the destination, true otherwise.
         * The result true also indicates the def is not AIM_ALL (because the
         * actual location has been queried).
         */
        bool query_destination( aim_location &def );
        /**
         * Add the item to the destination area.
         * @param destarea Where add the item to. This must not be AIM_ALL.
         * @param new_item The item to add.
         * @param count The amount to add items to add.
         * @return Returns the amount of items that weren't addable, 0 if everything went fine.
         */
        int add_item( aim_location destarea, item &new_item, int count = 1 );
        /**
         * Remove the item from source area. Must not be used on items with area
         *      AIM_ALL or AIM_INVENTORY!
         * @param sitem The item reference that should be removed, along with the source area.
         * @param count The amount to move of said item.
         * @return Returns the amount of items that weren't removable, 0 if everything went fine.
         */
        int remove_item( advanced_inv_listitem &sitem, int count = 1 );
        /**
         * Move content of source container into destination container (destination pane = AIM_CONTAINER)
         * @param src Source container
         * @param dest Destination container
         */
        bool move_content( item &src_container, item &dest_container, long amount_to_move );
        item *get_container( hybrid_inventory_pane &pane );
        long add_to_container( item *container, item *it, long amount );
        void remove_from_container( advanced_inv_listitem &sitem, item *container, item *it, long amount );
        void remove_charges_or_item( advanced_inv_listitem &sitem, item *it, long amount );
        /**
          * Move item into destination (source and / or destination pane = AIM_CONTAINER)
          * Internally calls move_content for moving liquids if necessary
          * @param spane Source pane
          * @param dpane Destination pane
          * @param amount_to_move Amount to move
          */
        bool move_item( hybrid_inventory_pane &spane, hybrid_inventory_pane &dpane,
                        long amount_to_move );
        /**
         * Unloads content from selected container
         * @param spane Source pane
         * @param dpane Destination pane
         */
        bool unload_content( hybrid_inventory_pane &spane, hybrid_inventory_pane &dpane );
        /**
          * Checks if we are moving liquid or not
          */
        bool moving_liquid( const advanced_inv_listitem &sitem, hybrid_inventory_pane &dpane );
        bool moving_liquid( const item &src, hybrid_inventory_pane &dpane );
        bool same_pane( hybrid_inventory_pane &spane, hybrid_inventory_pane &dpane ) const;
        /**
         * Setup how many items/charges (if counted by charges) should be moved.
         * @param dpane Where to move to. This must not be AIM_ALL.
         * @param sitem The source item, it must contain a valid reference to an item!
         * @param action The action we are querying
         * @param amount The input value is ignored, contains the amount that should
         *      be moved. Only valid if this returns true.
         * @return false if nothing should/can be moved. True only if there can and
         *      should be moved. A return value of true indicates that amount now contains
         *      a valid item count to be moved.
         */
        bool query_charges( hybrid_inventory_pane &dpane, const advanced_inv_listitem &sitem,
                            const std::string &action, long &amount );

        void menu_square( uilist &menu );

        static char get_location_key( aim_location area );
        static char get_direction_key( aim_location area );

        /**
         * Converts from screen relative location to game-space relative location
         * for control rotation in isometric mode.
        */
        static aim_location screen_relative_location( aim_location area );
};

#endif
