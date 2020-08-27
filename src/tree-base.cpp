#include "tree-base.hpp"

namespace tree {
namespace base {

/**
 * Internal implementation for add(), given only the raw pointer and the
 * name of its type for the error message.
 */
size_t PointerMap::add_raw(const void *ptr, const char *name) {
    size_t sequence = map.size();
    if (map.find(ptr) != map.end()) {
        std::ostringstream ss{};
        ss << "Duplicate node of type " << name;
        ss << "at address " << std::hex << ptr << " found in tree";
        throw NotWellFormed(ss.str());
    }
    map.emplace(ptr, sequence);
    return sequence;
}

/**
 * Internal implementation for get(), given only the raw pointer and the
 * name of its type for the error message.
 */
size_t PointerMap::get_raw(const void *ptr, const char *name) const {
    auto it = map.find(ptr);
    if (it == map.end()) {
        std::ostringstream ss{};
        ss << "Link to node of type " << name;
        ss << "at address " << std::hex << ptr << " not found in tree";
        throw NotWellFormed(ss.str());
    }
    return it->second;
}

/**
 * Checks whether the tree starting at this node is well-formed. That is:
 *  - all One, Link, and Many edges have (at least) one entry;
 *  - all the One entries internally stored by Any/Many have an entry;
 *  - all Link and filled OptLink nodes link to a node that's reachable from
 *    this node;
 *  - the nodes referred to be One/Maybe only appear once in the tree
 *    (except through links).
 * If it isn't well-formed, a NotWellFormed exception is thrown.
 */
void Completable::check_well_formed() const {
    PointerMap map{};
    find_reachable(map);
    check_complete(map);
}

/**
 * Returns whether the tree starting at this node is well-formed. That is:
 *  - all One, Link, and Many edges have (at least) one entry;
 *  - all the One entries internally stored by Any/Many have an entry;
 *  - all Link and filled OptLink nodes link to a node that's reachable from
 *    this node;
 *  - the nodes referred to be One/Maybe only appear once in the tree
 *    (except through links).
 */
bool Completable::is_well_formed() const {
    try {
        check_well_formed();
        return true;
    } catch (NotWellFormed &e) {
        return false;
    }
}

}
}
