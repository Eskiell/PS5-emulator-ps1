/* PSXS5 Installer: PS5 implementation of plat.h.
 *
 * The notification request layout follows the payload SDK's samples.
 * sysctl(CTL_KERN, KERN_ARND) and signal() use the SDK's system headers.
 */
#include "plat.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>

/* The payload SDK's ps sample uses this layout with sceKernelGetAppInfo. */
typedef struct {
  uint32_t app_id;
  uint64_t reserved1;
  uint32_t app_type;
  char title_id[10];
  char reserved2[0x3c];
} app_info_t;

int sceKernelGetAppInfo(pid_t pid, app_info_t *info);

int plat_app_running(const char *title_id) {
  if (!title_id || strlen(title_id) != 9)
    return -1;

  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PROC, 0};
  for (int attempt = 0; attempt < 3; ++attempt) {
    size_t needed = 0;
    if (sysctl(mib, 4, NULL, &needed, NULL, 0) != 0 || needed == 0 || needed > (8u << 20)) {
      log_line("process check: cannot size the PS5 process list (errno %d)", errno);
      return -1;
    }
    const size_t capacity = needed + (16u << 10);
    unsigned char *buffer = malloc(capacity);
    if (!buffer) {
      log_line("process check: cannot allocate %zu bytes", capacity);
      return -1;
    }
    size_t used = capacity;
    if (sysctl(mib, 4, buffer, &used, NULL, 0) != 0) {
      const int failure = errno;
      free(buffer);
      if (failure == ENOMEM)
        continue; /* Process list grew between the two sysctl calls. */
      log_line("process check: cannot read the PS5 process list (errno %d)", failure);
      return -1;
    }

    int result = 0;
    for (size_t offset = 0; offset < used;) {
      const size_t remaining = used - offset;
      int stride = 0;
      if (remaining >= sizeof(stride))
        memcpy(&stride, buffer + offset, sizeof(stride));
      if (stride < (int)(offsetof(struct kinfo_proc, ki_comm) + sizeof(((struct kinfo_proc *)0)->ki_comm)) ||
          (size_t)stride > remaining) {
        log_line("process check: invalid process record");
        result = -1;
        break;
      }
      const struct kinfo_proc *process = (const struct kinfo_proc *)(buffer + offset);
      if (process->ki_pid > 0 && process->ki_stat != SZOMB) {
        app_info_t info = {0};
        if (sceKernelGetAppInfo(process->ki_pid, &info) == 0) {
          if (memcmp(info.title_id, title_id, 9) == 0 && info.title_id[9] == '\0') {
            result = 1;
            break;
          }
        } else if (strncmp(process->ki_comm, "eboot.bin", sizeof(process->ki_comm)) == 0) {
          /* An unidentified app process could be PSXS5: do not replace it. */
          log_line("process check: cannot identify app process %d", process->ki_pid);
          result = -1;
        }
      }
      offset += (size_t)stride;
    }
    free(buffer);
    return result;
  }
  log_line("process check: process list kept changing");
  return -1;
}

typedef struct {
  int32_t type;            /* 0x00 */
  int32_t req_id;          /* 0x04 */
  int32_t priority;        /* 0x08 */
  int32_t msg_id;          /* 0x0C */
  int32_t target_id;       /* 0x10 */
  int32_t user_id;         /* 0x14 */
  int32_t unk1;            /* 0x18 */
  int32_t unk2;            /* 0x1C */
  int32_t app_id;          /* 0x20 */
  int32_t error_num;       /* 0x24 */
  int32_t unk3;            /* 0x28 */
  char use_icon_image_uri; /* 0x2C */
  char message[1024];      /* 0x2D */
  char uri[1024];          /* 0x42D */
  char unkstr[1024];       /* 0x82D */
  char pad[3];             /* to 0xC30 */
} notify_request_t;

_Static_assert(sizeof(notify_request_t) == 0xC30, "notification request must be 0xC30 bytes");
_Static_assert(offsetof(notify_request_t, message) == 0x2D, "message at 0x2D");

int sceKernelSendNotificationRequest(int32_t device, void *request, size_t size, int32_t blocking);

const char *plat_root(void) { return ""; }

void plat_notify_text(const char *text) {
  static notify_request_t req; /* 3 KB: not on the stack */
  memset(&req, 0, sizeof(req));
  req.type = 0;
  req.target_id = -1;
  req.use_icon_image_uri = 0; /* debug style: text only */
  size_t n = strlen(text);
  if (n > sizeof(req.message) - 1)
    n = sizeof(req.message) - 1;
  memcpy(req.message, text, n);
  (void)sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}

int plat_random(void *buf, size_t len) {
  uint8_t *p = buf;
  while (len) {
    size_t chunk = len > 256 ? 256 : len; /* kern.arandom hands out at most 256 bytes per call */
    size_t got = chunk;
    int mib[2] = {CTL_KERN, KERN_ARND};
    if (sysctl(mib, 2, p, &got, NULL, 0) != 0 || got == 0) {
      /* The same kernel generator through the device node. */
      const int fd = open("/dev/urandom", O_RDONLY);
      if (fd < 0)
        return -1;
      const ssize_t r = read(fd, p, chunk);
      close(fd);
      if (r <= 0)
        return -1;
      got = (size_t)r;
    }
    if (got > chunk)
      return -1;
    p += got;
    len -= got;
  }
  return 0;
}

const char *plat_test_env(const char *name) {
  (void)name;
  return NULL;
}

void plat_init(void) { signal(SIGPIPE, SIG_IGN); }
