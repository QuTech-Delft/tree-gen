import sys, os, traceback
TEST_DIR = os.path.realpath(sys.argv[1])
sys.path.append(TEST_DIR)


def marker():
    print('###MARKER###')


# Note: the # | comment contents of this file, together with the marker() lines
# and the output of the program, are used to automatically turn this into a
# restructured-text page for ReadTheDocs. The output is appended to the C++
# output.

# | Python usage
# | ============

# | Let us now turn our attention to tree-gen's Python support. As you may
# | remember from the introduction, tree-gen does not provide a direct interface
# | between Python and C++ using a tool like SWIG; rather, it provides
# | conversion between a CBOR tree representation and the in-memory
# | representations of both languages. It then becomes trivial for users of the
# | tree-gen structure to provide an API to users that converts from one to the
# | other, since only a single string of bytes has to be transferred.

# | Recall that we wrote the CBOR representation of the tree we constructed in
# | the C++ example to a file. Let's try loading this file with Python.
from directory import *

with open(os.path.join(TEST_DIR, 'tree.cbor'), 'rb') as f:
    tree = System.deserialize(f.read())

tree.check_well_formed()
print(tree)
marker()

# | And there we go; the same structure in pure Python.

# | As you might expect from a pure-Python structure, field access is much less
# | verbose than it is in C++.
print(tree.drives[0].letter)
print(tree.drives[0].root_dir.entries[0].name)
marker()

# | Perhaps contrary to Python intuition however, the structure provides
# | type-safety. You can't assign values to fields that don't exist...
try:
    tree.color_of_a_banana = 'yellow'
except AttributeError:
    traceback.print_exc(file=sys.stdout)
marker()

# | ... or assign values of incorrect types to ones that do:
try:
    tree.drives[0] = 'not-a-drive'
except TypeError:
    traceback.print_exc(file=sys.stdout)
marker()

# | Note that the setters will however try to "typecast" objects they don't know
# | about (to be more specific, anything that isn't an instance of Node). So if
# | we try to assign a string directly to tree.drives, you get something you
# | might not have expected:
try:
    tree.drives = 'not-a-set-of-drives'
except TypeError:
    traceback.print_exc(file=sys.stdout)
marker()

# | The setter for the drives property is trying to cast the string to a
# | MultiDrive, which interprets its string input as any other iterable, and
# | ultimately tries to cast the first letter to a Drive. There's not much that
# | can be done about this; it's just how duck typing works.

# | Manipulation of trees in Python works just as you might expect it to in
# | Python:
tree.drives.append(Drive('E'))
tree.drives[-1].root_dir = Directory([
    File('test', 'test-contents'),
    File('test2', 'test2-contents'),
])
del tree.drives[1]
assert not tree.is_well_formed()
tree.drives[0].root_dir.entries[-1].target = tree.drives[1].root_dir
tree.check_well_formed()
print(tree)
marker()

# | Once the Python code is done manipulating the structure, it can be
# | serialized again.
cbor = tree.serialize()
assert cbor == System.deserialize(cbor).serialize()
count = 0
for c in cbor:
    if count == 16:
        print()
        count = 0
    elif count > 0 and count % 4 == 0:
        print(' ', end='')
    print('%02X ' % c, end='')
    count += 1
print()
marker()
