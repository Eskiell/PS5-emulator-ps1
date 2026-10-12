#include "log.h"
#include "plat.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void write_log(const char *line) {
  char path[1024];
  const int length = snprintf(path, sizeof(path), "%s/data/PSXS5-Installer/installer.log", plat_root());
  if (length <= 0 || (size_t)length >= sizeof(path))
    return;
  const int fd = open(path, O_WRONLY | O_APPEND | O_CREAT | O_NOFOLLOW, 0666);
  if (fd < 0)
    return;
  (void)write(fd, line, strlen(line));
  (void)write(fd, "\n", 1);
  close(fd);
}

void log_line(const char *fmt, ...) {
  char line[1536];
  va_list args;
  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);
  write_log(line);
#ifdef PSXS5_HOST
  fprintf(stderr, "[installer] %s\n", line);
#endif
}

void notify(const char *fmt, ...) {
  char line[1024];
  va_list args;
  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);
  log_line("notification: %s", line);
  plat_notify_text(line);
}
