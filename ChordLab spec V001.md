# SheetsPlus - prototype=SP-V001a
# Gemini - alsaATgml.com
# About the app.
=======================================
--->>>SheetsPlus provides comprehensive support for ChordPro files, allowing users to import, edit, and render chord sheets with embedded chords, lyrics, and metadata. It natively reads .cho, .crd, .chordpro, and .pro formats, enabling automatic transposition, display formatting, and interactive features like auto-scroll.

--->>>Support for lead-sheets, chord-sheets, scale-sheets,
--->>>Graphics insertions,
--->>>Chords for Guitar, Mandolin, Ukulele, keyboards,
 and 
--->>>Key ChordPro Functionalities in SheetsPlus:
 - Importing: Directly import ChordPro files to instantly create formatted song entries, including title, artist, and key metadata.
 - Rendering: Automatically formats chords (in []) above lyrics, highlights choruses, and handles formatting directives like {soc} (start of chorus) and {eoc}.
 - Editing: Includes a built-in editor to modify ChordPro syntax directly on the device, with a live preview to see changes instantly.
 - Transposition: Change the key of a song on the fly, which recalculates the chords within the ChordPro structure automatically.
 - Special Sections: Supports {sot} (start of tab) and {eot} (end of tab) directives to display tab sections in a monospaced font.
 - Metadata Support: Reads directive tags for song metadata (tempo, time signature, key) to populate SheetsPlus fields automatically.
 - Backing Tracks: You can embed backing tracks (with integral -80/-90/100% tempo and +/- 6 semi tone pitch control) to play automatically with your ChordPro charts.
 - In built Context saving/loading when closing/starting the app.
 - In built library managers:
     o songs (called pieces) with up to 2 optional (can be blank/ignored) audio track associations - 'original' and/or 'backing tracks').
     o default, variations and new chord management for Guitar, Mandolin, Ukulele, keyboards.
     o set lists (called setlists, made up of pieces, which reatin audio track associations of the pieces.
     o books of songs (called songbooks).

 - In built .qss support for control and recall of app. themes and screen(s)/layout.
 - In built settings control for file storage paths, video window layout, and preferences for ChordPro handling.

Tips for Using ChordPro in SheetsPlus
 - SheetsPlus Manager: Use the SheetsPlus Manager on a desktop to easily manage and edit complex ChordPro files via a browser.
 - Fixing Formats: If a song doesn't render properly, you can use the "Convert to ChordPro" feature in the editor to fix the formatting.


# AI Context & Specification
=======================================
- **Environment:** Windows 11, Qt Creator, Git, GitHub.
- **Project Structure:** C++, CMake V4.
- **Qt Version:** Qt 6.11.0 (Core, GUI, Widgets, Multimedia).
- **Core Requirement:** High-performance audio/visual processing.

## Coding Standards
=======================================
1.  **C++ Standards:** Use C++20 standard features.
2.  **Qt Style:** Use `camelCase` for variables/methods, `PascalCase` for classes. Use `QObject` for signal/slot mechanisms.
3.  **Memory Management:** Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) or `QObject` parent-child hierarchies. Avoid raw pointers.
4.  **A/V Best Practices:** Handle audio in separate threads (`QThread`) to avoid UI freezing. Use `QAudioSource`/`QAudioSink`.
5.  **CMake:** Use modern CMake (v4.0+) target-based approach (`target_link_libraries`, `target_include_directories`).

## Project Structure
=======================================
- `/src`: Source code (.cpp, .h, .qml)
- `/include`: Header files
- `/assets`: Media resources
- `/docs`: Documentation
- `/build`: CMake build files

## Git Strategy
=======================================
- **Workflow:** Branching (feature/branch), conventional commits (feat:, fix:, docs:).

## USER INTERFACE
=======================================
Layout is made up from C++ code in Qt6.11

┌─────────────────────────────────────────┐
│ App Menu-bar (File)(Edit)(Settings)(Help)etc...             │
┌─────────────────────────────────────────┐
│ Tool-bar (icons)(set lists/settings/scroll-control, etc.)   │
├─────────────────────────────────────────┤
│ Audio Player Transport             [Time Sig.] [Key][Tempo] │
│ << < Play/Stop >  >> [Loop controls] [play head track time] │
├─────────────────────────────────────────┤
│ Sheets display goes here...                                 │
│                                                             │
│                                                             │
│                                                             │
│                                                             │
└─────────────────────────────────────────┘

