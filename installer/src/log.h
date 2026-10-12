#pragma once
void log_line(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void notify(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
