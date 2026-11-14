# gch - Auto Update for GoldHEN

Made by **Hamza Aichaoui**.

This repo keeps the latest GoldHEN file for my console payload `goldhen_checker`.

## How it works

When the console starts, the payload reads `latest.txt` from this repo and compares it with the installed version.

- **Up to date:** you see `You are on the latest GoldHEN version (x.y.z) - by hamza`.
- **New version found:** the payload downloads `goldhen.bin`, saves a backup (`goldhen.bin.bak`), replaces the file, and shows `RESTART console to activate it - by hamza`.

## Supported firmware

| GoldHEN version | Supported PS4 firmware |
| --- | --- |
| **v2.4b18.14** | **14.00 / 14.02** |
| v2.4b18.13 | 14.00 |
| v2.4b18.12 | 13.02 / 13.04 / 13.50 |
| v2.4b18.11 | 13.52 |
| v2.4b18.10 | 13.02 |
| v2.4b18.9 | 13.00 |
| v2.4b18.7 | 12.50 / 12.52 |
| v2.4b18.5 | 11.5x |
| v2.4b18.4 | 11.0x |
| v2.4b18.1 | 11.00 |
| v2.4b17.x | 5.05 / 6.72 / 9.00 / 11.00 |
| v2.4b16 and older | 5.05 / 6.72 / 9.00 / 10.00 / 10.01 |
| v2.0 - v2.3 | 5.05 / 6.72 / 9.00 |
| v1.x | 5.05 / 6.72 |

Release notes: [SiSTR0 posts on Ko-fi](https://ko-fi.com/sistro/posts)

## PS4 homebrew downloads

- [Apollo Save Tool PKG](https://github.com/hamzaaichaoui8/gch/releases/download/v1.0-homebrew/IV0000-APOL00004_00-APOLLO0000000PS4.pkg)
- [PS4-Xplorer 2.04 PKG](https://github.com/hamzaaichaoui8/gch/releases/download/v1.0-homebrew/PS4_LAPY20009_v2.04.pkg)
- [PS4-Xplorer 2.07 PKG (60 MB)](https://github.com/hamzaaichaoui8/gch/blob/main/PS4_LAPY20009_v2.07.pkg)
