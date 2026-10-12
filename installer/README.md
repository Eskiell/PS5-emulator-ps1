# PSX1 Installer

`PSX1Installer.elf` is a standalone payload for the PS5 ELF loader. Send it
again whenever you want to check for a newer stable release. It exits after the
check or installation; it does not stay running, collect reports or upload logs.

On each run it:

1. Checks the latest stable release of
   [Eskiell/PS5-emulator-ps1](https://github.com/Eskiell/PS5-emulator-ps1/releases)
   and reads the `PSXS5-vX.Y.Z.zip` asset's size and SHA-256 from GitHub.
2. Compares the tag with `/data/homebrew/PPSA97510/.psxs5-version`. An
   installation without that marker is treated as an older manual install.
3. Downloads the ZIP over verified HTTPS, with a size limit and SHA-256 check.
4. Validates the archive's paths, file types, title ID and version, then
   extracts it under `/data/PSX1-Installer/staged`.
5. When the app is closed, moves the old app to a backup and puts the staged
   app in `/data/homebrew/PPSA97510`. If power fails between those moves,
   the next run restores the old app.

The installer only replaces the app folder. Games, BIOS, saves and settings
under `/data/PSXS5` are untouched. It writes its own local diagnostic log to
`/data/PSX1-Installer/installer.log` and shows PS5 notifications.

The current installer targets the internal `/data/homebrew/PPSA97510` app
folder. If ShadowMountPlus has mounted the title from an external drive or an
image, it refuses to install into the wrong copy. A restart of
ShadowMountPlus or the console may be needed after a first installation.

## Build

Run `make installer` at the repository root. The command fetches pinned
Mbed TLS and PS5 SDK archives, verifies their SHA-256, builds the payload and
checks its ELF framing. The output is
`build/installer/PSX1Installer.elf`. Run `make test-installer` for host
tests of installation, version comparison, malicious ZIP rejection and
interrupted-install recovery.

Tagged releases build the app ZIP and the installer ELF together. The tag must
match `PSXS5_VERSION` in `src/psxs5.h`; the release workflow writes the same
tag into the app's `.psxs5-version` marker. No installer run can download a
release until the fork has published its first stable release.

The HTTPS, GitHub API and PS5 notification code is adapted from the
[PS5SX2 installer](https://github.com/Swordpdf/PS5SX2/tree/main/ps5/installer)
under GPL-3.0. The ZIP validator and extractor already in this repository
come from the self-update example.
