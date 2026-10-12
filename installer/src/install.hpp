// PSXS5 standalone release installer.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>

namespace psxs5_installer {
struct Paths {
    std::string data;
    std::string homebrew;
    std::string app;
    std::string work;
    std::string archive;
    std::string staged;
    std::string backup;
    std::string sandboxes;
};

Paths default_paths();
bool valid_tag(const std::string &tag);
bool newer_than(const std::string &available, const std::string &installed);
bool prepare(const Paths &paths, std::string &error);
bool recover(const Paths &paths, std::string &error);
bool installed_tag(const Paths &paths, std::string &tag);
bool install_archive(const Paths &paths, const std::string &tag, std::string &error);
int run();
}
