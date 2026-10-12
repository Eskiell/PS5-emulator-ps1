// Real ZIP and filesystem checks for the standalone installer.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../installer/src/install.hpp"
#include "../examples/self-update-helper/files.hpp"
#include "miniz/miniz.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>

static std::atomic<int> app_running{0};

extern "C" {
#include "../installer/src/github.h"
int gh_latest(gh_release *) { return -1; }
int gh_download(const gh_release *, const char *, gh_progress, void *) { return -1; }
int tls_global_init(void) { return -1; }
void tls_global_free(void) {}
const char *plat_root(void) { return ""; }
void plat_notify_text(const char *) {}
int plat_app_running(const char *) { return app_running.load(); }
void plat_init(void) {}
}

static int failures;
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #expr); ++failures; } } while (0)

static void write_file(const std::string &path, const std::string &text) {
    FILE *file = std::fopen(path.c_str(), "wb");
    CHECK(file != nullptr);
    if (file) {
        CHECK(std::fwrite(text.data(), 1, text.size(), file) == text.size());
        CHECK(std::fclose(file) == 0);
    }
}

static std::string read_file(const std::string &path) {
    std::string data;
    return self_update::read_small(path, 1 << 20, data) ? data : "<missing>";
}

static void make_zip(const std::string &path, const std::string &tag,
                     const std::string &program, bool malicious = false,
                     const std::string &marker = "") {
    mz_zip_archive zip{};
    CHECK(mz_zip_writer_init_heap(&zip, 0, 0));
    const std::string base = "PSXS5-" + tag + "/PPSA97510/";
    const std::string param = "{\"titleId\":\"PPSA97510\"}";
    CHECK(mz_zip_writer_add_mem(&zip, (base + "eboot.bin").c_str(),
                                program.data(), program.size(), MZ_DEFAULT_COMPRESSION));
    CHECK(mz_zip_writer_add_mem(&zip, (base + "sce_sys/param.json").c_str(),
                                param.data(), param.size(), MZ_DEFAULT_COMPRESSION));
    const std::string version = (marker.empty() ? tag : marker) + "\n";
    CHECK(mz_zip_writer_add_mem(&zip, (base + ".psxs5-version").c_str(),
                                version.data(), version.size(), MZ_DEFAULT_COMPRESSION));
    if (malicious)
        CHECK(mz_zip_writer_add_mem(&zip, (base + "../escape").c_str(), "bad", 3,
                                    MZ_DEFAULT_COMPRESSION));
    void *bytes = nullptr;
    size_t size = 0;
    CHECK(mz_zip_writer_finalize_heap_archive(&zip, &bytes, &size));
    FILE *file = std::fopen(path.c_str(), "wb");
    CHECK(file != nullptr);
    if (file) {
        CHECK(std::fwrite(bytes, 1, size, file) == size);
        CHECK(std::fclose(file) == 0);
    }
    mz_zip_writer_end(&zip);
    mz_free(bytes);
}

int main() {
    CHECK(psxs5_installer::valid_tag("v2.2.0"));
    CHECK(!psxs5_installer::valid_tag("v2.2.0-rc1"));
    CHECK(psxs5_installer::newer_than("v2.2.1", "v2.2.0"));
    CHECK(!psxs5_installer::newer_than("v2.2.0", "v2.2.1"));
    char pattern[] = "/tmp/psxs5-installer-test-XXXXXX";
    char *root = mkdtemp(pattern);
    CHECK(root != nullptr);
    if (!root)
        return 1;
    const std::string base = root;
    psxs5_installer::Paths p;
    p.data = base + "/data";
    p.homebrew = p.data + "/homebrew";
    p.app = p.homebrew + "/PPSA97510";
    p.work = p.data + "/PSXS5-Installer";
    p.archive = p.work + "/release.zip";
    p.staged = p.work + "/staged";
    p.backup = p.work + "/backup";
    CHECK(mkdir(p.data.c_str(), 0755) == 0);
    CHECK(mkdir((base + "/mnt").c_str(), 0755) == 0);
    CHECK(mkdir((base + "/mnt/sandbox").c_str(), 0755) == 0);
    // A leftover sandbox directory must not be mistaken for a live process.
    CHECK(mkdir((base + "/mnt/sandbox/PPSA97510_000").c_str(), 0755) == 0);
    CHECK(mkdir((p.data + "/PSXS5").c_str(), 0755) == 0);
    CHECK(mkdir((p.data + "/PSXS5/games").c_str(), 0755) == 0);
    write_file(p.data + "/PSXS5/games/user-game.bin", "personal data");
    std::string error;
    CHECK(psxs5_installer::prepare(p, error));

    make_zip(p.archive, "v2.2.0", "first program");
    CHECK(psxs5_installer::install_archive(p, "v2.2.0", error));
    CHECK(read_file(p.app + "/eboot.bin") == "first program");
    CHECK(read_file(p.app + "/.psxs5-version") == "v2.2.0\n");

    // Refuse a ZIP that could escape the app directory.
    make_zip(p.archive, "v2.2.1", "bad program", true);
    CHECK(!psxs5_installer::install_archive(p, "v2.2.1", error));
    CHECK(read_file(p.app + "/eboot.bin") == "first program");
    CHECK(self_update::kind(p.data + "/escape") == self_update::Kind::absent);

    // A release cannot claim a different version inside its app folder.
    make_zip(p.archive, "v2.2.1", "wrong version", false, "v9.0.0");
    CHECK(!psxs5_installer::install_archive(p, "v2.2.1", error));
    CHECK(read_file(p.app + "/eboot.bin") == "first program");

    // A running app cannot be replaced.
    make_zip(p.archive, "v2.2.1", "second program");
    app_running = 1;
    CHECK(!psxs5_installer::wait_for_app_exit(0, error));
    CHECK(error.find("still open") != std::string::npos);
    CHECK(!psxs5_installer::install_archive(p, "v2.2.1", error));
    CHECK(read_file(p.app + "/eboot.bin") == "first program");
    std::thread close_app([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        app_running = 0;
    });
    CHECK(psxs5_installer::wait_for_app_exit(2, error));
    close_app.join();
    app_running = -1;
    CHECK(!psxs5_installer::wait_for_app_exit(0, error));
    CHECK(error.find("Cannot check") != std::string::npos);
    app_running = 0;

    CHECK(psxs5_installer::install_archive(p, "v2.2.1", error));
    CHECK(read_file(p.app + "/eboot.bin") == "second program");
    CHECK(read_file(p.data + "/PSXS5/games/user-game.bin") == "personal data");
    CHECK(self_update::kind(p.backup) == self_update::Kind::absent);

    // A power cut while deleting the old backup leaves a partial directory.
    CHECK(mkdir(p.backup.c_str(), 0755) == 0);
    write_file(p.backup + "/partial", "old");
    CHECK(psxs5_installer::recover(p, error));
    CHECK(self_update::kind(p.backup) == self_update::Kind::absent);
    CHECK(read_file(p.app + "/eboot.bin") == "second program");

    // Simulate power loss after the old app was moved to backup.
    CHECK(rename(p.app.c_str(), p.backup.c_str()) == 0);
    CHECK(psxs5_installer::recover(p, error));
    CHECK(read_file(p.app + "/eboot.bin") == "second program");
    CHECK(self_update::kind(p.backup) == self_update::Kind::absent);

    CHECK(self_update::remove_tree(base));
    if (failures)
        std::fprintf(stderr, "%d installer check(s) failed\n", failures);
    return failures ? 1 : 0;
}
