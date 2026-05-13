#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>

class MainWindow : public QMainWindow {
    Q_OBJECT

enum class ChordDisplayMode {
    CIL, // Chord-In-Line (Standard ChordPro)
    CAL  // Chord-Above-Lyrics (Traditional Lead Sheet)
};

public:
    enum AppState { Idle, OpenEdit, PlayAlong };

    enum AppTheme { Light, Dark, Sepia };

    MainWindow(QWidget *parent = nullptr);
    void setAppState(AppState state);

private slots:
    void handleFileOpen();
    void handleFileSave();
    void toggleTheme();

private:
    void setupMenus();
    void setupLayout();
    void setupToolBar();
    void toggleDisplayMode();
    void shiftTransposition(int delta);

    QString runInitialParse(const QString &rawInput);
    QString transposeChord(const QString &chord, int semitones);
    QString processLineContent(const QString &line);

    ChordDisplayMode m_currentMode = ChordDisplayMode::CIL;

    // UI Elements
    QSplitter *mainSplitter;
    QPlainTextEdit *originalEditor;
    QTextEdit *parsedEditor;
    QLabel *statusLabel;

    // Toolbar Widgets (Declare here for global class access)
    int m_transposeShift = 0; // Tracks the current semitone offset
    QPushButton *m_btnTransposeUp;
    QPushButton *m_btnTransposeDown;

    QPushButton *m_viewToggleBtn; // Use m_ prefix for member variables
    QPushButton *m_opt2Btn;       // Future-proofing your blue buttons
    QPushButton *m_opt3Btn;
    QPushButton *m_opt4Btn;
    // Future buttons: m_themeBtn, m_listBtn, etc.
    QPushButton *m_btnTheme;

    // A helper to get the current CSS based on the theme
    QString getThemeStyles();

    AppTheme m_currentTheme = Light;
    AppState currentState;
};

#endif