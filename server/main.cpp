#include "config.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <pichi.h>
#include <stdio.h>
#ifdef HAS_UNISTD_H
#include <errno.h>
#include <unistd.h>
#endif // HAS_UNISTD_H
#ifdef HAS_PWD_H
#include <pwd.h>
#endif // HAS_PWD_H
#ifdef HAS_GRP_H
#include <grp.h>
#endif // HAS_GRP_H

using namespace std;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace sys = boost::system;

static auto const PID_FILE = (fs::path{PICHI_PREFIX} / "var" / "run" / "pichi.pid");
static auto const LOG_FILE = (fs::path{PICHI_PREFIX} / "var" / "log" / "pichi.log");

extern void run(string const&, uint16_t, string const&, string const&);

#ifdef HAS_UNISTD_H

class Exception : public exception {
public:
  explicit Exception(int e) : errno_{e} {}

  char const* what() const noexcept override { return strerror(errno_); }

private:
  int errno_;
};

[[noreturn]] static void fail() { throw Exception(errno); }

static void assertTrue(bool b)
{
  if (!b) fail();
}

static void assertFalse(bool b)
{
  if (b) fail();
}

#endif // HAS_UNISTD_H

int main(int argc, char const* argv[])
{
  auto listen = string{};
  auto port = uint16_t{};
  auto json = string{};
  auto geo = string{};
  auto user = string{};
  auto group = string{};
  auto desc = po::options_description{"Allow options"};
  desc.add_options()("help,h", "produce help message")(
      "listen,l", po::value<string>(&listen)->default_value("::1"),
      "API server address")("port,p", po::value<uint16_t>(&port), "API server port")(
      "geo,g", po::value<string>(&geo), "GEO file")("json", po::value<string>(&json),
                                                    "Initial configration(JSON format)")
#if defined(HAS_FORK) && defined(HAS_SETSID)
      ("daemon,d", "daemonize")
#endif // HAS_SETUID && HAS_GETPWNAM
#if defined(HAS_SETUID) && defined(HAS_GETPWNAM)
          ("user,u", po::value<string>(&user), "run as user")
#endif // HAS_SETUID && HAS_GETPWNAM
#if defined(HAS_SETGID) && defined(HAS_GETGRNAM)
              ("group", po::value<string>(&group), "run as group")
#endif // HAS_SETGID && HAS_GETGRNAM
      ;
  auto vm = po::variables_map{};

  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") || !vm.count("port") || !vm.count("geo")) {
      cout << desc << endl;
      return 1;
    }

    errno = 0;

#if defined(HAS_FORK) && defined(HAS_SETSID)
    if (vm.count("daemon")) {
      assertTrue(chdir(fs::path{PICHI_PREFIX}.root_directory().c_str()) == 0);
      auto pid = fork();
      assertFalse(pid < 0);
      if (pid > 0) {
        auto ec = sys::error_code{};
        fs::create_directories(PID_FILE.parent_path(), ec);
        if (!ec) ofstream{PID_FILE.string()} << pid << endl;
        exit(0);
      }
      setsid();
#ifdef HAS_CLOSE
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
#endif // HAS_CLOSE
      auto ec = sys::error_code{};
      fs::create_directories(LOG_FILE.parent_path(), ec);
      if (!ec) {
        assertTrue(freopen(LOG_FILE.c_str(), "a", stdout) != nullptr);
        assertTrue(freopen(LOG_FILE.c_str(), "a", stderr) != nullptr);
      }
    }
#endif // HAS_FORK && HAS_SETSID

#if defined(HAS_SETGID) && defined(HAS_GETGRNAM)
    if (!group.empty()) {
      auto grp = getgrnam(group.c_str());
      assertFalse(grp == nullptr);
      assertFalse(setgid(grp->gr_gid) == -1);
    }
#endif // HAS_SETGID && HAS_GETGRNAM

#if defined(HAS_SETUID) && defined(HAS_GETPWNAM)
    if (!user.empty()) {
      auto pw = getpwnam(user.c_str());
      assertFalse(pw == nullptr);
      assertFalse(setuid(pw->pw_uid) == -1);
    }
#endif // HAS_SETUID && HAS_GETPWNAM

    run(listen, port, json, geo);
    return 0;
  }
  catch (exception const& e) {
    cout << "ERROR: " << e.what() << endl;
    return 1;
  }
}
