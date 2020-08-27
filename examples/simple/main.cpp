#include <iostream>
#include <stdexcept>
#define ASSERT(x) do { if (!(x)) throw std::runtime_error("assertion failed: " #x " is false at " __FILE__ ":" + std::to_string(__LINE__)); } while (0)

// Include the generated file.
#include "directory.hpp"

int main() {

    // Make a new system tree.
    auto system = tree::base::make<directory::System>();

    std::cout << "Dumping an empty system node. The tree is not well-formed at "
              << "this time." << std::endl;
    system->dump();
    std::cout << std::endl;
    ASSERT(!system.is_well_formed());

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
    ASSERT(system.is_well_formed());

    // We can just change the drive letter by assignment, as you would expect.
    system->drives[0]->letter = 'C';

    // Before we add files and directories, let's make a shorthand variable for
    // the root directory. Because root_dir is an edge to another node rather
    // than the node itself, and thus acts like a pointer or reference to it,
    // we can just copy it into a variable and modify the variable to update the
    // tree. Note that you'd normally just write "auto" for the type for
    // brevity; the type is shown here to make explicit what it turns into.
    directory::One<directory::Directory> root = system->drives[0]->root_dir;

    // Let's make a "Program Files" subdirectory and add it to the root.
    auto programs = tree::base::make<directory::Directory>(
        tree::base::Any<directory::Entry>{},
        "Program Files");
    root->entries.add(programs);
    ASSERT(system.is_well_formed());

    // That's quite verbose. But in most cases it can be written way shorter.
    // Here's the same with the less versatile but also less verbose emplace()
    // method (which avoids the tree::make() call, but doesn't allow an
    // insertion index to be specified) and with a "using namespace" for the
    // tree. emplace() can also be chained, allowing multiple files and
    // directories to be added at once in this case.
    {
        using namespace directory;

        root->entries.emplace<Directory>(Any<Entry>{}, "Windows")
                     .emplace<Directory>(Any<Entry>{}, "Users")
                     .emplace<File>("lots of hibernation data", "hiberfil.sys")
                     .emplace<File>("lots of page file data", "pagefile.sys")
                     .emplace<File>("lots of swap data", "swapfile.sys");
    }
    ASSERT(system.is_well_formed());

    // In order to look for a file in a directory, you'll have to make your own
    // function to iterate over them. After all, tree-gen doesn't know that the
    // name field is a key; it has no concept of a key-value store. This is
    // simple enough to make, but to prevent this example from getting out of
    // hand we'll just use indices for now.

    // Let's try to read the "lots of data" string from pagefile.sys.
    ASSERT(root->entries[4]->name == "pagefile.sys");
    // ASSERT(root->entries[4]->contents == "lots of page file data");
    //  '-> No member named 'contents' in 'directory::Entry'

    // Hm, that didn't work so well, because C++ doesn't know that entry 4
    // happens to be a file rather than a directory or a mount. We have to tell
    // it to cast to a file first (which throws a std::bad_cast if it's not
    // actually a file). The easiest way to do that is like this:
    ASSERT(root->entries[4]->as_file()->contents == "lots of page file data");
    // No verbose casts required; tree-gen will make member functions for all
    // the possible subtypes.

    // While it's possible to put the same node in a tree twice (without using
    // a link), this is not allowed. This isn't checked until a well-formedness
    // check is performed, however (and in fact can't be without having access
    // to the root node).
    root->entries.add(root->entries[0]);
    ASSERT(!system.is_well_formed());

    // Note that we can index nodes Python-style with negative integers for
    // add() and remove(), so remove(-1) will remove the broken node we just
    // added. Note that the -1 is not actually necessary, though, as it is the
    // default.
    root->entries.remove(-1);
    ASSERT(system.is_well_formed());

    // We *can*, of course, add copies of nodes. That's what copy (shallow) and
    // clone (deep) are for. Usually you'll want a deep copy, but in this case
    // shallow is fine, because a File has no child nodes.
    root->entries.add(root->entries[0]->copy());
    ASSERT(system.is_well_formed());

    // Note that the generated classes don't care that there are now two
    // directories named Program Files in the root. As far as they're concerned,
    // they're two different directories with the same name. Let's remove it
    // again, though.
    root->entries.remove();

    // Something we haven't looked at yet are links. Links are edges in the tree
    // that, well, turn it into something that isn't strictly a tree anymore.
    // While One/Maybe/Any/Many require that nodes are unique, Link/OptLink
    // require that they are *not* unique, and are present elsewhere in the
    // tree. Let's make a new directory, and mount it in the Users directory.
    auto user_dir = tree::base::make<directory::Directory>(
        tree::base::Any<directory::Entry>{},
        "");
    root->entries.emplace<directory::Mount>(user_dir, "SomeUser");

    // Note that user_dir is not yet part of the tree. emplace works simply
    // because it doesn't check whether the directory is in the tree yet. But
    // the tree is no longer well-formed now.
    ASSERT(!system.is_well_formed());

    // To make it work again, we can add it as a root directory to a second
    // drive.
    system->drives.emplace<directory::Drive>('D', user_dir);
    ASSERT(system.is_well_formed());

    // A good way to confuse a filesystem is to make loops. tree-gen is okay
    // with this, though.
    system->drives[1]->root_dir->entries.emplace<directory::Mount>(root, "evil link to C:");
    ASSERT(system.is_well_formed());

    // The only place where it matters is in the dump function, which only goes
    // one level deep. After that, it'll just print an ellipsis.
    std::cout << "After continuing to build the tree:" << std::endl;
    system->dump();
    std::cout << std::endl;

    // TODO: serialization and deserialization.

    return 0;
}
