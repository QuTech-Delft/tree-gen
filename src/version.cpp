#include "version.hpp"


/* static */ std::string_view get_version() {
    return TREE_GEN_VERSION;
}

/* static */ static std::string_view get_release_year() {
    return TREE_GEN_RELEASE_YEAR;
}
