from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.files import copy
import os


class PichiConan(ConanFile):
  name = "pichi"
  license = "BSD-3-Clause License"
  author = "Pichi <pichi@elude.in>"
  url = "https://github.com/pichi-router/pichi"
  description = "Flexible Rule-Based Proxy"
  topics = ("shadowsocks", "proxy", "http-proxy",
            "trojan", "socks5-proxy", "trojan-gfw")
  settings = "os", "compiler", "build_type", "arch"
  options = {"shared": [True, False], "fPIC": [True, False],
             "build_test": [True, False], "build_server": [True, False],
             "tls_fingerprint": [True, False],
             "transparent": ["none", "pf", "iptables"]}
  default_options = {"shared": False, "fPIC": True, "build_test": True, "build_server": True,
                     "tls_fingerprint": False, "transparent": "none"}
  requires = "boost/[>=1.72.0]", "mbedtls/[>=3.0.0]", "libsodium/[>=1.0.12]", \
             "libmaxminddb/[>=1.5.0]", "rapidjson/1.1.0"
  exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "src/*", "server/*", "test/*"

  def config_options(self):
    if self.settings.os == "Windows":
      self.options.rm_safe("fPIC")
    if self.settings.os in ["Android", "iOS", "tvOS", "watchOS"]:
      self.options.build_test = False
      self.options.build_server = False
      self.options.shared = False
    self.settings.compiler.cppstd = "17"

  def configure(self):
    if self.options.shared:
      self.options.rm_safe("fPIC")

  def requirements(self):
    if self.options.tls_fingerprint:
      self.requires("brotli/[>=1.0.0]")
      self.requires("boringssl/[>=18]")
    else:
      self.requires("openssl/1.1.1q")

  def layout(self):
    cmake_layout(self)
  
  def generate(self):
    deps = CMakeDeps(self)
    deps.generate()

    tc = CMakeToolchain(self)
    tc.cache_variables["ENABLE_CONAN"] = True
    tc.cache_variables["INSTALL_DEVEL"] = True
    tc.cache_variables["VERSION"] = self.version
    tc.cache_variables["BUILD_SERVER"] = self.options.build_server
    tc.cache_variables["BUILD_TEST"] = self.options.build_test
    tc.cache_variables["TRANSPARENT_PF"] = self.options.transparent == "pf"
    tc.cache_variables["TRANSPARENT_IPTABLES"] = self.options.transparent == "iptables"
    tc.cache_variables["TLS_FINGERPRINT"] = self.options.tls_fingerprint
    tc.cache_variables["BUILD_SHARED_LIBS"] = self.options.shared
    tc.generate()

  def build(self):
    cmake = CMake(self)
    cmake.configure()
    cmake.build()
    if self.options.build_test:
      cmake.test()

  def package(self):
    cmake = CMake(self)
    cmake.install()

  def package_info(self):
    self.cpp_info.set_property("cmake_find_mode", "both")
    self.cpp_info.set_property("cmake_file_name", "pichi")
    self.cpp_info.set_property("cmake_target_name", "pichi::pichi")
    self.cpp_info.set_property("pkg_config_name", "libpichi")
    self.cpp_info.libs = ["pichi"]
