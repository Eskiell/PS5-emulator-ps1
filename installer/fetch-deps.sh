#!/usr/bin/env bash
# Fetch the pinned Mbed TLS 3.6.7 source and verify its SHA-256.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cache="$root/.deps/installer"
archive="$cache/mbedtls-3.6.7.tar.bz2"
url="https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.7/mbedtls-3.6.7.tar.bz2"
expected="a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6"
mkdir -p "$cache"
if [[ ! -f "$archive" ]]; then
    curl --fail --location --retry 3 --output "$archive.download" "$url"
    mv "$archive.download" "$archive"
fi
if command -v sha256sum >/dev/null; then
    actual=$(sha256sum "$archive" | cut -d' ' -f1)
else
    actual=$(shasum -a 256 "$archive" | cut -d' ' -f1)
fi
[[ "$actual" == "$expected" ]] || {
    echo "Mbed TLS download does not match the pinned SHA-256" >&2
    exit 1
}
[[ -d "$cache/mbedtls-3.6.7" ]] || tar -xjf "$archive" -C "$cache"

# The local /opt SDK can lack its C++ headers. Use the project's pinned v0.42
# SDK in the ignored cache, also used by the main app's Linux build.
sdk_cache="$root/.deps/native"
sdk="$sdk_cache/ps5-payload-sdk"
if [[ ! -f "$sdk/target/include/c++/v1/string" ]]; then
    mkdir -p "$sdk_cache"
    sdk_archive="$sdk_cache/ps5-payload-sdk.zip"
    curl --fail --location --retry 3 --output "$sdk_archive.download" \
        https://github.com/ps5-payload-dev/sdk/releases/download/v0.42/ps5-payload-sdk.zip
    mv "$sdk_archive.download" "$sdk_archive"
    if command -v sha256sum >/dev/null; then
        sdk_actual=$(sha256sum "$sdk_archive" | cut -d' ' -f1)
    else
        sdk_actual=$(shasum -a 256 "$sdk_archive" | cut -d' ' -f1)
    fi
    [[ "$sdk_actual" == "8cfbc7cd5811e719eb4f0c47eea668d3dc7b40bc8ab11c4a5031d40c23ec02da" ]] || {
        echo "PS5 SDK download does not match the pinned SHA-256" >&2
        exit 1
    }
    unzip -q -o "$sdk_archive" -d "$sdk_cache"
fi
