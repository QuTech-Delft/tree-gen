import os

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.files import copy
from conan.tools.scm import Version

from version import get_version


class TreeGenConan(ConanFile):
    name = "tree-gen"
    version = get_version()

    # Optional metadata
    license = "Apache-2.0"
    homepage = "https://github.com/QuTech-Delft/tree-gen"
    url = "https://github.com/conan-io/conan-center-index"
    description = "C++ and Python code generator for tree-like structures common in parser and compiler codebases."
    topics = "code generation"

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "asan_enabled": [True, False],
        "build_tests": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "asan_enabled": False,
        "build_tests": False
    }

    exports = "version.py", "include/version.hpp"
    exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "src/*", "test/*"

    @property
    def _min_cppstd(self):
        return 17

    @property
    def _compilers_minimum_version(self):
        return {
            "gcc": "8",
            "clang": "7",
            "apple-clang": "14",
            "Visual Studio": "16",
            "msvc": "192"
        }

    def build_requirements(self):
        if self.options.build_tests:
            self.test_requires("gtest/1.14.0")
        self.tool_requires("m4/1.4.19")
        if self.settings.os == "Windows":
            self.tool_requires("winflexbison/2.5.24")
        else:
            if self.settings.arch != "armv8" and self.settings.arch != "wasm":
                self.tool_requires("flex/2.6.4")
                self.tool_requires("bison/3.8.2")

    def requirements(self):
        self.requires("fmt/10.2.1")
        self.requires("range-v3/0.12.0")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self, src_folder="src")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["ASAN_ENABLED"] = self.options.asan_enabled
        tc.variables["TREE_GEN_BUILD_TESTS"] = self.options.build_tests
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def validate(self):
        if self.settings.compiler.cppstd:
            check_min_cppstd(self, self._min_cppstd)
        minimum_version = self._compilers_minimum_version.get(str(self.settings.compiler), False)
        if minimum_version and Version(self.settings.compiler.version) < minimum_version:
            raise ConanInvalidConfiguration(f"{self.ref} requires C++{self._min_cppstd}, which your compiler does not support.")

    def package(self):
        copy(self, "LICENSE.md", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["tree-gen"]
