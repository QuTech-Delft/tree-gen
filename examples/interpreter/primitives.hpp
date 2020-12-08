/** \file
 * Defines primitives used in the generated directory tree structure.
 */

#pragma once

#include <string>
#include "tree-base.hpp"

/**
 * Namespace with primitives used in the generated directory tree structure.
 */
namespace primitives {

/**
 * Integer primitive.
 */
using Int = int;

/**
 * Strings, used to represent filenames and file contents.
 */
using Str = std::string;

/**
 * Initialization function. This must be specialized for any types used as
 * primitives in a tree that are actual C primitives (int, char, bool, etc),
 * as these are not initialized by the T() construct.
 */
template <class T>
T initialize() { return T(); };

/**
 * Declare the default initializer for integers. It's declared inline to avoid
 * having to make a cpp file just for this.
 */
template <>
inline Int initialize<Int>() {
    return 0;
}

/**
 * Serialization function. This must be specialized for any types used as
 * primitives in a tree. The default implementation doesn't do anything.
 */
template <typename T>
void serialize(const T &obj, tree::cbor::MapWriter &map) {
}

/**
 * Serialization function for Int.
 */
template <>
inline void serialize<Int>(const Int &obj, tree::cbor::MapWriter &map) {
    map.append_int("val", obj);
}

/**
 * Serialization function for Str.
 */
template <>
inline void serialize<Str>(const Str &obj, tree::cbor::MapWriter &map) {
    map.append_string("val", obj);
}

/**
 * Deserialization function. This must be specialized for any types used as
 * primitives in a tree. The default implementation doesn't do anything.
 */
template <typename T>
T deserialize(const tree::cbor::MapReader &map) {
    return initialize<T>();
}

/**
 * Deserialization function for Int.
 */
template <>
inline Int deserialize<Int>(const tree::cbor::MapReader &map) {
    return map.at("val").as_int();
}

/**
 * Deserialization function for Str.
 */
template <>
inline Str deserialize<Str>(const tree::cbor::MapReader &map) {
    return map.at("val").as_string();
}

/**
 * Source location annotation object, containing source file line numbers etc.
 */
class SourceLocation: tree::annotatable::Serializable {
public:
    std::string filename;
    int line;
    int column;

    SourceLocation(
        const std::string &filename,
        uint32_t line = 0,
        uint32_t column = 0
    ) : filename(filename), line(line), column(column) {
    }

    /**
     * Serialization logic for SourceLocation.
     */
    inline void serialize(tree::cbor::MapWriter &map) const override {
        map.append_string("filename", filename);
        map.append_int("line", line);
        map.append_int("column", column);
    }

    /**
     * Deserialization logic for SourceLocation.
     */
    inline SourceLocation(const tree::cbor::MapReader &map) {
        filename = map.at("filename").as_string();
        line = map.at("line").as_int();
        column = map.at("column").as_int();
    }

};

/**
 * Stream << overload for source location objects.
 */
inline std::ostream& operator<<(std::ostream& os, const primitives::SourceLocation& object) {
    os << object.filename << ":" << object.line << ":" << object.column;
    return os;
}

} // namespace primitives
