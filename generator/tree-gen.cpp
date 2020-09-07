/** \file
 * Main source file for \ref tree-gen.
 */

#include "tree-gen.hpp"
#include "tree-gen-cpp.hpp"
#include "tree-gen-python.hpp"
#include "parser.hpp"
#include "lexer.hpp"

using namespace tree_gen;

/**
 * Main function for generating the the header and source file for a tree.
 */
int main(
    int argc,
    char *argv[]
) {

    // Check command line and open files.
    if (argc < 4 || argc > 5) {
        std::cerr << "Usage: tree-gen <spec-file> <header-file> <source-file> [python-file]" << std::endl;
        return 1;
    }

    // Create the scanner.
    yyscan_t scanner = nullptr;
    int retcode = yylex_init(&scanner);
    if (retcode) {
        std::cerr << "Failed to construct scanner: " << strerror(retcode) << std::endl;
        return 1;
    }

    // Try to open the file and read it to an internal string.
    auto filename = std::string(argv[1]);
    FILE *fptr = fopen(filename.c_str(), "r");
    if (!fptr) {
        std::cerr << "Failed to open input file " << filename << ": " << strerror(errno) << std::endl;
        return 1;
    }
    yyset_in(fptr, scanner);

    // Do the actual parsing.
    Specification specification;
    retcode = yyparse(scanner, specification);
    if (retcode == 2) {
        std::cerr << "Out of memory while parsing " << filename << std::endl;
        return 1;
    } else if (retcode) {
        std::cerr << "Failed to parse " << filename << std::endl;
        return 1;
    }
    try {
        specification.build();
    } catch (std::exception &e) {
        std::cerr << "Analysis error: " << e.what() << std::endl;
        return 1;
    }

    // Clean up.
    yylex_destroy(scanner);
    fclose(fptr);

    // Generate C++ code.
    cpp::generate(argv[2], argv[3], specification);

    // Generate Python code if requested.
    if (argc >= 5) {
        python::generate(argv[4], specification);
    }

    return 0;
}
