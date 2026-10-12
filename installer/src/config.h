/* PSXS5 standalone installer: repository identity and network limits. */
#pragma once

#define INSTALLER_UA "PSXS5-Installer/1"
#define GH_REPO "Eskiell/PS5-emulator-ps1"
#define GH_API_URL "https://api.github.com/repos/" GH_REPO "/releases/latest"
#define GH_DOWNLOAD_PREFIX "https://github.com/" GH_REPO "/releases/download/"
#define TITLE_ID "PPSA97510"
#define MAX_ZIP_BYTES (2ULL * 1024ULL * 1024ULL * 1024ULL)
