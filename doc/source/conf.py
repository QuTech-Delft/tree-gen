# -*- coding: utf-8 -*-
#
# Configuration file for the Sphinx documentation builder.
#
# This file does only contain a selection of the most common options. For a
# full list see the documentation:
# http://www.sphinx-doc.org/en/master/config

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

# -- C++ and doxygen build ---------------------------------------------------
import subprocess
import os
import sys

def rstify(output_filename, executable_filename, main_filename, python_filename, *extra_filenames):
    """Tool for turning example code & output into an RST page for
    Sphinx/ReadTheDocs."""

    # Call the example executable and get its output.
    return_to_dir = os.path.realpath(os.curdir)
    fname = os.path.realpath(executable_filename)
    os.chdir(os.path.dirname(executable_filename))
    output = subprocess.check_output(fname).decode('UTF-8').split('\n')
    os.chdir(return_to_dir)

    # Read the source files.
    with open(main_filename, 'r') as f:
        main = f.read()
    files = []
    for filename in extra_filenames:
        with open(filename, 'r') as f:
            contents = f.read()
        files.append((os.path.basename(filename), contents))
    files.append(('main.cpp', main))

    # In main.cpp, look for the actual "int main..." line and the corresponding
    # closing curly bracket.
    main_fn = main.split('int main(', maxsplit=1)[1].split('{\n', maxsplit=1)[1]
    main_fn = main_fn.split('\n}', maxsplit=1)[0].split('\n')

    # Parse the contents of main and stdout and match them together using the
    # markers.
    sections = []
    main_lines = iter(main_fn)
    out_lines = iter(output)
    try:
        while True:
            section = {}

            # Parse // comment block as markdown text.
            text = []
            strip_indent = 0
            while True:
                line = next(main_lines)
                parts = line.split('//', maxsplit=1)
                if not line.strip():
                    text.append('')
                elif len(parts) == 2:
                    comment = parts[1]
                    if comment.startswith(' '):
                        comment = comment[1:]
                    text.append(comment)
                    strip_indent = len(parts[0])
                else:
                    break
            section['text'] = '\n'.join(text).strip()

            # Parse the subsequent code for insertion as a C++ code block.
            code = []
            while True:
                if 'MARKER' in line:
                    break
                if not line[:strip_indent].strip():
                    line = line[strip_indent:]
                code.append(line)
                line = next(main_lines)
            section['code'] = '\n'.join(code)
            section['lang'] = 'C++'

            # Parse any output.
            out = []
            while True:
                line = next(out_lines)
                if '###MARKER###' in line:
                    break
                out.append(line)
            section['output'] = '\n'.join(out)

            sections.append(section)
    except StopIteration:
        pass

    if python_filename:

        # Run the example Python file and get its output.
        fname = os.path.realpath(python_filename)
        output_dir = os.path.dirname(os.path.realpath(executable_filename))
        return_to_dir = os.path.realpath(os.curdir)
        os.chdir(os.path.dirname(fname))
        output = subprocess.check_output([
            sys.executable, os.path.basename(fname), output_dir
        ]).decode('UTF-8').split('\n')
        os.chdir(return_to_dir)

        # Read the source file.
        with open(fname, 'r') as f:
            main = f.read()
        files.append(('main.py', main))

        # Strip header off of the file.
        main = '# | ' + main.split('\n# | ', maxsplit=1)[1]

        # Parse the contents of main and stdout and match them together using
        # the markers.
        main_lines = iter(main.split('\n'))
        out_lines = iter(output)
        try:
            while True:
                section = {}

                # Parse # | comment block as markdown text.
                text = []
                strip_indent = 0
                while True:
                    line = next(main_lines)
                    parts = line.split('# | ', maxsplit=1)
                    if not line.strip():
                        text.append('')
                    elif len(parts) == 2:
                        comment = parts[1]
                        if comment.startswith(' '):
                            comment = comment[1:]
                        text.append(comment)
                        strip_indent = len(parts[0])
                    else:
                        break
                section['text'] = '\n'.join(text).strip()

                # Parse the subsequent code for insertion as a Python code block.
                code = []
                while True:
                    if 'marker()' in line:
                        break
                    if not line[:strip_indent].strip():
                        line = line[strip_indent:]
                    code.append(line)
                    line = next(main_lines)
                if code[0] == 'try:':
                    code = [line[4:] for line in code[1:-2]]
                section['code'] = '\n'.join(code)
                section['lang'] = 'python3'

                # Parse any output.
                out = []
                while True:
                    line = next(out_lines)
                    if '###MARKER###' in line:
                        break
                    out.append(line)
                section['output'] = '\n'.join(out)

                sections.append(section)
        except StopIteration:
            pass

    rst = []

    # Print the tutorial-esque sections.
    for section in sections:
        rst.append(section['text'])
        rst.append('')
        if section['code'].strip():
            rst.append('.. code-block:: ' + section['lang'])
            rst.append('  ')
            for line in section['code'].split('\n'):
                rst.append('  ' + line)
            rst.append('')
            if section['output'].strip():
                rst.append('.. rst-class:: center')
                rst.append('')
                rst.append('↓')
                rst.append('')
                rst.append('.. code-block:: none')
                rst.append('  ')
                for line in section['output'].split('\n'):
                    rst.append('  ' + line)
                rst.append('')

    # Print the complete files afterwards.
    rst.append('Complete file listings')
    rst.append('======================')
    rst.append('')
    for name, contents in files:
        rst.append(name)
        rst.append('-' * len(name))
        rst.append('')
        if name.endswith('cpp') or name.endswith('hpp'):
            rst.append('.. code-block:: C++')
        elif name.endswith('txt'):
            rst.append('.. code-block:: CMake')
        else:
            rst.append('.. code-block:: none')
        rst.append('  :linenos:')
        rst.append('  ')
        for line in contents.split('\n'):
            rst.append('  ' + line)
        rst.append('')

    with open(output_filename, 'w') as f:
        f.write('\n'.join(rst))

