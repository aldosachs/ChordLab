#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QStringList>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // 1. Define the AppState enum type here so the whole class understands it
    enum AppState {
        Idle,
        OpenEdit,
        PlayAlong
    };

    // Define the ChordDisplayMode enum if you haven't already
    enum ChordDisplayMode {
        CIL,
        CAL
    };

    struct AudioTracks {
        QString fullPath;
        QString backingPath;
        QString slowPath;
    };

    // Define Theme enum
    enum Theme {
        Light,
        Dark
    };

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private slots:
    void handleFileOpen();
    void handleFileSave();
    void toggleDisplayMode();
    void toggleTheme();
    void togglePlaybackMode();

private:
    void setupMenus();
    void setupToolBar();
    void setupLayout();

    // 2. Declaration for state manager function
    void setAppState(AppState state);

    // 3. Helper to refresh your dynamic Fn-buttons
    void updateFunctionKeys();
    void shiftTransposition(int delta);

    void checkForCompanionAudio(const QString &chordProPath);
    void selectAudioTrack(QPushButton *clickedButton, const QString &filePath, const QString &trackType);

    AudioTracks m_audioTracks;
    QString m_selectedAudioPath; // The active track waiting for the Play button

    QMediaPlayer *m_mediaPlayer = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);

    // A simple container to hold individual song sections as we extract them
    struct ChordProSection {
        QString header; // e.g., "Verse 1", "Chorus"
        QString body;   // The raw or intermediate text content inside that section
    };
    // Slices raw text into visual HTML section pillars
    void parseChordProToGrid(const QString &rawText);

    // Stores the fully prepared HTML layout grid for full-screen display
    QString m_parsedSongContentGrid;

    // Master wrapper that injects CSS flexbox styling for full-screen mode
    QString generateFullScreenHtml(const QString& parsedSongContent);

    int m_zoomScaleLevel; // Tracks current zoom step (e.g., -3 to +5)

    void onZoomInTriggered(); // UI Action Slots for your Zoom Buttons
    void onZoomOutTriggered();

    QString runInitialParse(const QString &rawInput);
    QString processLineContent(const QString &line);
    QString transposeChord(const QString &chord, int semitones);

    QString getThemeStyles();
    QString m_rawSongContent; // Stores the master text copy

    // UI Layout Components
    QSplitter *mainSplitter;
    QPlainTextEdit *originalEditor;
    QTextEdit *parsedEditor;

    // Control Buttons
    QPushButton *m_btnTransposeUp;
    QPushButton *m_btnTransposeDown;
    QPushButton *m_btnTheme;
    QPushButton *m_viewToggleBtn;

    // Audio Selection Buttons
    QPushButton *m_btnTrackFull;
    QPushButton *m_btnTrackBkg;
    QPushButton *m_btnTrackSlow;

    // The 4 Context-Aware Function Buttons
    QPushButton *m_btnFn1;
    QPushButton *m_btnFn2;
    QPushButton *m_btnFn3;
    QPushButton *m_btnFn4;

    // Internal State Variables
    AppState currentState;
    ChordDisplayMode m_currentMode = CAL; // Set to chords above lyrics (CAL) as default expert mode
    Theme m_currentTheme = Light; // Or Light, depending on your default choice
    int m_transposeShift = 0;
    QString m_currentFilePath;
};

#endif // MAINWINDOW_H