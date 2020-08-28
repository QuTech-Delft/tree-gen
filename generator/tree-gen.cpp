/** \file
 * Source file for \ref tree-gen.
 */

#include <fstream>
#include <iostream>
#include <cctype>
#include <unordered_set>
#include "tree-gen.hpp"
#include "parser.hpp"
#include "lexer.hpp"

namespace tree_gen {

/**
 * Formats a docstring.
 */
static void format_doc(
    std::ofstream &stream,
    const std::string &doc,
    const std::string &indent = "",
    const std::string &annotation = ""
) {
    stream << indent << "/**";
    if (!annotation.empty()) {
        stream << " " << annotation;
    }
    stream << std::endl;
    auto word = std::ostringstream();
    auto line = std::ostringstream();
    line << indent << " *";
    bool line_empty = true;
    for (char c : doc) {
        bool flush = false;
        if (c == '\n') {
            line << " " << word.str();
            word.str("");
            flush = true;
        } else if (c == ' ') {
            line << " " << word.str();
            line_empty = false;
            word.str("");
        } else {
            word << c;
            flush = !line_empty && line.str().size() + word.str().size() > 79;
        }
        if (flush) {
            stream << line.str() << std::endl;
            line.str("");
            line << indent << " *";
            line_empty = true;
        }
    }
    if (!word.str().empty()) {
        line << " " << word.str();
        line_empty = false;
    }
    if (!line_empty) {
        stream << line.str() << std::endl;
    }
    stream << indent << " */" << std::endl;
}

/**
 * Generates the node type enumeration.
 */
static void generate_enum(
    std::ofstream &header,
    Nodes &nodes
) {

    // Gather the leaf types.
    std::vector<std::string> variants;
    for (auto &node : nodes) {
        if (node->derived.empty()) {
            variants.push_back(node->title_case_name);
        }
    }

    // Print the enum.
    format_doc(header, "Enumeration of all node types.");
    header << "enum class NodeType {" << std::endl;
    for (size_t i = 0; i < variants.size(); i++) {
        header << "    " << variants[i];
        if (i < variants.size() - 1) {
            header << ",";
        }
        header << std::endl;
    }
    header << "};" << std::endl << std::endl;

}

/**
 * Generates an `as_<type>` function.
 */
static void generate_typecast_function(
    std::ofstream &header,
    std::ofstream &source,
    const std::string &clsname,
    NodeType &into,
    bool allowed
) {
    for (int constant = 0; constant < 2; constant++) {
        std::string doc = "Interprets this node to a node of type "
                          + into.title_case_name
                          + ". Returns null if it has the wrong type.";
        format_doc(header, doc, "    ");
        header << "    ";
        if (!allowed) header << "virtual ";
        if (constant) header << "const ";
        header << into.title_case_name << " *";
        header << "as_" << into.snake_case_name << "()";
        if (constant) header << " const";
        if (allowed) header << " override";
        header << ";" << std::endl << std::endl;
        format_doc(source, doc);
        if (constant) source << "const ";
        source << into.title_case_name << " *";
        source << clsname << "::as_" << into.snake_case_name << "()";
        if (constant) source << " const";
        source << " {" << std::endl;
        if (allowed) {
            source << "    return dynamic_cast<";
            if (constant) source << "const ";
            source << into.title_case_name << "*>(this);" << std::endl;
        } else {
            source << "    return nullptr;" << std::endl;
        }
        source << "}" << std::endl << std::endl;
    }
}

/**
 * Generates the base class for the nodes.
 */
static void generate_base_class(
    std::ofstream &header,
    std::ofstream &source,
    Nodes &nodes,
    bool with_serdes
) {

    format_doc(header, "Main class for all nodes.");
    header << "class Node : public Base {" << std::endl;
    header << "public:" << std::endl << std::endl;

    format_doc(header, "Returns the `NodeType` of this node.", "    ");
    header << "    virtual NodeType type() const = 0;" << std::endl << std::endl;

    format_doc(header, "Returns a shallow copy of this node.", "    ");
    header << "    virtual One<Node> copy() const = 0;" << std::endl << std::endl;

    format_doc(header, "Returns a deep copy of this node.", "    ");
    header << "    virtual One<Node> clone() const = 0;" << std::endl << std::endl;

    format_doc(header, "Equality operator. Ignores annotations!", "    ");
    header << "    virtual bool operator==(const Node& rhs) const = 0;" << std::endl << std::endl;

    format_doc(header, "Inequality operator. Ignores annotations!", "    ");
    header << "    inline bool operator!=(const Node& rhs) const {" << std::endl;
    header << "        return !(*this == rhs);" << std::endl;
    header << "    }" << std::endl << std::endl;

    format_doc(header, "Visit this object.", "    ");
    header << "    virtual void visit(Visitor &visitor) = 0;" << std::endl << std::endl;

    format_doc(header, "Writes a debug dump of this node to the given stream.", "    ");
    header << "    void dump(std::ostream &out=std::cout, int indent=0);" << std::endl << std::endl;
    format_doc(source, "Writes a debug dump of this node to the given stream.");
    source << "void Node::dump(std::ostream &out, int indent) {" << std::endl;
    source << "    auto dumper = Dumper(out, indent);" << std::endl;
    source << "    visit(dumper);" << std::endl;
    source << "}" << std::endl << std::endl;

    for (auto &node : nodes) {
        generate_typecast_function(header, source, "Node", *node, false);
    }

    if (with_serdes) {
        format_doc(header, "Serializes this node to the given map.", "    ");
        header << "    virtual void serialize(" << std::endl;
        header << "        ::tree::cbor::MapWriter &map," << std::endl;
        header << "        const ::tree::base::PointerMap &ids" << std::endl;
        header << "    ) const = 0;" << std::endl << std::endl;

        format_doc(header, "Deserializes the given node.", "    ");
        header << "    static std::shared_ptr<Node> deserialize(" << std::endl;
        header << "         const ::tree::cbor::MapReader &map," << std::endl;
        header << "         ::tree::base::IdentifierMap &ids" << std::endl;
        header << "    );" << std::endl << std::endl;
        format_doc(source, "Writes a debug dump of this node to the given stream.");
        source << "std::shared_ptr<Node> Node::deserialize(" << std::endl;
        source << "    const ::tree::cbor::MapReader &map," << std::endl;
        source << "    ::tree::base::IdentifierMap &ids" << std::endl;
        source << ") {" << std::endl;
        source << "    auto type = map.at(\"@t\").as_string();" << std::endl;
        for (auto &node : nodes) {
            if (node->derived.empty()) {
                source << "    if (type == \"" << node->title_case_name << "\") ";
                source << "return " << node->title_case_name << "::deserialize(map, ids);" << std::endl;
            }
        }
        source << "    throw std::runtime_error(\"Schema validation failed: unexpected node type \" + type);" << std::endl;
        source << "}" << std::endl << std::endl;
    }

    header << "};" << std::endl << std::endl;

}

/**
 * Recursive function to print a muxing if statement for all node classes
 * derived from the given node class.
 */
void generate_deserialize_mux(
    std::ofstream &source,
    NodeType &node
) {
    if (node.derived.empty()) {
        source << "    if (type == \"" << node.title_case_name << "\") ";
        source << "return " << node.title_case_name << "::deserialize(map, ids);" << std::endl;
    } else {
        for (auto &derived : node.derived) {
            generate_deserialize_mux(source, *(derived.lock()));
        }
    }
}

/**
 * Generates the class for the given node.
 */
static void generate_node_class(
    std::ofstream &header,
    std::ofstream &source,
    Specification &spec,
    NodeType &node
) {
    const auto all_children = node.all_children();

    // Print class header.
    if (!node.doc.empty()) {
        format_doc(header, node.doc);
    }
    header << "class " << node.title_case_name << " : public ";
    if (node.parent) {
         header << node.parent->title_case_name;
    } else {
         header << "Node";
    }
    header << " {" << std::endl;
    header << "public:" << std::endl << std::endl;

    // Print children.
    for (auto &child : node.children) {
        if (!child.doc.empty()) {
            format_doc(header, child.doc, "    ");
        }
        header << "    ";
        switch (child.type) {
            case Maybe:   header << "Maybe<"   << child.node_type->title_case_name << "> "; break;
            case One:     header << "One<"     << child.node_type->title_case_name << "> "; break;
            case Any:     header << "Any<"     << child.node_type->title_case_name << "> "; break;
            case Many:    header << "Many<"    << child.node_type->title_case_name << "> "; break;
            case OptLink: header << "OptLink<" << child.node_type->title_case_name << "> "; break;
            case Link:    header << "Link<"    << child.node_type->title_case_name << "> "; break;
            case Prim:    header << child.prim_type << " "; break;
        }
        header << child.name << ";" << std::endl << std::endl;
    }

    // Print constructors.
    if (!all_children.empty()) {
        format_doc(header, "Constructor.", "    ");
        header << "    " << node.title_case_name << "(";
        bool first = true;
        for (auto &child : all_children) {
            if (first) {
                first = false;
            } else {
                header << ", ";
            }
            header << "const ";
            switch (child.type) {
                case Maybe:   header << "Maybe<"   << child.node_type->title_case_name << "> "; break;
                case One:     header << "One<"     << child.node_type->title_case_name << "> "; break;
                case Any:     header << "Any<"     << child.node_type->title_case_name << "> "; break;
                case Many:    header << "Many<"    << child.node_type->title_case_name << "> "; break;
                case OptLink: header << "OptLink<" << child.node_type->title_case_name << "> "; break;
                case Link:    header << "Link<"    << child.node_type->title_case_name << "> "; break;
                case Prim:    header << child.prim_type << " "; break;
            }
            header << "&" << child.name << " = ";
            switch (child.type) {
                case Maybe:   header << "Maybe<"   << child.node_type->title_case_name << ">()"; break;
                case One:     header << "One<"     << child.node_type->title_case_name << ">()"; break;
                case Any:     header << "Any<"     << child.node_type->title_case_name << ">()"; break;
                case Many:    header << "Many<"    << child.node_type->title_case_name << ">()"; break;
                case OptLink: header << "OptLink<" << child.node_type->title_case_name << ">()"; break;
                case Link:    header << "Link<"    << child.node_type->title_case_name << ">()"; break;
                case Prim:    header << spec.initialize_function << "<" << child.prim_type << ">()"; break;
            }
        }
        header << ");" << std::endl << std::endl;

        format_doc(source, "Constructor.", "");
        source << node.title_case_name << "::" << node.title_case_name << "(";
        first = true;
        for (auto &child : all_children) {
            if (first) {
                first = false;
            } else {
                source << ", ";
            }
            source << "const ";
            switch (child.type) {
                case Maybe:   source << "Maybe<"   << child.node_type->title_case_name << "> "; break;
                case One:     source << "One<"     << child.node_type->title_case_name << "> "; break;
                case Any:     source << "Any<"     << child.node_type->title_case_name << "> "; break;
                case Many:    source << "Many<"    << child.node_type->title_case_name << "> "; break;
                case OptLink: source << "OptLink<" << child.node_type->title_case_name << "> "; break;
                case Link:    source << "Link<"    << child.node_type->title_case_name << "> "; break;
                case Prim:    source << child.prim_type << " "; break;
            }
            source << "&" << child.name;
        }
        source << ")" << std::endl << "    : ";
        first = true;
        if (node.parent) {
            source << node.parent->title_case_name << "(";
            first = true;
            for (auto &child : node.parent->all_children()) {
                if (first) {
                    first = false;
                } else {
                    source << ", ";
                }
                source << child.name;
            }
            source << ")";
            first = false;
        }
        for (auto &child : node.children) {
            if (first) {
                first = false;
            } else {
                source << ", ";
            }
            source << child.name << "(" << child.name << ")";
        }
        source << std::endl << "{}" << std::endl << std::endl;
    }

    // Print find_reachable and check_complete functions.
    if (node.derived.empty()) {
        std::string doc = "Registers all reachable nodes with the given PointerMap.";
        format_doc(header, doc, "    ");
        header << "    void find_reachable(::tree::base::PointerMap &map) const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void " << node.title_case_name;
        source << "::find_reachable(::tree::base::PointerMap &map) const {" << std::endl;
        for (auto &child : all_children) {
            switch (child.type) {
                case Maybe:
                case One:
                case Any:
                case Many:
                case OptLink:
                case Link:
                    source << "    " << child.name << ".find_reachable(map);" << std::endl;
                    break;
                default:
                    break;
            }
        }
        source << "}" << std::endl << std::endl;

        doc = "Returns whether this `" + node.title_case_name + "` is complete/fully defined.";
        format_doc(header, doc, "    ");
        header << "    void check_complete(const ::tree::base::PointerMap &map) const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void " << node.title_case_name;
        source << "::check_complete(const ::tree::base::PointerMap &map) const {" << std::endl;
        if (node.is_error_marker) {
            source << "    throw ::tree::base::NotWellFormed(\"" << node.title_case_name << " error node in tree\");" << std::endl;
        } else {
            for (auto &child : all_children) {
                switch (child.type) {
                    case Maybe:
                    case One:
                    case Any:
                    case Many:
                    case OptLink:
                    case Link:
                        source << "    " << child.name << ".check_complete(map);" << std::endl;
                        break;
                    default:
                        break;
                }
            }
        }
        source << "}" << std::endl << std::endl;
    }

    // Print type() function.
    if (node.derived.empty()) {
        auto doc = "Returns the `NodeType` of this node.";
        format_doc(header, doc, "    ");
        header << "    NodeType type() const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "NodeType " << node.title_case_name;
        source << "::type() const {" << std::endl;
        source << "    return NodeType::" << node.title_case_name << ";" << std::endl;
        source << "}" << std::endl << std::endl;
    }

    // Print visitor function.
    if (node.derived.empty()) {
        auto doc = "Visit a `" + node.title_case_name + "` node.";
        format_doc(header, doc, "    ");
        header << "    void visit(Visitor &visitor) override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void " << node.title_case_name;
        source << "::visit(Visitor &visitor) {" << std::endl;
        source << "    visitor.visit_" << node.snake_case_name;
        source << "(*this);" << std::endl;
        source << "}" << std::endl << std::endl;
    }

    // Print conversion function.
    generate_typecast_function(header, source, node.title_case_name, node, true);

    // Print copy method.
    if (node.derived.empty()) {
        auto doc = "Returns a shallow copy of this node.";
        format_doc(header, doc, "    ");
        header << "    One<Node> copy() const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "One<Node> " << node.title_case_name;
        source << "::copy() const {" << std::endl;
        source << "    return ";
        if (!spec.tree_namespace.empty()) {
            source << spec.tree_namespace << "::";
        }
        source << "make<" << node.title_case_name << ">(*this);" << std::endl;
        source << "}" << std::endl << std::endl;
    }

    // Print clone method.
    if (node.derived.empty()) {
        auto doc = "Returns a deep copy of this node.";
        format_doc(header, doc, "    ");
        header << "    One<Node> clone() const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "One<Node> " << node.title_case_name;
        source << "::clone() const {" << std::endl;
        source << "    return ";
        if (!spec.tree_namespace.empty()) {
            source << spec.tree_namespace << "::";
        }
        source << "make<" << node.title_case_name << ">(" << std::endl;
        bool first = true;
        for (auto &child : all_children) {
            if (first) {
                first = false;
            } else {
                source << "," << std::endl;
            }
            source << "        " << child.name;
            switch (child.type) {
                case Maybe:
                case One:
                case Any:
                case Many:
                    source << ".clone()";
                    break;
                case Prim:
                    switch (child.ext_type) {
                        case Maybe:
                        case One:
                        case Any:
                        case Many:
                            source << ".clone()";
                            break;
                        default:
                            break;
                    }
                default:
                    break;
            }
        }
        source << std::endl;
        source << "    );" << std::endl;
        source << "}" << std::endl << std::endl;
    }

    // Print equality operator.
    if (node.derived.empty()) {
        auto doc = "Equality operator. Ignores annotations!";
        format_doc(header, doc, "    ");
        header << "    bool operator==(const Node& rhs) const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "bool " << node.title_case_name;
        source << "::operator==(const Node& rhs) const {" << std::endl;
        source << "    if (rhs.type() != NodeType::" << node.title_case_name << ") return false;" << std::endl;
        if (!all_children.empty()) {
            source << "    auto rhsc = dynamic_cast<const " << node.title_case_name << "&>(rhs);" << std::endl;
            for (auto &child : all_children) {
                source << "    if (this->" << child.name << " != rhsc." << child.name << ") return false;" << std::endl;
            }
        }
        source << "    return true;" << std::endl;
        source << "}" << std::endl << std::endl;
    }

    // Print serdes methods.
    if (!spec.serialize_fn.empty()) {
        if (node.derived.empty()) {
            format_doc(header, "Serializes this node to the given map.", "    ");
            header << "    void serialize(" << std::endl;
            header << "        ::tree::cbor::MapWriter &map," << std::endl;
            header << "        const ::tree::base::PointerMap &ids" << std::endl;
            header << "    ) const override;" << std::endl << std::endl;
            format_doc(source, "Serializes this node to the given map.");
            source << "void " << node.title_case_name << "::serialize(" << std::endl;
            source << "    ::tree::cbor::MapWriter &map," << std::endl;
            source << "    const ::tree::base::PointerMap &ids" << std::endl;
            source << ") const {" << std::endl;
            source << "    map.append_string(\"@t\", \"" << node.title_case_name << "\");" << std::endl;
            bool first = true;
            for (const auto &child : all_children) {
                source << "    ";
                if (first) {
                    source << "auto ";
                    first = false;
                }
                source << "submap = map.append_map(\"" << child.name << "\");" << std::endl;
                if (child.type == Prim && child.ext_type == Prim) {
                    source << "    " << spec.serialize_fn << "<" << child.prim_type << ">";
                    source << "(" << child.name << ", submap);" << std::endl;
                } else {
                    source << "    " << child.name << ".serialize(submap, ids);" << std::endl;
                }
                source << "    submap.close();" << std::endl;
            }
            source << "    serialize_annotations(map);" << std::endl;
            source << "}" << std::endl << std::endl;

            format_doc(header, "Deserializes the given node.", "    ");
            header << "    static std::shared_ptr<" << node.title_case_name << "> ";
            header << "deserialize(const ::tree::cbor::MapReader &map, ::tree::base::IdentifierMap &ids);" << std::endl << std::endl;
            format_doc(source, "Writes a debug dump of this node to the given stream.");
            source << "std::shared_ptr<" << node.title_case_name << "> ";
            source << node.title_case_name << "::deserialize(const ::tree::cbor::MapReader &map, ::tree::base::IdentifierMap &ids) {" << std::endl;
            source << "    auto type = map.at(\"@t\").as_string();" << std::endl;
            source << "    if (type != \"" << node.title_case_name << "\") {" << std::endl;
            source << "        throw std::runtime_error(\"Schema validation failed: unexpected node type \" + type);" << std::endl;
            source << "    }" << std::endl;
            source << "    auto node = std::make_shared<" << node.title_case_name << ">(" << std::endl;
            std::vector<ChildNode> links{};
            first = true;
            for (const auto &child : all_children) {
                if (first) {
                    first = false;
                } else {
                    source << "," << std::endl;
                }
                source << "        ";

                AttributeType type = (child.type != Prim) ? child.type : child.ext_type;
                std::string node_type = (child.type != Prim) ? child.node_type->title_case_name : child.prim_type;
                std::string edge_type;
                switch (type) {
                    case Maybe:   edge_type = "Maybe"; break;
                    case One:     edge_type = "One"; break;
                    case Any:     edge_type = "Any"; break;
                    case Many:    edge_type = "Many"; break;
                    case OptLink: edge_type = "OptLink"; break;
                    case Link:    edge_type = "Link"; break;
                    default:      edge_type = "<undefined>"; break;
                }
                if (type == Prim) {
                    source << spec.deserialize_fn << "<" << child.prim_type << ">";
                    source << "(map.at(\"" << child.name << "\").as_map())";
                } else {
                    source << edge_type << "<" << child.node_type->title_case_name << ">(";
                    source << "map.at(\"" << child.name << "\").as_map(), ids)";
                }
                if (type == OptLink || type == Link) {
                    links.push_back(child);
                }
            }
            source << std::endl;
            source << "    );" << std::endl;
            first = true;
            for (const auto &link : links) {
                source << "    ";
                if (first) {
                    first = false;
                    source << "auto ";
                }
                source << "link = map.at(\"" << link.name << "\").as_map().at(\"@l\");" << std::endl;
                source << "    if (!link.is_null()) {" << std::endl;
                source << "        ids.register_link(node->" << link.name << ", link.as_int());" << std::endl;
                source << "    }" << std::endl;
            }
            source << "    return node;" << std::endl;
            source << "}" << std::endl << std::endl;
        } else {
            format_doc(header, "Deserializes the given node.", "    ");
            header << "    static std::shared_ptr<" << node.title_case_name << "> ";
            header << "deserialize(const ::tree::cbor::MapReader &map, ::tree::base::IdentifierMap &ids);" << std::endl << std::endl;
            format_doc(source, "Writes a debug dump of this node to the given stream.");
            source << "std::shared_ptr<" << node.title_case_name << "> ";
            source << node.title_case_name << "::deserialize(const ::tree::cbor::MapReader &map, ::tree::base::IdentifierMap &ids) {" << std::endl;
            source << "    auto type = map.at(\"@t\").as_string();" << std::endl;
            for (auto &derived : node.derived) {
                generate_deserialize_mux(source, *(derived.lock()));
            }
            source << "    throw std::runtime_error(\"Schema validation failed: unexpected node type \" + type);" << std::endl;
            source << "}" << std::endl << std::endl;
        }
    }

    // Print class footer.
    header << "};" << std::endl << std::endl;
}

// Generate the visitor base class.
static void generate_visitor_base_class(
    std::ofstream &header,
    std::ofstream &source,
    Nodes &nodes
) {

    // Print class header.
    format_doc(
        header,
        "Base class for the visitor pattern for the tree.\n\n"
        "To operate on the tree, derive from this class, describe your "
        "operation by overriding the appropriate visit functions. and then "
        "call `node->visit(your_visitor)`. The default implementations for "
        "the node-specific functions fall back to the more generic functions, "
        "eventually leading to `visit_node()`, which must be implemented with "
        "the desired behavior for unknown nodes.");
    header << "class Visitor {" << std::endl;
    header << "public:" << std::endl << std::endl;

    // Virtual destructor.
    format_doc(header, "Virtual destructor for proper cleanup.", "    ");
    header << "    virtual ~Visitor() {};" << std::endl << std::endl;

    // Fallback for any kind of node.
    format_doc(header, "Fallback function for nodes of any type.", "    ");
    header << "    virtual void visit_node(Node &node) = 0;" << std::endl << std::endl;

    // Functions for all node types.
    for (auto &node : nodes) {
        std::string doc;
        if (node->derived.empty()) {
            doc = "Visitor function for `" + node->title_case_name + "` nodes.";
        } else {
            doc = "Fallback function for `" + node->title_case_name + "` nodes.";
        }
        format_doc(header, doc, "    ");
        header << "    virtual void visit_" << node->snake_case_name;
        header << "(" << node->title_case_name << " &node);" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void Visitor::visit_" << node->snake_case_name;
        source << "(" << node->title_case_name << " &node) {" << std::endl;
        if (node->parent) {
            source << "    visit_" << node->parent->snake_case_name << "(node);" << std::endl;
        } else {
            source << "    visit_node(node);" << std::endl;
        }
        source << "}" << std::endl << std::endl;
    }

    header << "};" << std::endl << std::endl;
}

// Generate the recursive visitor class.
static void generate_recursive_visitor_class(
    std::ofstream &header,
    std::ofstream &source,
    Nodes &nodes
) {

    // Print class header.
    format_doc(
        header,
        "Visitor base class defaulting to DFS traversal.\n\n"
        "The visitor functions for nodes with children default to DFS "
        "traversal instead of falling back to more generic node types.");
    header << "class RecursiveVisitor : public Visitor {" << std::endl;
    header << "public:" << std::endl << std::endl;

    // Functions for all node types.
    for (auto &node : nodes) {
        auto all_children = node->all_children();
        bool empty = true;
        for (auto &child : all_children) {
            if (child.node_type) {
                empty = false;
                break;
            }
        }
        if (empty) {
            continue;
        }
        auto doc = "Recursive traversal for `" + node->title_case_name + "` nodes.";
        format_doc(header, doc, "    ");
        header << "    void visit_" << node->snake_case_name;
        header << "(" << node->title_case_name << " &node) override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void RecursiveVisitor::visit_" << node->snake_case_name;
        source << "(" << node->title_case_name << " &node) {" << std::endl;
        for (auto &child : all_children) {
            if (child.node_type) {
                source << "    node." << child.name << ".visit(*this);" << std::endl;
            }
        }
        source << "}" << std::endl << std::endl;
    }

    header << "};" << std::endl << std::endl;
}

// Generate the dumper class.
static void generate_dumper_class(
    std::ofstream &header,
    std::ofstream &source,
    Nodes &nodes,
    std::string &source_location
) {

    // Print class header.
    format_doc(header, "Visitor class that debug-dumps a tree to a stream");
    header << "class Dumper : public RecursiveVisitor {" << std::endl;
    header << "protected:" << std::endl << std::endl;
    format_doc(header, "Output stream to dump to.", "    ");
    header << "    std::ostream &out;" << std::endl << std::endl;
    format_doc(header, "Current indentation level.", "    ");
    header << "    int indent = 0;" << std::endl << std::endl;
    format_doc(header, "Whether we're printing the contents of a link.", "    ");
    header << "    bool in_link = false;" << std::endl << std::endl;

    // Print function that prints indentation level.
    format_doc(header, "Writes the current indentation level's worth of spaces.", "    ");
    header << "    void write_indent();" << std::endl << std::endl;
    format_doc(source, "Writes the current indentation level's worth of spaces.");
    source << "void Dumper::write_indent() {" << std::endl;
    source << "    for (int i = 0; i < indent; i++) {" << std::endl;
    source << "        out << \"  \";" << std::endl;
    source << "    }" << std::endl;
    source << "}" << std::endl << std::endl;

    // Write constructor.
    header << "public:" << std::endl << std::endl;
    format_doc(header, "Construct a dumping visitor.", "    ");
    header << "    Dumper(std::ostream &out, int indent=0) : out(out), indent(indent) {};" << std::endl << std::endl;

    // Print fallback function.
    format_doc(header, "Dumps a `Node`.", "    ");
    header << "    void visit_node(Node &node) override;" << std::endl;
    format_doc(source, "Dumps a `Node`.");
    source << "void Dumper::visit_node(Node &node) {" << std::endl;
    source << "    (void)node;" << std::endl;
    source << "    write_indent();" << std::endl;
    source << "    out << \"!Node()\" << std::endl;" << std::endl;
    source << "}" << std::endl << std::endl;

    // Functions for all node types.
    for (auto &node : nodes) {
        auto doc = "Dumps a `" + node->title_case_name + "` node.";
        format_doc(header, doc, "    ");
        header << "    void visit_" << node->snake_case_name;
        header << "(" << node->title_case_name << " &node) override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void Dumper::visit_" << node->snake_case_name;
        source << "(" << node->title_case_name << " &node) {" << std::endl;
        source << "    write_indent();" << std::endl;
        auto children = node->all_children();
        source << "    out << \"" << node->title_case_name << "(\";" << std::endl;
        if (!source_location.empty()) {
            source << "    if (auto loc = node.get_annotation_ptr<" << source_location << ">()) {" << std::endl;
            source << "        out << \" # \" << *loc;" << std::endl;
            source << "    }" << std::endl;
        }
        source << "    out << std::endl;" << std::endl;
        if (!children.empty()) {
            source << "    indent++;" << std::endl;
            for (auto &child : children) {
                source << "    write_indent();" << std::endl;
                source << "    out << \"" << child.name;
                if (child.type == Link || child.type == OptLink) {
                    source << " --> ";
                } else {
                    source << ": ";
                }
                source << "\";" << std::endl;
                switch (child.ext_type) {
                    case Maybe:
                    case One:
                    case OptLink:
                    case Link:
                        source << "    if (node." << child.name << ".empty()) {" << std::endl;
                        if (child.type == One || child.type == Link) {
                            source << "        out << \"!MISSING\" << std::endl;" << std::endl;
                        } else {
                            source << "        out << \"-\" << std::endl;" << std::endl;
                        }
                        source << "    } else {" << std::endl;
                        source << "        out << \"<\" << std::endl;" << std::endl;
                        source << "        indent++;" << std::endl;
                        if (child.type == Prim) {
                            source << "        if (!node." << child.name << ".empty()) {" << std::endl;
                            source << "            node." << child.name << "->dump(out, indent);" << std::endl;
                            source << "        }" << std::endl;
                        } else if (child.type == Link || child.type == OptLink) {
                            source << "        if (!in_link) {" << std::endl;
                            source << "            in_link = true;" << std::endl;
                            source << "            node." << child.name << ".visit(*this);" << std::endl;
                            source << "            in_link = false;" << std::endl;
                            source << "        } else {" << std::endl;
                            source << "            write_indent();" << std::endl;
                            source << "            out << \"...\" << std::endl;" << std::endl;
                            source << "        }" << std::endl;
                        } else {
                            source << "        node." << child.name << ".visit(*this);" << std::endl;
                        }
                        source << "        indent--;" << std::endl;
                        source << "        write_indent();" << std::endl;
                        source << "        out << \">\" << std::endl;" << std::endl;
                        source << "    }" << std::endl;
                        break;

                    case Any:
                    case Many:
                        source << "    if (node." << child.name << ".empty()) {" << std::endl;
                        if (child.type == One) {
                            source << "        out << \"!MISSING\" << std::endl;" << std::endl;
                        } else {
                            source << "        out << \"[]\" << std::endl;" << std::endl;
                        }
                        source << "    } else {" << std::endl;
                        source << "        out << \"[\" << std::endl;" << std::endl;
                        source << "        indent++;" << std::endl;
                        source << "        for (auto &sptr : node." << child.name << ") {" << std::endl;
                        source << "            if (!sptr.empty()) {" << std::endl;
                        if (child.type == Prim) {
                            source << "                sptr->dump(out, indent);" << std::endl;
                        } else {
                            source << "                sptr->visit(*this);" << std::endl;
                        }
                        source << "            } else {" << std::endl;
                        source << "                write_indent();" << std::endl;
                        source << "                out << \"!NULL\" << std::endl;" << std::endl;
                        source << "            }" << std::endl;
                        source << "        }" << std::endl;
                        source << "        indent--;" << std::endl;
                        source << "        write_indent();" << std::endl;
                        source << "        out << \"]\" << std::endl;" << std::endl;
                        source << "    }" << std::endl;
                        break;

                    case Prim:
                        source << "    out << node." << child.name << " << std::endl;" << std::endl;
                        break;

                }
            }
            source << "    indent--;" << std::endl;
            source << "    write_indent();" << std::endl;
        }
        source << "    out << \")\" << std::endl;" << std::endl;
        source << "}" << std::endl << std::endl;
    }

    header << "};" << std::endl << std::endl;
}

} // namespace tree_gen

