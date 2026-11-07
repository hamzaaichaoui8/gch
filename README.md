# gch — PS4 Tools & GoldHEN Versions

مجموعة أدوات PS4 + إدارة إصدارات GoldHEN، من تعديل وتجميع **Hamza Aichaoui**.

## ماذا يوجد هنا؟

| المجلد/الملف | الوظيفة |
|--------------|---------|
| `GoldHEN_v*.7z` | إصدارات GoldHEN — تُضاف يدويًا حسب التوفر (من SiSTR0) |
| `goldhen_checker/` | بايلود ELF (C/OpenORBIS): يعرض إصدار GoldHEN المثبّت على شاشة الكونسول ويقارنه بأحدث إصدار — يُبنى بـ Docker |
| `extract_windows.py` | أداة استخراج/تعديل `download0.dat` (تحرير Sage الأصلي — تعديلات Hamza) |
| `ps4_ftp_patch.py` | نشر تلقائي عبر FTP: نسخ احتياطي من `download0.dat` ثم رفع النسخة المعدّلة |
| `run_tool.bat` | قائمة تشغيل الأداة على Windows |

> **على آلية التحقق في البايلود:** يفحص المسجَّل والمقاتل من `goldhen.bin` وباختصار: يُشعار الشاشة فقط إذا **اختلفت** قيمة `latest.txt` عن المثبّت — دون قراءة أي ميزات (بسيط وموثوق). سجل Changelog الكامل أدناه للاستفسار اليدوي.

## الاستخدام السريع

### 1) تحديث GoldHEN على الكونسول (يدوي)
1. نزّل `GoldHEN_vXXX.7z` المحجوز هنا (أو أضف إصدارًا جديدًا من مولد الس STR0).
2. فك الضغط → خذ `goldhen.bin` → رفعه عبر FTP على المسار `/data/GoldHEN/payloads/goldhen.bin` (اعتني بنسخة القديم أولًا).
   - عبر سوقنا: `python ps4_ftp_patch.py` — مبدئيًا نسخة الأداة موجهة للـ `download0.dat`؛ يمكن استخدام FileZilla يدويًا.
3. أعد التشغيل — إشعار «GoldHEN vX.Y.Z» يظهر عند الإقلاع.

### 2) تفعيل بايلود الفحص (بعد البناء)
1. انسخ `goldhen_checker.elf` المُنتَج إلى `/data/payloads/` (يفعّل تلقائيًا مع إقلاع GoldHEN).
2. ضع في `/data/goldhen_checker/latest.txt` سطرًا واحدًا: `2.4b18.13` (أو آخر إصدار — عدّله يدويًا بعد أي تحديث جديد).
3. عند الإقلاع تحصل على إشعار: «GoldHEN ... up to date» أو «... update available».

### 3) بناء البايلود (يتطلب Docker + WSL2)
```bat
docker run --rm -v %cd%\goldhen_checker:/code ghcr.io/openorbis/orbis-elf-builder:latest cmake -B /code/build -S /code
docker run --rm -v %cd%\goldhen_checker:/code ghcr.io/openorbis/orbis-elf-builder:latest cmake --build /code/build
```
الناتج: `goldhen_checker/build/goldhen_checker.elf`

### 4) أداة download0.dat (PS5 download0 على PS4-مستهدف)
- تدعم نظام 5.00–13.04 فقط (كما ورد في سكربت الأداة الأصلي)؛ التحديثات 14.00+ لم تُستهدف بعد.
- `run_tool.bat` → خيار 1 (استخراج) / 2 (تعديل وتحزيم) ثم `python ps4_ftp_patch.py` للنشر عبر FTP.

## دعم الأنظمة — خريطة بقية الإصدارات

| GoldHEN | أحدث نظام مدعوم |
|---------|-----------------|
| v2.4b18.13 | **14.00** |
| v2.4b18.12 | 13.02 / 13.04 / 13.50 |
| v2.4b18.11 | 13.52 |
| v2.4b18.10 | 13.02 (جهزة PayLoader) |
| v2.4b18.9  | 13.00 |
| v2.4b18.7  | 12.50 / 12.52 |
| v2.4b18.5  | 11.5x |
| v2.3–b18.4 | 5.05 → 12.02 (تفاصيل بالأسفل) |

# Changelog (GoldHEN official)

