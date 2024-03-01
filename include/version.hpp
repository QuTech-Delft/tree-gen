#pragma once

#include <string_view>


#define TREE_GEN_VERSION "1.0.7"
#define TREE_GEN_RELEASE_YEAR "2024"


static std::string_view get_version();
static std::string_view get_release_year();
