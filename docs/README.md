# ChordLab - prototype=ChordLabV001A
# About the app.
=======================================
--->>>ChordLab provides comprehensive support for ChordPro files, allowing users to import, edit, and render chord sheets with embedded chords, lyrics, and metadata. It natively reads .cho, .crd, .chordpro, .txt and .pro formats, enabling automatic transposition, display formatting, and interactive features like auto-scroll.
=======================================
- **Environment:** Windows 11, Qt Creator v19.0.1, Git, GitHub.
- **Project Structure:** C++, CMake V3 (maintain to latest Qt 6.11.1).
- **Qt Version:** Qt 6.11.1 (Core, GUI, Widgets, Multimedia).
- **Core Requirement:** High-performance audio/visual processing.

## Coding Standards
=======================================
1.  **C++ Standards:** Use C++20 standard features.
2.  **Qt Style:** Use `camelCase` for variables/methods, `PascalCase` for classes. Use `QObject` for signal/slot mechanisms.
3.  **Memory Management:** Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) or `QObject` parent-child hierarchies. Avoid raw pointers.
4.  **A/V Best Practices:** Handle audio in separate threads (`QThread`) to avoid UI freezing. Use `QAudioSource`/`QAudioSink`.
5.  **CMake:** Use modern CMake (v3.0+) target-based approach (`target_link_libraries`, `target_include_directories`).

## Project Structure
=======================================
- `/src`: Source code (.cpp, .h)
- `/include`: Header files
- `/assets`: Media resources
- `/docs`: Documentation
- `/build`: CMake build files

## Git Strategy
=======================================
- **Workflow:** Branching (feature/branch), conventional commits (feat:, fix:, docs:).

## USER INTERFACE
=======================================
Layout is made up from Widgets in discrete C++ code setup in Qt6.11.1

┌──────────────────────────────────────────────────┐
│ App Menu-bar (File)(Edit)(Settings)(Help)etc...  │
├──────────────────────────────────────────────────┤
│ Tool-bar (icons)(set lists/settings,             |
| playback transport controls, etc.)               │
├──────────────────────────────────────────────────┤
│ App. display goes here...                        │
│                                                  │
│                                                  │
│                                                  │
│                                                  │
└──────────────────────────────────────────────────┘

## Files and folder structure (prototype on Win 11 PC)
=======================================
D:/Qt/projects/ChordLabV001A
├── CMakeLists.txt           # Root CMake file: Defines project name, version, and modules.
├── main.cpp                 # Entry point: Initializes QGuiApplication/QApplication.
├── CMakeLists.txt.user      # Created by Qt Creator: Local user settings (do not commit).
│
├── /src                     # Source code directory (optional, recommended).
│   ├── main.cpp   
│   ├── mainwindow.cpp          # C++ logic files.
│   ├── mainwindow.h            # Header files.
│   ├── setlistmanager.cpp      # Set list manager module.
│   └── setlistmanager.h        # Set list manager header files.
│
├──XX /qml                     # this folder is not included QML is *not* used.
│
│
├── resources.qrc            # Qt Resource file: XML that maps physical files to :/<prefix> paths.
├── /resources               # Resource directory (images, fonts, translations).
│      ├── icons
│      │   └── *.png, .ico, .svg
│      ├── pieces
│      │   └── piece-name
│      │       └── *.cho, .jpg, .mp3, .mp3, .mp3, .json, etc.
│      └── setlists
│           └── *.set, .json, etc.
│
├── /build                   # Build directory (output): Contains binaries, Makefiles, etc.
└────── /resources          # Build directory copy of resources folders and files

###########################################################################################
