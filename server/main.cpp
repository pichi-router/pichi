#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <pichi/common/asserts.hpp>
#include <stdio.h>
#ifdef HAS_UNISTD_H
#include <errno.h>
#include <unistd.h>
#endif  // HAS_UNISTD_H
#ifdef HAS_PWD_H
#include <pwd.h>
#endif  // HAS_PWD_H
#ifdef HAS_GRP_H
#include <grp.h>
#endif  // HAS_GRP_H

using namespace std;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace sys = boost::system;

using pichi::assertSuccess;

static auto const PID_FILE = (fs::path{PICHI_PREFIX} / "var" / "run" / "pichi.pid");
static auto const LOG_FILE = (fs::path{PICHI_PREFIX} / "var" / "log" / "pichi.log");

extern void run(string const&, uint16_t, string const&, string const&);

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
#endif  // HAS_SETUID && HAS_GETPWNAM
#if defined(HAS_SETUID) && defined(HAS_GETPWNAM)
          ("user,u", po::value<string>(&user), "run as user")
#endif  // HAS_SETUID && HAS_GETPWNAM
#if defined(HAS_SETGID) && defined(HAS_GETGRNAM)
              ("group", po::value<string>(&group), "run as group")
#endif  // HAS_SETGID && HAS_GETGRNAM
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
      assertSuccess(chdir(fs::path{PICHI_PREFIX}.root_directory().c_str()));
      auto pid = fork();
      assertSuccess(pid);
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
#endif  // HAS_CLOSE
      auto ec = sys::error_code{};
      fs::create_directories(LOG_FILE.parent_path(), ec);
      if (!ec) {
        assertSuccess(freopen(LOG_FILE.c_str(), "a", stdout));
        assertSuccess(freopen(LOG_FILE.c_str(), "a", stderr));
      }
    }
#endif  // HAS_FORK && HAS_SETSID

#if defined(HAS_SETGID) && defined(HAS_GETGRNAM)
    if (!group.empty()) {
      auto grp = getgrnam(group.c_str());
      assertSuccess(grp);
      assertSuccess(setgid(grp->gr_gid));
    }
#endif  // HAS_SETGID && HAS_GETGRNAM

#if defined(HAS_SETUID) && defined(HAS_GETPWNAM)
    if (!user.empty()) {
      auto pw = getpwnam(user.c_str());
      assertSuccess(pw);
      assertSuccess(setuid(pw->pw_uid));
    }
#endif  // HAS_SETUID && HAS_GETPWNAM

    run(listen, port, json, geo);
    return 0;
  }
  catch (exception const& e) {
    cout << "ERROR: " << e.what() << endl;
    return 1;
  }
}
