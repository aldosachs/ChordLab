#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QStringList>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QKeyEvent>
#include "setlistmanager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

    class SetListManager;

public:
    bool onLoadSetlistLoadedCheck;

    enum AppState {
        Idle,
        OpenEdit,
        PlayAlong
    };

    enum ChordDisplayMode {
        CIL,
        CAL
    };

    struct AudioTracks {
        QString fullPath;
        QString backingPath;
        QString splitPath;
        QString slowPath;
    };

    struct SongLayoutState {
        bool hasUserOverride = false;
        bool hasPlayModeStyleOverride = false;

        int columnCount;      // 1, 2, 4, etc. (Handles your "Quadrant" idea)
        int fontSize;
        int capoShift;
        int savedColumns = 1;
        int savedZoomLevel = 0;
    };

    enum Theme {
        Light,
        Dark
    };

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

    // REPLACE the existing SongLayoutMetrics struct with this:
    struct SongLayoutMetrics {

        // Per-section breakdown (mirrors what parseChordProToGrid() actually gathers)
        struct SectionInfo {
            QString name;
            int lineCount  = 0;      // number of chord/lyric line pairs inside this section
            bool hasHeader = false;  // true if section has a visible heading row
        };

        int totalLines        = 0;  // cil_lof: total content lines
        int maxLineCharacters = 0;  // cil_wll: widest lyric-only line width in chars
        int sectionCount      = 0;  // total gatheredSections count (matches parseChordProToGrid)
        int targetColumns     = 1;  // result from autoOptimizeLayout() or legacy Radar

        QVector<SectionInfo> sections;  // NEW: populated by analyzeChordProMetaData()
    };

    QToolBar *m_settingsToolBar; // Promote from local variable to member

    // Font preference
    QString m_currentFont = "Courier Prime";  // default
    static const QStringList AVAILABLE_FONTS;  // defined in .cpp

protected:
    // This allows intercepting the Spacebar and Zoom keys anywhere inside the window frame
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void handleFileOpen();
    void handleFileSave();
    void toggleDisplayMode();
    void toggleTheme();
    void togglePlaybackMode();
    void loadStyleSheetFromFile(const QString &filePath);

    void handleAddSongToSetlist();
    void handleRemoveSongFromSetlist();
    void handleSaveSetlist();
    void handleSetlistItemClicked(const QModelIndex &index);
    void handleSetlistItemDoubleClicked(const QModelIndex &index);

private:
    void setupMenus();
    void setupToolBar();
    void setupLayout();

    void setAppState(AppState state);
    void updateFunctionKeys();
    void shiftTransposition(int delta);

    void onHamburgerClicked();

    void checkForCompanionAudio(const QString &chordProPath);
    void selectAudioTrack(QPushButton *clickedButton, const QString &filePath, const QString &trackType);

    void saveLayoutPreference(const QString &filePath, const SongLayoutState &layout);
    void saveSongLayoutPreference(const QString &filePath);
    void loadSongLayoutPreference(const QString &filePath);
    void loadSongQuietly(const QString &fileName);

    SongLayoutState loadLayoutPreference(const QString &filePath);

    QString getResourcesPath() const;

    AudioTracks m_audioTracks;
    QString m_selectedAudioPath;

    QMediaPlayer *m_mediaPlayer = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);

    SetlistManager *m_setlistManager;
    QTreeView *m_setlistView;
    void onLoadSetlistTriggered();
    QStringList getAvailableSetlists();

    struct ChordProSection {
        QString header;
        QString body;
    };

    SongLayoutMetrics m_currentSongMetrics; // Persistent tracker variable for active file context

    void analyzeChordProMetaData(const QString &rawInput); // The predictive pre-analysis engine

    void parseChordProToGrid(const QString &rawText);
    void updatePlayAlongLayoutDensity();

    void autoOptimizeLayout();
    QString getSongLayoutJsonPath(const QString &chordProPath) const;
    void saveSongLayoutToJson(const QString &chordProPath);
    void loadSongLayoutFromJson(const QString &chordProPath);

    QString m_parsedSongContentGrid;

    int m_zoomCoarse = 0;      // Ctrl+/-         coarse zoom, steps of 2pt
    int m_zoomFine   = 0;      // Ctrl+Shift+/-   fine zoom, steps of 0.5pt
    int m_columnOverride = 0;  // 0=auto (Radar), 1-4=manual lock

    int  m_sectionSpacingBonus = 0;   // Ctrl+Shift+Up/Down: extra px between sections (0–20)
    bool m_useAutoLayout       = true; // false = user has manually tuned this song's layout

    // --- Transposition & Instrument Mapping Matrix ---
    int m_transposeShift = 0;           // Concert Pitch / Singer shift (Key Up/Down)
    int m_capoShift;                // Extracted from {capo: X} or a manual setting
    int m_instrumentTuningOffset;   // E Standard = 0, D Standard = -2, Eb Standard = -1

    // --- Telemetry & Testing ---
    bool m_debugTelemetryEnabled;   // Toggles detailed string parsing dumps to Qt Console
    bool m_debugVerboseLevel;       // Toggles verbose administrative dumps to Qt Console
    bool m_debug_Setlist;
    bool m_isSetlistDirty = false;

    // --- Advanced Sub-Parsing Engines ---
    QString transposeSingleNoteToken(const QString &noteToken, int semitones);
    QString transposeChord(const QString &chordText, int semitones);
    QString parseGridLine(const QString &line, int semitones);
    QString parseTabLine(const QString &line, int chordDelta, int instrumentDelta);
    bool m_isLoadingFile = false;

    QString runInitialParse(const QString &rawInput);
    QString processLineContent(const QString &line);

    QString getThemeStyles();
    QString m_rawSongContent;

    // UI Layout Components
    QSplitter *mainSplitter;
    QSplitter *editorSplitter; // The new inner splitter
    QPlainTextEdit *originalEditor;
    QTextEdit *parsedEditor;

    // Control Buttons
    QPushButton *m_btnTransposeUp;
    QPushButton *m_btnTransposeDown;
    QPushButton *m_btnTheme;
    QPushButton *m_viewToggleBtn;
    QPushButton *m_btnModeToggle;
    QPushButton *m_btnToggleSetlist;

    // Audio Selection Buttons
    QPushButton *m_btnTrackFull;
    QPushButton *m_btnTrackBkg;
    QPushButton *m_btnTrackSplit;
    QPushButton *m_btnTrackSlow;    

    // The 4 Context-Aware Function Buttons
    QPushButton *m_btnFn1;
    QPushButton *m_btnFn2;
    QPushButton *m_btnFn3;
    QPushButton *m_btnFn4;

    // Setlist operations
    QPushButton *m_btnAddSong;
    QPushButton *m_btnRemoveSong;
    QPushButton *m_btnSaveSetlist;

    // Internal State Variables
    AppState currentState;
    ChordDisplayMode m_currentMode = CAL;
    Theme m_currentTheme = Light;
    QString m_currentFilePath;
};

#endif // MAINWINDOW_H