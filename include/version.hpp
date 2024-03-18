#pragma once


static const char* tree_gen_version{ "1.0.7" };
static const char* tree_gen_release_year{ "2024" };


[[nodiscard]] [[maybe_unused]] static const char* get_version() {
    return tree_gen_version;
}

[[nodiscard]] [[maybe_unused]] static const char* get_release_year() {
    return tree_gen_release_year;
}
