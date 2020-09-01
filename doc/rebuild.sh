#!/bin/sh
rm -rf build doxygen
rm -f source/*.gen.rst
make html
