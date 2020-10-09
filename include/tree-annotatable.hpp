/** \file
 * This file contains the magic needed to allow users to add their own data to
 * tree nodes without needing to change the tree structure.
 *
 * Data can be added by way of annotations. Annotations can be any kind of
 * object; in fact they are identified by their type, so each annotatable
 * object can have zero or one instance of every C++ type associated with it.
 * The goal is to allow users to use the AST, semantic tree, and a few other
 * things in their own code as they operate on the tree.
 */

#pragma once

#include <memory>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <functional>
#include "tree-cbor.hpp"

namespace tree {

/**
 * Namespace for the \ref tree::annotatable::Annotatable "Annotatable" base
 * class and support classes.
 */
namespace annotatable {

/**
 * Utility class for carrying any kind of value. Basically, `std::any` within
 * C++11.
 */
class Anything {
private:

    /**
     * Pointer to the contained data, or nullptr if no data is contained.
     */
    void *data;

    /**
     * Function used to free the contained data.
     */
    std::function<void(void *data)> destructor;

    /**
     * Type information.
     */
    std::type_index type;

    /**
     * Constructs an Anything object.
     */
    Anything(void *data, std::function<void(void *data)> destructor, std::type_index type);

public:

    /**
     * Constructs an empty Anything object.
     */
    Anything();

    /**
     * Constructs an Anything object by copying a value into it.
     */
    template <typename T>
    static Anything make(const T &ob) {
        return Anything(
            new T(ob),
            [](void *data) {
                delete static_cast<T*>(data);
            },
            std::type_index(typeid(T))
        );
    }

    /**
     * Constructs an Anything object by moving a value into it.
     */
    template <typename T>
    static Anything make(T &&ob) {
        return Anything(
            new T(std::move(ob)),
            [](void *data) {
                delete static_cast<T*>(data);
            },
            std::type_index(typeid(T))
        );
    }

    /**
     * Destructor.
     */
    ~Anything();

    // Anything objects are not copyable, because type information is lost
    // after the initial construction.
    Anything(const Anything&) = delete;
    Anything& operator=(const Anything&) = delete;

    /**
     * Move constructor.
     */
    Anything(Anything &&src);

    /**
     * Move assignment.
     */
    Anything& operator=(Anything &&src);

    /**
     * Returns a mutable pointer to the contents.
     *
     * @throws std::bad_cast when the type is incorrect.
     */
    template <typename T>
    T *get_mut() {
        if (std::type_index(typeid(T)) != type) {
            throw std::bad_cast();
        }
        return static_cast<T*>(data);
    }

    /**
     * Returns a const pointer to the contents.
     *
     * @throws std::bad_cast when the type is incorrect.
     */
    template <typename T>
    const T *get_const() const {
        if (std::type_index(typeid(T)) != type) {
            throw std::bad_cast();
        }
        return static_cast<T*>(data);
    }

    /**
     * Returns the type index corresponding to the wrapped object.
     */
    std::type_index get_type_index() const;

};

/**
 * Base class for serializable types. Note that a constructor that performs
 * the reverse operation must also be specified. See SerDesRegistry::add().
 */
class Serializable {
public:
    virtual ~Serializable() = default;
    virtual void serialize(cbor::MapWriter&) const = 0;
};

/**
 * Class keeping track of all registered serialization and deserialization
 * functions for annotation objects.
 */
class SerDesRegistry {
private:

    /**
     * Map from type index to serialization function.
     */
    std::unordered_map<
        std::type_index,
        std::function<void(const std::shared_ptr<Anything>&, cbor::MapWriter&)>
    > serializers;

    /**
     * Map from CBOR type identifier (C++ type name wrapped in curly braces) to
     * deserialization function.
     */
    std::unordered_map<
        std::string,
        std::function<std::shared_ptr<Anything>(const cbor::MapReader&)>
    > deserializers;

public:

