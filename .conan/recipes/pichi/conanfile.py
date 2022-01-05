from conans import ConanFile, CMake
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
             "tls_lib": ["libressl", "openssl"]}
  default_options = {"shared": False, "fPIC": True, "build_test": True, "build_server": True,
                     "tls_lib": "libressl"}
  generators = "cmake"
  requires = "boost/[>=1.72.0]@", "mbedtls/[>=2.7.0]@", "libsodium/[>=1.0.12]@", \
             "libmaxminddb/[>=1.5.0]@", "rapidjson/[>=1.1.0]@"

  def _configure_cmake(self):
    cmake = CMake(self)
    cmake.definitions["ENABLE_CONAN"] = "ON"
    cmake.definitions["BUILD_SERVER"] = "ON" if self.options.build_server else "OFF"
    cmake.definitions["BUILD_TEST"] = "ON" if self.options.build_test else "OFF"
    cmake.definitions["STATIC_LINK"] = "OFF" if self.options.shared else "ON"
    cmake.definitions["INSTALL_DEVEL"] = "ON"
    cmake.definitions["CMAKE_BUILD_TYPE"] = self.settings.build_type
    return cmake

  def config_options(self):
    if self.settings.os == "Windows":
      del self.options.fPIC
    if self.settings.os in ["Android", "iOS", "tvOS", "watchOS"]:
      self.options.build_test = False
      self.options.build_server = False
      self.options.shared = False
    self.settings.compiler.cppstd = "17"

  def requirements(self):
    if self.options.tls_lib == "libressl":
      self.requires("libressl/[>=3.0.0]@")
    else:
      self.requires("openssl/1.1.1m@")

  def build(self):
    cmake = self._configure_cmake()
    cmake.configure()
    cmake.build()
    if self.options.build_test:
      cmake.test(output_on_failure=True)

  def export_sources(self):
    for item in ["CMakeLists.txt", "cmake", "include", "src", "server", "test"]:
      target = os.path.join("..", "..", "..", item)
      if os.path.isdir(target):
        self.copy(os.path.join(target, "*"), dst=item)
      else:
        self.copy(target)

  def package(self):
    cmake = self._configure_cmake()
    cmake.install()

  def package_info(self):
    self.cpp_info.libs = ["pichi"]
