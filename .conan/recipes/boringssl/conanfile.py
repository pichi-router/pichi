from conans import ConanFile, CMake, tools


class BoringSSLConan(ConanFile):
  name = "boringssl"
  url = "https://boringssl.googlesource.com/boringssl"
  homepage = "https://boringssl.googlesource.com/boringssl"
  description = "BoringSSL is a fork of OpenSSL that is designed to meet Google's needs."
  topics = ("SSL", "TLS", "openssl")
  settings = "os", "compiler", "build_type", "arch"
  options = {"shared": [True, False], "fPIC": [True, False]}
  default_options = {"shared": False, "fPIC": True}
  exports_sources = "CMakeLists.txt"
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
    ret.definitions["FUZZ"] = False
    ret.definitions["RUST_BINDINGS"] = False
    ret.definitions["FIPS"] = False
    ret.definitions["BUILD_SHARED_LIBS"] = self.options.shared
    return ret

  def config_options(self):
    if self.settings.os == "Windows":
      del self.options.fPIC

  def configure(self):
    if self.options.shared:
      del self.options.fPIC

  def source(self):
    g = tools.Git(folder=self._source_subfolder)
    g.clone(self.url)

  def build(self):
    g = tools.Git(folder=self._source_subfolder)
    g.checkout(self.conan_data["versions"][self.version])
    cmake = self._cmake
    cmake.configure(build_folder=self._build_subfolder)
    cmake.build(build_dir=self._build_subfolder)

  def package(self):
    self._cmake.install(build_dir=self._build_subfolder)

  def package_info(self):
    self.cpp_info.set_property("cmake_find_mode", "both")
    self.cpp_info.set_property("cmake_file_name", "OpenSSL")
    self.cpp_info.set_property("pkg_config_name", "openssl")

    self.cpp_info.components["crypto"].set_property(
        "cmake_target_name", "OpenSSL::Crypto")
    self.cpp_info.components["crypto"].set_property(
        "pkg_config_name", "libcrypto")
    self.cpp_info.components["ssl"].set_property(
        "cmake_target_name", "OpenSSL::SSL")
    self.cpp_info.components["ssl"].set_property("pkg_config_name", "libssl")

    if self.settings.os == "Windows":
      self.cpp_info.components["crypto"].system_libs.extend(["crypt32", "ws2_32", "advapi32",
                                                             "user32", "bcrypt"])
    elif self.settings.os in ["Linux", "FreeBSD"]:
      self.cpp_info.components["crypto"].system_libs.extend(["dl", "rt"])
      self.cpp_info.components["ssl"].system_libs.append("dl")
      self.cpp_info.components["crypto"].system_libs.append("pthread")
      self.cpp_info.components["ssl"].system_libs.append("pthread")
