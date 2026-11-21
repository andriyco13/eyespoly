# AGENTS.md — Інструкції та архітектурні правила проєкту Eyespoly

Ти працюєш у репозиторії **Eyespoly** — кросплатформного застосунку для захисту зору (правило 20-20-20), відстеження екранного часу та контролю втоми очей.

## 1. Обов'язковий старт сесії та економія токенів
- **Перший крок:** Перед будь-якими діями прочитай `@GRAPH.xml` та `@ARCHITECTURE_MAP.md` (якщо вони є в корені). Не скануй увесь репозиторій і не читай файли, які не стосуються поточного завдання (особливо `apptracker.cpp`, `StatsPage.qml`, `eyespoly_en.ts`, якщо завдання їх не зачіпає).
- **Перевірка збірки:** Для перевірки компіляції використовуй ТІЛЬКИ `cmake --build build` (або `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`).
- **ЗАБОРОНЕНО:** Ніколи не викликай `g++` або `clang++` вручну для компіляції окремих `.cpp` файлів — ти пропустиш модулі Qt (`Qt6Sql`, `Qt6DBus`, `Qt6Quick`, `LayerShellQt`) і витратиш токени на фейкові помилки.

---

## 2. Поточна архітектура та стек (Desktop: Linux + Windows)
- **Мова та фреймворк:** C++17 + Qt 6 (`Core`, `Gui`, `Quick`, `Qml`, `Multimedia`, `Sql`, `Widgets`, `Network`, `LinguistTools`; `DBus` та опціональний `LayerShellQt` — лише на Linux).
- **Точка входу (`main.cpp`):**
  - Використовує `QApplication` (потрібен для `QSystemTrayIcon` та `QWidget`-оверлеїв) + `QQmlApplicationEngine`.
  - Встановлює `QQuickStyle::setStyle("Basic")` (критично для Windows, щоб уникнути білих квадратів замість контролів).
  - Передає C++ сервіси в QML через `setContextProperty`: `langManager`, `autostartManager`, `idleMonitor`, `appTracker`, `updateChecker`, `nightLightManager`, `trayIcon`.
- **UI (`main.qml`, `BreakOverlay.qml`, `StatsPage.qml`):**
  - Налаштування зберігаються локально через `QtCore` `Settings { id: appSettings; category: "timer" }`.
  - Усі пункти в `settingsCol` (`main.qml`) оформлюються через внутрішній компонент `CenteredSetting` із розділювачами `Rectangle { width: 150; height: 1 ... }`.
  - Підтримується локалізація через `qsTr(...)` та `LanguageManager` (основна мова в коді — українська, переклад у `eyespoly_en.ts`).
- **Локальна БД (`apptracker.cpp`):**
  - SQLite (`app_usage.db` у `QStandardPaths::AppDataLocation`), таблиця `app_sessions(id, app_name, window_title, start_time, end_time, duration_seconds, date)`.
  - Має захист від сплячого режиму (якщо інтервал між тіками > 30 с, сесія розривається).

---

## 3. Жорсткі правила кросплатформності (Чого НЕ ДАЙ БОГ не робити)

1. **Не ламай збірку під Windows (`WIN32` / MinGW / MSVC):**
   - Увесь Linux-специфічний код (DBus, `xdotool`, `LayerShellQt`, `.desktop` файли) **завжди** обгортай у `#if defined(Q_OS_LINUX)` (а для `LayerShellQt` — додатково `#if defined(HAVE_LAYER_SHELL_QT)`).
   - Увесь WinAPI-код (`windows.h`, `psapi.h`, реєстр) обгортай у `#if defined(Q_OS_WIN)`.
   - **Ніколи** не викликай `qputenv("QT_QPA_PLATFORM", "wayland")` у `main.cpp` — це вб'є запуск на Windows та X11. Перевіряй платформу в рантаймі через `QGuiApplication::platformName() == QStringLiteral("wayland")`.
2. **Не використовуй приватні заголовки Qt (`Qt6::GuiPrivate` тощо):**
   - Вони ламають CI/CD у `.github/workflows/release.yml` (Flatpak, Fedora RPM, Arch PKGBUILD) та локальні збірки при оновленні версії Qt.
3. **Не змінюй `QQuickStyle::setStyle("Basic")` у `main.cpp`:**
   - На Windows без цього стилю ламаються кастомні `SpinBox`, `Switch` та `ComboBox` у `main.qml`.
