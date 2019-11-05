from conans import ConanFile, CMake, tools


class SmsppConan(ConanFile):
    name = "SMSpp"
    version = "0.1"
    # license = "<Put the package license here>" TODO
    author = "Niccolò Iardella niccolo.iardella@for.unipi.it"
    url = "https://gitlab.com/smspp/smspp"
    description = "A C++ library for modeling and solving block-structured mathematical models"
    # topics = ("<Put some tag here>", "<here>", "<and here>") TODO
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    generators = "cmake"
    requires = ("boost/[>= 1.71.0]@conan/stable",
                "eigen/[>= 3.3.7]@conan/stable",
                "netcdf-cxx/[>= 4.3.1]@smspp/testing")
    exports_sources = ["CMakeLists.txt",
                       "src/*",
                       "include/*",
                       "cmake/SMSppConfig.cmake.in",
                       "cmake/FindnetCDFCxx.cmake"]

    def source(self):
        tools.replace_in_file("CMakeLists.txt",
                              '''project(SMS++ VERSION 0.1.0 LANGUAGES CXX)''',
                              '''project(SMS++ VERSION 0.1.0 LANGUAGES CXX)\n''' +
                              '''include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)\n''' +
                              '''conan_basic_setup()''')

    def configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_TESTING"] = False
        cmake.configure()
        return cmake

    def build(self):
        cmake = self.configure_cmake()
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="include")
        self.copy("*SMSpp.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["SMSpp"]
