/* PSXS5 Installer: platform services used by the PS5 build and host tests. */
#pragma once

#include <stddef.h>
#include <sys/types.h>

/* Put in front of logical paths ("/data/..."): empty on the PS5. */
const char *plat_root(void);

/* Show a text-only PS5 system notification; host tests provide a stub. */
void plat_notify_text(const char *text);

/* 1 if this title has a live process, 0 if not, -1 if the check is inconclusive. */
int plat_app_running(const char *title_id);

/* The kernel's random numbers; 0 on success. No weak fallback. */
int plat_random(void *buf, size_t len);

/* Test overrides are unavailable in the PS5 build. */
const char *plat_test_env(const char *name);

/* Ignore SIGPIPE etc. */
void plat_init(void);
