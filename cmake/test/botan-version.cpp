#include <botan/version.h>
#include <iostream>

int main()
{
  std::cout << Botan::short_version_string();
  return 0;
}