## v2.4b18.13
- Added support for 14.00
- Fixed FTP SIZE issue (https://github.com/GoldHEN/GoldHEN/issues/401)

## v2.4b18.12
- Added support for 13.02, 13.04 and 13.50

## v2.4b18.11
- Added support for 13.52

## v2.4b18.10
- Added PayLoader LaunchPad and AutoRun support
- Fixed sceUserServiceInitialize failure on ELF payloads

## v2.4b18.9
- Added support for 13.00
- Fixed MC4 cheat parser
- Fixed Linux kexec support
- Fixed Orbis Toolbox support

## v2.4b18.8
- Added Game Patch Menu and Downloader (from illusion repository)
- Improved FTP speed
- Improved ELF support in PayLoader server (ex BinLoader)
- Fixed Remote Play enabler for 12.02

## v2.4b18.7
- Added support for 12.50 and 12.52
- Added support for Testkit (same fw as retail)

## v2.4b18.6
- Added BD-App AutoKill and Disc AutoEject
- Added Plugins Downloader (Thanks to [ctn](https://github.com/ctn123))
- Added AIO fix plugin support (Thanks to [jocover](https://github.com/jocover))
- Updated GoldHEN top supporters page (Thanks to all)

## v2.4b18.5
- Added support for 7.0x, 7.5x, 8.0x, 8.5x, 9.0x, 9.5x, 11.0x, 11.5x
- Added Game Stats support
- Added Date/Time auto update
- Improved BinLoader server (ELF support)

## v2.4b18.4
- Added support for 12.00 and 12.02

## v2.4b18.3
- Added Auto-Apply Cheats on game start
- Added support for 9.03
- Improved Klog server

## v2.4b18.2
- Added PPPoE Patch to avoid multiple exploitations
- Package Scanning Improvements:
  Added support for excluding folders during package scanning. Folders starting with a dot (e.g., ".test") and those containing a ".ignore" file will be automatically skipped.

## v2.4b18
- Added Cheat Downloader
- Added support for 10.50, 10.70 and 10.71
- Improved rest mode support
- Fixed network servers configuration save
- Moved Klog server to userland (better network disconnection supports)

## v2.4b17.3
- Added support for 9.60
- Improved BinLoader server (more payloads supported)
- Improved Klog server (network disconnection supported)
- Changed network servers configuration to declarative

## v2.4b17.2
- Added support for 10.00 / 10.01

## v2.4b17
- Added support for kernel loaders
- Added support for 11.00
- Added syscall 11

## v2.4b16.2
- Improved SysInfo stats
- Fixed IP Address setting

## v2.4b16
- Fixed IP Address overlay position bug

## v2.4b15
- Improved plugin loader (Thanks to [illusion0001](https://github.com/illusion0001))
- Improved FTP (Thanks to [hippie68](https://github.com/hippie68))
- Added game patch plugin (Thanks to [illusion0001](https://github.com/illusion0001))
- Added game overlay SysInfo (Thanks to [OSM](https://github.com/OSM-Made))
- Added GoldHEN top supporters page in about section (Thanks to all)

## v2.4b14
- Improved plugin loader (Thanks to [illusion0001](https://github.com/illusion0001))
- Improved FTP (Thanks to [hippie68](https://github.com/hippie68))
- Improved MC4 support (Thanks to [ctn](https://github.com/ctn123))
- Added cheat description support (Thanks to [ctn](https://github.com/ctn123))
- Added background package installation support
- Added package info on package installation support
- Fixed Orbis Toolbox support

## v2.4b13
- Improved xml/shn parser
- Skipped /disc and $RECYCLE.BIN in pkg subdirectories search

## v2.4b12
- Fixed USB Subdirectories path

## v2.4b11
- Fixed "Search Subdirectories" feature
- Improved "Package Source" feature

## v2.4b10
- Improved FTP self decryption
- Fixed cheat navigation after rest mode
- Added "Search Subdirectories" and "Show Package Path" features in Debug Settings

## v2.4b5
- Added FTP Server v2.1 (Thanks to [hippie68](https://github.com/hippie68))
- Added multi-cheats support
- Added multi-firmware support [5.05 / 6.72 / 9.00]
- Added package installer source settings

## v2.3
- Added [Plugins](https://github.com/GoldHEN/GoldHEN_Plugins_Repository) support
- Added FPS counter
- Added TitleId label feature
- Added MC4 cheat format support
- Added Scanlines overlay
- Added internal pkg installation support (/data/pkg) (Thanks to [OSM](https://github.com/OSM-Made))

## v2.2.4
- Fixed KLog tty redirect

## v2.2.3
- Added debug settings
- Added cheat settings
- Added KLog settings
- Added GoldHEN shortcut
- Added PS2 cheat support
- Added Southbridge info
- Improved app version detection
- Refactored GoldHEN menu

## v2.2.2
- Added cheat absolute offset support
- Fixed close option bug
- Fixed Orbis Toolbox support bug

## v2.2.1
- Improved Cheat Menu stability

## v2.2
- Added Cheat Menu initial version
- Added tty redirect to klog

## v2.1.2
- Fixed config parser

## v2.1.1
- Fixed multiple injections

## v2.1
- Added config file (/data/GoldHEN/config.ini)
- Added Klog server on 3232 port
- Improved rest mode support

## v2.0b2
- Improved BinLoader server (still beta version)

## v2.0b
- Added BinLoader server (beta version)
- Added UI menu
- Improved stability and FTP server
- Fixed trophy timestamps

## v1.1
- Improved stability and FTP server

## v1.0
- Public release

# Known issue

## v2.4b17
- klog server: After the pppwn, the klog server is not reachable immediately. A rest mode resume is needed to make it work.
