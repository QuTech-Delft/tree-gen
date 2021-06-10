/** \file
 * C++ generation source file for \ref tree-gen.
 */

#include <fstream>
#include <iostream>
#include <cctype>
#include <unordered_set>
#include "tree-gen-cpp.hpp"

namespace tree_gen {
namespace cpp {

/**
 * Formats a C++ docstring.
 */
void format_doc(
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
void generate_enum(
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
void generate_typecast_function(
    std::ofstream &header,
    std::ofstream &source,
    const std::string &clsname,
    Node &into,
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
void generate_base_class(
    std::ofstream &header,
    std::ofstream &source,
    Nodes &nodes,
    bool with_serdes,
    const std::string &support_ns
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

    format_doc(header, "Value-based equality operator. Ignores annotations!", "    ");
    header << "    virtual bool equals(const Node& rhs) const = 0;" << std::endl << std::endl;

    format_doc(header, "Pointer-based equality operator.", "    ");
    header << "    virtual bool operator==(const Node& rhs) const = 0;" << std::endl << std::endl;

    format_doc(header, "Pointer-based inequality operator.", "    ");
    header << "    inline bool operator!=(const Node& rhs) const {" << std::endl;
    header << "        return !(*this == rhs);" << std::endl;
    header << "    }" << std::endl << std::endl;

    header << "protected:" << std::endl << std::endl;
    format_doc(header, "Internal helper method for visiter pattern.", "    ");
    header << "    virtual void visit_internal(VisitorBase &visitor, void *retval=nullptr) = 0;" << std::endl << std::endl;

    header << "public:" << std::endl << std::endl;
    format_doc(header, "Visit this object.", "    ");
    header << "    template <typename T>" << std::endl;
    header << "    T visit(Visitor<T> &visitor);" << std::endl << std::endl;

    /*header << "    T visit(Visitor<T> &visitor) {" << std::endl;
    header << "        T retval;" << std::endl;
    header << "        this->visit_internal(visitor, &retval);" << std::endl;
    header << "        return retval;" << std::endl;
    header << "    }" << std::endl << std::endl;

    format_doc(header, "Visit this object.", "    "); TODO
    header << "    template <>" << std::endl;
    header << "    void visit<void>(Visitor &visitor) {" << std::endl;
    header << "        this->visit_internal(visitor);" << std::endl;
    header << "    }" << std::endl << std::endl;*/

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
        header << "        " << support_ns << "::cbor::MapWriter &map," << std::endl;
        header << "        const " << support_ns << "::base::PointerMap &ids" << std::endl;
        header << "    ) const = 0;" << std::endl << std::endl;

        format_doc(header, "Deserializes the given node.", "    ");
        header << "    static std::shared_ptr<Node> deserialize(" << std::endl;
        header << "         const " << support_ns << "::cbor::MapReader &map," << std::endl;
        header << "         " << support_ns << "::base::IdentifierMap &ids" << std::endl;
        header << "    );" << std::endl << std::endl;
        format_doc(source, "Writes a debug dump of this node to the given stream.");
        source << "std::shared_ptr<Node> Node::deserialize(" << std::endl;
        source << "    const " << support_ns << "::cbor::MapReader &map," << std::endl;
        source << "    " << support_ns << "::base::IdentifierMap &ids" << std::endl;
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
    Node &node
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
void generate_node_class(
    std::ofstream &header,
    std::ofstream &source,
    Specification &spec,
    Node &node
) {
    const auto all_fields = node.all_fields();
    const auto &support_ns = spec.support_namespace;

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

    // Print fields.
    for (auto &field : node.fields) {
        if (!field.doc.empty()) {
            format_doc(header, field.doc, "    ");
        }
        header << "    ";
        switch (field.type) {
            case Maybe:   header << "Maybe<"   << field.node_type->title_case_name << "> "; break;
            case One:     header << "One<"     << field.node_type->title_case_name << "> "; break;
            case Any:     header << "Any<"     << field.node_type->title_case_name << "> "; break;
            case Many:    header << "Many<"    << field.node_type->title_case_name << "> "; break;
            case OptLink: header << "OptLink<" << field.node_type->title_case_name << "> "; break;
            case Link:    header << "Link<"    << field.node_type->title_case_name << "> "; break;
            case Prim:    header << field.prim_type << " "; break;
        }
        header << field.name << ";" << std::endl << std::endl;
    }

    // Print constructors.
    if (!all_fields.empty()) {
        format_doc(header, "Constructor.", "    ");
        header << "    " << node.title_case_name << "(";
        bool first = true;
        for (auto &field : all_fields) {
            if (first) {
                first = false;
            } else {
                header << ", ";
            }
            header << "const ";
            switch (field.type) {
                case Maybe:   header << "Maybe<"   << field.node_type->title_case_name << "> "; break;
                case One:     header << "One<"     << field.node_type->title_case_name << "> "; break;
                case Any:     header << "Any<"     << field.node_type->title_case_name << "> "; break;
                case Many:    header << "Many<"    << field.node_type->title_case_name << "> "; break;
                case OptLink: header << "OptLink<" << field.node_type->title_case_name << "> "; break;
                case Link:    header << "Link<"    << field.node_type->title_case_name << "> "; break;
                case Prim:    header << field.prim_type << " "; break;
            }
            header << "&" << field.name << " = ";
            switch (field.type) {
                case Maybe:   header << "Maybe<"   << field.node_type->title_case_name << ">()"; break;
                case One:     header << "One<"     << field.node_type->title_case_name << ">()"; break;
                case Any:     header << "Any<"     << field.node_type->title_case_name << ">()"; break;
                case Many:    header << "Many<"    << field.node_type->title_case_name << ">()"; break;
                case OptLink: header << "OptLink<" << field.node_type->title_case_name << ">()"; break;
                case Link:    header << "Link<"    << field.node_type->title_case_name << ">()"; break;
                case Prim:    header << spec.initialize_function << "<" << field.prim_type << ">()"; break;
            }
        }
        header << ");" << std::endl << std::endl;

        format_doc(source, "Constructor.", "");
        source << node.title_case_name << "::" << node.title_case_name << "(";
        first = true;
        for (auto &field : all_fields) {
            if (first) {
                first = false;
            } else {
                source << ", ";
            }
            source << "const ";
            switch (field.type) {
                case Maybe:   source << "Maybe<" << field.node_type->title_case_name << "> "; break;
                case One:     source << "One<" << field.node_type->title_case_name << "> "; break;
                case Any:     source << "Any<" << field.node_type->title_case_name << "> "; break;
                case Many:    source << "Many<" << field.node_type->title_case_name << "> "; break;
                case OptLink: source << "OptLink<" << field.node_type->title_case_name << "> "; break;
                case Link:    source << "Link<" << field.node_type->title_case_name << "> "; break;
                case Prim:    source << field.prim_type << " "; break;
            }
            source << "&" << field.name;
        }
        source << ")" << std::endl << "    : ";
        first = true;
        if (node.parent) {
            source << node.parent->title_case_name << "(";
            first = true;
            for (auto &field : node.parent->all_fields()) {
                if (first) {
                    first = false;
                } else {
                    source << ", ";
                }
                source << field.name;
            }
            source << ")";
            first = false;
        }
        for (auto &field : node.fields) {
            if (first) {
                first = false;
            } else {
                source << ", ";
            }
            source << field.name << "(" << field.name << ")";
        }
        source << std::endl << "{}" << std::endl << std::endl;
    }

    // Print find_reachable and check_complete functions.
    if (node.derived.empty()) {
        std::string doc = "Registers all reachable nodes with the given PointerMap.";
        format_doc(header, doc, "    ");
        header << "    void find_reachable(" << support_ns << "::base::PointerMap &map) const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void " << node.title_case_name;
        source << "::find_reachable(" << support_ns << "::base::PointerMap &map) const {" << std::endl;
        source << "    (void)map;" << std::endl;
        for (auto &field : all_fields) {
            auto type = (field.type == Prim) ? field.ext_type : field.type;
            switch (type) {
                case Maybe:
                case One:
                case Any:
                case Many:
                case OptLink:
                case Link:
                    source << "    " << field.name << ".find_reachable(map);" << std::endl;
                    break;
                default:
                    break;
            }
        }
        source << "}" << std::endl << std::endl;

        doc = "Returns whether this `" + node.title_case_name + "` is complete/fully defined.";
        format_doc(header, doc, "    ");
        header << "    void check_complete(const " << support_ns << "::base::PointerMap &map) const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void " << node.title_case_name;
        source << "::check_complete(const " << support_ns << "::base::PointerMap &map) const {" << std::endl;
        source << "    (void)map;" << std::endl;
        if (node.is_error_marker) {
            source << "    throw " << support_ns << "::base::NotWellFormed(\"" << node.title_case_name << " error node in tree\");" << std::endl;
        } else {
            for (auto &field : all_fields) {
                auto type = (field.type == Prim) ? field.ext_type : field.type;
                switch (type) {
                    case Maybe:
                    case One:
                    case Any:
                    case Many:
                    case OptLink:
                    case Link:
                        source << "    " << field.name << ".check_complete(map);" << std::endl;
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
        auto doc = "Helper method for visiting nodes.";
        header << "protected:" << std::endl << std::endl;
        format_doc(header, doc, "    ");
        header << "    void visit_internal(VisitorBase &visitor, void *retval) override;" << std::endl << std::endl;
        header << "public:" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void " << node.title_case_name;
        source << "::visit_internal(VisitorBase &visitor, void *retval) {" << std::endl;
        source << "    visitor.raw_visit_" << node.snake_case_name;
        source << "(*this, retval);" << std::endl;
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
        source << "    auto node = ";
        if (!spec.tree_namespace.empty()) {
            source << spec.tree_namespace << "::";
        }
        source << "make<" << node.title_case_name << ">(*this);" << std::endl;
        for (auto &field : all_fields) {
            auto type = (field.type == Prim) ? field.ext_type : field.type;
            if (type == Maybe || type == One || type == Any || type == Many) {
                source << "    node->" << field.name << " = this->" << field.name << ".clone();" << std::endl;
            }
        }
        source << "    return node;" << std::endl;
        source << "}" << std::endl << std::endl;
    }

    // Print equality operator.
    if (node.derived.empty()) {
        auto doc = "Value-based equality operator. Ignores annotations!";
        format_doc(header, doc, "    ");
        header << "    bool equals(const Node &rhs) const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "bool " << node.title_case_name;
        source << "::equals(const Node &rhs) const {" << std::endl;
        source << "    if (rhs.type() != NodeType::" << node.title_case_name << ") return false;" << std::endl;
        if (!all_fields.empty()) {
            source << "    auto rhsc = dynamic_cast<const " << node.title_case_name << "&>(rhs);" << std::endl;
            for (auto &field : all_fields) {
                if (field.type == Prim && field.ext_type == Prim) {
                    source << "    if (this->" << field.name << " != rhsc." << field.name << ") return false;" << std::endl;
                } else {
                    source << "    if (!this->" << field.name << ".equals(rhsc." << field.name << ")) return false;" << std::endl;
                }
            }
        }
        source << "    return true;" << std::endl;
        source << "}" << std::endl << std::endl;

        doc = "Pointer-based equality operator.";
        format_doc(header, doc, "    ");
        header << "    bool operator==(const Node &rhs) const override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "bool " << node.title_case_name;
        source << "::operator==(const Node &rhs) const {" << std::endl;
        source << "    if (rhs.type() != NodeType::" << node.title_case_name << ") return false;" << std::endl;
        if (!all_fields.empty()) {
            source << "    auto rhsc = dynamic_cast<const " << node.title_case_name << "&>(rhs);" << std::endl;
            for (auto &field : all_fields) {
                source << "    if (this->" << field.name << " != rhsc." << field.name << ") return false;" << std::endl;
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
            header << "        " << support_ns << "::cbor::MapWriter &map," << std::endl;
            header << "        const " << support_ns << "::base::PointerMap &ids" << std::endl;
            header << "    ) const override;" << std::endl << std::endl;
            format_doc(source, "Serializes this node to the given map.");
            source << "void " << node.title_case_name << "::serialize(" << std::endl;
            source << "    " << support_ns << "::cbor::MapWriter &map," << std::endl;
            source << "    const " << support_ns << "::base::PointerMap &ids" << std::endl;
            source << ") const {" << std::endl;
            source << "    (void)ids;" << std::endl;
            source << "    map.append_string(\"@t\", \"" << node.title_case_name << "\");" << std::endl;
            bool first = true;
            for (const auto &field : all_fields) {
                source << "    ";
                if (first) {
                    source << "auto ";
                    first = false;
                }
                source << "submap = map.append_map(\"" << field.name << "\");" << std::endl;
                if (field.type == Prim && field.ext_type == Prim) {
                    source << "    " << spec.serialize_fn << "<" << field.prim_type << ">";
                    source << "(" << field.name << ", submap);" << std::endl;
                } else {
                    source << "    " << field.name << ".serialize(submap, ids);" << std::endl;
                }
                source << "    submap.close();" << std::endl;
            }
            source << "    serialize_annotations(map);" << std::endl;
            source << "}" << std::endl << std::endl;

            format_doc(header, "Deserializes the given node.", "    ");
            header << "    static std::shared_ptr<" << node.title_case_name << "> ";
            header << "deserialize(const " << support_ns << "::cbor::MapReader &map, " << support_ns << "::base::IdentifierMap &ids);" << std::endl << std::endl;
            format_doc(source, "Writes a debug dump of this node to the given stream.");
            source << "std::shared_ptr<" << node.title_case_name << "> ";
            source << node.title_case_name << "::deserialize(const " << support_ns << "::cbor::MapReader &map, " << support_ns << "::base::IdentifierMap &ids) {" << std::endl;
            source << "    (void)ids;" << std::endl;
            source << "    auto type = map.at(\"@t\").as_string();" << std::endl;
            source << "    if (type != \"" << node.title_case_name << "\") {" << std::endl;
            source << "        throw std::runtime_error(\"Schema validation failed: unexpected node type \" + type);" << std::endl;
            source << "    }" << std::endl;
            source << "    auto node = std::make_shared<" << node.title_case_name << ">(" << std::endl;
            std::vector<Field> links{};
            first = true;
            for (const auto &field : all_fields) {
                if (first) {
                    first = false;
                } else {
                    source << "," << std::endl;
                }
                source << "        ";

                EdgeType type = (field.type != Prim) ? field.type : field.ext_type;
                if (field.type != Prim) {
                    switch (field.type) {
                        case Maybe:   source << "Maybe"; break;
                        case One:     source << "One"; break;
                        case Any:     source << "Any"; break;
                        case Many:    source << "Many"; break;
                        case OptLink: source << "OptLink"; break;
                        case Link:    source << "Link"; break;
                        default:      source << "<?>"; break;
                    }
                    source << "<" << field.node_type->title_case_name << ">(";
                    source << "map.at(\"" << field.name << "\").as_map(), ids)";
                } else if (field.ext_type != Prim) {
                    source << field.prim_type << "(";
                    source << "map.at(\"" << field.name << "\").as_map(), ids)";
                } else {
                    source << spec.deserialize_fn << "<" << field.prim_type << ">";
                    source << "(map.at(\"" << field.name << "\").as_map())";
                }
                if (type == OptLink || type == Link) {
                    links.push_back(field);
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
            source << "    node->deserialize_annotations(map);" << std::endl;
            source << "    return node;" << std::endl;
            source << "}" << std::endl << std::endl;
        } else {
            format_doc(header, "Deserializes the given node.", "    ");
            header << "    static std::shared_ptr<" << node.title_case_name << "> ";
            header << "deserialize(const " << support_ns << "::cbor::MapReader &map, " << support_ns << "::base::IdentifierMap &ids);" << std::endl << std::endl;
            format_doc(source, "Writes a debug dump of this node to the given stream.");
            source << "std::shared_ptr<" << node.title_case_name << "> ";
            source << node.title_case_name << "::deserialize(const " << support_ns << "::cbor::MapReader &map, " << support_ns << "::base::IdentifierMap &ids) {" << std::endl;
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

/**
 * Generate the visitor base class.
 */
void generate_visitor_base_class(
    std::ofstream &header,
    std::ofstream &source,
    Nodes &nodes
) {

    // Print class header.
    format_doc(
        header,
        "Internal class for implementing the visitor pattern.");
    header << "class VisitorBase {" << std::endl;
    header << "public:" << std::endl << std::endl;

    // Virtual destructor.
    format_doc(header, "Virtual destructor for proper cleanup.", "    ");
    header << "    virtual ~VisitorBase() = default;" << std::endl << std::endl;

    // Raw visit methods are protected.
    header << "protected:" << std::endl << std::endl;
    header << "    friend class Node;" << std::endl;
    for (auto &node : nodes) {
        header << "    friend class " << node->title_case_name << ";" << std::endl;
    }
    header << std::endl;


    // Fallback for any kind of node.
    format_doc(header, "Internal visitor function for nodes of any type.", "    ");
    header << "    virtual void raw_visit_node(Node &node, void *retval) = 0;" << std::endl << std::endl;

    // Functions for all node types.
    for (auto &node : nodes) {
        format_doc(header, "Internal visitor function for `" + node->title_case_name + "` nodes.", "    ");
        header << "    virtual void raw_visit_" << node->snake_case_name;
        header << "(" << node->title_case_name << " &node, void *retval) = 0;" << std::endl << std::endl;
    }

    header << "};" << std::endl << std::endl;
}

/**
 * Generate the templated visitor class.
 */
void generate_visitor_class(
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
    header << "template <typename T>" << std::endl;
    header << "class Visitor : public VisitorBase {" << std::endl;
    header << "protected:" << std::endl << std::endl;

    // Internal function for any kind of node.
    format_doc(header, "Internal visitor function for nodes of any type.", "    ");
    header << "    void raw_visit_node(Node &node, void *retval) override;" << std::endl << std::endl;

    // Internal functions for all node types.
    for (auto &node : nodes) {
        format_doc(header, "Internal visitor function for `" + node->title_case_name + "` nodes.", "    ");
        header << "    void raw_visit_" << node->snake_case_name;
        header << "(" << node->title_case_name << " &node, void *retval) override;" << std::endl << std::endl;
    }

    header << "public:" << std::endl << std::endl;

    // Fallback for any kind of node.
    format_doc(header, "Fallback function for nodes of any type.", "    ");
    header << "    virtual T visit_node(Node &node) = 0;" << std::endl << std::endl;

    // Functions for all node types.
    for (auto &node : nodes) {
        std::string doc;
        if (node->derived.empty()) {
            doc = "Visitor function for `" + node->title_case_name + "` nodes.";
        } else {
            doc = "Fallback function for `" + node->title_case_name + "` nodes.";
        }
        format_doc(header, doc, "    ");
        header << "    virtual T visit_" << node->snake_case_name;
        header << "(" << node->title_case_name << " &node) {" << std::endl;
        if (node->parent) {
            header << "        return visit_" << node->parent->snake_case_name << "(node);" << std::endl;
        } else {
            header << "        return visit_node(node);" << std::endl;
        }
        header << "    }" << std::endl << std::endl;
    }

    header << "};" << std::endl << std::endl;

    // Internal function for any kind of node.
    format_doc(header, "Internal visitor function for nodes of any type.", "    ");
    header << "    template <typename T>" << std::endl;
    header << "    void Visitor<T>::raw_visit_node(Node &node, void *retval) {" << std::endl;
    header << "        if (retval == nullptr) {" << std::endl;
    header << "            this->visit_node(node);" << std::endl;
    header << "        } else {" << std::endl;
    header << "            *((T*)retval) = this->visit_node(node);" << std::endl;
    header << "        };" << std::endl;
    header << "    }" << std::endl << std::endl;

    format_doc(header, "Internal visitor function for nodes of any type.", "    ");
    header << "    template <>" << std::endl;
    header << "    void Visitor<void>::raw_visit_node(Node &node, void *retval);" << std::endl << std::endl;

    format_doc(source, "Internal visitor function for nodes of any type.");
    source << "template <>" << std::endl;
    source << "void Visitor<void>::raw_visit_node(Node &node, void *retval) {" << std::endl;
    source << "    (void)retval;" << std::endl;
    source << "    this->visit_node(node);" << std::endl;
    source << "}" << std::endl << std::endl;

    // Internal functions for all node types.
    for (auto &node : nodes) {
        format_doc(header, "Internal visitor function for `" + node->title_case_name + "` nodes.", "    ");
        header << "    template <typename T>" << std::endl;
        header << "    void Visitor<T>::raw_visit_" << node->snake_case_name;
        header << "(" << node->title_case_name << " &node, void *retval) {" << std::endl;
        header << "        if (retval == nullptr) {" << std::endl;
        header << "            this->visit_" << node->snake_case_name << "(node);" << std::endl;
        header << "        } else {" << std::endl;
        header << "            *((T*)retval) = this->visit_" << node->snake_case_name << "(node);" << std::endl;
        header << "        };" << std::endl;
        header << "    }" << std::endl << std::endl;

        format_doc(header, "Internal visitor function for `" + node->title_case_name + "` nodes.", "    ");
        header << "    template <>" << std::endl;
        header << "    void Visitor<void>::raw_visit_" << node->snake_case_name;
        header << "(" << node->title_case_name << " &node, void *retval);" << std::endl << std::endl;

        format_doc(source, "Internal visitor function for `" + node->title_case_name + "` nodes.");
        source << "template <>" << std::endl;
        source << "void Visitor<void>::raw_visit_" << node->snake_case_name;
        source << "(" << node->title_case_name << " &node, void *retval) {" << std::endl;
        source << "    (void)retval;" << std::endl;
        source << "    this->visit_" << node->snake_case_name << "(node);" << std::endl;
        source << "}" << std::endl << std::endl;
    }

}

/**
 * Generate the recursive visitor class.
 */
void generate_recursive_visitor_class(
    std::ofstream &header,
    std::ofstream &source,
    Nodes &nodes
) {

    // Print class header.
    format_doc(
        header,
        "Visitor base class defaulting to DFS pre-order traversal.\n\n"
        "The visitor functions for nodes with subnode fields default to DFS "
        "traversal in addition to falling back to more generic node types."
        "Links and OptLinks are *not* followed."
    );
    header << "class RecursiveVisitor : public Visitor<void> {" << std::endl;
    header << "public:" << std::endl << std::endl;

    // Functions for all node types.
    for (auto &node : nodes) {
        auto doc = "Recursive traversal for `" + node->title_case_name + "` nodes.";
        format_doc(header, doc, "    ");
        header << "    void visit_" << node->snake_case_name;
        header << "(" << node->title_case_name << " &node) override;" << std::endl << std::endl;
        format_doc(source, doc);
        source << "void RecursiveVisitor::visit_" << node->snake_case_name;
        source << "(" << node->title_case_name << " &node) {" << std::endl;
        if (node->parent) {
            source << "    visit_" << node->parent->snake_case_name << "(node);" << std::endl;
        } else {
            source << "    visit_node(node);" << std::endl;
        }
        for (auto &field : node->fields) {
            if (field.node_type) {
                if (field.type != Link && field.type != OptLink) {
                    source << "    node." << field.name << ".visit(*this);" << std::endl;
                }
            }
        }
        source << "}" << std::endl << std::endl;
    }

    header << "};" << std::endl << std::endl;
}

/**
 * Generate the dumper class.
 */
void generate_dumper_class(
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
        auto attributes = node->all_fields();
        source << "    out << \"" << node->title_case_name << "(\";" << std::endl;
        if (!source_location.empty()) {
            source << "    if (auto loc = node.get_annotation_ptr<" << source_location << ">()) {" << std::endl;
            source << "        out << \" # \" << *loc;" << std::endl;
            source << "    }" << std::endl;
        }
        source << "    out << std::endl;" << std::endl;
        if (!attributes.empty()) {
            source << "    indent++;" << std::endl;
            for (auto &attrib : attributes) {
                source << "    write_indent();" << std::endl;
                source << "    out << \"" << attrib.name;
                if (attrib.ext_type == Link || attrib.ext_type == OptLink) {
                    source << " --> ";
                } else {
                    source << ": ";
                }
                source << "\";" << std::endl;
                switch (attrib.ext_type) {
                    case Maybe:
                    case One:
                    case OptLink:
                    case Link:
                        source << "    if (node." << attrib.name << ".empty()) {" << std::endl;
                        if (attrib.ext_type == One || attrib.ext_type == Link) {
                            source << "        out << \"!MISSING\" << std::endl;" << std::endl;
                        } else {
                            source << "        out << \"-\" << std::endl;" << std::endl;
                        }
                        source << "    } else {" << std::endl;
                        source << "        out << \"<\" << std::endl;" << std::endl;
                        source << "        indent++;" << std::endl;
                        if (attrib.ext_type == Link || attrib.ext_type == OptLink) {
                            source << "        if (!in_link) {" << std::endl;
                            source << "            in_link = true;" << std::endl;
                            if (attrib.type == Prim) {
                                source << "            if (!node." << attrib.name << ".empty()) {" << std::endl;
                                source << "                node." << attrib.name << "->dump(out, indent);" << std::endl;
                                source << "            }" << std::endl;
                            } else {
                                source << "            node." << attrib.name << ".visit(*this);" << std::endl;
                            }
                            source << "            in_link = false;" << std::endl;
                            source << "        } else {" << std::endl;
                            source << "            write_indent();" << std::endl;
                            source << "            out << \"...\" << std::endl;" << std::endl;
                            source << "        }" << std::endl;
                        } else if (attrib.type == Prim) {
                            source << "        if (!node." << attrib.name << ".empty()) {" << std::endl;
                            source << "            node." << attrib.name << "->dump(out, indent);" << std::endl;
                            source << "        }" << std::endl;
                        } else {
                            source << "        node." << attrib.name << ".visit(*this);" << std::endl;
                        }
                        source << "        indent--;" << std::endl;
                        source << "        write_indent();" << std::endl;
                        source << "        out << \">\" << std::endl;" << std::endl;
                        source << "    }" << std::endl;
                        break;

                    case Any:
                    case Many:
                        source << "    if (node." << attrib.name << ".empty()) {" << std::endl;
                        if (attrib.ext_type == Many) {
                            source << "        out << \"!MISSING\" << std::endl;" << std::endl;
                        } else {
                            source << "        out << \"[]\" << std::endl;" << std::endl;
                        }
                        source << "    } else {" << std::endl;
                        source << "        out << \"[\" << std::endl;" << std::endl;
                        source << "        indent++;" << std::endl;
                        source << "        for (auto &sptr : node." << attrib.name << ") {" << std::endl;
                        source << "            if (!sptr.empty()) {" << std::endl;
                        if (attrib.type == Prim) {
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
                        source << "    out << node." << attrib.name << " << std::endl;" << std::endl;
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

/**
 * Generate the complete C++ code (source and header).
 */
void generate(
    const std::string &header_filename,
    const std::string &source_filename,
    Specification &specification
) {
    auto nodes = specification.nodes;

    // Open the output files.
    auto header = std::ofstream(header_filename);
    if (!header.is_open()) {
        std::cerr << "Failed to open header file for writing" << std::endl;
        std::exit(1);
    }
    auto source = std::ofstream(source_filename);
    if (!source.is_open()) {
        std::cerr << "Failed to open source file for writing" << std::endl;
        std::exit(1);
    }

    // Strip the path from the header filename such that it can be used for the
    // include guard and the #include directive in the source file.
    auto sep_pos = header_filename.rfind('/');
    auto backslash_pos = header_filename.rfind('\\');
    if (backslash_pos != std::string::npos && backslash_pos > sep_pos) {
        sep_pos = backslash_pos;
    }
    auto header_basename = header_filename.substr(sep_pos + 1);

    // Generate the include guard name.
    std::string include_guard = header_basename;
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
                for (auto field : node->fields) {
                    EdgeType typ;
                    if (field.node_type) {
                        header << " *   " << node->title_case_name;
                        header << " -> " << field.node_type->title_case_name;
                        typ = field.type;
                    } else {
                        std::string full_name = field.prim_type;
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
                        typ = field.ext_type;
                        prim_id++;
                    }
                    header << " [ label=\"" << field.name;
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
    if (!specification.tree_namespace.empty()) {
        auto tree_namespace = specification.tree_namespace + "::";
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
    if (!specification.header_fname.empty()) {
        source << "#include \"" << specification.header_fname << "\"" << std::endl;
    } else {
        source << "#include \"" << header_basename << "\"" << std::endl;
    }
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
    header << "class VisitorBase;" << std::endl;
    header << "template <typename T = void>" << std::endl;
    header << "class Visitor;" << std::endl;
    header << "class RecursiveVisitor;" << std::endl;
    header << "class Dumper;" << std::endl;
    header << std::endl;

    // Generate the NodeType enum.
    generate_enum(header, nodes);

    // Generate the base class.
    generate_base_class(
        header,
        source,
        nodes,
        !specification.serialize_fn.empty(),
        specification.support_namespace
    );

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
    generate_visitor_class(header, source, nodes);
    generate_recursive_visitor_class(header, source, nodes);
    generate_dumper_class(header, source, nodes, specification.source_location);

    // Generate the templated visit method and its specialization for void
    // return type.
    format_doc(header, "Visit this object.");
    header << "template <typename T>" << std::endl;
    header << "T Node::visit(Visitor<T> &visitor) {" << std::endl;
    header << "    T retval;" << std::endl;
    header << "    this->visit_internal(visitor, &retval);" << std::endl;
    header << "    return retval;" << std::endl;
    header << "}" << std::endl << std::endl;

    format_doc(header, "Visit this object.");
    header << "template <>" << std::endl;
    header << "void Node::visit(Visitor<void> &visitor);" << std::endl << std::endl;

    format_doc(source, "Visit this object.");
    source << "template <>" << std::endl;
    source << "void Node::visit(Visitor<void> &visitor) {" << std::endl;
    source << "    this->visit_internal(visitor);" << std::endl;
    source << "}" << std::endl << std::endl;

    // Overload the stream write operator.
    format_doc(header, "Stream << overload for tree nodes (writes debug dump).");
    header << "std::ostream &operator<<(std::ostream &os, const Node &object);" << std::endl << std::endl;
    format_doc(source, "Stream << overload for tree nodes (writes debug dump).");
    source << "std::ostream &operator<<(std::ostream &os, const Node &object) {" << std::endl;
    source << "    const_cast<Node&>(object).dump(os);" << std::endl;
    source << "    return os;" << std::endl;
    source << "}" << std::endl << std::endl;

    // Close the namespaces.
    for (auto name_it = specification.namespaces.rbegin(); name_it != specification.namespaces.rend(); name_it++) {
        header << "} // namespace " << *name_it << std::endl;
        source << "} // namespace " << *name_it << std::endl;
    }
    header << std::endl;
    source << std::endl;

}

} // namespace cpp
} // namespace tree_gen
