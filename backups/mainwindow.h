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

    MainWindow(QWidget *parent = nullptr);
    void setAppState(AppState state);

private slots:
    void handleFileOpen();
    void handleFileSave();

private:
    void setupMenus();
    void setupLayout();
    void setupToolBar();
    void toggleDisplayMode();
    QString runInitialParse(const QString &rawInput);

    ChordDisplayMode m_currentMode = ChordDisplayMode::CIL;

    // UI Elements
    QSplitter *mainSplitter;
    QPlainTextEdit *originalEditor;
    QTextEdit *parsedEditor;
    QLabel *statusLabel;

    // Toolbar Widgets (Declare here for global class access)
    QPushButton *m_viewToggleBtn; // Use m_ prefix for member variables
    QPushButton *m_opt2Btn;       // Future-proofing your blue buttons
    QPushButton *m_opt3Btn;
    QPushButton *m_opt4Btn;
    // Future buttons: m_themeBtn, m_listBtn, etc.

    AppState currentState;
};

#endif