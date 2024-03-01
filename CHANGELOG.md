# Change Log

All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [ 1.0.5 ] - [ 2024-02-29 ]

### Added
- Copy *.inc files as well during installation.

### Changed
- Update conanfile.py with some changes from the Conan Center tree-gen recipe.
- Update fmt version to 10.2.1.
- Change `requires` to `test_requires` (gtest).
- Change `-Wno-error=unused-but-set-variable` to `-Wno-error=unused` (apple-clang 13).
- Do not `tool_requires` flex and bison when building for wasm architecture.

### Removed
-

## [ 1.0.1 ] - [ 2024-01-12 ]

### Added
- 

### Changed
- Minimal changes in order to allow creation of a tree-gen Conan Center package.

### Removed
-
