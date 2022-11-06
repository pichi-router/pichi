#include <brotli/decode.h>
#include <iostream>

int main(int, char**)
{
  auto version = BrotliDecoderVersion();
  std::cout << (version >> 24) << "." << ((version >> 12) & 0xfff) << "." << (version & 0xfff);
  return 0;
}