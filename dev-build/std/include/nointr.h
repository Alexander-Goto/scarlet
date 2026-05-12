#pragma once

#include "include.h"

static inline int nointr_close(int const fildes) {
     #pragma nounroll
     while (true) {
          int const result = close(fildes);
          if (result == 0 || errno != EINTR) return result;
     }
}

static inline int nointr_closedir(DIR* const dirp) {
     #pragma nounroll
     while (true) {
          int const result = closedir(dirp);
          if (result == 0 || errno != EINTR) return result;
     }
}

static inline int nointr_ftruncate(int const fildes, off_t const length) {
     #pragma nounroll
     while (true) {
          int const result = ftruncate(fildes, length);
          if (result == 0 || errno != EINTR) return result;
     }
}

static inline int nointr_dup2(int const fildes, int const fildes2) {
     #pragma nounroll
     while (true) {
          int const result = dup2(fildes, fildes2);
          if (result != -1 || errno != EINTR) return result;
     }
}

static inline pid_t nointr_waitpid(pid_t const pid, int* const stat_loc, int const options) {
     #pragma nounroll
     while (true) {
          pid_t const result = waitpid(pid, stat_loc, options);
          if (result == pid || errno != EINTR) return result;
     }
}

static inline struct passwd* nointr_getpwuid(uid_t const uid) {
     #pragma nounroll
     while (true) {
          struct passwd* const result = getpwuid(uid);
          if (result != nullptr || errno != EINTR) return result;
     }
}

static inline ssize_t nointr_read(int const fildes, void* const buf, size_t const nbyte) {
     #pragma nounroll
     while (true) {
          ssize_t const result = read(fildes, buf, nbyte);
          if (result != -1 || errno != EINTR) return result;
     }
}

static inline ssize_t nointr_write(int const fildes, const void* const buf, size_t const nbyte) {
     #pragma nounroll
     while (true) {
          ssize_t const result = write(fildes, buf, nbyte);
          if (result != -1 || errno != EINTR) return result;
     }
}

#define close(fildes)                   nointr_close(fildes)
#define closedir(dirp)                  nointr_closedir(dirp)
#define ftruncate(fildes, length)       nointr_ftruncate(fildes, length)
#define dup2(fildes, fildes2)           nointr_dup2(fildes, fildes2)
#define waitpid(pid, stat_loc, options) nointr_waitpid(pid, stat_loc, options)
#define getpwuid(uid)                   nointr_getpwuid(uid)
#define read(fildes, buf, nbyte)        nointr_read(fildes, buf, nbyte)
#define write(fildes, buf, nbyte)       nointr_write(fildes, buf, nbyte)