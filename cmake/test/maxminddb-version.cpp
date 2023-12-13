#include <iostream>
#include <maxminddb.h>

int main(int, char**)
{
  std::cout << MMDB_lib_version() << std::endl;
  return 0;
}