    /**
     * Registers a serialization and deserialization function for the given type
     * using CBOR.
     *
     * The serialization function takes a constant reference to an object of
     * type T and a reference to a CBOR map writer. The serialization function
     * is to serialize the contents of the object to zero or more CBOR map
     * key/value pairs using the append_*() functions on the map writer object.
     * Type information does not need to be serialized. MapWriter::close() may
     * but does not have to be called.
     *
     * The deserialization function should do the opposite. Given a
     * representation of the CBOR map in the form of an STL map containing CBOR
     * readers for the values, it must produce an object of type T.
     *
     * The name used to reference the type in the CBOR data can optionally be
     * set with the name parameter. If it is empty or left unspecified, it will
     * be determined using typeid(T).name().
     */
    template <typename T>
    void add(
        std::function<void(const T&, cbor::MapWriter&)> serialize,
        std::function<T(const cbor::MapReader&)> deserialize,
        const std::string &name = ""
    ) {
        const std::string full_name = std::string(
            "{" + (name.empty() ? std::string(typeid(T).name()) : name) + "}");
        serializers.insert(std::make_pair(
            std::type_index(typeid(T)),
            [serialize, full_name](const std::shared_ptr<Anything> &anything, cbor::MapWriter &map) {
                auto submap = map.append_map(full_name);
                serialize(*(anything->get_const<T>()), submap);
            }
        ));
        deserializers.insert(std::make_pair(
            full_name,
            [deserialize](const cbor::MapReader &map) -> std::shared_ptr<Anything> {
                return std::make_shared<Anything>(Anything::make(deserialize(map)));
            }
        ));
    }

    /**
     * Registers a serialization and deserialization function for the given type
     * using CBOR.
     *
     * T must have a const method called serialize() that takes a
     * `cbor::MapWriter&`, which, when called, is to serialize the contents of
     * the object to zero or more CBOR map key/value pairs using the append_*()
     * functions on the map writer object. The Serializable class may be used as
     * an interface class for this. Type information does not need to be
     * serialized. MapWriter::close() may but does not have to be called.
     *
     * T must furthermore have an (explicit) constructor that takes a
     * `const cbor::MapReader&` as its sole parameter. It should perform the
     * inverse operation: given a representation of the CBOR map in the form of
     * an STL map containing CBOR readers for the values, it must produce the
     * original object again, or something deemed close enough to it.
     *
     * The name used to reference the type in the CBOR data can optionally be
     * set with the name parameter. If it is empty or left unspecified, it will
     * be determined using typeid(T).name().
     */
    template <typename T>
    void add(const std::string &name = "") {
        const std::string full_name = std::string(
            "{" + (name.empty() ? std::string(typeid(T).name()) : name) + "}");
        serializers.insert(std::make_pair(
            std::type_index(typeid(T)),
            [full_name](const std::shared_ptr<Anything> &anything, cbor::MapWriter &map) {
                auto submap = map.append_map(full_name);
                anything->get_const<T>()->serialize(submap);
            }
        ));
        deserializers.insert(std::make_pair(
            full_name,
            [](const cbor::MapReader &map) -> std::shared_ptr<Anything> {
                return std::make_shared<Anything>(Anything::make(T(map)));
            }
        ));
    }

    /**
     * Serializes the given Anything object to a single value in the given
     * map, if and only if a serializer was previously registered for this type.
     * If no serializer is known, this is no-op.
     */
    void serialize(std::shared_ptr<Anything> obj, cbor::MapWriter &map) const;

    /**
     * Deserializes the given CBOR key/value pair to the corresponding Anything
     * object, if the type is known. If the type is not known, an empty/null
     * shared_ptr is returned.
     */
    std::shared_ptr<Anything> deserialize(const std::string &key, const cbor::Reader &value) const;

};

/**
 * Global variable keeping track of all registered serialization and
 * deserialization functions for annotation objects.
 */
#ifdef _MSC_VER
#ifdef BUILDING_TREE_LIB
__declspec(dllexport)
#else
__declspec(dllimport)
#endif
#endif
extern SerDesRegistry serdes_registry;

/**
 * Base class for anything that can have user-specified annotations.
 */
class Annotatable {
private:

    /**
     * The annotations stored with this node.
     */
    std::unordered_map<std::type_index, std::shared_ptr<Anything>> annotations;

public:

    /**
     * We're using inheritance, so we need a virtual destructor for proper
     * cleanup.
     */
    virtual ~Annotatable();

