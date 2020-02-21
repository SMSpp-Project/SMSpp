from conans import ConanFile, CMake, tools


class SmsppConan(ConanFile):
    name = "smspp"
    version = "0.1.1"
    description = "A C++ library for modeling and solving mathematical models"
    topics = ("conan", "smspp")
    url = "https://gitlab.com/smspp/smspp"
    homepage = "https://gitlab.com/smspp/smspp"
    license = "GPL-3.0-only"
    generators = "cmake"

    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    requires = (
        "boost/1.71.0@conan/stable",
        "eigen/3.3.7@conan/stable",
        "netcdf-cxx4/4.3.1@smspp/testing"
    )

    exports_sources = [
        "CMakeLists.txt",
        "src/*",
        "include/*",
        "cmake/SMSppConfig.cmake.in",
        "cmake/FindnetCDFCxx.cmake"
    ]

    def source(self):
        tools.replace_in_file(
            "CMakeLists.txt",
            '''project(SMS++ VERSION 0.1.1 LANGUAGES CXX)''',
            '''project(SMS++ VERSION 0.1.1 LANGUAGES CXX)\n''' +
            '''include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)\n''' +
            '''conan_basic_setup()'''
        )

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_TESTING"] = False
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy(pattern="LICENSE", dst="licenses")
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.includedirs = ["include", "include/SMS++"]
        self.cpp_info.libs = ["SMSpp"]
