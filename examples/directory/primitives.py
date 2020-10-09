
class Letter(str):
    """Letter primitive, used to represent drive letters."""
    def __new__(cls, s='A'):
        return str.__new__(cls, s)


class String(str):
    """Strings, used to represent filenames and file contents."""
    pass


def serialize(typ, val):
    """Serialization function."""

    # Serialization formats of primitives.
    if typ is Letter:
        return {'val': ord(val)}
    if typ is String:
        return {'val': val}

    # Serialization formats of annotations.
    if isinstance(typ, str):
        return val

    # Some unknown type.
    raise TypeError('no known serialization of type %r, value %r' % (typ, val))


def deserialize(typ, val):
    """Deserialization function."""

    # Serialization formats of primitives.
    if typ is Letter:
        return Letter(chr(val['val']))
    if typ is String:
        return String(val['val'])

    # Serialization formats of annotations.
    if isinstance(typ, str):
        return val

    # Some unknown type.
    raise TypeError('no known deserialization for type %r, value %r' % (typ, val))
