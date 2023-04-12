from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout
from conan.tools.files import get, replace_in_file
import os


class BoringSSLConan(ConanFile):
  name = "boringssl"

  url = "https://boringssl.googlesource.com/boringssl"
  homepage = "https://boringssl.googlesource.com/boringssl"
  description = "BoringSSL is a fork of OpenSSL that is designed to meet Google's needs."
  topics = ("SSL", "TLS", "openssl")

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
    get(self, **self.conan_data["sources"][self.version], verify=False)
    s = "install_if_enabled(TARGETS bssl DESTINATION ${INSTALL_DESTINATION_DEFAULT})"
    a = "set_target_properties(bssl PROPERTIES MACOSX_BUNDLE False)"
    replace_in_file(self, os.path.join(self.source_folder, "tool", "CMakeLists.txt"),
                    s, f"{a}\n{s}")
  
  def generate(self):
    tc = CMakeToolchain(self)
    tc.cache_variables["FUZZ"] = False
    tc.cache_variables["RUST_BINDINGS"] = False
    tc.cache_variables["FIPS"] = False
    tc.cache_variables["BUILD_SHARED_LIBS"] = self.options.shared
    tc.generate()

  def build(self):
    cmake = CMake(self)
    cmake.configure()
    # Only target 'bssl' is necessary for installation
    cmake.build(target="bssl")

  def package(self):
    # CMake.install invokes 'cmake --target install', which leads to building tests.
    # As a result, try 'cmake --install' instead
    self.run("cmake --install %s --config %s" %
             (os.path.join(self.source_folder, self.build_folder), self.settings.build_type))

  def package_info(self):
    self.cpp_info.set_property("cmake_find_mode", "both")
    self.cpp_info.set_property("cmake_file_name", "BoringSSL")
    self.cpp_info.set_property("pkg_config_name", "openssl")

    # Crypto
    self.cpp_info.components["crypto"].set_property(
        "cmake_target_name", "BoringSSL::Crypto")
    self.cpp_info.components["crypto"].set_property(
        "pkg_config_name", "libcrypto")
    self.cpp_info.components["crypto"].libs = ["crypto"]

    # SSL
    self.cpp_info.components["ssl"].set_property(
        "cmake_target_name", "BoringSSL::SSL")
    self.cpp_info.components["ssl"].set_property("pkg_config_name", "libssl")
    self.cpp_info.components["ssl"].libs = ["ssl"]
    self.cpp_info.components["ssl"].requires = ["crypto"]

    if self.settings.os == "Windows":
      self.cpp_info.components["crypto"].system_libs.extend(["crypt32", "ws2_32", "advapi32",
                                                             "user32", "bcrypt"])
    elif self.settings.os in ["Linux", "FreeBSD"]:
      self.cpp_info.components["crypto"].system_libs.extend(["dl", "rt", "pthread"])
