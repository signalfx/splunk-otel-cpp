#include <unistd.h>

inline ssize_t SocketWrite(int fd, const char* buf, size_t len) {
  const ssize_t towrite = (ssize_t)len;
  ssize_t written = 0;
  while (written != towrite) {
    ssize_t r = write(fd, buf + written, towrite - written);
    if (r == 0)
      return written;
    if (r == -1) {
      return -1;
    }

    written += r;
  }

  return written;
}
