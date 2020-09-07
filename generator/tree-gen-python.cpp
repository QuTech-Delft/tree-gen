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
 * Replaces all occurrences of `from` in `str` to `to`.
 */
std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

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
 * Generates the class for the given node.
 */
void generate_node_class(
    std::ofstream &output,
    Specification &spec,
    NodeType &node
) {
    auto all_children = node.all_children();

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

    // Print slots for the attributes.
    output << "    __slots__ = [";
    if (!node.children.empty()) {
        output << std::endl;
        for (const auto &attrib : node.children) {
            output << "        '_attr_" << attrib.name << "'," << std::endl;
        }
        output << "    ";
    }
    output << "]" << std::endl << std::endl;

    // Print constructor.
    output << "    def __init__(";
    if (all_children.empty()) {
        output << "self";
    } else {
        output << std::endl;
        output << "        self," << std::endl;
        for (const auto &attrib : all_children) {
            output << "        " << attrib.name << "=None," << std::endl;
        }
        output << "    ";
    }
    output << "):" << std::endl;
    output << "        super().__init__(";
    if (node.parent) {
        bool first = true;
        for (const auto &attrib : node.parent->all_children()) {
            if (first) {
                first = false;
            } else {
                output << ", ";
            }
            output << attrib.name << "=" << attrib.name;
        }
    }
    output << ")" << std::endl;
    for (const auto &attrib : node.children) {
        output << "        self." << attrib.name << " = " << attrib.name << std::endl;
    }
    output << std::endl;

    // Print the attribute getters, setters, and deleters.
    for (const auto &attrib : node.children) {

        // Attributes representing Any or Many edges require a list-like class
        // to sit between the attribute accessors and the user to make things
        // like indexing work with type-safety. First figure out if this is such
        // an attribute.
        bool is_any = attrib.type == Any || (attrib.type == Prim && attrib.ext_type == Any);
        bool is_many = attrib.type == Many || (attrib.type == Prim && attrib.ext_type == Many);
        bool is_prim = attrib.type == Prim && attrib.ext_type == Prim;
        bool is_any_or_many = is_any || is_many;
        std::string type = (attrib.type == Prim) ? replace_all(attrib.prim_type, "::", ".") : attrib.node_type->title_case_name;

        if (is_any_or_many) {
            type = "Multi" + type;
        }

        // Getter.
        output << "    @property" << std::endl;
        output << "    def " << attrib.name << "(self):" << std::endl;
        if (!attrib.doc.empty()) {
            format_doc(output, attrib.doc, "        ");
        }
        output << "        return self._attr_" << attrib.name << std::endl << std::endl;

        // Setter. Assigning None is the same as deleting.
        output << "    @" << attrib.name << ".setter" << std::endl;
        output << "    def " << attrib.name << "(self, value):" << std::endl;
        output << "        if value is None:" << std::endl;
        output << "            del self." << attrib.name << std::endl;
        output << "            return" << std::endl;
        output << "        if not isinstance(" << type << ", value):" << std::endl;
        output << "            # Try to \"typecast\" if this isn't an obvious mistake." << std::endl;
        output << "            if isinstance(Node, value):" << std::endl;
        output << "                raise TypeError('" << attrib.name << " must be of type " << type << "')" << std::endl;
        output << "            value = " << type << "(value)" << std::endl;
        output << "        self._attr_" << attrib.name << " = ";
        output << type << "(value)" << std::endl << std::endl;

        // Deleter. Doesn't actually delete, but rather replaces with the
        // default value.
        output << "    @" << attrib.name << ".deleter" << std::endl;
        output << "    def " << attrib.name << "(self):" << std::endl;
        output << "        self._attr_" << attrib.name;
        if (is_prim || is_any_or_many) {
            output << " = " << type << "()";
        } else {
            output << " = None";
        }
        output << std::endl << std::endl;

    }

    // Print find_reachable() function.
    if (node.derived.empty()) {
        output << "    def find_reachable(self, id_map=None):" << std::endl;
        format_doc(
            output, "Returns a dictionary mapping Python id() values to "
            "stable sequence numbers for all nodes in the tree rooted at this "
            "node. If id_map is specified, found nodes are appended to it.", "        ");
        output << "        if id_map is None:" << std::endl;
        output << "            id_map = {}" << std::endl;
        output << "        def add(node):" << std::endl;
        output << "            if node is None:" << std::endl;
        output << "                return" << std::endl;
        output << "            if id(node) in id_map:" << std::endl;
        output << "                raise NotWellFormed('";
        output << "node {!r} with id {} occurs more than once'.format(node, id(node)))" << std::endl;
        output << "            id_map[id(node)] = len(id_map)" << std::endl;
        for (const auto &attrib : all_children) {
            AttributeType type = (attrib.type == Prim) ? attrib.ext_type : attrib.type;
            switch (type) {
                case Maybe:
                case One:
                    output << "        add(self._attr_" << attrib.name << ")" << std::endl;
                    break;
                case Any:
                case Many:
                    output << "        for child in self._attr_" << attrib.name << ":" << std::endl;
                    output << "            add(child)" << std::endl;
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
        format_doc(
            output, "Raises NotWellFormed if the tree rooted at this "
            "node is not well-formed. If id_map is specified, this tree is only a "
            "subtree in the context of a larger tree, and id_map must be a dict "
            "mapping from Python id() codes to tree indices for all reachable "
            "nodes.", "        ");
        output << "        if id_map is None:" << std::endl;
        output << "            id_map = self.find_reachable()" << std::endl;
        for (const auto &attrib : all_children) {
            AttributeType type = (attrib.type == Prim) ? attrib.ext_type : attrib.type;
            switch (type) {
                case One:
                    output << "        if self._attr_" << attrib.name << " is None:" << std::endl;
                    output << "            raise NotWellFormed('";
                    output << attrib.name << " is required but not set')" << std::endl;
                    // fallthrough
                case Maybe:
                    output << "        if self._attr_" << attrib.name << " is not None:" << std::endl;
                    output << "            self._attr_" << attrib.name << ".check_well_formed(id_map)" << std::endl;
                    break;
                case Many:
                    output << "        if not self._attr_" << attrib.name << ":" << std::endl;
                    output << "            raise NotWellFormed('";
                    output << attrib.name << " needs at least one node but has zero')" << std::endl;
                    // fallthrough
                case Any:
                    output << "        for child in self._attr_" << attrib.name << ":" << std::endl;
                    output << "            child.check_well_formed(id_map)" << std::endl;
                    break;
                case Link:
                    output << "        if self._attr_" << attrib.name << " is None:" << std::endl;
                    output << "            raise NotWellFormed('";
                    output << attrib.name << " is required but not set')" << std::endl;
                    // fallthrough
                case OptLink:
                    output << "        if self._attr_" << attrib.name << " is not None:" << std::endl;
                    output << "            if id(self._attr_" << attrib.name << ") not in id_map:" << std::endl;
                    output << "                raise NotWellFormed('";
                    output << attrib.name << " links to unreachable node')" << std::endl;
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
        for (const auto &attrib : all_children) {
            if (first) {
                first = false;
            } else {
                output << "," << std::endl;
            }
            output << "            " << attrib.name << "=self._attr_" << attrib.name;
        }
        output << std::endl << "        )" << std::endl << std::endl;
    }

    // Print clone() function.
    if (node.derived.empty()) {
        output << "    def clone(self):" << std::endl;
        format_doc(output, "Returns a deep copy of this node.", "        ");
        output << "        return " << node.title_case_name << "(" << std::endl;
        bool first = true;
        for (const auto &attrib : all_children) {
            if (first) {
                first = false;
            } else {
                output << "," << std::endl;
            }
            output << "            " << attrib.name << "=self._attr_" << attrib.name;
            if (attrib.type == Prim && attrib.ext_type == Prim) {
                output << ".clone()";
            }
        }
        output << std::endl << "        )" << std::endl << std::endl;
    }

    output << std::endl;
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
    for (auto &include : specification.python_includes) {
        output << include << std::endl;
    }
    output << std::endl << std::endl;

    // Write the classes that are always the same.
    output << R"PY(
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
        return self._annot[id(key)]

    def __setitem__(self, key, value):
        """Assigns the annotation object with the specified key."""
        if not isinstance(key, str):
            raise TypeError('indexing a node with something other than an '
                            'annotation key string')
        self._annot[id(key)] = value

    def __delitem__(self, key):
        """Deletes the annotation object with the specified key."""
        if not isinstance(key, str):
            raise TypeError('indexing a node with something other than an '
                            'annotation key string')
        del self._annot[id(key)]

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
         - all Link and filled OptLink nodes link to a node that's reachable from
           this node;
         - the nodes referred to be One/Maybe only appear once in the tree
           (except through links).

        If it isn't well-formed, a NotWellFormed is thrown."""
        self.check_complete()

    def is_well_formed(self):
        """Returns whether the tree starting at this node is well-formed. That is:

         - all One, Link, and Many edges have (at least) one entry;
         - all the One entries internally stored by Any/Many have an entry;
         - all Link and filled OptLink nodes link to a node that's reachable from
           this node;
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


@functools.total_ordering
class _Multiple(object):
    """Base class for the Any* and Many* edge helper classes. Inheriting
    classes must set the class constant _T to the node type they are made
    for."""

    __slots__ = ['_l']

    def __init__(self,  *args, **kwargs):
        super().__init__()
        self._l = list(*args, **kwargs)
        for index, value in enumerate(self._l):
            if not isinstance(value, self._T):
                raise TypeError(
                    'object {!r} at index {:d} is not an instance of {!r}'
                    .format(value, index, self._T))

    def __repr__(self):
        return '{}({!r})'.format(type(self).__name__, self._l)

    def clone(self):
        return self.__class__(map(lambda node: node.clone(), self._l))

    def __len__(self):
        return len(self._l)

    def __getitem__(self, index):
        return self._l[index]

    def __setitem__(self, index, value):
        if not isinstance(value, self._T):
            raise TypeError(
                'object {!r} at index {:d} is not an instance of {!r}'
                .format(value, index, self._T))
        self._l[index] = value

    def __delitem__(self, index):
        del self._l[index]

    def __iter__(self):
        return iter(self._l)

    def __reversed__(self):
        return reversed(self._l)

    def __contains__(self, value):
        return value in self._l

    def append(self, value):
        if not isinstance(value, self._T):
            raise TypeError(
                'object {!r} is not an instance of {!r}'
                .format(value, self._T))
        self._l.append(value)

    def extend(self, iterable):
        for value in iterable:
            self.append(value)

    def insert(self, index, value):
        if not isinstance(value, self._T):
            raise TypeError(
                'object {!r} is not an instance of {!r}'
                .format(value, self._T))
        self._l.insert(index, value)

    def remote(self, value):
        self._l.remove(value)

    def pop(self, index=-1):
        return self._l.pop(index)

    def clear(self):
        self._l.clear()

    def index(self, value, start=0, end=-1):
        return self._l.index(value, start, end)

    def count(self, value):
        return self._l.count(value)

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
