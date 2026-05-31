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
#include <QKeyEvent>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
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
        QString slowPath;
    };

    enum Theme {
        Light,
        Dark
    };

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

    // ChoPro file metrics storage module
    struct SongLayoutMetrics {
        int totalLines = 0;          // cil_lof: Chords-in-line total line count of the file
        int maxLineCharacters = 0;   // cil_wll: Character width of the single longest line
        int sectionCount = 0;        // Total number of sections (Verses, Choruses, etc.)
        int targetColumns = 1;       // Auto-calculated initial columns layout rule (1, 2, or 3)
    };

protected:
    // This allows intercepting the Spacebar and Zoom keys anywhere inside the window frame
    void keyPressEvent(QKeyEvent *event) override;

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

    void setAppState(AppState state);
    void updateFunctionKeys();
    void shiftTransposition(int delta);

    void checkForCompanionAudio(const QString &chordProPath);
    void selectAudioTrack(QPushButton *clickedButton, const QString &filePath, const QString &trackType);

    AudioTracks m_audioTracks;
    QString m_selectedAudioPath;

    QMediaPlayer *m_mediaPlayer = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);

    struct ChordProSection {
        QString header;
        QString body;
    };

    SongLayoutMetrics m_currentSongMetrics; // Persistent tracker variable for active file context

    void analyzeChordProMetaData(const QString &rawInput); // The predictive pre-analysis engine

    void parseChordProToGrid(const QString &rawText);
    void updatePlayAlongLayoutDensity();

    QString m_parsedSongContentGrid;

    int m_zoomScaleLevel;

    void onZoomInTriggered();
    void onZoomOutTriggered();

    bool m_isLoadingFile = false;

    QString runInitialParse(const QString &rawInput);
    QString processLineContent(const QString &line);
    QString transposeChord(const QString &chord, int semitones);

    QString getThemeStyles();
    QString m_rawSongContent;

    // UI Layout Components
    QSplitter *mainSplitter;
    QPlainTextEdit *originalEditor;
    QTextEdit *parsedEditor;

    // Control Buttons
    QPushButton *m_btnTransposeUp;
    QPushButton *m_btnTransposeDown;
    QPushButton *m_btnTheme;
    QPushButton *m_viewToggleBtn;
    QPushButton *m_btnModeToggle;

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
    ChordDisplayMode m_currentMode = CAL;
    Theme m_currentTheme = Light;
    int m_transposeShift = 0;
    QString m_currentFilePath;
};

#endif // MAINWINDOW_H