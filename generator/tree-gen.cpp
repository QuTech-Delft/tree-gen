/** \file
 * Main source file for \ref tree-gen.
 */

#include "tree-gen.hpp"
#include "tree-gen-cpp.hpp"
#include "tree-gen-python.hpp"
#include "parser.hpp"
#include "lexer.hpp"

namespace tree_gen {

/**
 * Gathers all child nodes, including those in parent classes.
 */
std::vector<Field> Node::all_fields() const {
    std::vector<Field> children = this->fields;
    if (parent) {
        auto from_parent = parent->all_fields();
        children.insert(children.end(), from_parent.begin(), from_parent.end());
    }
    if (order.empty()) {
        return children;
    }
    std::vector<Field> reordered;
    for (const auto &name : order) {
        bool done = false;
        for (auto it = children.begin(); it != children.end(); ++it) {
            if (it->name == name) {
                reordered.push_back(*it);
                children.erase(it);
                done = true;
                break;
            }
        }
        if (!done) {
            throw std::runtime_error("Unknown field in field order: " + name);
        }
    }
    reordered.insert(reordered.end(), children.begin(), children.end());
    return reordered;
}

/**
 * Convenience method for replacing all occurrences of a substring in a string
 * with another string.
 */
std::string replace_all(std::string str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

/**
 * Construct a node with the given snake_case name and class documentation.
 */
NodeBuilder::NodeBuilder(const std::string &name, const std::string &doc) {
    node = std::make_shared<Node>();
    node->snake_case_name = name;
    node->doc = doc;
    node->is_error_marker = false;

    // Generate title case name.
    auto snake_ss = std::stringstream(name);
    auto title_ss = std::ostringstream();
    std::string token;
    while (std::getline(snake_ss, token, '_')) {
        title_ss << (char)std::toupper(token[0]) << token.substr(1);
    }
    node->title_case_name = title_ss.str();
}

/**
 * Marks this node as deriving from the given node type.
 */
NodeBuilder *NodeBuilder::derive_from(std::shared_ptr<Node> parent) {
    node->parent = parent;
    parent->derived.push_back(node);
    return this;
}

/**
 * Adds a child node. `type` should be one of the edge types.
 */
NodeBuilder *NodeBuilder::with_child(
    EdgeType type,
    const std::string &node_name,
    const std::string &name,
    const std::string &doc
) {
    auto child = Field();
    child.type = type;
    child.prim_type = node_name;
    child.py_prim_type = "";
    child.py_multi_type = "";
    child.name = name;
    child.doc = doc;
    child.ext_type = type;
    node->fields.push_back(std::move(child));
    return this;
}

/**
 * Adds a child primitive.
 */
NodeBuilder *NodeBuilder::with_prim(
    const std::string &prim,
    const std::string &name,
    const std::string &doc,
    EdgeType type
) {
    auto child = Field();
    child.type = Prim;
    switch (type) {
        case Maybe:   child.prim_type = "Maybe<" + prim + ">"; break;
        case One:     child.prim_type = "One<" + prim + ">"; break;
        case Any:     child.prim_type = "Any<" + prim + ">"; break;
        case Many:    child.prim_type = "Many<" + prim + ">"; break;
        case OptLink: child.prim_type = "OptLink<" + prim + ">"; break;
        case Link:    child.prim_type = "Link<" + prim + ">"; break;
        default:      child.prim_type = prim; break;
    }
    child.py_prim_type = replace_all(prim, "::", ".");
    auto pos = child.py_prim_type.find_last_of('.');
    if (pos == std::string::npos) {
        child.py_multi_type = "Multi" + child.py_prim_type;
    } else {
        child.py_multi_type = child.py_prim_type.substr(0, pos + 1)
                            + "Multi"
                            + child.py_prim_type.substr(pos + 1);
    }
    child.name = name;
    child.doc = doc;
    child.ext_type = type;
    node->fields.push_back(std::move(child));
    return this;
}

/**
 * Sets the order in which the parameters must appear in the dumps and
 * constructor.
 */
NodeBuilder *NodeBuilder::with_order(std::list<std::string> &&order) {
    node->order = std::move(order);
    return this;
}

/**
 * Indicate that this node marks a recovered parse error.
 */
NodeBuilder *NodeBuilder::mark_error() {
    node->is_error_marker = true;
    return this;
}

/**
 * Sets the source file documentation.
 */
void Specification::set_source_doc(const std::string &doc) {
    source_doc = doc;
}

/**
 * Sets the header file documentation.
 */
void Specification::set_header_doc(const std::string &doc) {
    header_doc = doc;
}

/**
 * Sets the Python file documentation.
 */
void Specification::set_python_doc(const std::string &doc) {
    python_doc = doc;
}

/**
 * Sets the tree namespace.
 */
void Specification::set_tree_namespace(const std::string &name_space) {
    if (!tree_namespace.empty()) {
        throw std::runtime_error("duplicate tree namespace declaration");
    }
    tree_namespace = name_space;
}

/**
 * Sets the support namespace.
 */
void Specification::set_support_namespace(const std::string &name_space) {
    if (!support_namespace.empty()) {
        throw std::runtime_error("duplicate tree namespace declaration");
    }
    support_namespace = name_space;
}

/**
 * Sets the initialization function.
 */
void Specification::set_initialize_function(const std::string &init_fn) {
    if (!initialize_function.empty()) {
        throw std::runtime_error("duplicate initialization function declaration");
    }
    initialize_function = init_fn;
}

/**
 * Sets the serialization/deserialization functions.
 */
void Specification::set_serdes_functions(const std::string &ser_fn, const std::string &des_fn) {
    if (!serialize_fn.empty()) {
        throw std::runtime_error("duplicate serialize/deserialize function declaration");
    }
    serialize_fn = ser_fn;
    py_serialize_fn = replace_all(ser_fn, "::", ".");
    deserialize_fn = des_fn;
    py_deserialize_fn = replace_all(des_fn, "::", ".");
}

/**
 * Sets the source location object.
 */
void Specification::set_source_location(const std::string &ident) {
    if (!source_location.empty()) {
        throw std::runtime_error("duplicate source location object declaration");
    }
    source_location = ident;
}

/**
 * Adds an include statement to the header file.
 */
void Specification::add_include(const std::string &include) {
    includes.push_back(include);
}

/**
 * Adds an include statement to the source file.
 */
void Specification::add_src_include(const std::string &include) {
    src_includes.push_back(include);
}

/**
 * Adds an import statement to the Python file.
 */
void Specification::add_python_include(const std::string &include) {
    python_includes.push_back(include);
}

/**
 * Adds a namespace level.
 */
void Specification::add_namespace(const std::string &name_space, const std::string &doc) {
    namespaces.push_back(name_space);
    if (!doc.empty()) {
        namespace_doc = doc;
    }
}

/**
 * Adds the given node.
 */
void Specification::add_node(std::shared_ptr<NodeBuilder> &node_builder) {
    auto name = node_builder->node->snake_case_name;
    if (builders.count(name)) {
        throw std::runtime_error("duplicate node name " + name);
    }
    builders.insert(std::make_pair(name, node_builder));
}

/**
 * Checks for errors, resolves node names, and builds the nodes vector.
 */
void Specification::build() {
    if (initialize_function.empty()) {
        throw std::runtime_error("initialization function not specified");
    }
    if (support_namespace.empty()) {
        support_namespace = "::tree";
    }
    for (auto &it : builders) {
        for (auto &child : it.second->node->fields) {
            if (child.type != Prim) {
                auto name = child.prim_type;
                child.prim_type = "";
                auto nb_it = builders.find(name);
                if (nb_it == builders.end()) {
                    throw std::runtime_error("use of undefined node " + name);
                }
                child.node_type = nb_it->second->node;
            }
        }
        nodes.push_back(it.second->node);
    }
}

}

/**
 * Main function for generating the the header and source file for a tree.
 */
int main(
    int argc,
    char *argv[]
) {
    using namespace tree_gen;

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
