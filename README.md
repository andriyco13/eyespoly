<div align="center">
  <img src="photo_2026-07-26_16-36-20(2).png" alt="Eyespoly Logo" width="100%">
  <br>
  <h1>Eyespoly</h1>
  <p><b>Your smart companion for preserving eyesight during long computer sessions.</b></p>
  <p>🇺🇦 Доступно українською | 🇬🇧 Available in English</p>
</div>

## 👁️ About Eyespoly

**Eyespoly** is a lightweight, unobtrusive desktop application designed to protect your vision using the proven **20-20-20 rule**: every 20 minutes, look at something 20 feet away for 20 seconds. 

Whether you are studying, coding, or just browsing, Eyespoly ensures you give your eyes the rest they deserve.

### ✨ Key Features
* **Smart Idle Detection:** The timer automatically pauses when you step away from your keyboard and resumes when you return.
* **Guided Exercises:** Short, relaxing eye exercises are displayed during your breaks to help reduce eye strain.
* **Screen-time Statistics:** Keep track of your work sessions and breaks with a simple, built-in stats page.
* **System Integration:** Runs quietly in the system tray without cluttering your taskbar.
* **Customizable:** Adjust work and break intervals to fit your personal workflow.

---

## 🚀 Installation

Pre-built packages for **Windows**, **Flatpak**, **Fedora**, and **Arch Linux** are available on the [Releases](https://github.com/andriyco13/eyespoly/releases) page.

### 🪟 Windows (`.zip`)
To install Eyespoly on Windows, you need to extract eyespoly_win.zip to a secure location and run eyespoly.exe

### 📦 Flatpak (Recommended)
Flatpak is the easiest and most reliable way to install Eyespoly across any Linux distribution.
```bash
flatpak install ./eyespoly.flatpak
```

### 🎩 Fedora (`.rpm`)
```bash
sudo dnf install ./eyespoly-*.x86_64.rpm
```

### 🦅 Arch Linux (`.pkg.tar.zst`)
```bash
sudo pacman -U eyespoly-*-x86_64.pkg.tar.zst
```

---

## 🛠️ Building from Source

If you prefer to compile Eyespoly yourself, ensure you have **CMake ≥ 3.16** and **Qt 6** (`Core`, `Gui`, `Quick`, `Qml`, `DBus`, `Multimedia`, `Sql`, `LinguistTools`) installed on your system.

```bash
git clone [https://github.com/andriyco13/eyespoly.git](https://github.com/andriyco13/eyespoly.git)
cd eyespoly
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```
For detailed release instructions and packaging guidelines, please refer to [RELEASING.md](RELEASING.md).

---

## 📄 License & Credits

* **Developer:** Andrii Bodnar ([andriyco13](https://github.com/andriyco13))
* **Logo Design:** minzuxx
* **License:** Distributed under the [MIT License](LICENSE).
