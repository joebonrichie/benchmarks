// g++ -std=c++0x -Wall -O2 -DNDEBUG -o memtest memtest.cpp

#include <chrono>
#include <cstring>
#include <iostream>
#include <cstdint>

class Timer
{
 public:
  Timer()
      : mStart(),
        mStop()
  {
    update();
  }

  void update()
  {
    mStart = std::chrono::high_resolution_clock::now();
    mStop  = mStart;
  }

  double elapsedMs()
  {
    mStop = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(mStop - mStart);
    return elapsed_ms.count();
  }

 private:
  std::chrono::high_resolution_clock::time_point mStart;
  std::chrono::high_resolution_clock::time_point mStop;
};

std::string formatBytes(std::uint64_t bytes)
{
  static const int num_suffix = 5;
  static const char* suffix[num_suffix] = { "B", "KB", "MB", "GB", "TB" };
  double dbl_s_byte = bytes;
  int i = 0;
  for (; (int)(bytes / 1024.) > 0 && i < num_suffix;
       ++i, bytes /= 1024.)
  {
    dbl_s_byte = bytes / 1024.0;
  }

  const int buf_len = 64;
  char buf[buf_len];

  // use snprintf so there is no buffer overrun
  int res = snprintf(buf, buf_len,"%0.2f%s", dbl_s_byte, suffix[i]);

  // snprintf returns number of characters that would have been written if n had
  //       been sufficiently large, not counting the terminating null character.
  //       if an encoding error occurs, a negative number is returned.
  if (res >= 0)
  {
    return std::string(buf);
  }
  return std::string();
}

void doMemmove(void* pDest, const void* pSource, std::size_t sizeBytes)
{
  memmove(pDest, pSource, sizeBytes);
}

int main(int argc, char* argv[])
{
  std::uint64_t SIZE_BYTES = 1073741824; // 1GB

  if (argc > 1)
  {
    SIZE_BYTES = std::stoull(argv[1]);
    std::cout << "Using buffer size from command line: " << formatBytes(SIZE_BYTES)
              << std::endl;
  }
  else
  {
    std::cout << "To specify a custom buffer size: big_memcpy_test [SIZE_BYTES] \n"
              << "Using built in buffer size: " << formatBytes(SIZE_BYTES)
              << std::endl;
  }


  // big array to use for testing
  char* p_big_array = NULL;

  /////////////
  // malloc 
  {
    Timer timer;

    p_big_array = (char*)malloc(SIZE_BYTES * sizeof(char));
    if (p_big_array == NULL)
    {
      std::cerr << "ERROR: malloc of " << SIZE_BYTES << " returned NULL!"
                << std::endl;
      return 1;
    }

    std::cout << "malloc for " << formatBytes(SIZE_BYTES) << " took "
              << timer.elapsedMs() << "ms"
              << std::endl;
  }

  /////////////
  // memset
  {
    Timer timer;

    // set all data in p_big_array to 0
    memset(p_big_array, 0xF, SIZE_BYTES * sizeof(char));

    double elapsed_ms = timer.elapsedMs();
    std::cout << "memset for " << formatBytes(SIZE_BYTES) << " took "
              << elapsed_ms << "ms "
              << "(" << formatBytes(SIZE_BYTES / (elapsed_ms / 1.0e3)) << " bytes/sec)"
              << std::endl;
  }

  /////////////
  // memcpy 
  {
    char* p_dest_array = (char*)malloc(SIZE_BYTES);
    if (p_dest_array == NULL)
    {
      std::cerr << "ERROR: malloc of " << SIZE_BYTES << " for memcpy test"
                << " returned NULL!"
                << std::endl;
      return 1;
    }
    memset(p_dest_array, 0xF, SIZE_BYTES * sizeof(char));

    // time only the memcpy FROM p_big_array TO p_dest_array
    Timer timer;

    memcpy(p_dest_array, p_big_array, SIZE_BYTES * sizeof(char));

    double elapsed_ms = timer.elapsedMs();
    std::cout << "memcpy for " << formatBytes(SIZE_BYTES) << " took "
              << elapsed_ms << "ms "
              << "(" << formatBytes(SIZE_BYTES / (elapsed_ms / 1.0e3)) << " bytes/sec)"
              << std::endl;

    // cleanup p_dest_array
    free(p_dest_array);
    p_dest_array = NULL;
  }

  /////////////
  // memmove
  {
    char* p_dest_array = (char*)malloc(SIZE_BYTES);
    if (p_dest_array == NULL)
    {
      std::cerr << "ERROR: malloc of " << SIZE_BYTES << " for memmove test"
                << " returned NULL!"
                << std::endl;
      return 1;
    }
    memset(p_dest_array, 0xF, SIZE_BYTES * sizeof(char));

    // time only the memmove FROM p_big_array TO p_dest_array
    Timer timer;

    // memmove(p_dest_array, p_big_array, SIZE_BYTES * sizeof(char));
    doMemmove(p_dest_array, p_big_array, SIZE_BYTES * sizeof(char));

    double elapsed_ms = timer.elapsedMs();
    std::cout << "memmove for " << formatBytes(SIZE_BYTES) << " took "
              << elapsed_ms << "ms "
              << "(" << formatBytes(SIZE_BYTES / (elapsed_ms / 1.0e3)) << " bytes/sec)"
              << std::endl;

    // cleanup p_dest_array
    free(p_dest_array);
    p_dest_array = NULL;
  }


  // cleanup
  free(p_big_array);
  p_big_array = NULL;

  return 0;
}
