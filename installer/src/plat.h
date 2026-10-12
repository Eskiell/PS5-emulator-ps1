/* PS5SX2 Installer: what differs between the PS5 and the host test build. */
#pragma once

#include <stddef.h>
#include <sys/types.h>

/* Put in front of logical paths ("/data/..."): empty on the PS5. */
const char *plat_root(void);

/* A PS5 system notification (debug style, text only), like the helper's "PS5SX2 Helper Loaded!".
 * On the host: a line on stderr and in $PS5SX2_ROOT/notifications.txt. */
void plat_notify_text(const char *text);

/* The kernel's random numbers; 0 on success. No weak fallback. */
int plat_random(void *buf, size_t len);

/* Test overrides are unavailable in the PS5 build. */
const char *plat_test_env(const char *name);

/* Ignore SIGPIPE etc. */
void plat_init(void);
