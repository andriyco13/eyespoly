# Eyespoly

Eyespoly reminds you to take regular breaks to protect your eyesight
while working at the computer, following the well-known 20-20-20
method. It pauses automatically when you step away from the keyboard
(idle detection), shows short guided eye-relaxation exercises during
each break, and keeps simple screen-time statistics.

Доступно українською та англійською.

## Встановлення

Готові пакети для Debian/Ubuntu (`.deb`), Fedora (`.rpm`) та Arch
Linux (`.pkg.tar.zst`) публікуються на сторінці
[Releases](https://github.com/andriyco13/eyespoly/releases).

```bash
# Debian / Ubuntu
sudo apt install ./eyespoly_0.1.0-1_amd64.deb

# Fedora
sudo dnf install ./eyespoly-0.1.0-1.fc*.x86_64.rpm

# Arch Linux
sudo pacman -U eyespoly-0.1.0-1-x86_64.pkg.tar.zst
```

## Збірка з джерел

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

Потрібні: Qt 6 (Core, Gui, Quick, Qml, DBus, Multimedia, Sql,
LinguistTools) та CMake ≥ 3.16.

Інструкції з релізу та збірки пакетів — у [RELEASING.md](RELEASING.md).

## Ліцензія

MIT, див. [LICENSE](LICENSE).
