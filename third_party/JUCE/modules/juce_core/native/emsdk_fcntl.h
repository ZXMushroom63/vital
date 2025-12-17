// emscripten_fcntl.h
#define EMSCRIPTEN_FCNTL_H

#include <stdint.h>

#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0200
#define O_EXCL      0x0400
#define O_TRUNC     0x02000
#define O_APPEND    0x0008
#define O_NONBLOCK  0x8000

extern "C" {
    int open(const char* pathname, int flags, ...);
    int close(int fd);
    int fcntl(int fd, int cmd, ...);
}