## Files and folder structure (prototype on Win 11 PC)
=======================================
D:/Qt/projects/SheetsPlusV001A
├── CMakeLists.txt           # Root CMake file: Defines project name, version, and modules.
├── main.cpp                 # Entry point: Initializes QGuiApplication/QApplication.
├── main.qml                 # Main QML file: Defines the GUI layout.
├── CMakeLists.txt.user      # Created by Qt Creator: Local user settings (do not commit).
│
├── /src                     # Source code directory (optional, recommended).
│   ├── moduleNN.cpp          # C++ logic files.
│   └── moduleNN.h            # Header files.
│
├──XX /qml                     # this folder is not included QML is *not* used.
│
├── /resources               # Resource directory (images, fonts, translations).
│   └── images
│       └── *.png, .ico, . jpg, .mp3
│
├── resources.qrc            # Qt Resource file: XML that maps physical files to :/<prefix> paths.
└── /build                   # Build directory (output): Contains binaries, Makefiles, etc.

## audio-visual assets (example for Win 11 PC)
=======================================
PC physical file/folder links have "\" but .txt, .json, etc. files will use and parse as cross platform "/"

D:\Users\aldos\AppData\Roaming\QPlayer\assets\  --> base address for assets
28-Apr-26  07:54 PM    <DIR>          icons
20-Jul-25  09:05 PM    <DIR>          loops
20-Jul-25  09:05 PM    <DIR>          metronome
20-Oct-25  03:21 PM    <DIR>          playlists
10-Nov-25  06:18 PM    <DIR>          stems
26-Oct-25  09:00 PM    <DIR>          styles

D:\Users\aldos\AppData\Roaming\QPlayer\assets\playlists
contains text files -> * standard playlists*.txt, e.g.:
D:\Users\aldos\AppData\Roaming\QPlayer>type assets\playlists\MSB1-playlist.txt
"Yellow Submarine/MSB 1.1 The Beatles - Yellow Submarine [G 108bpm]retuned.mp3
Irish Heartbeat/MSB 1.2. Irish Heartbeat Retweaked V2 [G 100 bpm].mp3
Saltwater Cowboy/MSB 1.3 Pigram Brothers - Saltwater Cowboy [D 95 bpm].mp3
Take Me Home Country Roads/MSB 1.4 Take Me Home, Country Roads  - John Denver [in G 84 bpm].mp3
The house of the rising sun/MSB 1.5 The Animals - House of the Rising Sun [Am 110 bpm]no solo.mp3
Norwegian Wood/MSB 1.6 Norwegian Wood -The Beatles -Remastered 2009-[D 91.5 bpm].mp3
True blue/MSB 1.7 True blue - John Williamson [G 115bpm].mp3
I am Australian/MSB 1.8 I am Australian STITCHED [C Major 144BPM].mp3
Sloop John B/MSB 1.9 OLD Sloop John B - [D 124bpm].mp3
Blowin in the wind/MSB 1.10 Blowin in the wind - Seekers [C 90 bpm].mp3"

D:\Users\aldos\AppData\Roaming\QPlayer\assets\playlists\pieces
sub-folders per piece: e.g.:
D:\Users\aldos\AppData\Roaming\QPlayer\assets\playlists\pieces\Yellow Submarine\
      MSB 1.1 The Beatles - Yellow Submarine [G 108bpm]retuned_original.mp3
           === original to practice with
      MSB 1.1 The Beatles - Yellow Submarine [G 108bpm]retuned_backing-track.mp3
           === original to play along or perform live with
      MSB 1.1 The Beatles - Yellow Submarine [G 108bpm]retuned.chopro
           === ChordPro "Chord symbols+Lyrics" file
      MSB 1.1 The Beatles - Yellow Submarine [G 108bpm]retuned.jpg
           === custom made image/screen capture/created songsheet/variations 
           === image can be displayed as visual aid for practice/performance etc. 
               in place of (e.g. alternative) ChordPro file rendering to the screen
               we load ChordPro if there is correctly named files, other wise try to load .png or .jpg

###########################################################################################


