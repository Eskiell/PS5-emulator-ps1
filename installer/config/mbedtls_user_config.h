/* PSX1 Installer: Mbed TLS 3.6 settings.
 * (MBEDTLS_USER_CONFIG_FILE is read at the end of mbedtls_config.h).
 *
 * The installer is a TLS client only: HTTPS to api.github.com, github.com and
 * release-assets.githubusercontent.com. Platform access goes through our own code:
 *   - sockets: src/net.c (no MBEDTLS_NET_C);
 *   - entropy: mbedtls_hardware_poll() uses the kernel's random generator;
 *   - no CA file on PS5: the bundle is compiled into cacert_data.c.
 */
#ifndef PSXS5_MBEDTLS_USER_CONFIG_H
#define PSXS5_MBEDTLS_USER_CONFIG_H

/* Platform glue we don't use. */
#undef MBEDTLS_NET_C
#undef MBEDTLS_TIMING_C
#undef MBEDTLS_FS_IO
#undef MBEDTLS_PSA_ITS_FILE_C
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C

/* Entropy: only the kernel's random numbers (see above). */
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT

/* Client only: no server side, no DTLS. */
#undef MBEDTLS_SSL_SRV_C
#undef MBEDTLS_SSL_PROTO_DTLS
#undef MBEDTLS_SSL_DTLS_ANTI_REPLAY
#undef MBEDTLS_SSL_DTLS_HELLO_VERIFY
#undef MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE
#undef MBEDTLS_SSL_DTLS_CONNECTION_ID
#undef MBEDTLS_SSL_DTLS_SRTP
#undef MBEDTLS_SSL_COOKIE_C
#undef MBEDTLS_SSL_CACHE_C
#undef MBEDTLS_SSL_TICKET_C
#undef MBEDTLS_SSL_EARLY_DATA

/* Size. */
#undef MBEDTLS_SELF_TEST
#undef MBEDTLS_DEBUG_C

#endif
