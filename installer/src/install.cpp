// PSXS5 standalone release installer.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "install.hpp"
#include "config.h"
extern "C" {
#include "github.h"
#include "json.h"
#include "log.h"
#include "plat.h"
#include "tls.h"
#include "util.h"
}
#include "../../examples/self-update-helper/archive.hpp"
#include "../../examples/self-update-helper/files.hpp"
#include "../../examples/self-update-helper/updater.hpp"
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/file.h>
#if defined(__linux__)
#include <sys/vfs.h>
#else
#include <sys/mount.h>
#endif
#include <sys/stat.h>
#include <unistd.h>

namespace psxs5_installer {
namespace {
using self_update::Kind;

bool tag_parts(const std::string &tag, std::uint32_t parts[3]) {
    if (tag.size() < 6 || tag.size() > 32 || tag[0] != 'v')
        return false;
    int part = 0;
    bool digit = false;
    parts[0] = parts[1] = parts[2] = 0;
    for (std::size_t i = 1; i < tag.size(); ++i) {
        const char c = tag[i];
        if (c >= '0' && c <= '9') {
            const std::uint32_t value = static_cast<std::uint32_t>(c - '0');
            if (parts[part] > (UINT32_MAX - value) / 10)
                return false;
            parts[part] = parts[part] * 10 + value;
            digit = true;
        } else if (c == '.' && digit && part < 2) {
            ++part;
            digit = false;
        } else {
            return false;
        }
    }
    return part == 2 && digit;
}

bool read_version(const std::string &path, std::string &tag) {
    std::string data;
    if (!self_update::read_small(path, 128, data))
        return false;
    while (!data.empty() && (data.back() == '\n' || data.back() == '\r'))
        data.pop_back();
    if (!valid_tag(data))
        return false;
    tag = data;
    return true;
}

bool param_title(const std::string &folder) {
    std::string text;
    if (!self_update::read_small(folder + "/sce_sys/param.json", 64 * 1024, text))
        return false;
    js_node *root = js_parse(text.data(), text.size());
    if (!root)
        return false;
    const char *title = js_str(js_get(root, "titleId"));
    const bool valid = title && std::strcmp(title, TITLE_ID) == 0;
    js_free(root);
    return valid;
}

bool complete_app(const std::string &folder) {
    return self_update::kind(folder + "/eboot.bin") == Kind::file &&
           self_update::kind(folder + "/sce_sys/param.json") == Kind::file &&
           param_title(folder);
}

bool set_permissions(const std::string &path, unsigned depth = 0) {
    if (depth > 32)
        return false;
    const Kind what = self_update::kind(path);
    if (what != Kind::file && what != Kind::directory)
        return false;
    if (chmod(path.c_str(), 0777) != 0)
        return false;
    if (what == Kind::file)
        return true;
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return false;
    bool ok = true;
    std::size_t count = 0;
    for (;;) {
        errno = 0;
        const dirent *entry = readdir(dir);
        if (!entry) {
            if (errno)
                ok = false;
            break;
        }
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0)
            continue;
        if (++count > self_update::kArchiveEntries ||
            !set_permissions(path + "/" + entry->d_name, depth + 1)) {
            ok = false;
            break;
        }
    }
    closedir(dir);
    return ok;
}

bool has_space(const std::string &path, std::uint64_t needed) {
    struct statfs stats {};
    if (statfs(path.c_str(), &stats) != 0)
        return false;
    return static_cast<std::uint64_t>(stats.f_bavail) *
               static_cast<std::uint64_t>(stats.f_bsize) >= needed;
}

bool mounted_here(const Paths &p) {
    const std::string record = std::string(plat_root()) + "/user/app/" TITLE_ID;
    if (self_update::kind(record + "/mount_img.lnk") != Kind::absent)
        return false;
    if (self_update::kind(record + "/mount.lnk") == Kind::absent)
        return true;
    std::string path;
    if (!self_update::read_small(record + "/mount.lnk", 1024, path))
        return false;
    while (!path.empty() && (path.back() == '\n' || path.back() == '\r' ||
                             path.back() == ' ' || path.back() == '/' ||
                             path.back() == '\0'))
        path.pop_back();
    return path == p.app;
}

bool lock(const Paths &paths, int &fd) {
    const std::string path = paths.work + "/install.lock";
    fd = open(path.c_str(), O_CREAT | O_RDWR | O_NOFOLLOW, 0666);
    return fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) == 0;
}