using namespace tree_gen;

/**
 * Main function for generating the the header and source file for a tree.
 */
int main(
    int argc,
    char *argv[]
) {

    // Check command line and open files.
    if (argc != 4) {
        std::cerr << "Usage: tree-gen <spec-file> <header-dir> <source-dir>" << std::endl;
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
    auto nodes = specification.nodes;

    // Clean up.
    yylex_destroy(scanner);
    fclose(fptr);

    // Open the output files.
    auto header_filename = std::string(argv[2]);
    auto header = std::ofstream(header_filename);
    if (!header.is_open()) {
        std::cerr << "Failed to open header file for writing" << std::endl;
        return 1;
    }
    auto source = std::ofstream(std::string(argv[3]));
    if (!source.is_open()) {
        std::cerr << "Failed to open source file for writing" << std::endl;
        return 1;
    }

    // Strip the path from the header filename such that it can be used for the
    // include guard and the #include directive in the source file.
    auto sep_pos = header_filename.rfind('/');
    auto backslash_pos = header_filename.rfind('\\');
    if (backslash_pos != std::string::npos && backslash_pos > sep_pos) {
        sep_pos = backslash_pos;
    }
    header_filename = header_filename.substr(sep_pos + 1);

    // Generate the include guard name.
    std::string include_guard = header_filename;
    std::transform(
        include_guard.begin(), include_guard.end(), include_guard.begin(),
        [](unsigned char c){
            if (std::isalnum(c)) {
                return std::toupper(c);
            } else {
                return static_cast<int>('_');
            }
        }
    );

    // Header for the header file.
    if (!specification.header_doc.empty()) {
        format_doc(header, specification.header_doc, "", "\\file");
        header << std::endl;
    }
    header << "#pragma once" << std::endl;
    header << std::endl;
    header << "#include <iostream>" << std::endl;
    for (auto &include : specification.includes) {
        header << "#" << include << std::endl;
    }
    header << std::endl;
    for (size_t i = 0; i < specification.namespaces.size(); i++) {
        if (i == specification.namespaces.size() - 1 && !specification.namespace_doc.empty()) {
            header << std::endl;
            format_doc(header, specification.namespace_doc);

            // Generate dot graph for the tree for the doxygen documentation.
            header << "/**" << std::endl;
            header << " * \\dot" << std::endl;
            header << " * digraph example {" << std::endl;
            header << " *   node [shape=record, fontname=Helvetica, fontsize=10];" << std::endl;
            std::ostringstream ns;
            for (auto &name : specification.namespaces) {
                ns << name << "::";
            }
            for (auto &node : nodes) {
                header << " *   " << node->title_case_name;
                header << " [ label=\"" << node->title_case_name;
                header << "\" URL=\"\\ref " << ns.str() << node->title_case_name;
                header << "\"";
                if (!node->derived.empty()) {
                    header << ", style=dotted";
                }
                header << "];" << std::endl;
            }
            for (auto &node : nodes) {
                if (node->parent) {
                    header << " *   " << node->parent->title_case_name;
                    header << " -> " << node->title_case_name;
                    header << " [ arrowhead=open, style=dotted ];" << std::endl;
                }
            }
            int prim_id = 0;
            for (auto &node : nodes) {
                for (auto child : node->children) {
                    AttributeType typ;
                    if (child.node_type) {
                        header << " *   " << node->title_case_name;
                        header << " -> " << child.node_type->title_case_name;
                        typ = child.type;
                    } else {
                        std::string full_name = child.prim_type;
                        auto pos = full_name.find("<");
                        if (pos != std::string::npos) {
                            full_name = full_name.substr(pos + 1);
                        }
                        pos = full_name.rfind(">");
                        if (pos != std::string::npos) {
                            full_name = full_name.substr(0, pos);
                        }
                        std::string brief_name = full_name;
                        pos = brief_name.rfind("::");
                        if (pos != std::string::npos) {
                            pos = brief_name.rfind("::", pos - 1);
                            if (pos != std::string::npos) {
                                brief_name = brief_name.substr(pos + 2);
                            }
                        }
                        header << " *   prim" << prim_id;
                        header << " [ label=\"" << brief_name;
                        header << "\" URL=\"\\ref " << full_name;
                        header << "\"];" << std::endl;
                        header << " *   " << node->title_case_name;
                        header << " -> prim" << prim_id;
                        typ = child.ext_type;
                        prim_id++;
                    }
                    header << " [ label=\"" << child.name;
                    switch (typ) {
                        case Any: header << "*\", arrowhead=open, style=bold, "; break;
                        case OptLink: header << "@?\", arrowhead=open, style=dashed, "; break;
                        case Maybe: header << "?\", arrowhead=open, style=solid, "; break;
                        case Many: header << "+\", arrowhead=normal, style=bold, "; break;
                        case Link: header << "@\", arrowhead=normal, style=dashed, "; break;
                        default: header << "\", arrowhead=normal, style=solid, "; break;
                    }
                    header << "fontname=Helvetica, fontsize=10];" << std::endl;
                }
            }
            header << " * }" << std::endl;
            header << " * \\enddot" << std::endl;
            header << " */" << std::endl;

        }
        header << "namespace " << specification.namespaces[i] << " {" << std::endl;
    }
    header << std::endl;

    // Determine the namespace that the base and edge classes are defined in.
    // If it's not the current namespace, pull the types into it using typedefs.
    std::string tree_namespace = "";
    if (!specification.tree_namespace.empty()) {
        tree_namespace = specification.tree_namespace + "::";
        header << "// Base classes used to construct the tree." << std::endl;
        header << "using Base = " << tree_namespace << "Base;" << std::endl;
        header << "template <class T> using Maybe   = " << tree_namespace << "Maybe<T>;" << std::endl;
        header << "template <class T> using One     = " << tree_namespace << "One<T>;" << std::endl;
        header << "template <class T> using Any     = " << tree_namespace << "Any<T>;" << std::endl;
        header << "template <class T> using Many    = " << tree_namespace << "Many<T>;" << std::endl;
        header << "template <class T> using OptLink = " << tree_namespace << "OptLink<T>;" << std::endl;
        header << "template <class T> using Link    = " << tree_namespace << "Link<T>;" << std::endl;
        header << std::endl;
    }

    // Header for the source file.
    if (!specification.source_doc.empty()) {
        format_doc(source, specification.source_doc, "", "\\file");
        source << std::endl;
    }
    for (auto &include : specification.src_includes) {
        source << "#" << include << std::endl;
    }
    source << "#include \"" << header_filename << "\"" << std::endl;
    source << std::endl;
    for (auto &name : specification.namespaces) {
        source << "namespace " << name << " {" << std::endl;
    }
    source << std::endl;

    // Generate forward references for all the classes.
    header << "// Forward declarations for all classes." << std::endl;
    header << "class Node;" << std::endl;
    for (auto &node : nodes) {
        header << "class " << node->title_case_name << ";" << std::endl;
    }
    header << "class Visitor;" << std::endl;
    header << "class RecursiveVisitor;" << std::endl;
    header << "class Dumper;" << std::endl;
    header << std::endl;

    // Generate the NodeType enum.
    generate_enum(header, nodes);

    // Generate the base class.
    generate_base_class(header, source, nodes, !specification.serialize_fn.empty());

    // Generate the node classes.
    std::unordered_set<std::string> generated;
    for (auto node : nodes) {
        if (generated.count(node->snake_case_name)) {
            continue;
        }
        auto ancestors = Nodes();
        while (node) {
            ancestors.push_back(node);
            node = node->parent;
        }
        for (auto node_it = ancestors.rbegin(); node_it != ancestors.rend(); node_it++) {
            node = *node_it;
            if (generated.count(node->snake_case_name)) {
                continue;
            }
            generated.insert(node->snake_case_name);
            generate_node_class(header, source, specification, *node);
        }
    }

    // Generate the visitor classes.
    generate_visitor_base_class(header, source, nodes);
    generate_recursive_visitor_class(header, source, nodes);
    generate_dumper_class(header, source, nodes, specification.source_location);

    // Close the namespaces.
    for (auto name_it = specification.namespaces.rbegin(); name_it != specification.namespaces.rend(); name_it++) {
        header << "} // namespace " << *name_it << std::endl;
        source << "} // namespace " << *name_it << std::endl;
    }
    header << std::endl;
    source << std::endl;

    // Overload the stream write operator.
    std::string name_space = "";
    for (auto &name : specification.namespaces) {
        name_space += "::" + name;
    }
    format_doc(header, "Stream << overload for AST nodes (writes debug dump).");
    header << "std::ostream& operator<<(std::ostream& os, const " << name_space << "::Node& object);" << std::endl << std::endl;
    format_doc(source, "Stream << overload for AST nodes (writes debug dump).");
    source << "std::ostream& operator<<(std::ostream& os, const " << name_space << "::Node& object) {" << std::endl;
    source << "    const_cast<" << name_space << "::Node&>(object).dump(os);" << std::endl;
    source << "    return os;" << std::endl;
    source << "}" << std::endl << std::endl;

    return 0;
}
