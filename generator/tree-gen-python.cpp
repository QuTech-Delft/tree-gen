/** \file
 * Python generation source file for \ref tree-gen.
 */

#include <fstream>
#include <iostream>
#include <unordered_set>
#include "tree-gen-python.hpp"

namespace tree_gen {
namespace python {

/**
 * Formats a Python docstring.
 */
void format_doc(
    std::ofstream &stream,
    const std::string &doc,
    const std::string &indent = ""
) {
    auto word = std::ostringstream();
    auto line = std::ostringstream();
    line << indent << "\"\"\"";
    bool line_empty = false;
    bool first_word = true;
    for (char c : doc) {
        bool flush = false;
        if (c == '\n' || c == ' ') {
            if (first_word) {
                first_word = false;
            } else {
                line << " ";
            }
            line << word.str();
            word.str("");
            line_empty = false;
            if (c == '\n') {
                flush = true;
            }
        } else {
            word << c;
            flush = !line_empty && line.str().size() + word.str().size() > 79;
        }
        if (flush) {
            stream << line.str() << std::endl;
            line.str("");
            line << indent;
            line_empty = true;
            first_word = true;
        }
    }
    if (!word.str().empty()) {
        line << " " << word.str();
        line_empty = false;
    }
    if (!line_empty) {
        stream << line.str();
    }
    if (line.str().size() + 3 > 79) {
        stream << std::endl << indent;
    }
    stream << "\"\"\"" << std::endl;
}

/**
 * Recursive function to print a muxing if statement for all node classes
 * derived from the given node class.
 */
void generate_deserialize_mux(
    std::ofstream &output,
    NodeType &node
) {
    if (node.derived.empty()) {
        output << "        if typ == '" << node.title_case_name << "':" << std::endl;
        output << "            return " << node.title_case_name << "._deserialize(cbor, seq_to_ob, links)" << std::endl;
    } else {
        for (auto &derived : node.derived) {
            generate_deserialize_mux(output, *(derived.lock()));
        }
    }
}

/**
 * Generates the class for the given node.
 */
void generate_node_class(
    std::ofstream &output,
    Specification &spec,
    NodeType &node
) {
    auto all_fields = node.all_children();

    // Print class header.
    output << "class " << node.title_case_name << "(";
    if (node.parent) {
        output << node.parent->title_case_name;
    } else {
        output << "Node";
    }
    output << "):" << std::endl;
    if (!node.doc.empty()) {
        format_doc(output, node.doc, "    ");
        output << std::endl;
    }

    // Print slots for the fields.
    output << "    __slots__ = [";
    if (!node.children.empty()) {
        output << std::endl;
        for (const auto &field : node.children) {
            output << "        '_attr_" << field.name << "'," << std::endl;
        }
        output << "    ";
    }
    output << "]" << std::endl << std::endl;

    // Print constructor.
    output << "    def __init__(";
    if (all_fields.empty()) {
        output << "self";
    } else {
        output << std::endl;
        output << "        self," << std::endl;
        for (const auto &field : all_fields) {
            output << "        " << field.name << "=None," << std::endl;
        }
        output << "    ";
    }
    output << "):" << std::endl;
    output << "        super().__init__(";
    if (node.parent) {
        bool first = true;
        for (const auto &field : node.parent->all_children()) {
            if (first) {
                first = false;
            } else {
                output << ", ";
            }
            output << field.name << "=" << field.name;
        }
    }
    output << ")" << std::endl;
    for (const auto &field : node.children) {
        output << "        self." << field.name << " = " << field.name << std::endl;
    }
    output << std::endl;

    // Print the field getters, setters, and deleters.
    for (const auto &field : node.children) {

        // Attributes representing Any or Many edges require a list-like class
        // to sit between the field accessors and the user to make things
        // like indexing work with type-safety. First figure out if this is such
        // a field.
        bool is_prim = field.type == Prim && field.ext_type == Prim;
        bool is_any = field.type == Any || (field.type == Prim && field.ext_type == Any);
        bool is_many = field.type == Many || (field.type == Prim && field.ext_type == Many);
        bool is_any_or_many = is_any || is_many;
        bool is_link = field.type == Link || (field.type == Prim && field.ext_type == Link);
        is_link |= field.type == OptLink || (field.type == Prim && field.ext_type == OptLink);
        std::string type = (field.type == Prim) ? field.py_prim_type : field.node_type->title_case_name;

        if (is_any_or_many) {
            type = "Multi" + type;
        }

        // Getter.
        output << "    @property" << std::endl;
        output << "    def " << field.name << "(self):" << std::endl;
        if (!field.doc.empty()) {
            format_doc(output, field.doc, "        ");
        }
        output << "        return self._attr_" << field.name << std::endl << std::endl;

        // Setter. Assigning None is the same as deleting.
        output << "    @" << field.name << ".setter" << std::endl;
        output << "    def " << field.name << "(self, val):" << std::endl;
        output << "        if val is None:" << std::endl;
        output << "            del self." << field.name << std::endl;
        output << "            return" << std::endl;
        output << "        if not isinstance(val, " << type << "):" << std::endl;
        if (!is_link) {
            output << "            # Try to \"typecast\" if this isn't an obvious mistake." << std::endl;
            output << "            if isinstance(val, Node):" << std::endl;
            output << "                raise TypeError('" << field.name << " must be of type " << type << "')" << std::endl;
            output << "            val = " << type << "(val)" << std::endl;
        } else {
            // Can't typecast links; making a new object makes no sense.
            output << "            raise TypeError('" << field.name << " must be of type " << type << "')" << std::endl;
        }
        output << "        self._attr_" << field.name << " = ";
        output << type << "(val)" << std::endl << std::endl;

        // Deleter. Doesn't actually delete, but rather replaces with the
        // default value.
        output << "    @" << field.name << ".deleter" << std::endl;
        output << "    def " << field.name << "(self):" << std::endl;
        output << "        self._attr_" << field.name;
        if (is_prim || is_any_or_many) {
            output << " = " << type << "()";
        } else {
            output << " = None";
        }
        output << std::endl << std::endl;

    }

    // Print equality function.
    if (node.derived.empty()) {
        output << "    def __eq__(self, other):" << std::endl;
        format_doc(output, "Equality operator. Ignores annotations!", "        ");
        output << "        if not isinstance(other, " << node.title_case_name << "):" << std::endl;
        output << "            return False" << std::endl;
        for (const auto &field : all_fields) {
            AttributeType type = (field.type == Prim) ? field.ext_type : field.type;
            switch (type) {
                case Maybe:
                case One:
                case Any:
                case Many:
                case Prim:
                    output << "        if self." << field.name << " != other." << field.name << ":" << std::endl;
                    break;
                case Link:
                case OptLink:
                    output << "        if self." << field.name << " is not other." << field.name << ":" << std::endl;
                    break;
            }
            output << "            return False" << std::endl;
        }
        output << "        return True" << std::endl << std::endl;
    }

    // Print dump function.
    if (node.derived.empty()) {
        output << "    def dump(self, indent=0, annotations=None, links=1):" << std::endl;
        format_doc(output,
                   "Returns a debug representation of this tree as a "
                   "multiline string. indent is the number of double spaces "
                   "prefixed before every line. annotations, if specified, "
                   "must be a set-like object containing the key strings of "
                   "the annotations that are to be printed. links specifies "
                   "the maximum link recursion depth.", "        ");
        output << "        s = ['  '*indent]" << std::endl;
        output << "        s.append('" << node.title_case_name << "(')" << std::endl;
        output << "        if annotations is None:" << std::endl;
        output << "            annotations = []" << std::endl;
        output << "        for key in annotations:" << std::endl;
        output << "            if key in self:" << std::endl;
        output << "                s.append(' # {}: {}'.format(key, self[key]))" << std::endl;
        output << "        s.append('\\n')" << std::endl;
        if (!all_fields.empty()) {
            output << "        indent += 1" << std::endl;
            for (auto &field : all_fields) {
                AttributeType type = (field.type == Prim) ? field.ext_type : field.type;
                output << "        s.append('  '*indent)" << std::endl;
                output << "        s.append('" << field.name;
                if (type == Link || type == OptLink) {
                    output << " --> ";
                } else {
                    output << ": ";
                }
                output << "')" << std::endl;
                switch (type) {
                    case Maybe:
                    case One:
                    case OptLink:
                    case Link:
                        output << "        if self." << field.name << " is None:" << std::endl;
                        if (type == One || type == Link) {
                            output << "            s.append('!MISSING\\n')" << std::endl;
                        } else {
                            output << "            s.append('-\\n')" << std::endl;
                        }
                        output << "        else:" << std::endl;
                        output << "            s.append('<\\n')" << std::endl;
                        if (field.ext_type == Link || field.ext_type == OptLink) {
                            output << "            if links:" << std::endl;
                            output << "                s.append(self." << field.name << ".dump(indent + 1, annotations, links - 1) + '\\n')" << std::endl;
                            output << "            else:" << std::endl;
                            output << "                s.append('  '*(indent+1) + '...\\n')" << std::endl;
                        } else {
                            output << "            s.append(self." << field.name << ".dump(indent + 1, annotations, links) + '\\n')" << std::endl;
                        }
                        output << "            s.append('  '*indent + '>\\n')" << std::endl;
                        break;

                    case Any:
                    case Many:
                        output << "        if not self." << field.name << ":" << std::endl;
                        if (field.ext_type == Many) {
                            output << "            s.append('!MISSING\\n')" << std::endl;
                        } else {
                            output << "            s.append('-\\n')" << std::endl;
                        }
                        output << "        else:" << std::endl;
                        output << "            s.append('[\\n')" << std::endl;
                        output << "            for child in self." << field.name << ":" << std::endl;
                        output << "                s.append(self." << field.name << ".dump(indent + 1, annotations, links) + '\\n')" << std::endl;
                        output << "            s.append('  '*indent + ']\\n')" << std::endl;
                        break;

                    case Prim:
                        output << "        s.append(str(self." << field.name << ") + '\\n')" << std::endl;
                        break;

                }
            }
            output << "        indent -= 1" << std::endl;
            output << "        s.append('  '*indent)" << std::endl;
        }
        output << "        s.append(')')" << std::endl;
        output << "        return ''.join(s)" << std::endl << std::endl;
        output << "    __str__ = dump" << std::endl;
        output << "    __repr__ = dump" << std::endl << std::endl;
    }

    // Print find_reachable() function.
    if (node.derived.empty()) {
        output << "    def find_reachable(self, id_map=None):" << std::endl;
        format_doc(output,
                    "Returns a dictionary mapping Python id() values to "
                    "stable sequence numbers for all nodes in the tree rooted "
                    "at this node. If id_map is specified, found nodes are "
                    "appended to it.", "        ");
        output << "        if id_map is None:" << std::endl;
        output << "            id_map = {}" << std::endl;
        output << "        if id(self) in id_map:" << std::endl;
        output << "            raise NotWellFormed('node {!r} with id {} occurs more than once'.format(self, id(self)))" << std::endl;
        output << "        id_map[id(self)] = len(id_map)" << std::endl;
        for (const auto &field : all_fields) {
            AttributeType type = (field.type == Prim) ? field.ext_type : field.type;
            switch (type) {
                case Maybe:
                case One:
                    output << "        if self._attr_" << field.name << " is not None:" << std::endl;
                    output << "            self._attr_" << field.name << ".find_reachable(id_map)" << std::endl;
                    break;
                case Any:
                case Many:
                    output << "        for el in self._attr_" << field.name << ":" << std::endl;
                    output << "            el.find_reachable(id_map)" << std::endl;
                    break;
                case Link:
                case OptLink:
                case Prim:
                    break;
            }
        }
        output << "        return id_map" << std::endl << std::endl;
    }

    // Print check_complete() function.
    if (node.derived.empty()) {
        output << "    def check_complete(self, id_map=None):" << std::endl;
        format_doc(output,
                   "Raises NotWellFormed if the tree rooted at this node "
                   "is not well-formed. If id_map is specified, this tree is "
                   "only a subtree in the context of a larger tree, and id_map "
                   "must be a dict mapping from Python id() codes to tree "
                   "indices for all reachable nodes.", "        ");
        output << "        if id_map is None:" << std::endl;
        output << "            id_map = self.find_reachable()" << std::endl;
        for (const auto &field : all_fields) {
            AttributeType type = (field.type == Prim) ? field.ext_type : field.type;
            switch (type) {
                case One:
                    output << "        if self._attr_" << field.name << " is None:" << std::endl;
                    output << "            raise NotWellFormed('";
                    output << field.name << " is required but not set')" << std::endl;
                    // fallthrough
                case Maybe:
                    output << "        if self._attr_" << field.name << " is not None:" << std::endl;
                    output << "            self._attr_" << field.name << ".check_well_formed(id_map)" << std::endl;
                    break;
                case Many:
                    output << "        if not self._attr_" << field.name << ":" << std::endl;
                    output << "            raise NotWellFormed('";
                    output << field.name << " needs at least one node but has zero')" << std::endl;
                    // fallthrough
                case Any:
                    output << "        for child in self._attr_" << field.name << ":" << std::endl;
                    output << "            child.check_well_formed(id_map)" << std::endl;
                    break;
                case Link:
                    output << "        if self._attr_" << field.name << " is None:" << std::endl;
                    output << "            raise NotWellFormed('";
                    output << field.name << " is required but not set')" << std::endl;
                    // fallthrough
                case OptLink:
                    output << "        if self._attr_" << field.name << " is not None:" << std::endl;
                    output << "            if id(self._attr_" << field.name << ") not in id_map:" << std::endl;
                    output << "                raise NotWellFormed('";
                    output << field.name << " links to unreachable node')" << std::endl;
                    break;
                case Prim:
                    break;
            }
        }
        output << std::endl;
    }

    // Print copy() function.
    if (node.derived.empty()) {
        output << "    def copy(self):" << std::endl;
        format_doc(output, "Returns a shallow copy of this node.", "        ");
        output << "        return " << node.title_case_name << "(" << std::endl;
        bool first = true;
        for (const auto &field : all_fields) {
            if (first) {
                first = false;
            } else {
                output << "," << std::endl;
            }
            output << "            " << field.name << "=";
            auto type = (field.type != Prim) ? field.type : field.ext_type;
            switch (type) {
                case Maybe:
                case One:
                case OptLink:
                case Link:
                case Prim:
                    output << "self._attr_" << field.name;
                    break;
                case Any:
                case Many:
                    output << "self._attr_" << field.name << ".copy()";
                    break;
            }
        }
        output << std::endl << "        )" << std::endl << std::endl;
    }

    // Print clone() function.
    if (node.derived.empty()) {
        output << "    def clone(self):" << std::endl;
        format_doc(output,
                   "Returns a deep copy of this node. This mimics the "
                   "C++ interface, deficiencies with links included; that is, "
                   "links always point to the original tree. If you're not "
                   "cloning a subtree in a context where this is the desired "
                   "behavior, you may want to use the copy.deepcopy() from the "
                   "stdlib instead, which should copy links correctly.",
                   "        ");
        output << "        return " << node.title_case_name << "(" << std::endl;
        bool first = true;
        for (const auto &field : all_fields) {
            if (first) {
                first = false;
            } else {
                output << "," << std::endl;
            }
            output << "            " << field.name << "=";
            auto type = (field.type != Prim) ? field.type : field.ext_type;
            switch (type) {
                case Maybe:
                case One:
                case Any:
                case Many:
                case Prim:
                    output << "_cloned(self._attr_" << field.name << ")";
                    break;
                case OptLink:
                case Link:
                    output << "self._attr_" << field.name;
                    break;
            }
        }
        output << std::endl << "        )" << std::endl << std::endl;
    }

    // Print deserialize() function.
    output << "    @staticmethod" << std::endl;
    output << "    def _deserialize(cbor, seq_to_ob, links):" << std::endl;
    format_doc(output,
               "Attempts to deserialize the given cbor object (in Python "
               "primitive representation) into a node of this type. All "
               "(sub)nodes are added to the seq_to_ob dict, indexed by their "
               "cbor sequence number. All links are registered in the links "
               "list by means of a two-tuple of the setter function for the "
               "link field and the sequence number of the target node.",
               "        ");
    output << "        if not isinstance(cbor, dict):" << std::endl;
    output << "            throw TypeError('node description object must be a dict')" << std::endl;
    output << "        typ = cbor.get('@t', None)" << std::endl;
    output << "        if typ is None:" << std::endl;
    output << "            raise ValueError('type (@t) field is missing from node serialization')" << std::endl;
    if (node.derived.empty()) {
        output << "        if typ != '" << node.title_case_name << "':" << std::endl;
        output << "            raise ValueError('found node serialization for ' + typ + ', but expected ";
        output << node.title_case_name << "')" << std::endl;
        if (all_fields.empty()) {
            output << std::endl;
            output << "        # Construct the " << node.title_case_name << " node." << std::endl;
            output << "        node = " << node.title_case_name << "()" << std::endl;
        } else {
            std::vector<std::string> links;
            for (const auto &field : all_fields) {
                output << std::endl;
                output << "        # Deserialize the " << field.name << " field." << std::endl;
                output << "        field = cbor.get('" << field.name << "', None)" << std::endl;
                output << "        if not isinstance(field, dict):" << std::endl;
                output << "            raise ValueError('missing or invalid serialization of field " << field.name << "')" << std::endl;
                auto type = (field.type == Prim) ? field.ext_type : field.type;
                auto type_name = (field.type == Prim) ? field.py_prim_type : field.node_type->title_case_name;
                auto multi_name = (field.type == Prim) ? field.py_multi_type : ("Multi" + field.node_type->title_case_name);
                if (type != Prim) {
                    output << "        if field.get('@T') != '";
                    switch (type) {
                        case Maybe:   output << "?"; break;
                        case One:     output << "1"; break;
                        case Any:     output << "*"; break;
                        case Many:    output << "+"; break;
                        case OptLink: output << "@"; break;
                        case Link:    output << "$"; break;
                    }
                    output << "':" << std::endl;
                    output << "            raise ValueError('unexpected edge type for field " << field.name << "')" << std::endl;
                }
                switch (type) {
                    case Maybe:
                    case One:
                        output << "        if field.get('@t', None) is None:" << std::endl;
                        output << "            f_" << field.name << " = None" << std::endl;
                        output << "        else:" << std::endl;
                        output << "            f_" << field.name << " = " << type_name << "._deserialize(field, seq_to_ob, links)" << std::endl;
                        break;
                    case Any:
                    case Many:
                        output << "        data = field.get('@d', None)" << std::endl;
                        output << "        if not isinstance(data, list):" << std::endl;
                        output << "            raise ValueError('missing serialization of Any/Many contents')" << std::endl;
                        output << "        f_" << field.name << " = " << multi_name << "()" << std::endl;
                        output << "        for element in data:" << std::endl;
                        output << "            if element.get('@T') != '1':" << std::endl;
                        output << "                raise ValueError('unexpected edge type for Any/Many element')" << std::endl;
                        output << "            f_" << field.name << ".append(" << type_name << "._deserialize(element, seq_to_ob, links))" << std::endl;
                        break;
                    case Link:
                    case OptLink:
                        output << "        f_" << field.name << " = None" << std::endl;
                        output << "        l_" << field.name << " = field.get('@l', None)" << std::endl;
                        links.push_back(field.name);
                        break;
                    case Prim:
                        output << "        if hasattr(" << field.py_prim_type << ", 'deserialize_cbor'):" << std::endl;
                        output << "            f_" << field.name << " = " << field.py_prim_type << ".deserialize_cbor(field)" << std::endl;
                        output << "        else:" << std::endl;
                        if (spec.py_deserialize_fn.empty()) {
                            output << "            raise ValueError('no deserialization function seems to exist for field type " << field.py_prim_type << "')" << std::endl;
                        } else {
                            output << "            f_" << field.name << " = " << spec.py_deserialize_fn << "(" << field.py_prim_type << ", field)" << std::endl;
                        }
                        break;
                }
            }
            output << std::endl;
            output << "        # Construct the " << node.title_case_name << " node." << std::endl;
            output << "        node = " << node.title_case_name << "(";
            bool first = true;
            for (const auto &field : all_fields) {
                if (first) {
                    first = false;
                } else {
                    output << ", ";
                }
                output << "f_" << field.name;
            }
            output << ")" << std::endl;
            if (!links.empty()) {
                output << std::endl;
                output << "        # Register links to be made after tree construction." << std::endl;
                for (const auto &link : links) {
                    output << "        links.append((lambda val: " << node.title_case_name << "." << link << ".fset(node, val), l_" << link << "))" << std::endl;
                }
            }
        }
        output << std::endl;
        output << "        # Deserialize annotations." << std::endl;
        output << "        for key, val in cbor.items():" << std::endl;
        output << "            if not (key.startswith('{') and key.endswith('}')):" << std::endl;
        output << "                continue" << std::endl;
        output << "            key = key[1:-1]" << std::endl;
        if (spec.py_deserialize_fn.empty()) {
            output << "            node[key] = val" << std::endl;
        } else {
            output << "            node[key] = " << spec.py_deserialize_fn << "(key, val)" << std::endl;
        }
        output << std::endl;
        output << "        # Register node in sequence number lookup." << std::endl;
        output << "        seq = cbor.get('@i', None)" << std::endl;
        output << "        if not isinstance(seq, int):" << std::endl;
        output << "            raise ValueError('sequence number field (@i) is not an integer or missing from node serialization')" << std::endl;
        output << "        if seq in seq_to_ob:" << std::endl;
        output << "            raise ValueError('duplicate sequence number %d' % seq)" << std::endl;
        output << "        seq_to_ob[seq] = node" << std::endl << std::endl;
        output << "        return node" << std::endl;
    } else {
        generate_deserialize_mux(output, node);
        output << "        raise ValueError('unknown or unexpected type (@t) found in node serialization')" << std::endl;
    }
    output << std::endl;

    // Print serialize() function.
    output << "    def _serialize(self, id_map):" << std::endl;
    format_doc(output,
               "Serializes this node to the Python primitive "
               "representation of its CBOR serialization. The tree that the "
               "node belongs to must be well-formed. id_map must match Python "
               "id() calls for all nodes to unique integers, to use for the "
               "sequence number representation of links.",
               "        ");
    output << "        cbor = {'@i': id_map[id(self)], '@t': '" << node.title_case_name << "'}" << std::endl;
    for (const auto &field : all_fields) {
        output << std::endl;
        output << "        # Serialize the " << field.name << " field." << std::endl;
        auto type = (field.type == Prim) ? field.ext_type : field.type;
        auto type_name = (field.type == Prim) ? field.py_prim_type : field.node_type->title_case_name;
        if (type == Prim) {
            output << "        if hasattr(self._attr_" << field.name << ", 'serialize_cbor'):" << std::endl;
            output << "            cbor['" << field.name << "'] = self._attr_" << field.name << ".serialize_cbor()" << std::endl;
            output << "        else:" << std::endl;
            if (spec.py_serialize_fn.empty()) {
                output << "            raise ValueError('no serialization function seems to exist for field type " << field.py_prim_type << "')" << std::endl;
            } else {
                output << "            cbor['" << field.name << "'] = " << spec.py_serialize_fn << "(" << field.py_prim_type << ", self._attr_" << field.name << ")" << std::endl;
            }
        } else {
            output << "        field = {'@T:': '";
            switch (type) {
                case Maybe:   output << "?"; break;
                case One:     output << "1"; break;
                case Any:     output << "*"; break;
                case Many:    output << "+"; break;
                case OptLink: output << "@"; break;
                case Link:    output << "$"; break;
            }
            output << "'}" << std::endl;
            switch (type) {
                case Maybe:
                case One:
                    output << "        if self._attr_" << field.name << " is None:" << std::endl;
                    output << "            field['@t'] = None" << std::endl;
                    output << "        else:" << std::endl;
                    output << "            field.update(self._attr_" << field.name << "._serialize(id_map)" << std::endl;
                    break;
                case Any:
                case Many:
                    output << "        lst = []" << std::endl;
                    output << "        for el in self._attr_" << field.name << ":" << std::endl;
                    output << "            el = el._serialize(id_map)" << std::endl;
                    output << "            el['@T'] = '1'" << std::endl;
                    output << "            lst.append(el)" << std::endl;
                    output << "        field['@d'] = lst" << std::endl;
                    break;
                case Link:
                case OptLink:
                    output << "        if self._attr_" << field.name << " is None:" << std::endl;
                    output << "            field['@l'] = None" << std::endl;
                    output << "        else:" << std::endl;
                    output << "            field['@l'] = id_map[id(self)]" << std::endl;
                    break;
            }
            output << "        cbor['" << field.name << "'] = field" << std::endl;
        }
    }
    output << std::endl;
    output << "        # Serialize annotations." << std::endl;
    output << "        for key, val in cbor.items():" << std::endl;
    if (spec.py_serialize_fn.empty()) {
        output << "            try:" << std::endl;
        output << "                cbor['{%s}' % key] = _py_to_cbor(val)" << std::endl;
        output << "            except TypeError:" << std::endl;
        output << "                pass" << std::endl;
    } else {
        output << "            cbor['{%s}' % key] = _py_to_cbor(" << spec.py_serialize_fn << "(key, val))" << std::endl;
    }
    output << std::endl;
    output << "        return cbor" << std::endl << std::endl;

    output << std::endl;

    // Print Multi* class.
    output << "class Multi" << node.title_case_name << "(_Multiple):" << std::endl;
    auto doc = "Wrapper for an edge with multiple " + node.title_case_name + " objects.";
    format_doc(output, doc, "    ");
    output << std::endl;
    output << "    _T = " << node.title_case_name << std::endl;
    output << std::endl << std::endl;

}

/**
 * Generates the complete Python code.
 */
void generate(
    const std::string &python_filename,
    Specification &specification
) {
    auto nodes = specification.nodes;

    // Open the output file.
    auto output = std::ofstream(python_filename);
    if (!output.is_open()) {
        std::cerr << "Failed to open Python file for writing" << std::endl;
        std::exit(1);
    }

    // Generate header.
    if (!specification.python_doc.empty()) {
        format_doc(output, specification.python_doc);
        output << std::endl;
    }
    output << "import functools" << std::endl;
    output << "import struct" << std::endl;
    for (auto &include : specification.python_includes) {
        output << include << std::endl;
    }
    output << std::endl;

    // Write the classes that are always the same.
    output << R"PY(
def _cbor_read_intlike(cbor, offset, info):
    """Parses the additional information and reads any additional bytes it
    specifies the existence of, and returns the encoded integer. offset
    should point to the byte immediately following the initial byte. Returns
    the encoded integer and the offset immediately following the object."""

    # Info less than 24 is a shorthand for the integer itself.
    if info < 24:
        return info, offset

    # 25 is 8-bit following the info byte.
    if info == 25:
        return cbor[offset], offset + 1

    # 26 is 16-bit following the info byte.
    if info == 26:
        val, = struct.unpack('>H', cbor[offset:offset+2])
        return val, offset + 2

    # 27 is 32-bit following the info byte.
    if info == 27:
        val, = struct.unpack('>I', cbor[offset:offset+4])
        return val, offset + 4

    # 28 is 64-bit following the info byte.
    if info == 28:
        val, = struct.unpack('>Q', cbor[offset:offset+8])
        return val, offset + 8

    # Info greater than or equal to 28 is illegal. Note that 31 is used for
    # indefinite lengths, so this must be checked prior to calling this
    # method.
    raise ValueError("invalid CBOR: illegal additional info for integer or object length")


def _sub_cbor_to_py(cbor, offset):
    """Converts the CBOR object starting at cbor[offset] to its Python
    representation for as far as tree-gen supports CBOR. Returns this Python
    representation and the offset immediately following the CBOR representation
    thereof. Supported types:

     - 0: unsigned integer (int)
     - 1: negative integer (int)
     - 2: byte string (bytes)
     - 3: UTF-8 string (str)
     - 4: array (list)
     - 5: map (dict)
     - 6: semantic tag (ignored)
     - 7.20: false (bool)
     - 7.21: true (bool)
     - 7.22: null (NoneType)
     - 7.27: double-precision float (float)

    Both definite-length and indefinite-length notation is supported for sized
    objects (strings, arrays, maps). A ValueError is thrown if the CBOR is
    invalid or contains unsupported structures."""

    # Read the initial byte.
    initial = cbor[offset]
    typ = initial >> 5
    info = initial & 0x1F
    offset += 1

    # Handle unsigned integer (0) and negative integer (1).
    if typ <= 1:
        value, offset = _cbor_read_intlike(cbor, offset, info)
        if typ == 1:
            value = -1 - value
        return value, offset

    # Handle byte string (2) and UTF-8 string (3).
    if typ <= 3:

        # Gather components of the string in here.
        if info == 31:

            # Handle indefinite length strings. These consist of a
            # break-terminated (0xFF) list of definite-length strings of the
            # same type.
            value = []
            while True:
                sub_initial = cbor[offset]; offset += 1
                if sub_initial == 0xFF:
                    break
                sub_typ = sub_initial >> 5
                sub_info = sub_initial & 0x1F
                if sub_typ != typ:
                    raise ValueError('invalid CBOR: illegal indefinite-length string component')

                # Seek past definite-length string component. The size in
                # bytes is encoded as an integer.
                size, offset = _cbor_read_intlike(cbor, offset, sub_info)
                value.append(cbor[offset:offset + size])
                offset += size
            value = b''.join(value)

        else:

            # Handle definite-length strings. The size in bytes is encoded as
            # an integer.
            size, offset = _cbor_read_intlike(cbor, offset, info)
            value = cbor[offset:offset + size]
            offset += size

        if typ == 3:
            value = value.decode('UTF-8')
        return value, offset

    # Handle array (4) and map (5).
    if typ <= 5:

        # Create result container.
        container = [] if typ == 4 else {}

        # Handle indefinite length arrays and maps.
        if info == 31:

            # Read objects/object pairs until we encounter a break.
            while cbor[offset] != 0xFF:
                if typ == 4:
                    value, offset = _sub_cbor_to_py(cbor, offset)
                    container.append(value)
                else:
                    key, offset = _sub_cbor_to_py(cbor, offset)
                    if not isinstance(key, str):
                        raise ValueError('invalid CBOR: map key is not a UTF-8 string')
                    value, offset = _sub_cbor_to_py(cbor, offset)
                    container[key] = value

            # Seek past the break.
            offset += 1

        else:

            # Handle definite-length arrays and maps. The amount of
            # objects/object pairs is encoded as an integer.
            size, offset = _cbor_read_intlike(cbor, offset, info)
            for _ in range(size):
                if typ == 4:
                    value, offset = _sub_cbor_to_py(cbor, offset)
                    container.append(value)
                else:
                    key, offset = _sub_cbor_to_py(cbor, offset)
                    if not isinstance(key, str):
                        raise ValueError('invalid CBOR: map key is not a UTF-8 string')
                    value, offset = _sub_cbor_to_py(cbor, offset)
                    container[key] = value

        return container, offset

    # Handle semantic tags.
    if typ == 6:

        # We don't use semantic tags for anything, but ignoring them is
        # legal and reading past them is easy enough.
        _, offset = _cbor_read_intlike(cbor, offset, info)
        return _sub_cbor_to_py(cbor, offset)

    # Handle major type 7. Here, the type is defined by the additional info.
    # Additional info 24 is reserved for having the type specified by the
    # next byte, but all such values are unassigned.
    if info == 20:
        # false
        return False, offset

    if info == 21:
        # true
        return True, offset

    if info == 22:
        # null
        return None, offset

    if info == 23:
        # Undefined value.
        raise ValueError('invalid CBOR: undefined value is not supported')

    if info == 25:
        # Half-precision float.
        raise ValueError('invalid CBOR: half-precision float is not supported')

    if info == 26:
        # Single-precision float.
        raise ValueError('invalid CBOR: single-precision float is not supported')

    if info == 27:
        # Double-precision float.
        value, = struct.unpack('>d', cbor[offset:offset+8])
        return value, offset + 8

    if info == 31:
        # Break value used for indefinite-length objects.
        raise ValueError('invalid CBOR: unexpected break')

    raise ValueError('invalid CBOR: unknown type code')


def _cbor_to_py(cbor):
    """Converts the given CBOR object (bytes) to its Python representation for
    as far as tree-gen supports CBOR. Supported types:

     - 0: unsigned integer (int)
     - 1: negative integer (int)
     - 2: byte string (bytes)
     - 3: UTF-8 string (str)
     - 4: array (list)
     - 5: map (dict)
     - 6: semantic tag (ignored)
     - 7.20: false (bool)
     - 7.21: true (bool)
     - 7.22: null (NoneType)
     - 7.27: double-precision float (float)

    Both definite-length and indefinite-length notation is supported for sized
    objects (strings, arrays, maps). A ValueError is thrown if the CBOR is
    invalid or contains unsupported structures."""

    value, length = _sub_cbor_to_py(cbor, 0)
    if length < len(cbor):
        raise ValueError('invalid CBOR: garbage at the end')
    return value


class _Cbor(bytes):
    """Marker class indicating that this bytes object represents CBOR."""
    pass


def _cbor_write_intlike(value, major=0):
    """Converts the given integer to its minimal representation in CBOR. The
    major code can be overridden to write lengths for strings, arrays, and
    maps."""

    # Negative integers use major code 1.
    if value < 0:
        major = 1
        value = -1 - value
    initial = major << 5

    # Use the minimal representation.
    if value < 24:
        return struct.pack('>B', initial | value)
    if value < 0x100:
        return struct.pack('>BB', initial | 24, value)
    if value < 0x10000:
        return struct.pack('>BH', initial | 25, value)
    if value < 0x100000000:
        return struct.pack('>BI', initial | 26, value)
    if value < 0x10000000000000000:
        return struct.pack('>BQ', initial | 27, value)

    raise ValueError('integer too large for CBOR (bigint not supported)')


def _py_to_cbor(value, type_converter=None):
    """Inverse of _cbor_to_py(). type_converter optionally specifies a function
    that takes a value and either converts it to a primitive for serialization,
    converts it to a _Cbor object manually, or raises a TypeError if no
    conversion is known. If no type_converter is specified, a TypeError is
    raised in all cases the type_converter would otherwise be called. The cbor
    serialization is returned using a _Cbor object, which is just a marker class
    behaving just like bytes."""
    if isinstance(value, _Cbor):
        return value

    if isinstance(value, int):
        return _Cbor(_cbor_write_intlike(value))

    if isinstance(value, float):
        return _Cbor(struct.pack('>Bd', 0xFB, value))

    if isinstance(value, str):
        value = value.encode('UTF-8')
        return _Cbor(_cbor_write_intlike(len(value), 3) + value)

    if isinstance(value, bytes):
        return _Cbor(_cbor_write_intlike(len(value), 2) + value)

    if value is False:
        return _Cbor(b'\xF4')

    if value is True:
        return _Cbor(b'\xF5')

    if value is None:
        return _Cbor(b'\xF6')

    if isinstance(value, (list, tuple)):
        cbor = [_cbor_write_intlike(len(value), 4)]
        for val in value:
            cbor.append(_py_to_cbor(val, type_converter))
        return _Cbor(b''.join(cbor))

    if isinstance(value, dict):
        cbor = [_cbor_write_intlike(len(value), 5)]
        for key, val in sorted(value.items()):
            if not isinstance(key, str):
                raise TypeError('dict keys must be strings')
            cbor.append(_py_to_cbor(key, type_converter))
            cbor.append(_py_to_cbor(val, type_converter))
        return _Cbor(b''.join(cbor))

    if type_converter is not None:
        return _py_to_cbor(type_converter(value))

    raise TypeError('unsupported type for conversion to cbor: %r' % (value,))


class NotWellFormed(ValueError):
    """Exception class for well-formedness checks."""

    def __init__(self, msg):
        super().__init__('not well-formed: ' + str(msg))


class Node(object):
    """Base class for nodes."""

    __slots__ = ['_annot']

    def __init__(self):
        super().__init__()
        self._annot = {}

    def __getitem__(self, key):
        """Returns the annotation object with the specified key, or raises
        KeyError if not found."""
        if not isinstance(key, str):
            raise TypeError('indexing a node with something other than an '
                            'annotation key string')
        return self._annot[key]

    def __setitem__(self, key, val):
        """Assigns the annotation object with the specified key."""
        if not isinstance(key, str):
            raise TypeError('indexing a node with something other than an '
                            'annotation key string')
        self._annot[key] = val

    def __delitem__(self, key):
        """Deletes the annotation object with the specified key."""
        if not isinstance(key, str):
            raise TypeError('indexing a node with something other than an '
                            'annotation key string')
        del self._annot[key]

    def __contains__(self, key):
        """Returns whether an annotation exists for the specified key."""
        return key in self._annot

    @staticmethod
    def find_reachable(self, id_map=None):
        """Returns a dictionary mapping Python id() values to stable sequence
        numbers for all nodes in the tree rooted at this node. If id_map is
        specified, found nodes are appended to it. Note that this is overridden
        by the actual node class implementations; this base function does very
        little."""
        if id_map is None:
            id_map = {}
        return id_map

    def check_complete(self, id_map=None):
        """Raises NotWellFormed if the tree rooted at this node is not
        well-formed. If id_map is specified, this tree is only a subtree in the
        context of a larger tree, and id_map must be a dict mapping from Python
        id() codes to tree indices for all reachable nodes. Note that this is
        overridden by the actual node class implementations; this base function
        always raises an exception."""
        raise NotWellFormed('found node of abstract type ' + type(self).__name__)

    def check_well_formed(self):
        """Checks whether the tree starting at this node is well-formed. That
        is:

         - all One, Link, and Many edges have (at least) one entry;
         - all the One entries internally stored by Any/Many have an entry;
         - all Link and filled OptLink nodes link to a node that's reachable
           from this node;
         - the nodes referred to be One/Maybe only appear once in the tree
           (except through links).

        If it isn't well-formed, a NotWellFormed is thrown."""
        self.check_complete()

    def is_well_formed(self):
        """Returns whether the tree starting at this node is well-formed. That
        is:

         - all One, Link, and Many edges have (at least) one entry;
         - all the One entries internally stored by Any/Many have an entry;
         - all Link and filled OptLink nodes link to a node that's reachable
           from this node;
         - the nodes referred to be One/Maybe only appear once in the tree
           (except through links)."""
        try:
            self.check_well_formed()
            return True
        except NotWellFormed:
            return False

    def copy(self):
        """Returns a shallow copy of this node. Note that this is overridden by
        the actual node class implementations; this base function always raises
        an exception."""
        raise TypeError('can\'t copy node of abstract type ' + type(self).__name__)

    def clone(self):
        """Returns a deep copy of this node. Note that this is overridden by
        the actual node class implementations; this base function always raises
        an exception."""
        raise TypeError('can\'t clone node of abstract type ' + type(self).__name__)

    @classmethod
    def deserialize(cls, cbor):
        """Attempts to deserialize the given cbor object (either as bytes or as
        its Python primitive representation) into a node of this type."""
        if isinstance(cbor, bytes):
            cbor = _cbor_to_py(cbor)
        seq_to_ob = {}
        links = []
        cls._deserialize(cbor, seq_to_ob, links)
        for link_setter, seq in links:
            ob = seq_to_ob.get(seq, None)
            if ob is None:
                raise ValueError('found link to nonexistent object')
            link_setter(ob)

    def serialize(self):
        """Serializes this node into its cbor representation in the form of a
        bytes object."""
        id_map = self.find_reachable()
        self.check_well_formed(id_map)
        return _py_to_cbor(self._serialize(id_map))

    @staticmethod
    def _deserialize(cbor, seq_to_ob, links):
        raise NotImplementedError('please call deserialize() on the node type you\'re expecting')


@functools.total_ordering
class _Multiple(object):
    """Base class for the Any* and Many* edge helper classes. Inheriting
    classes must set the class constant _T to the node type they are made
    for."""

    __slots__ = ['_l']

    def __init__(self,  *args, **kwargs):
        super().__init__()
        self._l = list(*args, **kwargs)
        for idx, val in enumerate(self._l):
            if not isinstance(val, self._T):
                raise TypeError(
                    'object {!r} at index {:d} is not an instance of {!r}'
                    .format(val, idx, self._T))

    def __repr__(self):
        return '{}({!r})'.format(type(self).__name__, self._l)

    def clone(self):
        return self.__class__(map(lambda node: node.clone(), self._l))

    def __len__(self):
        return len(self._l)

    def __getitem__(self, idx):
        return self._l[idx]

    def __setitem__(self, idx, val):
        if not isinstance(val, self._T):
            raise TypeError(
                'object {!r} at index {:d} is not an instance of {!r}'
                .format(val, idx, self._T))
        self._l[idx] = val

    def __delitem__(self, idx):
        del self._l[idx]

    def __iter__(self):
        return iter(self._l)

    def __reversed__(self):
        return reversed(self._l)

    def __contains__(self, val):
        return val in self._l

    def append(self, val):
        if not isinstance(val, self._T):
            raise TypeError(
                'object {!r} is not an instance of {!r}'
                .format(val, self._T))
        self._l.append(val)

    def extend(self, iterable):
        for val in iterable:
            self.append(val)

    def insert(self, idx, val):
        if not isinstance(val, self._T):
            raise TypeError(
                'object {!r} is not an instance of {!r}'
                .format(val, self._T))
        self._l.insert(idx, val)

    def remote(self, val):
        self._l.remove(val)

    def pop(self, idx=-1):
        return self._l.pop(idx)

    def clear(self):
        self._l.clear()

    def idx(self, val, start=0, end=-1):
        return self._l.idx(val, start, end)

    def count(self, val):
        return self._l.count(val)

    def sort(self, key=None, reverse=False):
        self._l.sort(key=key, reverse=reverse)

    def reverse(self):
        self._l.reverse()

    def copy(self):
        return self.__class__(self)

    def __eq__(self, other):
        if not isinstance(other, _Multiple):
            return False
        return self._l == other._l

    def __lt__(self, other):
        return self._l < other._l

    def __iadd__(self, other):
        self.extend(other)

    def __add__(self, other):
        copy = self.copy()
        copy += other
        return copy

    def __imul__(self, other):
        self._l *= other

    def __mul__(self, other):
        copy = self.copy()
        copy *= other
        return copy

    def __rmul__(self, other):
        copy = self.copy()
        copy *= other
        return copy


def _cloned(obj):
    """Attempts to clone the given object by calling its clone() method, if it
    has one."""
    if hasattr(obj, 'clone'):
        return obj.clone()
    return obj


)PY";

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
            generate_node_class(output, specification, *node);
        }
    }

}

} // namespace python
} // namespace tree_gen
