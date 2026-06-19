#include "pichi/common/config.hpp"
#include <boost/program_options.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
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

namespace fs  = std::filesystem;
namespace po  = boost::program_options;
namespace sys = boost::system;

using pichi::assertSuccess;

extern void run(std::string const&, uint16_t, std::string const&, std::string const&);

int main(int argc, char const* argv[])
{
  auto listen = std::string{};
  auto port   = uint16_t{};
  auto json   = std::string{};
  auto geo    = std::string{};
  auto user   = std::string{};
  auto group  = std::string{};
  auto pid_fn = std::string{};
  auto log_fn = std::string{};
  auto desc   = po::options_description{"Allow options"};
  desc.add_options()("help,h", "produce help message")("listen,l", po::value<std::string>(&listen)->default_value("::1"), "API server address")("port,p", po::value<uint16_t>(&port), "API server port")("geo,g", po::value<std::string>(&geo), "GEO file")("json", po::value<std::string>(&json), "Initial configration(JSON format)")("version,v", "show version")

#if defined(HAS_FORK) && defined(HAS_SETSID)
  ("daemon,d", "daemonize")("pid", po::value<std::string>(&pid_fn)->default_value("/var/run/pichi.pid"), "pid file")
    ("log", po::value<std::string>(&log_fn)->default_value("/var/log/pichi.log"), "log file")
#endif  // HAS_SETUID && HAS_GETPWNAM
#if defined(HAS_SETUID) && defined(HAS_GETPWNAM)
          ("user,u", po::value<std::string>(&user), "run as user")
#endif  // HAS_SETUID && HAS_GETPWNAM
#if defined(HAS_SETGID) && defined(HAS_GETGRNAM)
              ("group", po::value<std::string>(&group), "run as group")
#endif  // HAS_SETGID && HAS_GETGRNAM
      ;
  auto vm = po::variables_map{};

  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }

    if (vm.count("version")) {
      std::cout << std::format("Pichi version {}\n", PICHI_VERSION);
      return 0;
    }

    if (!vm.count("port")) {
      std::cout << desc << std::endl;
      return 1;
    }

    errno = 0;

#if defined(HAS_FORK) && defined(HAS_SETSID)
    if (vm.count("daemon")) {
      assertSuccess(chdir("/"));
      auto pid_file = fs::path{pid_fn};
      auto log_file = fs::path{log_fn};
      auto pid      = fork();
      assertSuccess(pid);
      if (pid > 0) {
        auto ec = sys::error_code{};
        fs::create_directories(pid_file.parent_path(), ec);
        if (!ec) std::ofstream{pid_file.string()} << pid << std::endl;
        exit(0);
      }
      setsid();
#ifdef HAS_CLOSE
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
#endif  // HAS_CLOSE
      auto ec = sys::error_code{};
      fs::create_directories(log_file.parent_path(), ec);
      if (!ec) {
        assertSuccess(freopen(log_file.c_str(), "a", stdout));
        assertSuccess(freopen(log_file.c_str(), "a", stderr));
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
  catch (std::exception const& e) {
    std::cout << "ERROR: " << e.what() << std::endl;
    return 1;
  }
}