    /**
     * Adds an annotation object to this node.
     *
     * Annotations are keyed by their type. That is, a node can contain zero or
     * one annotation for every C++ type, meaning you can attach any data you
     * want to a node by defining your own struct or class.
     *
     * The annotations object is copied into the node. If you don't want to
     * make a copy, you can store a (smart) pointer to the object instead, in
     * which case the copied object is the pointer.
     */
    template <typename T>
    void set_annotation(const T &ob) {
        annotations[std::type_index(typeid(T))] = std::make_shared<Anything>(Anything::make<T>(ob));
    }

    /**
     * Adds an annotation object to this node.
     *
     * Annotations are keyed by their type. That is, a node can contain zero or
     * one annotation for every C++ type, meaning you can attach any data you
     * want to a node by defining your own struct or class.
     *
     * The annotations object is moved into the node.
     */
    template <typename T>
    void set_annotation(T &&ob) {
        annotations[std::type_index(typeid(T))] = std::make_shared<Anything>(Anything::make<T>(std::move(ob)));
    }

    /**
     * Returns whether this object holds an annotation object of the given
     * type.
     */
    template <typename T>
    bool has_annotation() const {
        return annotations.count(std::type_index(typeid(T)));
    }

    /**
     * Returns a mutable pointer to the annotation object of the given type
     * held by this object, or `nullptr` if there is no such annotation.
     */
    template <typename T>
    T *get_annotation_ptr() {
        try {
            return annotations.at(std::type_index(typeid(T)))->get_mut<T>();
        } catch (const std::out_of_range&) {
            return nullptr;
        }
    }

    /**
     * Returns an immutable pointer to the annotation object of the given type
     * held by this object, or `nullptr` if there is no such annotation.
     */
    template <typename T>
    const T *get_annotation_ptr() const {
        try {
            return annotations.at(std::type_index(typeid(T)))->get_const<T>();
        } catch (const std::out_of_range&) {
            return nullptr;
        }
    }

    /**
     * Returns a mutable pointer to the annotation object of the given type
     * held by this object, or `nullptr` if there is no such annotation.
     */
    template <typename T>
    T &get_annotation() {
        if (auto annotation = get_annotation_ptr<T>()) {
            return *annotation;
        } else {
            throw std::runtime_error("object does not have an annotation of this type");
        }
    }

    /**
     * Returns an immutable pointer to the annotation object of the given type
     * held by this object, or `nullptr` if there is no such annotation.
     */
    template <typename T>
    const T &get_annotation() const {
        if (auto annotation = get_annotation_ptr<T>()) {
            return *annotation;
        } else {
            throw std::runtime_error("object does not have an annotation of this type");
        }
    }

    /**
     * Removes the annotation object of the given type, if any.
     */
    template <typename T>
    void erase_annotation() {
        annotations.erase(std::type_index(typeid(T)));
    }

    /**
     * Copies the annotation of type T from the source object to this object.
     * If the source object doesn't have an annotation of type T, any
     * such annotation on this object is removed.
     */
    template <typename T>
    void copy_annotation(const Annotatable &src) {
        if (auto annotation = src.get_annotation_ptr<T>()) {
            set_annotation(*annotation);
        } else {
            erase_annotation<T>();
        }
    }

    /**
     * Serializes all the annotations that have a known serialization format
     * (previously registered through serdes_registry.add()) to the given map.
     * Each annotation results in a single map entry, with the C++ typename
     * wrapped in curly braces as key, and a type-dependent submap populated by
     * the registered serialization function as value. Annotations with no known
     * serialization format are silently ignored.
     */
    void serialize_annotations(cbor::MapWriter &map) const;

    /**
     * Deserializes all annotations that have a known deserialization function
     * (previously registered through serdes_registry.add()) into the annotation
     * list. Annotations are expected to have a key formed by the C++ typename
     * wrapped in curly braces and a value of type map, of which the contents
     * are passed to the registered deserialization function. Previously added
     * annotations with conflicting types are silently overwritten. Any unknown
     * annotation types are silently ignored.
     */
    void deserialize_annotations(const cbor::MapReader &map);

};

} // namespace annotatable
} // namespace tree
