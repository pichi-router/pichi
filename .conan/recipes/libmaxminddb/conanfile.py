from conans import ConanFile, CMake, tools
import os

class LibmaxminddbConan(ConanFile):
  name = "libmaxminddb"
  license = "Apache-2.0 License"
  author = "Maxmind <support@maxmind.com>"
  url = "http://www.maxmind.com/"
  description = "C library for the MaxMind DB file format"
  topics = ()
  settings = "os", "compiler", "build_type", "arch"
  options = {"shared": [True, False], "fPIC": [True, False],
             "with_bin": [True, False]}
  default_options = {"shared": False, "fPIC": True, "with_bin": True}
  generators = "cmake"

  @property
  def _source_subfolder(self):
    return "source_subfolder"
  
  @property
  def _build_subfolder(self):
    return "build_subfolder"
  
  @property
  def _cmake(self):
    ret = CMake(self)
    ret.definitions["BUILD_TESTING"] = "OFF"
    ret.definitions["BUILD_BIN"] = "ON" if self.options.with_bin else "OFF"
    return ret

  def config_options(self):
    if self.settings.os == "Windows":
      del self.options.fPIC
    if self.settings.os in ["Android", "iOS", "tvOS", "watchOS"]:
      self.options.with_bin = False
  
  def configure(self):
    if self.options.shared:
      del self.options.fPIC
    del self.settings.compiler.libcxx
    del self.settings.compiler.cppstd

  def source(self):
    g = tools.Git(folder=self._source_subfolder)
    g.clone("https://github.com/maxmind/libmaxminddb.git", branch=self.version)
    tools.replace_in_file(os.path.join(self._source_subfolder, "CMakeLists.txt"),
                          "option(BUILD_TESTING \"Build test programs\" ON)",
                          """option(BUILD_TESTING \"Build test programs\" ON)
option(BUILD_BIN \"Build mmdblookup programs\" ON)

include(${CMAKE_BINARY_DIR}/../conanbuildinfo.cmake)
conan_basic_setup(KEEP_RPATHS)""")
    tools.replace_in_file(os.path.join(self._source_subfolder, "CMakeLists.txt"),
                          "add_subdirectory(bin)",
                          """if (BUILD_BIN)
  add_subdirectory(bin)
endif ()""")

  def build(self):
    cmake = self._cmake
    cmake.configure(source_folder=self._source_subfolder, build_folder=self._build_subfolder)
    cmake.build(build_dir=self._build_subfolder)

  def package(self):
    self._cmake.install(build_dir=self._build_subfolder)

  def package_info(self):
    self.cpp_info.libs = ["maxminddb"]