void unlock(int fd) {
    if (fd >= 0) {
        (void)flock(fd, LOCK_UN);
        close(fd);
    }
}
} // namespace

Paths default_paths() {
    const std::string prefix = plat_root();
    Paths p;
    p.data = prefix + "/data";
    p.homebrew = p.data + "/homebrew";
    p.app = p.homebrew + "/" TITLE_ID;
    p.work = p.data + "/PSX1-Installer";
    p.archive = p.work + "/release.zip";
    p.staged = p.work + "/staged";
    p.backup = p.work + "/backup";
    p.sandboxes = prefix + "/mnt/sandbox";
    return p;
}

bool valid_tag(const std::string &tag) {
    std::uint32_t parts[3];
    return tag_parts(tag, parts);
}

bool newer_than(const std::string &available, const std::string &installed) {
    if (!valid_tag(available))
        return false;
    if (!valid_tag(installed))
        return true; // An older/manual install has no trusted version marker.
    std::uint32_t a[3], b[3];
    if (!tag_parts(available, a) || !tag_parts(installed, b))
        return false;
    for (int i = 0; i < 3; ++i)
        if (a[i] != b[i])
            return a[i] > b[i];
    return false;
}

bool prepare(const Paths &p, std::string &error) {
    if (self_update::kind(p.data) != Kind::directory ||
        !self_update::make_directory(p.homebrew) ||
        !self_update::make_directory(p.work)) {
        error = "Cannot prepare /data/homebrew or the installer work directory";
        return false;
    }
    if (self_update::kind(p.app) == Kind::other ||
        self_update::kind(p.work) != Kind::directory) {
        error = "An installer or app path is not a real directory";
        return false;
    }
    return true;
}

bool recover(const Paths &p, std::string &error) {
    const Kind backup = self_update::kind(p.backup);
    const Kind target = self_update::kind(p.app);
    if (backup == Kind::absent)
        return true;
    if (backup != Kind::directory) {
        error = "The previous backup path is not a real directory";
        return false;
    }
    if (target == Kind::absent) {
        if (!complete_app(p.backup)) {
            error = "The previous backup is incomplete; it was left untouched";
            return false;
        }
        if (rename(p.backup.c_str(), p.app.c_str()) != 0) {
            error = "Cannot restore the previous app after an interrupted install";
            return false;
        }
        (void)self_update::sync_directory(p.homebrew);
        return true;
    }
    if (target != Kind::directory || !complete_app(p.app)) {
        error = "The app and its backup need manual inspection; neither was removed";
        return false;
    }
    // A previous cleanup may have stopped halfway through deleting backup.
    // The complete new app is already in place, so the partial backup is disposable.
    if (!self_update::remove_tree(p.backup)) {
        error = "Cannot clear the backup from the previous install";
        return false;
    }
    return true;
}

bool installed_tag(const Paths &p, std::string &tag) {
    return read_version(p.app + "/.psxs5-version", tag);
}