4. **Не руйнуй логіку системного трею та закриття вікна:**
   - `app.setQuitOnLastWindowClosed(false)` та перехоплення `onClosing` у `main.qml` (`closeEvent.accepted = false; mainWindow.hide()`) обов'язкові, щоб програма жила у треї.
   - У `main.cpp` є watchdog на `aboutToQuit` (примусовий `std::_Exit(0)` через 1000 мс), щоб фонові потоки не підвішували процес при виході.
5. **Wayland Overlay (`NightLightManager` / `BreakOverlay`):**
   - На Wayland (`cosmic-comp`, GNOME, KDE) для click-through оверлеїв обов'язково поєднувати `LayerShellQt::Window` (`LayerOverlay`, `setExclusiveZone(-1)`, `KeyboardInteractivityNone`) + `Qt::WindowTransparentForInput` + `Qt::WA_TransparentForMouseEvents` + `setMask(QRegion())` у `showEvent` та `resizeEvent`.

---

## 4. Архітектурний вектор на майбутнє (Акаунти, Синхронізація, Мобільна версія)

Проєкт готується до розширення: **акаунти користувачів, хмарна синхронізація між ПК та смартфоном (Android/iOS), мобільний клієнт**. Будь-який новий код у трекінгу часу, БД та налаштуваннях має враховувати ці вимоги вже зараз:

### А. Бекенд та Акаунти (Supabase / REST + Auth)
- **Ізоляція мережевого шару:** Уся робота з API, авторизацією (JWT / OAuth / Email) та синхронізацією має бути винесена в окремі C++ класи (наприклад, `AuthManager`, `SyncManager` на базі `QNetworkAccessManager`, за аналогією з `updatechecker.cpp`), а не розмазана по QML.
- **Offline-First (Локальний пріоритет):** Застосунок **зобов'язаний** повноцінно працювати без інтернету та без акаунта. Локальна SQLite (`app_usage.db`) завжди є первинним джерелом правди (Source of Truth); хмара використовується для бекапу та об'єднання статистики між пристроями.
- **Безпека:** Ніколи не зберігати паролі у відкритому вигляді в `QSettings` чи SQLite. Зберігати лише `access_token` / `refresh_token`.

### Б. Схема даних та Синхронізація сесій (КРИТИЧНО для мультидевайсу)
При модифікації `AppTracker` або таблиці `app_sessions` дотримуйся таких правил:
1. **Ідентифікація пристрою та запису:**
   - Кожен пристрій повинен мати стабільний `device_id` (UUID, згенерований один раз при першому запуску і збережений у `QSettings`) та `platform` (`"linux"`, `"windows"`, `"android"`, `"ios"`).
   - Кожна сесія для синхронізації повинна мати глобально унікальний `uuid` (щоб уникнути дублікатів при повторній відправці пакета), `updated_at` (UTC timestamp) та прапорець `synced` (0/1).
2. **Дедуплікація та об'єднання інтервалів (Interval Merging):**
   - Користувач може одночасно працювати за ПК і дивитися в телефон. Загальний екранний час за день **не можна** рахувати простим додаванням `SUM(duration_seconds)` з усіх пристроїв, якщо інтервали `[start_time, end_time]` перетинаються!
   - Усі мітки часу (`start_time`, `end_time`) зберігати в **Unix Epoch UTC (секунди)**, а локальну дату (`date`) обчислювати з урахуванням часового поясу для відображення.
   - Логіка підрахунку загального часу з кількох пристроїв повинна підтримувати злиття перехресних часових відрізків (`merge overlapping intervals`).

### В. Підготовка до мобільної версії (Android / iOS)
- **Розділення бізнес-логіки та платформи:**
  - Ядро проєкту (таймер 20-20-20, математика статистики, синхронізація, моделі даних) не повинно залежати від десктопних API (`QSystemTrayIcon`, `QWidget`, `X11`, `WinAPI`, `LayerShellQt`).
  - Усе, що стосується вікон десктопу (`QWidget`), тримай ізольованим (як `NightLightManager`), щоб QML-шар та моделі даних можна було перевикористати або легко портувати на мобільний клієнт.

---

## 5. Чеклист перед завершенням будь-якого завдання
1. Код чисто компілюється через `cmake --build build` без нових попереджень (`warnings`).
2. Нові `.cpp`/`.h` файли додані в `EYESPOLY_SOURCES` у `CMakeLists.txt`.
3. Якщо додано нову системну залежність на Linux — вона опціональна (`QUIET`) у `CMakeLists.txt` і додана в `packaging/rpm/eyespoly.spec`, `packaging/arch/PKGBUILD` та `.github/workflows/release.yml` (за потреби).
4. Нові рядки інтерфейсу в QML обгорнуті в `qsTr(...)`.
5. Тимчасові файли збірки чи тестування не засмічують корінь репозиторію.
