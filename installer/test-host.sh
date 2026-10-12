#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
out="$root/build/tests/installer"
mkdir -p "$out"
cc=${HOST_CC:-clang}
cxx=${HOST_CXX:-clang++}
san=(-fsanitize=address,undefined -fno-omit-frame-pointer)
features=(-D_POSIX_C_SOURCE=200809L)
if [[ "$(uname -s)" == Darwin ]]; then
    features+=(-D_DARWIN_C_SOURCE)
fi
includes=(-I"$root/third_party" -I"$root/examples/update-check" -I"$root/examples/self-update")
for name in util json log; do
    "$cc" -std=c11 -O1 -g "${san[@]}" "${features[@]}" "${includes[@]}" \
        -c "$root/installer/src/$name.c" -o "$out/$name.o"
done
for name in miniz miniz_tinfl miniz_tdef miniz_zip; do
    "$cc" -std=c11 -O1 -g "${san[@]}" "${features[@]}" "${includes[@]}" \
        -c "$root/third_party/miniz/$name.c" -o "$out/$name.o"
done
"$cc" -std=c11 -O1 -g "${san[@]}" "${features[@]}" -DUPDATE_CHECK_NO_NETWORK "${includes[@]}" \
    -c "$root/examples/update-check/update_check.c" -o "$out/update_check.o"
"$cc" -std=c11 -O1 -g "${san[@]}" "${features[@]}" "${includes[@]}" \
    -c "$root/examples/self-update/self_update_sha256.c" -o "$out/self_update_sha256.o"
"$cxx" -std=c++20 -O1 -g "${san[@]}" "${features[@]}" "${includes[@]}" \
    "$root/tests/test_installer.cpp" "$root/installer/src/install.cpp" \
    "$root/examples/self-update-helper/archive.cpp" \
    "$root/examples/self-update-helper/files.cpp" \
    "$root/examples/self-update-helper/updater.cpp" \
    "$out"/*.o -pthread -o "$out/test_installer"
"$out/test_installer"
