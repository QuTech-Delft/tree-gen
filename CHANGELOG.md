# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [ 1.0.8 ] - [ 2024-08-15 ]

### Changed
- Bump dependencies versions.

## [ 1.0.7 ] - [ 2024-03-18 ]

### Added
- Add Dumper::dump_raw_pointers API.

### Changed
- Update GitHub actions and workflows to look more like libqasm's.

## [ 1.0.6 ] - [ 2024-02-29 ]

### Added
- Copy *.inc files as well during installation.
- Add get_version and get_release_year API.

### Changed
- Update conanfile.py with some changes from the Conan Center tree-gen recipe.
- Update fmt version to 10.2.1.
- Change `requires` to `test_requires` (gtest).
- Change `-Wno-error=unused-but-set-variable` to `-Wno-error=unused` (apple-clang 13).
- Do not `tool_requires` flex and bison when building for wasm architecture.

## [ 1.0.1 ] - [ 2024-01-12 ]

### Changed
- Minimal changes in order to allow creation of a tree-gen Conan Center package.