original_workdir = os.getcwd()
try:
    root_dir = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
    os.chdir(root_dir)
    if not os.path.exists('doc/doxygen/doxy'):
        if not os.path.exists('cbuild'):
            os.mkdir('cbuild')

        # Build tree-gen.
        os.chdir('cbuild')
        subprocess.check_call(['cmake', '..', '-DTREE_GEN_BUILD_TESTS=ON'])
        subprocess.check_call(['make', '-j', '8'])

        # Generate a page from the markdown at the top of tree-gen.hpp.
        with open('../generator/tree-gen.hpp', 'r') as f:
            text = f.read()
        text = text.split('\page tree-gen tree-gen', maxsplit=1)[1].split('*/', maxsplit=1)[0]
        lines = ['# Overview', '']
        for line in text.split('\n'):
            if line.startswith(' *'):
                line = line[3:]
            if line.startswith('\\section'):
                line = '## ' + line[9:].strip().split(maxsplit=1)[1]
            elif line.startswith('\\subsection'):
                line = '### ' + line[12:].strip().split(maxsplit=1)[1]
            elif line.startswith('\\subsubsection'):
                line = '#### ' + line[15:].strip().split(maxsplit=1)[1]
            lines.append(line)
        with open('../doc/source/tree-gen.gen.md', 'w') as f:
            f.write('\n'.join(lines))

        # Run the examples and generate pages from them.
        rstify(
            '../doc/source/directory.gen.rst',
            'examples/directory/directory-example',
            '../examples/directory/main.cpp',
            '../examples/directory/main.py',
            '../examples/directory/directory.tree',
            '../examples/directory/primitives.hpp',
            '../examples/directory/primitives.py',
            '../examples/directory/CMakeLists.txt'
        )

        rstify(
            '../doc/source/interpreter.gen.rst',
            'examples/interpreter/interpreter-example',
            '../examples/interpreter/main.cpp',
            None,
            '../examples/interpreter/value.tree',
            '../examples/interpreter/program.tree',
            '../examples/interpreter/primitives.hpp',
            '../examples/interpreter/CMakeLists.txt'
        )

        # Run Doxygen.
        os.chdir('../doc')
        subprocess.check_call(['doxygen'])
        subprocess.check_call(['mv', '-f', 'doxygen/html', 'doxygen/doxy'])

finally:
    os.chdir(original_workdir)

html_extra_path = ['../doxygen']

# -- Project information -----------------------------------------------------

project = 'tree-gen'
copyright = '2020, QuTech, TU Delft'
author = 'QuTech, TU Delft'

# The short X.Y version
version = ''
# The full version, including alpha/beta/rc tags
release = ''


# -- General configuration ---------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.mathjax',
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
# source_suffix = ['.rst', '.md']
source_suffix = ['.rst', '.md']

source_parsers = {
   '.md': 'recommonmark.parser.CommonMarkParser',
}

# The master toctree document.
master_doc = 'index'

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = None

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path .
exclude_patterns = []

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
import sphinx_rtd_theme
html_theme = 'sphinx_rtd_theme'
html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#
# html_theme_options = {}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

html_css_files = ['center.css']
# Custom sidebar templates, must be a dictionary that maps document names
# to template names.
#
# The default sidebars (for documents that don't match any pattern) are
# defined by theme itself.  Builtin themes are using these templates by
# default: ``['localtoc.html', 'relations.html', 'sourcelink.html',
# 'searchbox.html']``.
#
# html_sidebars = {}


# -- Options for HTMLHelp output ---------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'treegendoc'


# -- Options for LaTeX output ------------------------------------------------

latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    #
    # 'papersize': 'letterpaper',

    # The font size ('10pt', '11pt' or '12pt').
    #
    # 'pointsize': '10pt',

    # Additional stuff for the LaTeX preamble.
    #
    # 'preamble': '',

    # Latex figure (float) alignment
    #
    # 'figure_align': 'htbp',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    (master_doc, 'tree-gen.tex', 'tree-gen Documentation',
     'QuTech, TU Delft', 'manual'),
]


# -- Options for manual page output ------------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    (master_doc, 'tree-gen', 'tree-gen Documentation',
     [author], 1)
]


# -- Options for Texinfo output ----------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
    (master_doc, 'tree-gen', 'tree-gen Documentation',
     author, 'tree-gen', 'C++ source code generator for tree structures.',
     'Miscellaneous'),
]


# -- Extension configuration -------------------------------------------------