bool install_archive(const Paths &p, const std::string &tag, std::string &error) {
    if (!valid_tag(tag)) {
        error = "The release tag is not a stable version";
        return false;
    }
    if (self_update::title_running(p.sandboxes, TITLE_ID) != 0) {
        error = "The app is running, or its state cannot be checked";
        return false;
    }
    if (!self_update::remove_tree(p.staged)) {
        error = "Cannot clear the previous staging directory";
        return false;
    }
    self_update::ArchiveInfo info;
    if (!self_update::inspect_archive(p.archive, TITLE_ID, info, error))
        return false;
    if (!has_space(p.work, info.unpacked + (64ULL << 20))) {
        error = "Not enough free space to unpack the release";
        return false;
    }
    std::atomic<bool> cancelled{false};
    std::atomic<std::uint64_t> written{0};
    if (!self_update::extract_archive(p.archive, TITLE_ID, p.staged, cancelled, written,
                                      error)) {
        (void)self_update::remove_tree(p.staged);
        return false;
    }
    std::string packaged_tag;
    if (!complete_app(p.staged) ||
        !read_version(p.staged + "/.psxs5-version", packaged_tag) ||
        packaged_tag != tag) {
        error = "The ZIP app identity or version does not match the release";
        (void)self_update::remove_tree(p.staged);
        return false;
    }
    if (!set_permissions(p.staged)) {
        error = "Cannot set the permissions needed to start the app";
        (void)self_update::remove_tree(p.staged);
        return false;
    }
    // Re-check after extraction; the app must be closed throughout the replacement.
    if (self_update::title_running(p.sandboxes, TITLE_ID) != 0) {
        error = "The app started while the update was prepared";
        (void)self_update::remove_tree(p.staged);
        return false;
    }
    if (self_update::kind(p.app) == Kind::directory) {
        if (rename(p.app.c_str(), p.backup.c_str()) != 0) {
            error = "Cannot move the previous app to the backup";
            return false;
        }
        (void)self_update::sync_directory(p.homebrew);
    } else if (self_update::kind(p.app) != Kind::absent) {
        error = "The app path is not a real directory";
        return false;
    }
    if (rename(p.staged.c_str(), p.app.c_str()) != 0) {
        if (self_update::kind(p.backup) == Kind::directory)
            (void)rename(p.backup.c_str(), p.app.c_str());
        error = "Cannot place the new app; the previous app was restored if possible";
        return false;
    }
    (void)self_update::sync_directory(p.homebrew);
    if (self_update::kind(p.backup) == Kind::directory &&
        !self_update::remove_tree(p.backup)) {
        error = "The new app is installed, but its old backup could not be removed";
        return false;
    }
    return true;
}

int run() {
    plat_init();
    const Paths p = default_paths();
    std::string error;
    if (!prepare(p, error)) {
        notify("PSX1 Installer: %s", error.c_str());
        return 1;
    }
    int lock_fd = -1;
    if (!lock(p, lock_fd)) {
        if (lock_fd >= 0)
            close(lock_fd);
        notify("PSX1 Installer: another install is already running");
        return 1;
    }
    // Recovery is repeated under the lock in case another copy finished first.
    if (!recover(p, error)) {
        notify("PSX1 Installer: %s", error.c_str());
        unlock(lock_fd);
        return 1;
    }
    if (!mounted_here(p)) {
        notify("PSX1 Installer: the mounted app is not the /data folder install");
        unlock(lock_fd);
        return 1;
    }
    if (tls_global_init() != 0) {
        notify("PSX1 Installer: HTTPS could not start");
        unlock(lock_fd);
        return 1;
    }
    gh_release release{};
    if (gh_latest(&release) != 0 || !valid_tag(release.tag)) {
        notify("PSX1 Installer: no usable release (%s)", err_get());
        tls_global_free();
        unlock(lock_fd);
        return 1;
    }
    std::string current;
    (void)installed_tag(p, current);
    if (!newer_than(release.tag, current)) {
        notify("PSX1 is already up to date (%s)", current.c_str());
        tls_global_free();
        unlock(lock_fd);
        return 0;
    }
    if (!has_space(p.work, static_cast<std::uint64_t>(release.asset_size) + (64ULL << 20))) {
        notify("PSX1 Installer: not enough free space for the download");
        tls_global_free();
        unlock(lock_fd);
        return 1;
    }
    notify("PSX1 Installer: downloading %s", release.tag);
    const int download_ok = gh_download(&release, p.archive.c_str(), nullptr, nullptr);
    tls_global_free();
    if (download_ok != 0) {
        notify("PSX1 Installer: %s", err_get());
        unlock(lock_fd);
        return 1;
    }
    notify("PSX1 Installer: installing %s", release.tag);
    const bool installed = install_archive(p, release.tag, error);
    (void)self_update::remove_tree(p.archive);
    if (!installed) {
        notify("PSX1 Installer: %s", error.c_str());
        unlock(lock_fd);
        return 1;
    }
    notify("PSX1 %s installed. Restart the app to use it.", release.tag);
    unlock(lock_fd);
    return 0;
}
}
