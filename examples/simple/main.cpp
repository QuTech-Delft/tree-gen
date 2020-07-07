#include <iostream>
#include <stdexcept>
#define ASSERT(x) do { if (!(x)) throw std::runtime_error("assertion failed: " #x " is false at " __FILE__ ":" + std::to_string(__LINE__)); } while (0)

// Include the generated file.
#include "directory.hpp"

int main() {

    // Make a new system tree.
    auto system = tree::base::make<directory::System>();

    std::cout << "Dumping an empty system node. The node is incomplete (at "
              << "least one drive required), so a ! is printed in front."
              << std::endl;
    system->dump();
    std::cout << std::endl;
    ASSERT(!system.is_complete());

    // Add a default drive. This should get drive letter A, because
    // primitives::initialize() is specialized to return that for Letters.
    system->drives.add(tree::base::make<directory::Drive>());

    // We have to give it a directory tree as well to complete it.
    system->drives[0]->root_dir = tree::base::make<directory::Directory>();

    std::cout << "Adding a drive with an empty directory tree completes it, "
              << "as the entries in a directory are of type Any and thus can "
              << "be empty." << std::endl;
    system->dump();
    std::cout << std::endl;
    ASSERT(system.is_complete());

    // TODO: this should probably be extended at some point.

    return 0;
}
