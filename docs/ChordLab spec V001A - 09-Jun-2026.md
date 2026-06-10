# ChordLab - prototype=ChordLabV001A
# About the app.
=======================================
--->>>ChordLab provides comprehensive support for ChordPro files, allowing users to import, edit, and render chord sheets with embedded chords, lyrics, and metadata. It natively reads .cho, .crd, .chordpro, .txt and .pro formats, enabling automatic transposition, display formatting, and interactive features like auto-scroll.

--->>>Support for lead-sheets, chord-sheets, scale-sheets,
--->>>Graphics insertions,
--->>>Chords for Guitar, Mandolin, Ukulele, keyboards,
 and 
--->>>Key ChordPro Functionalities in ChordLab:
 - Importing: Directly import ChordPro files to instantly create formatted song entries, including title, artist, and key metadata.
 - Rendering: Automatically formats chords (in []) above lyrics, highlights choruses, and handles formatting directives like {soc} (start of chorus) and {eoc}.
 - Editing: Includes a built-in editor to modify ChordPro syntax directly on the device, with a live preview to see changes instantly.
 - Transposition: Change the key of a song on the fly, which recalculates the chords within the ChordPro structure automatically.
 - Special Sections: Supports {sot} (start of tab) and {eot} (end of tab) directives to display tab sections in a monospaced font.
 - Metadata Support: Reads directive tags for song metadata (tempo, time signature, key) to populate ChordLab fields automatically.
 - Backing Tracks: You can embed backing tracks (with integral -80/-90/100% tempo and +/- 6 semi tone pitch control) to play automatically with your ChordPro charts.
 - In built Context saving/loading when closing/starting the app.
 - In built library managers:
     o songs (called pieces) with up to 2 optional (can be blank/ignored) audio track associations - 'original' and/or 'backing tracks').
     o default, variations and new chord management for Guitar, Mandolin, Ukulele, keyboards.
     o set lists (called setlists, made up of pieces, which reatin audio track associations of the pieces.
     o books of songs (called songbooks).

 - In built .qss support for control and recall of app. themes and screen(s)/layout.
 - In built settings control for file storage paths, video window layout, and preferences for ChordPro handling.

Tips for Using ChordPro in ChordLab
 - ChordLab Manager: Use the ChordLab Manager on a desktop to easily manage and edit complex ChordPro files via a browser.
 - Fixing Formats: If a song doesn't render properly, you can use the "Convert to ChordPro" feature in the editor to fix the formatting.


# AI Context & Specification
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
Layout is made up from Widgets in discrete C++ code setup in Qt6.11.1

┌──────────────────────────────────────────────────┐
│ App Menu-bar (File)(Edit)(Settings)(Help)etc...             │
┌──────────────────────────────────────────────────┐
│ Tool-bar (icons)(set lists/settings/scroll-control,         |
| playback transport controls, etc.)                          │
├──────────────────────────────────────────────────┤
│ Audio Player Transport             [Time Sig.] [Key][Tempo] │
│ << < Play/Stop >  >> [Loop controls] [play head track time] │
├──────────────────────────────────────────────────┤
│ App. display goes here...                                 │
│                                                             │
│                                                             │
│                                                             │
│                                                             │
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
{noformat}
Qt 6.10.3 (x86_64-little_endian-llp64 shared (dynamic) release build; by MSVC 2022) on "windows" 
OS: Windows 11 Version 24H2 [winnt version 10.0.26100]

Architecture: x86_64; features: SSE2 SSE3 SSSE3 SSE4.1 SSE4.2 AVX AVX2 AES

Environment:
  QT_DISABLE_SHADER_DISK_CACHE="1"

Features: QT_NO_EXCEPTIONS

Library info:
  PrefixPath: D:\Qt\Tools\QtCreator\bin
  DocumentationPath: D:\Qt\Tools\QtCreator\bin\doc
  HeadersPath: D:\Qt\Tools\QtCreator\bin\include
  LibrariesPath: D:\Qt\Tools\QtCreator\bin
  LibraryExecutablesPath: D:\Qt\Tools\QtCreator\bin\bin
  BinariesPath: D:\Qt\Tools\QtCreator\bin
  PluginsPath: D:\Qt\Tools\QtCreator\bin\plugins
  QmlImportsPath: D:\Qt\Tools\QtCreator\bin\qml
  ArchDataPath: D:\Qt\Tools\QtCreator\bin
  DataPath: D:\Qt\Tools\QtCreator\bin
  TranslationsPath: D:\Qt\Tools\QtCreator\bin\translations
  ExamplesPath: D:\Qt\Tools\QtCreator\bin\examples
  TestsPath: D:\Qt\Tools\QtCreator\bin\tests
  SettingsPath: 

Standard paths [*...* denote writable entry]:
  DesktopLocation: "Desktop" *C:\Users\lgoff\Desktop*
  DocumentsLocation: "Documents" *C:\Users\lgoff\Documents*
  FontsLocation: "Fonts" *C:\WINDOWS\Fonts*
  ApplicationsLocation: "Applications" *C:\Users\lgoff\AppData\Roaming\Microsoft\Windows\Start Menu\Programs*
  MusicLocation: "Music" *C:\Users\lgoff\Music*
  MoviesLocation: "Movies" *C:\Users\lgoff\Videos*
  PicturesLocation: "Pictures" *C:\Users\lgoff\Pictures*
  TempLocation: "Temporary Directory" *C:\Users\lgoff\AppData\Local\Temp*
  HomeLocation: "Home" *C:\Users\lgoff*
  AppLocalDataLocation: "Application Data" *C:\Users\lgoff\AppData\Local\QtProject\qtdiag* C:\ProgramData\QtProject\qtdiag D:\Qt\Tools\QtCreator\bin D:\Qt\Tools\QtCreator\bin\data D:\Qt\Tools\QtCreator\bin\data\QtProject\qtdiag
  CacheLocation: "Cache" *C:\Users\lgoff\AppData\Local\QtProject\qtdiag\cache*
  GenericDataLocation: "Shared Data" *C:\Users\lgoff\AppData\Local* C:\ProgramData D:\Qt\Tools\QtCreator\bin D:\Qt\Tools\QtCreator\bin\data
  RuntimeLocation: "Runtime" *C:\Users\lgoff*
  ConfigLocation: "Configuration" *C:\Users\lgoff\AppData\Local\QtProject\qtdiag* C:\ProgramData\QtProject\qtdiag D:\Qt\Tools\QtCreator\bin D:\Qt\Tools\QtCreator\bin\data D:\Qt\Tools\QtCreator\bin\data\QtProject\qtdiag
  DownloadLocation: "Downloads" *C:\Users\lgoff\Downloads*
  GenericCacheLocation: "Shared Cache" *C:\Users\lgoff\AppData\Local\cache*
  GenericConfigLocation: "Shared Configuration" *C:\Users\lgoff\AppData\Local* C:\ProgramData D:\Qt\Tools\QtCreator\bin D:\Qt\Tools\QtCreator\bin\data
  AppDataLocation: "Application Configuration" *C:\Users\lgoff\AppData\Roaming\QtProject\qtdiag* C:\ProgramData\QtProject\qtdiag D:\Qt\Tools\QtCreator\bin D:\Qt\Tools\QtCreator\bin\data D:\Qt\Tools\QtCreator\bin\data\QtProject\qtdiag
  AppConfigLocation: "Application Configuration" *C:\Users\lgoff\AppData\Local\QtProject\qtdiag* C:\ProgramData\QtProject\qtdiag D:\Qt\Tools\QtCreator\bin D:\Qt\Tools\QtCreator\bin\data D:\Qt\Tools\QtCreator\bin\data\QtProject\qtdiag

File selectors (increasing order of precedence):
  en_AU windows winnt

Network:
  Using "OpenSSL 3.4.0 22 Oct 2024", version: 0x30400000

Platform capabilities: ThreadedPixmaps OpenGL ThreadedOpenGL WindowMasks MultipleWindows ForeignWindows NonFullScreenWindows NativeWidgets WindowManagement RasterGLSurface AllGLFunctionsQueryable

Style hints:
  mouseDoubleClickInterval: 500
  mousePressAndHoldInterval: 800
  startDragDistance: 10
  startDragTime: 500
  startDragVelocity: 0
  keyboardInputInterval: 400
  keyboardAutoRepeatRateF: 32
  cursorFlashTime: 1060
  showIsFullScreen: 0
  showIsMaximized: 0
  passwordMaskDelay: 0
  passwordMaskCharacter: U+25CF
  fontSmoothingGamma: 1.2
  useRtlExtensions: 0
  setFocusOnTouchRelease: 0
  tabFocusBehavior: Qt::TabFocusAllControls 
  singleClickActivation: 0

Additional style hints (QPlatformIntegration):
  ReplayMousePressOutsidePopup: 1

Theme:
  Platforms requested : windows
            available : 
  Styles requested    : Windows11,WindowsVista,Windows
         available    : windows11,windowsvista,Windows,Fusion
  System font         : "Segoe UI" 9
  Native file dialog

Fonts:
  General font : "Segoe UI" 9
  Fixed font   : "Courier New" 9
  Title font   : "Segoe UI" 9
  Smallest font: "Segoe UI" 9

Palette:
  QPalette::WindowText: #ff000000
  QPalette::Button: #fff0f0f0
  QPalette::Light: #ffffffff
  QPalette::Midlight: #ffe3e3e3
  QPalette::Dark: #ffa0a0a0
  QPalette::Mid: #ffa0a0a0
  QPalette::Text: #ff000000
  QPalette::BrightText: #ffffffff
  QPalette::ButtonText: #ff000000
  QPalette::Base: #ffffffff
  QPalette::Window: #fff0f0f0
  QPalette::Shadow: #ff696969
  QPalette::Highlight: #ffda3b01
  QPalette::HighlightedText: #ffffffff
  QPalette::Link: #ff661c01
  QPalette::LinkVisited: #ff421200
  QPalette::AlternateBase: #ffe9e7e3
  QPalette::NoRole: #ff000000
  QPalette::ToolTipBase: #ffffffdc
  QPalette::ToolTipText: #ff000000
  QPalette::PlaceholderText: #80000000
  QPalette::Accent: #ff9c2b02

Screens: 2, High DPI scaling: inactive
# 0 "LEN-24AST-B  " Depth: 32 Primary: yes
  Manufacturer: Inc
  Model: LEN-24AST-B  
  Serial number: 1
  Geometry: 1920x1080+0+0 Available: 1920x1032+0+0
  Virtual geometry: 3840x1080+0+0 Available: 3840x1032+0+0
  2 virtual siblings
  Physical size: 527x296 mm  Refresh: 60 Hz Power state: 0
  Physical DPI: 92.5389,92.6757 Logical DPI: 96,96 Subpixel_None
  DevicePixelRatio: 1
  Primary orientation: 2 Orientation: 2 Native orientation: 0

# 1 "HP M27fw FHD" Depth: 32 Primary: no
  Manufacturer: K.
  Model: HP M27fw FHD
  Serial number: 3CM3210XGZ
  Geometry: 1920x1080+1920+0 Available: 1920x1032+1920+0
  Virtual geometry: 3840x1080+0+0 Available: 3840x1032+0+0
  2 virtual siblings
  Physical size: 597x336 mm  Refresh: 59.9401 Hz Power state: 0
  Physical DPI: 81.6884,81.6429 Logical DPI: 96,96 Subpixel_None
  DevicePixelRatio: 1
  Primary orientation: 2 Orientation: 2 Native orientation: 0

Input devices: 1
  TouchScreen "", capabilities: Position Area NormalizedPosition MouseEmulation


Dynamic GL LibGL Vendor: ATI Technologies Inc.
Renderer: AMD Radeon R7 Graphics
Version: 4.6.148202 Compatibility Profile Context 20.50.18 27.20.15018.3
Shading language: 4.60
Format: Version: 4.6 Profile: 2 Swap behavior: 2 Buffer size (RGBA): 8,8,8,8 Stencil buffer: 8
Profile: None (QOpenGLFunctions_4_6)

Vulkan instance available
Supported instance extensions:
  VK_KHR_device_group_creation, version 1
  VK_KHR_external_fence_capabilities, version 1
  VK_KHR_external_memory_capabilities, version 1
  VK_KHR_external_semaphore_capabilities, version 1
  VK_KHR_get_physical_device_properties2, version 2
  VK_KHR_get_surface_capabilities2, version 1
  VK_KHR_surface, version 25
  VK_KHR_win32_surface, version 6
  VK_EXT_debug_report, version 9
  VK_EXT_debug_utils, version 2
  VK_EXT_swapchain_colorspace, version 4
  VK_KHR_portability_enumeration, version 1
  VK_LUNARG_direct_driver_loading, version 1
Supported layers:
  VK_LAYER_AMD_switchable_graphics, version 1, spec version 1.2.170, AMD switchable graphics layer
Available physical devices:
  API version 1.2.170, vendor 0x1002, device 0x6900, AMD Radeon R7 Graphics, type 2, driver version 2.0.179  API version 1.2.170, vendor 0x1002, device 0x9874, AMD Radeon R7 Graphics, type 1, driver version 2.0.179


GPU #1:
         Card name         : AMD Radeon R7 Graphics
       Driver Name         : aticfx64.dll
    Driver Version         : 27.20.15018.3
         Vendor ID         : 0x1002
         Device ID         : 0x9874
         SubSys ID         : 0x36DA17AA
       Revision ID         : 0x00C8

GPU #2:
         Card name         : AMD Radeon R7 Graphics
       Driver Name         : aticfx64.dll
    Driver Version         : 27.20.15018.3
         Vendor ID         : 0x1002
         Device ID         : 0x9874
         SubSys ID         : 0x36DA17AA
       Revision ID         : 0x00C8

Qt Rendering Hardware Interface supported backends:
OpenGL (with default QSurfaceFormat):
  Driver Info: Device: ATI Technologies Inc. AMD Radeon R7 Graphics 4.6.148202 Compatibility Profile Context 20.50.18 27.20.15018.3 Device ID: 0x0 Vendor ID: 0x0 Device type: Unknown
  Min Texture Size: 1
  Max Texture Size: 16384
  Max Color Attachments: 8
  Frames in Flight: 1
  Async Readback Limit: 1
  MaxThreadGroupsPerDimension: 65535
  MaxThreadsPerThreadGroup: 1024
  MaxThreadGroupX: 1024
  MaxThreadGroupY: 1024
  MaxThreadGroupZ: 1024
  TextureArraySizeMax: 2048
  MaxUniformBufferRange: 65536
  MaxVertexInputs: 29
  MaxVertexOutputs: 32
  Uniform Buffer Alignment: 1
  Supported MSAA sample counts: 1,2,4,8
  Features:
    v MultisampleTexture
    v MultisampleRenderBuffer
    - DebugMarkers
    v Timestamps
    v Instancing
    - CustomInstanceStepRate
    v PrimitiveRestart
    v NonDynamicUniformBuffers
    v NonFourAlignedEffectiveIndexBufferOffset
    v NPOTTextureRepeat
    - RedOrAlpha8IsRed
    v ElementIndexUint
    v Compute
    v WideLines
    v VertexShaderPointSize
    v BaseVertex
    - BaseInstance
    v TriangleFanTopology
    v ReadBackNonUniformBuffer
    v ReadBackNonBaseMipLevel
    v TexelFetch
    v RenderToNonBaseMipLevel
    v IntAttributes
    v ScreenSpaceDerivatives
    - ReadBackAnyTextureFormat
    v PipelineCacheDataLoadSave
    v ImageDataStride
    v RenderBufferImport
    v ThreeDimensionalTextures
    v RenderTo3DTextureSlice
    v TextureArrays
    v Tessellation
    v GeometryShader
    - TextureArrayRange
    v NonFillPolygonMode
    v OneDimensionalTextures
    v OneDimensionalTextureMipmaps
    v HalfAttributes
    v RenderToOneDimensionalTexture
    v ThreeDimensionalTextureMipmaps
  Texture formats: RGBA8 BGRA8 R8 RG8 R16 RG16 RED_OR_ALPHA8 RGBA16F RGBA32F R16F R32F RGB10A2 D16 D32F BC1 BC2 BC3 ETC2_RGB8 ETC2_RGB8A1 ETC2_RGBA8
Vulkan:
  Driver Info: Device: AMD Radeon R7 Graphics Device ID: 0x6900 Vendor ID: 0x1002 Device type: Discrete
  Min Texture Size: 1
  Max Texture Size: 16384
  Max Color Attachments: 8
  Frames in Flight: 2
  Async Readback Limit: 2
  MaxThreadGroupsPerDimension: 65535
  MaxThreadsPerThreadGroup: 1024
  MaxThreadGroupX: 1024
  MaxThreadGroupY: 1024
  MaxThreadGroupZ: 1024
  TextureArraySizeMax: 2048
  MaxUniformBufferRange: 2147483647
  MaxVertexInputs: 64
  MaxVertexOutputs: 32
  Uniform Buffer Alignment: 16
  Supported MSAA sample counts: 1,2,4,8
  Features:
    v MultisampleTexture
    v MultisampleRenderBuffer
    v DebugMarkers
    v Timestamps
    v Instancing
    - CustomInstanceStepRate
    v PrimitiveRestart
    v NonDynamicUniformBuffers
    v NonFourAlignedEffectiveIndexBufferOffset
    v NPOTTextureRepeat
    v RedOrAlpha8IsRed
    v ElementIndexUint
    v Compute
    v WideLines
    v VertexShaderPointSize
    v BaseVertex
    v BaseInstance
    v TriangleFanTopology
    v ReadBackNonUniformBuffer
    v ReadBackNonBaseMipLevel
    v TexelFetch
    v RenderToNonBaseMipLevel
    v IntAttributes
    v ScreenSpaceDerivatives
    v ReadBackAnyTextureFormat
    v PipelineCacheDataLoadSave
    v ImageDataStride
    - RenderBufferImport
    v ThreeDimensionalTextures
    - RenderTo3DTextureSlice
    v TextureArrays
    v Tessellation
    v GeometryShader
    v TextureArrayRange
    v NonFillPolygonMode
    v OneDimensionalTextures
    v OneDimensionalTextureMipmaps
    v HalfAttributes
    v RenderToOneDimensionalTexture
    v ThreeDimensionalTextureMipmaps
  Texture formats: RGBA8 BGRA8 R8 RG8 R16 RG16 RED_OR_ALPHA8 RGBA16F RGBA32F R16F R32F RGB10A2 D16 D32F BC1 BC2 BC3 BC4 BC5 BC6H BC7
Direct3D 11:
  Driver Info: Device: AMD Radeon R7 Graphics Device ID: 0x9874 Vendor ID: 0x1002 Device type: Unknown
  Min Texture Size: 1
  Max Texture Size: 16384
  Max Color Attachments: 8
  Frames in Flight: 1
  Async Readback Limit: 1
  MaxThreadGroupsPerDimension: 65535
  MaxThreadsPerThreadGroup: 1024
  MaxThreadGroupX: 1024
  MaxThreadGroupY: 1024
  MaxThreadGroupZ: 64
  TextureArraySizeMax: 2048
  MaxUniformBufferRange: 65536
  MaxVertexInputs: 32
  MaxVertexOutputs: 32
  Uniform Buffer Alignment: 256
  Supported MSAA sample counts: 1,2,4,8
  Features:
    v MultisampleTexture
    v MultisampleRenderBuffer
    v DebugMarkers
    v Timestamps
    v Instancing
    v CustomInstanceStepRate
    v PrimitiveRestart
    - NonDynamicUniformBuffers
    v NonFourAlignedEffectiveIndexBufferOffset
    v NPOTTextureRepeat
    v RedOrAlpha8IsRed
    v ElementIndexUint
    v Compute
    - WideLines
    - VertexShaderPointSize
    v BaseVertex
    v BaseInstance
    - TriangleFanTopology
    v ReadBackNonUniformBuffer
    v ReadBackNonBaseMipLevel
    v TexelFetch
    v RenderToNonBaseMipLevel
    v IntAttributes
    v ScreenSpaceDerivatives
    v ReadBackAnyTextureFormat
    v PipelineCacheDataLoadSave
    v ImageDataStride
    - RenderBufferImport
    v ThreeDimensionalTextures
    v RenderTo3DTextureSlice
    v TextureArrays
    v Tessellation
    v GeometryShader
    v TextureArrayRange
    v NonFillPolygonMode
    v OneDimensionalTextures
    v OneDimensionalTextureMipmaps
    v HalfAttributes
    v RenderToOneDimensionalTexture
    v ThreeDimensionalTextureMipmaps
  Texture formats: RGBA8 BGRA8 R8 RG8 R16 RG16 RED_OR_ALPHA8 RGBA16F RGBA32F R16F R32F RGB10A2 D16 D24 D24S8 D32F BC1 BC2 BC3 BC4 BC5 BC6H BC7
Direct3D 12:
  Driver Info: Device: AMD Radeon R7 Graphics Device ID: 0x9874 Vendor ID: 0x1002 Device type: Unknown
  Min Texture Size: 1
  Max Texture Size: 16384
  Max Color Attachments: 8
  Frames in Flight: 2
  Async Readback Limit: 2
  MaxThreadGroupsPerDimension: 65535
  MaxThreadsPerThreadGroup: 1024
  MaxThreadGroupX: 1024
  MaxThreadGroupY: 1024
  MaxThreadGroupZ: 1024
  TextureArraySizeMax: 2048
  MaxUniformBufferRange: 65536
  MaxVertexInputs: 32
  MaxVertexOutputs: 32
  Uniform Buffer Alignment: 256
  Supported MSAA sample counts: 1,2,4,8
  Features:
    v MultisampleTexture
    v MultisampleRenderBuffer
    v DebugMarkers
    v Timestamps
    v Instancing
    v CustomInstanceStepRate
    v PrimitiveRestart
    - NonDynamicUniformBuffers
    v NonFourAlignedEffectiveIndexBufferOffset
    v NPOTTextureRepeat
    v RedOrAlpha8IsRed
    v ElementIndexUint
    v Compute
    - WideLines
    - VertexShaderPointSize
    v BaseVertex
    v BaseInstance
    - TriangleFanTopology
    v ReadBackNonUniformBuffer
    v ReadBackNonBaseMipLevel
    v TexelFetch
    v RenderToNonBaseMipLevel
    v IntAttributes
    v ScreenSpaceDerivatives
    v ReadBackAnyTextureFormat
    - PipelineCacheDataLoadSave
    v ImageDataStride
    - RenderBufferImport
    v ThreeDimensionalTextures
    v RenderTo3DTextureSlice
    v TextureArrays
    v Tessellation
    v GeometryShader
    v TextureArrayRange
    v NonFillPolygonMode
    v OneDimensionalTextures
    - OneDimensionalTextureMipmaps
    v HalfAttributes
    v RenderToOneDimensionalTexture
    v ThreeDimensionalTextureMipmaps
  Texture formats: RGBA8 BGRA8 R8 RG8 R16 RG16 RED_OR_ALPHA8 RGBA16F RGBA32F R16F R32F RGB10A2 D16 D24 D24S8 D32F BC1 BC2 BC3 BC4 BC5 BC6H BC7


Plugin information:

+ android                           19.0.2
  ant                               19.0.2
+ autotest                          19.0.2
  autotoolsprojectmanager           19.0.2
  axivion                           19.0.2
  baremetal                         19.0.2
  bazaar                            19.0.2
  beautifier                        19.0.2
+ bineditor                         19.0.2
  boot2qt                           19.0.2
  cargo                             19.0.2
+ clangcodemodel                    19.0.2
+ clangformat                       19.0.2
+ clangtools                        19.0.2
+ classview                         19.0.2
  clearcase                         19.0.2
+ cmakeprojectmanager               19.0.2
  coco                              19.0.2
+ codepaster                        19.0.2
  compilationdatabaseprojectmanager 19.0.2
  compilerexplorer                  19.0.2
  conan                             19.0.2
  copilot                           19.0.2
+ core                              19.0.2
  cppcheck                          19.0.2
+ cppeditor                         19.0.2
+ ctfvisualizer                     19.0.2
  cvs                               19.0.2
+ debugger                          19.0.2
+ designer                          19.0.2
  devcontainer                      19.0.2
+ diffeditor                        19.0.2
  docker                            19.0.2
  dotnet                            19.0.2
  effectcomposer                    19.0.2
  emacskeys                         19.0.2
+ extensionmanager                  19.0.2
+ fakevim                           19.0.2
  fossil                            19.0.2
+ genericprojectmanager             19.0.2
+ git                               19.0.2
  gitlab                            19.0.2
+ glsleditor                        19.0.2
  gradle                            19.0.2
  helloworld                        19.0.2
+ help                              19.0.2
+ imageviewer                       19.0.2
+ incredibuild                      19.0.2
  ios                               19.0.2
+ languageclient                    19.0.2
+ learning                          19.0.2
+ lua                               19.0.2
+ lualanguageclient                 19.0.2
+ lualanguageserver                 1.0.0
  luatests                          1.0.0
+ macros                            19.0.2
  mcpserver                         19.0.2
  mcusupport                        19.0.2
  mercurial                         19.0.2
  mesonprojectmanager               19.0.2
+ modeleditor                       19.0.2
  multipropertyeditor               19.0.2
  nim                               19.0.2
  perforce                          19.0.2
+ perfprofiler                      19.0.2
+ projectexplorer                   19.0.2
+ python                            19.0.2
+ qbsprojectmanager                 19.0.2
+ qmakeprojectmanager               19.0.2
  qmldesigner                       19.0.2
+ qmljseditor                       19.0.2
+ qmljstools                        19.0.2
+ qmlpreview                        19.0.2
+ qmlprofiler                       19.0.2
+ qmlprojectmanager                 19.0.2
+ qnx                               19.0.2
+ qtapplicationmanagerintegration   19.0.2
+ qtsupport                         19.0.2
+ remotelinux                       19.0.2
+ resourceeditor                    19.0.2
+ rustlanguageserver                1.0.0
  saferenderer                      19.0.2
  screenrecorder                    19.0.2
+ scxmleditor                       19.0.2
  serialterminal                    19.0.2
  silversearcher                    19.0.2
  squish                            19.0.2
  subversion                        19.0.2
  swift                             19.0.2
+ tellajoke                         1.0.0
+ terminal                          19.0.2
+ texteditor                        19.0.2
  todo                              19.0.2
+ updateinfo                        19.0.2
+ valgrind                          19.0.2
  vcpkg                             19.0.2
+ vcsbase                           19.0.2
+ webassembly                       19.0.2
+ welcome                           19.0.2

Used settingspath: ~/AppData/Roaming/QtProject

UI configuration:

Color: #666666
Theme: light-2024 "Light (2024)"
Theme color scheme: Qt::ColorScheme::Light
System color scheme: Qt::ColorScheme::Light
Toolbar style: Utils::StyleHelper::ToolbarStyleRelaxed
Language: en_US
Device pixel ratio: 1, Qt::HighDpiScaleFactorRoundingPolicy::Round
Font DPI: 96
Utils::StyleHelper::UiElement:
  H1: Titillium Web,27,-1,5,600,0,0,0,0,0,0,0,0,2,0,1
  H2: Titillium Web,21,-1,5,600,0,0,0,0,0,0,0,0,0,0,1
  H3: Segoe UI,12,-1,5,700,0,0,0,0,0,1,0,0,0,0,1
  H4: Segoe UI,12,-1,5,700,0,0,0,0,0,0,0,0,0,0,1
  H5: Segoe UI,10.5,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  H6: Segoe UI,9,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  H6Capital: Segoe UI,9,-1,5,400,0,0,0,0,0,1,0,0,0,0,1
  Body1: Segoe UI,10.5,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  Body2: Segoe UI,9,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  ButtonMedium: Segoe UI,9,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  ButtonSmall: Segoe UI,7.5,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  LabelMedium: Segoe UI,9,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  LabelSmall: Segoe UI,7.5,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  CaptionStrong: Segoe UI,7.5,-1,5,600,0,0,0,0,0,0,0,0,0,0,1
  Caption: Segoe UI,7.5,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  IconStandard: Segoe UI,9,-1,5,400,0,0,0,0,0,0,0,0,0,0,1
  IconActive: Segoe UI,9,-1,5,400,0,0,0,0,0,0,0,0,0,0,1

Product: Qt Creator 19.0.2
Based on: Qt 6.10.3 (MSVC 2022, x86_64)
Built on: Mon May 11 08:08:03 2026
From revision: 41c25c247e


{noformat}


