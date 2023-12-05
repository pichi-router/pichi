from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import get, replace_in_file
import os


class LibmaxminddbConan(ConanFile):
  name = "libmaxminddb"
  license = "Apache-2.0 License"
  author = "Maxmind <support@maxmind.com>"
  url = "http://www.maxmind.com/"
  description = "C library for the MaxMind DB file format"
  topics = ()
  settings = "os", "compiler", "build_type", "arch"
  options = {"shared": [True, False], "fPIC": [True, False]}
  default_options = {"shared": False, "fPIC": True}

  def config_options(self):
    if self.settings.os == "Windows":
      self.options.rm_safe("fPIC")

  def configure(self):
    if self.options.shared:
      self.options.rm_safe("fPIC")

  def layout(self):
    cmake_layout(self, src_folder="src")

  def source(self):
    get(self, **self.conan_data["sources"][self.version],
        destination=self.source_folder, strip_root=True)
    replace_in_file(self, os.path.join(self.source_folder, "bin", "CMakeLists.txt"),
                    "target_link_libraries(mmdblookup maxminddb pthread)",
                    """
                    find_package(Threads REQUIRED)
                    target_link_libraries(mmdblookup maxminddb Threads::Threads)
                    """)
  
  def generate(self):
    tc = CMakeToolchain(self)
    tc.cache_variables["BUILD_TESTING"] = False
    tc.cache_variables["BUILD_SHARED_LIBS"] = self.options.shared
    tc.generate()

  def build(self):
    cmake = CMake(self)
    cmake.configure()
    cmake.build()

  def package(self):
    cmake = CMake(self)
    cmake.install()

  def package_info(self):
    self.cpp_info.set_property("cmake_find_mode", "both")
    self.cpp_info.set_property("cmake_file_name", "MaxmindDB")
    self.cpp_info.set_property("cmake_target_name", "maxminddb::maxminddb")
    self.cpp_info.set_property("pkg_config_name", "libmaxminddb")
    self.cpp_info.libs = ["maxminddb"]
