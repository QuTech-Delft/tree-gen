#include "version.hpp"


static std::string_view tree_gen_version{ "1.0.6" };
static std::string_view tree_gen_release_year{ "2024" };


/* static */ std::string_view get_version() {
    return tree_gen_version;
}

/* static */ static std::string_view get_release_year() {
    return tree_gen_release_year;
}
