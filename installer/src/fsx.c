#include "fsx.h"
#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int fs_write_all(int fd, const void *data, size_t size) {
  const char *bytes = data;
  while (size) {
    const ssize_t count = write(fd, bytes, size);
    if (count < 0 && errno == EINTR)
      continue;
    if (count <= 0)
      return -1;
    bytes += count;
    size -= (size_t)count;
  }
  return 0;
}

int fs_create_work_file(const char *path, mode_t mode) {
  struct stat st;
  if (lstat(path, &st) == 0) {
    if (!S_ISREG(st.st_mode) || unlink(path) != 0) {
      err_set("the partial download is not a regular file");
      return -1;
    }
  } else if (errno != ENOENT) {
    err_set("cannot inspect the partial download: %s", strerror(errno));
    return -1;
  }
  const int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, mode);
  if (fd < 0)
    err_set("cannot create the partial download: %s", strerror(errno));
  return fd;
}

int fs_remove_work_file(const char *path) {
  struct stat st;
  if (lstat(path, &st) != 0)
    return errno == ENOENT ? 0 : -1;
  return S_ISREG(st.st_mode) ? unlink(path) : -1;
}
