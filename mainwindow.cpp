#include "MainWindow.h"
#include <QMenuBar>
#include <QToolBar>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStatusBar>
#include <QGuiApplication>
#include <QScreen>
#include <QFileDialog>
#include <QFile>
#include <QString>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTextCharFormat>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QMediaPlayer>
#include <QtMath>
#include <QSettings>
#include <QMenu>
#include <QActionGroup>
#include <QTimer>
#include <qapplication.h>

static const QStringList NOTE_SCALE_SHARPS = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const QStringList NOTE_SCALE_FLATS  = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // Core Hardware & Audio Engine Instantiation First
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8f);

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::handlePlaybackStateChanged);

    setWindowTitle("ChordLab V001A");
    QIcon icon(":/resources/icons/CL-icon.ico");
    setWindowIcon(icon);

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QCoreApplication::setOrganizationName("AldoMusic");
    QCoreApplication::setApplicationName("ChordLab");

    QSettings settings;
    QString savedStyle = settings.value("theme/lastStyle").toString();

    // Clear Internal State Variables Before UI Draws
    m_currentFilePath = "";
    m_parsedSongContentGrid = "";
    m_zoomScaleLevel = 0;
    m_transposeShift = 0;
    m_capoShift = 0;                  // default to open neck
    m_instrumentTuningOffset = 0;     // default to standard guitar tuning
    m_debugTelemetryEnabled = false;   // enable/disable tracking telemetry out to Qt App Output window
    m_debugVerboseLevel = true;
    m_isLoadingFile = false;
    m_currentMode = ChordDisplayMode::CIL;
    m_currentTheme = Theme::Light;

    // Sequential UI Component Construction
    setupMenus();
    setupLayout(); // Prepares central widgets and splitters
    setupToolBar();

    // --- Set App Window Style & Theme ---
    if (!savedStyle.isEmpty() && QFile::exists(savedStyle)) {
        loadStyleSheetFromFile(savedStyle);
    } else {
        QString fallbackStyle = (":/resources/styles/Adaptic(default).qss");
        if (QFile::exists(fallbackStyle)) {
            loadStyleSheetFromFile(fallbackStyle);
        } else {
            qDebug() << "[Theme] No saved or fallback style found. Using default styling.";
        }
    }

    // --- Set App Window Metric Policy (Check settings first, fallback to screen metric) ---
    if (settings.contains("window/geometry")) {
        restoreGeometry(settings.value("window/geometry").toByteArray());
        restoreState(settings.value("window/state").toByteArray());
        if (m_debugVerboseLevel) {
            qDebug() << "MainWindow: Successfully restored window footprint and state.";
        }
    } else {
        // Fallback layout ONLY if it's the very first time launching ChordLab
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            QSize size = screen->availableGeometry().size();
            resize(size.width() * 0.6, size.height() * 0.6);
        } else {
            resize(1024, 768); // Baseline hard fallback if screen geometries can't be fetched
        }
        if (m_debugVerboseLevel) {
            qDebug() << "MainWindow: No saved geometry found. Defaulted to initial screen footprint.";
        }
    }

    // --- Set App Window Metric Policy ---
    if (settings.contains("window/geometry")) {
        restoreGeometry(settings.value("window/geometry").toByteArray());
        restoreState(settings.value("window/state").toByteArray());
        if (m_debugVerboseLevel) {
            qDebug() << "MainWindow: Successfully restored window footprint and state.";
        }
    } else {
        // Pure fallback for the absolute first-time launch
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            QSize size = screen->availableGeometry().size();
            resize(size.width() * 0.6, size.height() * 0.6);
        }
    }

    // Explicitly make sure NO hard minimum size constraints are left locking the widget layout
    this->setMinimumSize(0, 0);
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;

        QSettings settings;

        if (m_debugVerboseLevel) {
            qDebug() << "[ShowEvent Initialized] Actual Width:" << this->width() << "Height:" << this->height();
        }

        if (!isMaximized() && this->height() < 600) {
            this->resize(this->width() > 1024 ? this->width() : 1024, 768);
        }

        // 🚀 THE CURE: Defer loading song structures by 50ms!
        // This lets the OS window manager completely finish sizing the central splitter
        // and central widgets, ensuring the Radar reads the true physical layout width!
        QTimer::singleShot(50, this, [=]() {
            QSettings delayedSettings;
            QString lastFile = delayedSettings.value("app/lastOpenedFile").toString();
            int lastMode = delayedSettings.value("app/lastMode", static_cast<int>(PlayAlong)).toInt();

            if (!lastFile.isEmpty() && QFile::exists(lastFile)) {
                loadSongQuietly(lastFile);
                setAppState(static_cast<AppState>(lastMode));
            } else {
                setAppState(Idle);
            }
        });
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);

    // 🚀 LIVE LAYOUT LINK: If the user maximizes or manually resizes the frame,
    // re-trigger the engine calculations instantly to adjust columns live!
    if (currentState == PlayAlong && !m_rawSongContent.isEmpty() && !m_isLoadingFile) {
        parseChordProToGrid(m_rawSongContent);
    }
}

void MainWindow::setupMenus() {
    // --- File Menu ---
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Open...", QKeySequence::Open, this, &MainWindow::handleFileOpen);
    fileMenu->addAction("&Save Standardized", QKeySequence::Save, this, &MainWindow::handleFileSave);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close);

    QMenu *prefMenu = menuBar()->addMenu("&Preferences");

    // Add sub-menus and SAVE THE POINTERS
    prefMenu->addMenu("Settings and Preferences");
    fileMenu->addSeparator();
    QMenu *themeMenu = prefMenu->addMenu("Themes");

    QActionGroup *prefGroup = new QActionGroup(this);
    prefGroup->setExclusive(true);

    QDir themeDir(":/resources/styles");
    QStringList themes = themeDir.entryList(QStringList() << "*.qss", QDir::Files);

    // If the directory doesn't exist or is empty, this will be empty
    if (themes.isEmpty()) {
        qDebug() << "[Theme] No themes found in" << themeDir.absolutePath();
    }

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    helpMenu->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction(QIcon(":/resources/icons/about.png"), tr("About"), this, [this]() {
        QMessageBox::about(this, "About QPlayer", "ChordLab\nVersion 1a\n15-Jun-2026...\nA chordpro multimedia app \nfor musicians.");
    });

    QSettings settings;
    QString currentPath = settings.value("theme/lastStyle").toString();
    qDebug() << "Available resources at :/resources/styles:" << themeDir.entryList();
    for (const QString &themeFile : themes) {
        QString fullPath = ":/resources/styles/" + themeFile;
        QString menuName = themeFile;
        menuName.remove(".qss");

        // ADD TO themeMenu, NOT prefMenu
        QAction *action = themeMenu->addAction(menuName);
        action->setCheckable(true);
        action->setActionGroup(prefGroup);

        if (fullPath == currentPath) {
            action->setChecked(true);
        }

        connect(action, &QAction::triggered, this, [=]() {
            loadStyleSheetFromFile(fullPath);
        });
    }
}

void MainWindow::setupToolBar() {
    QToolBar *settingsToolBar = addToolBar("Critical Settings");
    settingsToolBar->setObjectName("criticalSettingsToolBar");

    m_btnTransposeUp = new QPushButton("Key Up");
    m_btnTransposeDown = new QPushButton("Key Down");
    m_btnTheme = new QPushButton("Light theme...");
    m_btnTheme->setStyleSheet("QPushButton { background-color: #0047AB; color: white; padding: 5px; min-width: 80px; }");

    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(10);

    QString style = "QPushButton { background-color: #004060; color: white; padding: 5px; min-width: 80px; }";
    m_btnTransposeUp->setStyleSheet(style);
    m_btnTransposeDown->setStyleSheet(style);

    QString buttonStyle = "QPushButton { background-color: #555555; color: white; border-radius: 4px; padding: 5px; min-width: 65px; font-weight: bold; }";
    m_btnFn1 = new QPushButton("Fn-1");
    m_btnFn2 = new QPushButton("Fn-2");
    m_btnFn3 = new QPushButton("Fn-3");
    m_btnFn4 = new QPushButton("Fn-4");

    m_btnFn1->setStyleSheet(buttonStyle);
    m_btnFn2->setStyleSheet(buttonStyle);
    m_btnFn3->setStyleSheet(buttonStyle);
    m_btnFn4->setStyleSheet(buttonStyle);

    layout->addWidget(m_btnFn1);
    layout->addWidget(m_btnFn2);
    layout->addWidget(m_btnFn3);
    layout->addWidget(m_btnFn4);

    QString audioBtnStyle = "QPushButton { background-color: #2D2D2D; color: #777777; border: 1px solid #444; border-radius: 4px; padding: 5px; min-width: 50px; font-weight: bold; }"
                            "QPushButton:enabled { color: #E0E0E0; border-color: #555; }"
                            "QPushButton:checked { background-color: #006644; color: white; border-color: #00FF88; }";
    m_btnTrackFull = new QPushButton("Full");
    m_btnTrackBkg  = new QPushButton("Bkg");
    m_btnTrackSlow = new QPushButton("Slow");

    m_btnTrackFull->setCheckable(true);
    m_btnTrackBkg->setCheckable(true);
    m_btnTrackSlow->setCheckable(true);

    m_btnTrackFull->setStyleSheet(audioBtnStyle);
    m_btnTrackBkg->setStyleSheet(audioBtnStyle);
    m_btnTrackSlow->setStyleSheet(audioBtnStyle);

    m_btnTrackFull->setEnabled(false);
    m_btnTrackBkg->setEnabled(false);
    m_btnTrackSlow->setEnabled(false);

    layout->addWidget(m_btnTrackFull);
    layout->addWidget(m_btnTrackBkg);
    layout->addWidget(m_btnTrackSlow);

    connect(m_btnTrackFull, &QPushButton::clicked, this, [=]() { selectAudioTrack(m_btnTrackFull, m_audioTracks.fullPath, "Full Track"); });
    connect(m_btnTrackBkg,  &QPushButton::clicked, this, [=]() { selectAudioTrack(m_btnTrackBkg, m_audioTracks.backingPath, "Backing Track"); });
    connect(m_btnTrackSlow, &QPushButton::clicked, this, [=]() { selectAudioTrack(m_btnTrackSlow, m_audioTracks.slowPath, "Slow Practice Track"); });

    QPushButton *btnReset = new QPushButton("Reset Key");
    btnReset->setStyleSheet("QPushButton { background-color: #0047ff; color: white; padding: 5px; }");
    connect(btnReset, &QPushButton::clicked, this, [=](){
        m_transposeShift = 0;
        statusBar()->showMessage("Key Reset to Original");
        if (currentState == PlayAlong) {
            parseChordProToGrid(originalEditor->toPlainText());
        } else {
            parsedEditor->setHtml(runInitialParse(originalEditor->toPlainText()));
        }
    });

    m_btnModeToggle = new QPushButton("Mode: Edit 📝");
    m_btnModeToggle->setStyleSheet("QPushButton { background-color: #004060; color: white; padding: 5px; min-width: 100px; }");
    connect(m_btnModeToggle, &QPushButton::clicked, this, &MainWindow::togglePlaybackMode);
    layout->addWidget(m_btnModeToggle);

    layout->addWidget(m_btnTheme);
    layout->addWidget(m_btnTransposeDown);
    layout->addWidget(btnReset);
    layout->addWidget(m_btnTransposeUp);

    settingsToolBar->addWidget(container);

    m_viewToggleBtn = new QPushButton("ChordPro-CIL");
    m_viewToggleBtn->setStyleSheet("QPushButton { background-color: #0047AB; color: white; }");
    connect(m_viewToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleDisplayMode);
    layout->addWidget(m_viewToggleBtn);

    connect(m_btnTransposeUp, &QPushButton::clicked, this, [=](){ shiftTransposition(1); });
    connect(m_btnTransposeDown, &QPushButton::clicked, this, [=](){ shiftTransposition(-1); });
    connect(m_btnTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);
}

void MainWindow::loadStyleSheetFromFile(const QString &filePath) {
    QFile file(filePath);
    if (m_debugVerboseLevel){
        qDebug() << "[MW l.189] loadstylesheet called with: " << filePath;
    }
    if (!QApplication::instance()) return;
    qApp->setStyleSheet(QString());
    if (file.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);

        QSettings settings;
        settings.setValue("theme/lastStyle", filePath);
    }
    if (m_debugVerboseLevel){
        qDebug() << "[Theme] Loaded:" << filePath;
    } else {
        qDebug() << "[Theme] Failed to load:" << filePath;
    }
}

void MainWindow::setupLayout() {
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    originalEditor = new QPlainTextEdit(this);
    originalEditor->setPlaceholderText("Original File Content...");
    QFont monoFont("Consolas", 10);
    originalEditor->setFont(monoFont);

    parsedEditor = new QTextEdit(this);
    parsedEditor->setPlaceholderText("Parsed & Standardized Output...");
    parsedEditor->setFont(monoFont);
    parsedEditor->setReadOnly(true);

    mainSplitter->addWidget(originalEditor);
    mainSplitter->addWidget(parsedEditor);

    connect(originalEditor, &QPlainTextEdit::textChanged, this, [=]() {
        if (m_isLoadingFile) return;
        m_rawSongContent = originalEditor->toPlainText();
        if (currentState == OpenEdit) {
            parsedEditor->setHtml(runInitialParse(m_rawSongContent));
        } else if (currentState == PlayAlong) {
            parseChordProToGrid(m_rawSongContent);
        }
    });
    setCentralWidget(mainSplitter);
}

void MainWindow::togglePlaybackMode() {
    if (currentState == Idle) {
        statusBar()->showMessage("Please open a song file before entering Play-Along mode.");
        return;
    }
    if (currentState == OpenEdit) {
        setAppState(PlayAlong);
    } else {
        setAppState(OpenEdit);
    }
}

void MainWindow::setAppState(AppState state) {
    currentState = state;
    updateFunctionKeys();

    switch(state) {
    case Idle:
        m_btnModeToggle->setText("Mode: Edit 📝");
        statusBar()->showMessage("Ready. Open a ChordPro file to begin.");
        mainSplitter->hide();
        break;

    case OpenEdit:
        m_btnModeToggle->setText("Mode: Edit 📝");
        statusBar()->showMessage("Editing Mode📝: Analyzing ChordPro syntax...");

        // Show split panels
        mainSplitter->show();
        originalEditor->show();
        parsedEditor->show();

        // 🚀 Dynamically split the screen 50/50 instead of forcing 400px
        mainSplitter->setSizes(QList<int>({this->width() / 2, this->width() / 2}));
        // Render crisp editor text layout without multi-column table interference
        parsedEditor->setHtml(runInitialParse(m_rawSongContent));
        break;

    case PlayAlong:
        m_btnModeToggle->setText("Mode: Play 🎤");
        statusBar()->showMessage("Play-along Mode 🎤Active. Space: Toggle Play | Ctrl +/-: Zoom");

        // Maximize output window surface
        originalEditor->hide();
        parsedEditor->show();
        mainSplitter->show();
        mainSplitter->setSizes(QList<int>({0, this->width()}));

        // Trigger multi-column generation specifically for performance tracking
        parseChordProToGrid(m_rawSongContent);
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings;

    // Save the window size and desktop location
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());

    // NEW: Save the active song file path!
    if (!m_currentFilePath.isEmpty()) {
        settings.setValue("app/lastOpenedFile", m_currentFilePath);
    }
    // 🚀 Save the active mode at "exit-time"
    settings.setValue("app/lastMode", static_cast<int>(currentState));
    QMainWindow::closeEvent(event);
}

void MainWindow::loadSongQuietly(const QString &filePath) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) return;

    m_isLoadingFile = true;
    m_currentFilePath = filePath;

    // --- 🚀 RECALL PER-SONG TRACKING PREFERENCE ---
    // Read the song's specific layout preference block before generating the display data
    loadSongLayoutPreference(filePath);

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        m_rawSongContent = in.readAll();
        originalEditor->setPlainText(m_rawSongContent);
        file.close();
    }

    m_isLoadingFile = false;
}
void MainWindow::updateFunctionKeys() {
    // 1. Break old signal routing tables cleanly
    disconnect(m_btnFn1, &QPushButton::clicked, nullptr, nullptr);
    disconnect(m_btnFn2, &QPushButton::clicked, nullptr, nullptr);
    disconnect(m_btnFn3, &QPushButton::clicked, nullptr, nullptr);
    disconnect(m_btnFn4, &QPushButton::clicked, nullptr, nullptr);

    // 2. High-readability local layout engine helper
    auto configureButton = [](QPushButton *btn, const QString &iconPath, const QString &fallbackText) {
        if (!iconPath.isEmpty() && QFile::exists(iconPath)) {
            btn->setIcon(QIcon(iconPath));
            btn->setText("");              // Clear textual representation to keep icons square
            btn->setToolTip(fallbackText); // Accessible user-friendly context tooltips
        } else {
            btn->setIcon(QIcon());          // Explicitly purge icon allocations
            btn->setText(fallbackText);     // Fallback to text emojis seamlessly
            btn->setToolTip("");
        }
    };

    // 3. Render Mode Layout Switchboard
    if (currentState == OpenEdit || currentState == Idle) {
        configureButton(m_btnFn1, "", "📂 Open");
        configureButton(m_btnFn2, "", "💾 Save");
        configureButton(m_btnFn3, "", "💾 As...");
        configureButton(m_btnFn4, "", "🔄 Parse");

        connect(m_btnFn1, &QPushButton::clicked, this, &MainWindow::handleFileOpen);
        connect(m_btnFn2, &QPushButton::clicked, this, &MainWindow::handleFileSave);
        connect(m_btnFn3, &QPushButton::clicked, this, [=]() {
            QString oldPath = m_currentFilePath;
            m_currentFilePath.clear();
            handleFileSave();
            if (m_currentFilePath.isEmpty()) m_currentFilePath = oldPath;
        });
        connect(m_btnFn4, &QPushButton::clicked, this, [=]() {
            parsedEditor->setHtml(runInitialParse(originalEditor->toPlainText()));
        });

    } else if (currentState == PlayAlong) {
        configureButton(m_btnFn1, ":/resources/icons/rewindtostart.png", "⏮ Rewind");

        // Real-time tracking of active audio driver playback engine state
        if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            configureButton(m_btnFn2, ":/resources/icons/pause.png", "⏸ Pause");
        } else {
            configureButton(m_btnFn2, ":/resources/icons/play.png", "▶ Play");
        }

        configureButton(m_btnFn3, ":/resources/icons/stop.png", "■ Stop");
        configureButton(m_btnFn4, ":/resources/icons/fastfwdtoend.png", "⏭ Skip");

        // Action routing allocations
        connect(m_btnFn1, &QPushButton::clicked, this, [=]() {
            m_mediaPlayer->setPosition(0);
            statusBar()->showMessage("Rewound to beginning.");
        });

        connect(m_btnFn2, &QPushButton::clicked, this, [=]() {
            if (m_selectedAudioPath.isEmpty()) {
                statusBar()->showMessage("No audio track selected or available for playback.");
                return;
            }
            if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
                m_mediaPlayer->pause();
            } else {
                m_mediaPlayer->play();
            }
        });

        connect(m_btnFn3, &QPushButton::clicked, this, [=]() {
            m_mediaPlayer->stop();
        });

        connect(m_btnFn4, &QPushButton::clicked, this, [=]() {
            m_mediaPlayer->setPosition(m_mediaPlayer->duration());
        });
    }
}

void MainWindow::handleFileOpen() {
    // Determine the location where the executable is currently running
    QString appDir = QCoreApplication::applicationDirPath();

    // Look for a local 'resources/pieces' directory relative to the build/installation folder
    QString initialPath = appDir + "/resources/pieces";

    // Fallback gracefully if that folder structure doesn't exist yet
    if (!QDir(initialPath).exists()) {
        initialPath = QDir::homePath(); // fallback to standard system User Documents folder
    }

    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open ChordPro File"),
        initialPath, // 🚀 Automatically spins up inside your targeted pieces directory!
        tr("ChordPro Files (*.chopro *.cho *.pro *.txt *.crd)")
        );

    if (fileName.isEmpty()) return;

    m_currentFilePath = fileName;

    // 🚀 RESET COUNTERS: Prevent cumulative transposition states on new files
    m_transposeShift = 0;
    m_capoShift = 0;
    m_instrumentTuningOffset = 0;

    // Clear status bar indicators if you have them displayed on screen
    statusBar()->showMessage("Loaded new song. Transposition tracking reset to 0.");

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file: ") + file.errorString());
        return;
        // Render out the baseline layout cleanly
        parsedEditor->setHtml(runInitialParse(m_rawSongContent));
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    m_rawSongContent = content;

    m_isLoadingFile = true; // Block signal loops safely
    originalEditor->setPlainText(content);
    m_isLoadingFile = false; // RE-ENABLE safely (fixed bug here)

    loadSongLayoutPreference(fileName);
    // Set state explicitly handles rendering separation cleanly
    setAppState(OpenEdit);

    checkForCompanionAudio(m_currentFilePath);
    statusBar()->showMessage(tr("Loaded: ") + fileName);
}

void MainWindow::handleFileSave() {
    if (m_currentFilePath.isEmpty()) {
        // 1. Determine the path to the running executable
        QString appDir = QCoreApplication::applicationDirPath();

        // 2. Point to our standard user workspace folder
        QString initialPath = appDir + "/resources/pieces";

        // 3. Fallback safely if the workspace directory isn't deployed yet
        if (!QDir(initialPath).exists()) {
            initialPath = QDir::homePath();
        }

        // 🚀 Synchronized: Opens the Save Dialog right in the same folder as Open Dialog!
        QString saveName = QFileDialog::getSaveFileName(
            this,
            tr("Save ChordPro File"),
            initialPath,
            tr("ChordPro Files (*.chopro *.cho *.pro *.txt *.crd)")
            );

        if (saveName.isEmpty()) return;
        m_currentFilePath = saveName;
    }

    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save Error"), tr("Could not write to file: ") + file.errorString());
        return;
    }

    QTextStream out(&file);
    out << originalEditor->toPlainText();
    file.close();

    // Show the actual file name cleanly in the status bar
    statusBar()->showMessage(tr("Saved successfully: ") + QFileInfo(m_currentFilePath).fileName(), 3000);
}

QString MainWindow::runInitialParse(const QString &rawInput) {
    // Calculate translation offsets explicitly
    int chordShift = m_transposeShift;
    int instrumentShift = -(m_capoShift + m_instrumentTuningOffset);

    if (m_debugTelemetryEnabled) {
        qDebug() << "\n--- CHORDLAB TRANSPOSE ENGINE TELEMETRY ---";
        qDebug() << "Concert Transpose Offset (Chords):" << chordShift;
        qDebug() << "Instrument Structural Shift (Tabs/Capo/Tuning):" << instrumentShift;
    }

    QString result = "<html><head><style>"
                     "body { font-family: 'Consolas', monospace; white-space: pre; font-size: 10pt; }"
                     + getThemeStyles() +
                     "</style></head><body>";

    // Extract Metadata Block Headers
    QRegularExpression titleRegex(R"(\{(?:title|t):\s*(.*?)\})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression artistRegex("\\{artist:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression keyRegex("\\{key:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression tempoRegex("\\{tempo:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression capoRegex("\\{capo:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);

    QString title  = titleRegex.match(rawInput).captured(1).trimmed();
    QString artist = artistRegex.match(rawInput).captured(1).trimmed();
    QString key    = keyRegex.match(rawInput).captured(1).trimmed();
    QString tempo  = tempoRegex.match(rawInput).captured(1).trimmed();
    QString capo   = capoRegex.match(rawInput).captured(1).trimmed();

    if (!title.isEmpty()) {
        result += "<div class='header-block'>";
        result += "<h1 style='margin:0; font-size: 16pt;'>" + title + "</h1>";
        if (!artist.isEmpty()) result += "Artist: <b>" + artist + "</b><br>";
        if (!key.isEmpty())    result += "Key: <b>" + key + "</b><br>";
        if (!tempo.isEmpty())  result += "Tempo: <b>" + tempo + "</b><br>";
        if (!capo.isEmpty())   result += "Capo: <b>" + capo + "</b>";
        result += "</div><br>"; // 🚀 Added explicit break after metadata block
    }

    QStringList lines = rawInput.split('\n');
    bool inTabBlock = false;
    bool inGridBlock = false;
    bool inProtectedBlock = false;
    bool lastLineWasEmpty = false;
    bool inChorus = false;
    bool inVerse = false;

    for (const QString &line : lines) {
        QString workingLine = line;
        workingLine.remove('\r');
        QString trimmedLine = workingLine.trimmed();

        QString safeInput = rawInput;
        safeInput.replace(QChar(0xA0), ' ');

        if (trimmedLine.startsWith("{capo:", Qt::CaseInsensitive)) {
            QRegularExpression rx("^\\{capo:\\s*([^}]*)\\}?", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = rx.match(trimmedLine);
            if (match.hasMatch()) {
                m_capoShift = match.captured(1).trimmed().toInt();
            }
        }

        // --- Block Boundary Interceptions ---
        if (trimmedLine.startsWith("{start_of_tab}") || trimmedLine.startsWith("{sot}")) {
            inTabBlock = true;
            // wrap block labels in standard layout lines followed by explicit HTML line breaks
            result += "<div style='font-family:sans-serif; color:#008800; font-weight:bold;'>[Tablature Block Start]</div><br>";
            continue;
        }
        if (trimmedLine.startsWith("{end_of_tab}") || trimmedLine.startsWith("{eot}")) {
            inTabBlock = false;
            result += "<div style='font-family:sans-serif; color:#008800; font-weight:bold;'>[Tablature Block End]</div><br>";
            continue;
        }
        if (trimmedLine.startsWith("{start_of_grid}") || trimmedLine.startsWith("{sog}")) {
            inGridBlock = true;
            result += "<div style='font-family:sans-serif; color:#880088; font-weight:bold;'>[Chord Grid Block Start]</div><br>";
            continue;
        }
        if (trimmedLine.startsWith("{end_of_grid}") || trimmedLine.startsWith("{eog}")) {
            inGridBlock = false;
            result += "<div style='font-family:sans-serif; color:#880088; font-weight:bold;'>[Chord Grid Block End]</div><br>";
            continue;
        }

        // --- Specialized Rendering Engine Modules ---
        if (inTabBlock) {
            // pass chordShift and instrumentShift directly to the tab line processing module
            QString processedTab = parseTabLine(workingLine, chordShift, instrumentShift);
            if (m_debugTelemetryEnabled) qDebug() << "[TAB Engine]:" << workingLine.trimmed() << " -> [RENDER]:" << processedTab;
            result += processedTab + "<br>";
            continue;
        }

        if (inGridBlock) {
            QString processedGrid = parseGridLine(workingLine, chordShift);
            if (m_debugTelemetryEnabled) qDebug() << "[GRID Engine]:" << workingLine.trimmed() << " -> [RENDER]:" << processedGrid;
            result += processedGrid + "<br>";
            continue;
        }

        // --- Standard ChordPro Structural Content Parsing ---
        if (trimmedLine.startsWith("{title:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{t:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{artist:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{key:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{tempo:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{capo:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("#")) {
            continue;
        }

        if (trimmedLine.isEmpty()) {
            if (lastLineWasEmpty) continue;
            lastLineWasEmpty = true;
            continue;
        }
        lastLineWasEmpty = false;

        bool isChorusStart = trimmedLine.startsWith("{start_of_chorus}") ||
                             trimmedLine.startsWith("{soc") ||
                             (trimmedLine.startsWith("{c:") && trimmedLine.contains("Chorus", Qt::CaseInsensitive));

        bool lineHandled = false;

        if (isChorusStart) {
            if (inVerse) { result += "</div>"; inVerse = false; }
            inChorus = true;
            result += "<div class='chorus-box'><span class='section-header'>Chorus</span><br>";
            lineHandled = true;
        }
        else if (trimmedLine.startsWith("{c:") || trimmedLine.startsWith("{comment:")) {
            if (inVerse) result += "</div>";
            if (inChorus) { result += "</div>"; inChorus = false; }

            // Safe Regex Extraction
            QRegularExpression rx("^\\{c(?:omment)?:?\\s*([^}]*)\\}?", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = rx.match(trimmedLine);

            QString commentText = "..."; // Prevents empty layout collapse while typing
            if (match.hasMatch() && !match.captured(1).trimmed().isEmpty()) {
                commentText = match.captured(1).trimmed();
            }

            inVerse = true;
            result += "<div class='verse-box'><span class='section-header'>" + commentText + "</span><br>";
            lineHandled = true;
        }

        else if (trimmedLine.startsWith("{sot")) { inProtectedBlock = true; lineHandled = true; }
        else if (trimmedLine.startsWith("{eot")) { inProtectedBlock = false; lineHandled = true; }

        if (!lineHandled) {
            if (inProtectedBlock) {
                result += workingLine + "<br>";
            }
            else if (inChorus) {
                result += processLineContent(workingLine) + "<br>";
            }
            else {
                if (!inVerse) {
                    inVerse = true;
                    result += "<div class='verse-box'>";
                }
                result += processLineContent(workingLine) + "<br>";
            }
        }
    }

    if (inVerse || inChorus) {
        result += "</div>";
    }

    result += "</body></html>";
    return result;
}

void MainWindow::toggleDisplayMode() {
    m_currentMode = (m_currentMode == ChordDisplayMode::CIL) ? ChordDisplayMode::CAL : ChordDisplayMode::CIL;
    m_viewToggleBtn->setText(m_currentMode == ChordDisplayMode::CIL ? "ChordPro-CIL" : "LeadSheet-CAL");

    if (currentState == PlayAlong) {
        parseChordProToGrid(m_rawSongContent);
    } else {
        parsedEditor->setHtml(runInitialParse(m_rawSongContent));
    }
}

void MainWindow::shiftTransposition(int delta) {
    m_transposeShift += delta;
    if (m_transposeShift >= 12 || m_transposeShift <= -12) {
        m_transposeShift = 0;
    }

    statusBar()->showMessage(QString("Transposition: %1 semitones").arg(m_transposeShift));

    if (currentState == PlayAlong) {
        parseChordProToGrid(m_rawSongContent);
    } else {
        // process the current live string text data
        QString currentLiveText = originalEditor->toPlainText();
        parsedEditor->setHtml(runInitialParse(currentLiveText));
    }
}

void MainWindow::toggleTheme() {
    m_currentTheme = (m_currentTheme == Light) ? Dark : Light;
    m_btnTheme->setText(m_currentTheme == Light ? "Light theme" : "Dark theme");
    if (currentState == PlayAlong) {
        parseChordProToGrid(m_rawSongContent);
    } else {
        parsedEditor->setHtml(runInitialParse(m_rawSongContent));
    }
}

void MainWindow::parseChordProToGrid(const QString &rawInput) {
    analyzeChordProMetaData(rawInput);
    QStringList lines = rawInput.split('\n');
    QStringList gatheredSections;
    QString currentSectionHtml = "";
    bool insideSectionBlock = false;
    bool inGridBlock = false;
    bool inTabBlock = false;

    // Toggle this boolean flag to enable/disable your visual alignment spacer checkline!
    bool showDiagnosticCheckline = false;

    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        // 1. Grid & Tab State Toggles
        if (line.startsWith("{start_of_grid}") || line.startsWith("{sog}")) {
            inGridBlock = true;
            currentSectionHtml += "<div class='song-section'><div style='font-family: Consolas; white-space: pre; background: rgba(136, 0, 136, 0.1); padding: 5px; border-radius: 4px;'>";
            continue;
        }
        if (line.startsWith("{end_of_grid}") || line.startsWith("{eog}")) {
            inGridBlock = false;
            currentSectionHtml += "</div></div>";
            continue;
        }
        if (line.startsWith("{start_of_tab}") || line.startsWith("{sot}")) {
            inTabBlock = true;
            currentSectionHtml += "<div class='song-section'><div style='font-family: Consolas; white-space: pre; background: rgba(0, 136, 0, 0.1); padding: 5px; border-radius: 4px;'>";
            continue;
        }
        if (line.startsWith("{end_of_tab}") || line.startsWith("{eot}")) {
            inTabBlock = false;
            currentSectionHtml += "</div></div>";
            continue;
        }

        // 2. Specialized Block Routing (Bypass standard lyric logic)
        if (inGridBlock) {
            currentSectionHtml += parseGridLine(line, m_transposeShift) + "<br>";
            continue;
        }
        if (inTabBlock) {
            int instrumentShift = -(m_capoShift + m_instrumentTuningOffset);
            currentSectionHtml += parseTabLine(line, m_transposeShift, instrumentShift) + "<br>";
            continue;
        }

        // 3. Standard Section Headers
//        if (line.startsWith('{') && (line.contains("comment") || line.contains("c:"))) {


        if (line.startsWith('{') && (line.startsWith("{c:") || line.startsWith("{comment") || line.startsWith("{c}"))) {
            if (insideSectionBlock) {
                currentSectionHtml += "</div>";
                gatheredSections.append(currentSectionHtml);
                currentSectionHtml = "";
            }

            // Safe Regex Capture Group
            // This safely grabs anything between the colon and the closing bracket.
            // If the bracket is missing or the text is empty, it just returns an empty string without crashing!
            QRegularExpression rx("^\\{c(?:omment)?:?\\s*([^}]*)\\}?", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = rx.match(line);

            QString sectionName = "";
            if (match.hasMatch()) {
                sectionName = match.captured(1).trimmed();
            }

            // Fallback: If they type an empty {c:} or are half-way through typing {c:V
            if (sectionName.isEmpty()) {
                sectionName = "..."; // Keeps the UI stable while typing
            }
            currentSectionHtml += "<div class='song-section'>";
            currentSectionHtml += QString("<div class='section-heading'>%1</div>").arg(sectionName);
            insideSectionBlock = true;
            continue;
        }
        // is this the right one...
//        if (line.startsWith('{')) continue;

        if (insideSectionBlock) {
            QString chordLine = "";
            QString dashLine = "";  // Diagnostic marker row
            QString lyricLine = "";

            // Dual-Cursor Trackers tracking absolute character coordinates
            int visualChordCursor = 0;
            int visualLyricCursor = 0;
            int linePos = 0;

            QRegularExpression chordRegex("\\[(.*?)\\]");
            auto matches = chordRegex.globalMatch(line);

            while (matches.hasNext()) {
                auto match = matches.next();
                int matchStart = match.capturedStart();

                // 1. Process all text preceding the chord token
                while (linePos < matchStart) {
                    QChar ch = line[linePos++];
                    if (ch == ' ') lyricLine += "&nbsp;"; else lyricLine += ch;
                    visualLyricCursor++;

                    // Equalize chord cursor track with lyric track
                    while (visualChordCursor < visualLyricCursor) {
                        chordLine += "&nbsp;";
                        visualChordCursor++;
                    }
                }

                // 2. Parse and Drop Chord Token
                QString chordText = match.captured(1);
                QString transposed = transposeChord(chordText, m_transposeShift);
                int chordLen = transposed.length();

                // Catch up spacing track before drawing chord text anchor
                while (visualChordCursor < visualLyricCursor) {
                    chordLine += "&nbsp;";
                    visualChordCursor++;
                }

                chordLine += QString("<span class='chord-line'>%1</span>").arg(transposed);
                visualChordCursor += chordLen; // Jump the cursor forward by physical string length

                linePos = match.capturedEnd(); // Step past closing bracket
            }

            // 3. Process remaining trailing lyric strings
            while (linePos < line.length()) {
                QChar ch = line[linePos++];
                if (ch == ' ') lyricLine += "&nbsp;"; else lyricLine += ch;
                visualLyricCursor++;

                while (visualChordCursor < visualLyricCursor) {
                    chordLine += "&nbsp;";
                    visualChordCursor++;
                }
            }

            // 4. Equalize tail-end balance (handles trailing chords that extend past short lyric fragments)
            while (visualLyricCursor < visualChordCursor) {
                lyricLine += "&nbsp;";
                visualLyricCursor++;
            }

            // 5. Generate the User's Spacing Checkline
            if (showDiagnosticCheckline) {
                for (int d = 0; d < visualChordCursor; ++d) {
                    dashLine += "-";
                }
            }

            // Render output blocks matching structural monospaced font dimensions
            QString lineBlockHtml = "<div style='white-space: nowrap; line-height: 1.1;'>";

            if (!chordLine.trimmed().isEmpty()) {
                lineBlockHtml += QString("<p style='margin: 0; padding: 0;' class='chord-line'>%1</p>").arg(chordLine);
            } else {
                lineBlockHtml += "<p style='margin: 0; padding: 0;' class='chord-line'>&nbsp;</p>";
            }

            if (showDiagnosticCheckline && !chordLine.trimmed().isEmpty()) {
                lineBlockHtml += QString("<p style='margin: 0; padding: 0; color: #555555; font-size: 8pt;'>%1</p>").arg(dashLine);
            }

            lineBlockHtml += QString("<p style='margin: 0 0 8px 0; padding: 0;' class='lyric-text'>%1</p>").arg(lyricLine.isEmpty() ? "&nbsp;" : lyricLine);
            lineBlockHtml += "</div>";
            currentSectionHtml += lineBlockHtml;
        }
    }

    if (insideSectionBlock) {
        currentSectionHtml += "</div>";
        gatheredSections.append(currentSectionHtml);
    }

    // Establish dynamic column generation limits
    int numCols = m_currentSongMetrics.targetColumns;
    if (m_zoomScaleLevel == 1 && numCols > 2) numCols = 2;
    else if (m_zoomScaleLevel >= 2) numCols = 1;
    if (numCols < 1) numCols = 1;

    QString tableGridHtml = "";
    if (numCols <= 1 || gatheredSections.isEmpty()) {
        for (const QString &section : gatheredSections) {
            tableGridHtml += section;
        }
    } else {
        tableGridHtml = "<table width='100%' border='0' cellspacing='0' cellpadding='0'><tr valign='top'>";
        int sectionsPerCol = qCeil((double)gatheredSections.size() / numCols);
        int cellWidthPercentage = 100 / numCols;
        for (int col = 0; col < numCols; ++col) {
            QString rightPadding = (col < numCols - 1) ? "padding-right: 35px;" : "";
            tableGridHtml += QString("<td width='%1%' style='%2'>").arg(cellWidthPercentage).arg(rightPadding);

            for (int i = 0; i < sectionsPerCol; ++i) {
                int index = (col * sectionsPerCol) + i;
                if (index < gatheredSections.size()) {
                    tableGridHtml += gatheredSections[index];
                }
            }
            tableGridHtml += "</td>";
        }
        tableGridHtml += "</tr></table>";
    }

    m_parsedSongContentGrid = tableGridHtml;
    updatePlayAlongLayoutDensity();
}

void MainWindow::updatePlayAlongLayoutDensity() {
    // Standardizing baseline scale settings (Dropped to 10pt base down from 12pt to fix overflow issues)
    int baseFontSize = 10 + (m_zoomScaleLevel * 2);
    int headerMarginBottom = 4 + m_zoomScaleLevel;
    int sectionMarginBottom = 12 + (m_zoomScaleLevel * 3);
    double activeLineHeight = 1.15 + (m_zoomScaleLevel * 0.04);
    bool hasPlayModeStyleOverride = true;

    parsedEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    parsedEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // future User preference override might go here...
    QString bgColor = (m_currentTheme == Dark) ? "#20204f" : "#e0F0ff";
    QString txtColor = (m_currentTheme == Dark) ? "#E0E0E0" : "#222222";
    QString chordColor = (m_currentTheme == Dark) ? "#6080f0" : "#c22222";

    // 🚀 NEW: Extract Metadata for the Play Along Header safely
    QRegularExpression titleRx(R"(\{(?:title|t):\s*([^}]*)\})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression artistRx(R"(\{(?:artist|st|subtitle):\s*([^}]*)\})", QRegularExpression::CaseInsensitiveOption);
    QString title = titleRx.match(m_rawSongContent).captured(1).trimmed();
    QString artist = artistRx.match(m_rawSongContent).captured(1).trimmed();

    QString headerHtml = "";
    if (!title.isEmpty()) {
        headerHtml += "<div style='text-align: center; margin-bottom: 20px; padding-bottom: 10px; border-bottom: 2px solid " + chordColor + ";'>";
        headerHtml += "<h1 style='margin: 0; font-size: " + QString::number(baseFontSize + 8) + "pt; color: " + txtColor + ";'>" + title + "</h1>";
        if (!artist.isEmpty()) {
            headerHtml += "<h3 style='margin: 5px 0 0 0; font-size: " + QString::number(baseFontSize + 2) + "pt; color: #777; font-weight: normal;'>" + artist + "</h3>";
        }
        headerHtml += "</div>";
    }

    // Now build the base HTML
    QString baseHtml = "<html><head><style>"
                       "body { background-color: " + bgColor + "; color: " + txtColor + "; margin: 15px; padding: 0; }"

//    QString baseHtml = "<html><head><style>"
//                       "body {"
//                       // two lines
//                            "  background-color: " + bgColor + ";"
//                            "  color: " + txtColor + ";"
//                       "  margin: 10px; padding: 0;"
//                       "}"
                       "h1 { font-size: " + QString::number(baseFontSize + 4) + "pt; font-weight: bold; font-family: sans-serif; margin: 0 0 4px 0; }"
                       ".section-heading {"
                       "  font-size: " + QString::number(baseFontSize + 1) + "pt;"
                       // next line
                       "  color: #007acc;"
                       "  font-weight: bold;"
                       "  margin: 0 0 " + QString::number(headerMarginBottom) + "px 0;"
                       "  border-bottom: 2px solid #eef;"
                       "  font-family: sans-serif;"
                       "}"
                       ".song-section {"
                       "  margin-bottom: " + QString::number(sectionMarginBottom) + "px;"
                       "}"
                       ".chord-line {"
                       "  font-weight: bold;"
                        // next line
                       " color: " + chordColor + ";"
                       "  font-family: 'Consolas', 'Courier New', monospace !important;"
                       "  font-size: " + QString::number(baseFontSize) + "pt;"
                       "}"
                       ".lyric-text {"
                        // next line
                       "  color: " + txtColor + ";"
                       "  font-family: 'Consolas', 'Courier New', monospace !important;"
                       "  font-size: " + QString::number(baseFontSize) + "pt;"
                       "}"
                       "</style></head><body>"
                       "<div class='song-canvas' style='line-height: " + QString::number(activeLineHeight) + ";'>"
                       + headerHtml + m_parsedSongContentGrid +
                       "</div></body></html>";

//    hasPlayModeStyleOverride = false;

    parsedEditor->setHtml(baseHtml);
}

// Ensure missing helper hooks exist natively inside compilation unit
void MainWindow::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state) {
    switch (state) {
    case QMediaPlayer::PlayingState:
        statusBar()->showMessage(tr("▶ Playing: %1").arg(QFileInfo(m_selectedAudioPath).fileName()));
        break;
    case QMediaPlayer::PausedState:
        statusBar()->showMessage("⏸ Playback Paused.");
        break;
    case QMediaPlayer::StoppedState:
        if (m_mediaPlayer->position() >= m_mediaPlayer->duration() && m_mediaPlayer->duration() > 0) {
            statusBar()->showMessage("Track finished.");
        } else {
            statusBar()->showMessage("Playback Stopped.");
        }
        break;
    }
    updateFunctionKeys();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (currentState == PlayAlong) {
        if (event->key() == Qt::Key_Space) {
            if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
                m_mediaPlayer->pause();
            } else {
                if (!m_selectedAudioPath.isEmpty()) m_mediaPlayer->play();
            }
            event->accept();
            return;
        }

        if (event->modifiers() & Qt::ControlModifier) {
            if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
                onZoomInTriggered();
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_Minus) {
                onZoomOutTriggered();
                event->accept();
                return;
            }
            if (event->key() == Qt::Key_0) {
                m_zoomScaleLevel = 0;
                updatePlayAlongLayoutDensity();
                event->accept();
                return;
            }
        }
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::onZoomInTriggered() {
    if (m_zoomScaleLevel < 6) {
        m_zoomScaleLevel++;
        // if User adjusts the scaling tracking index, ChordLab remembers it instantly
        saveSongLayoutPreference(m_currentFilePath);
        if (currentState == PlayAlong) {
            parseChordProToGrid(m_rawSongContent); // Re-calculate target columns dynamically when zooming!
        }
    }
}

void MainWindow::onZoomOutTriggered() {
    if (m_zoomScaleLevel > -4) {
        m_zoomScaleLevel--;
        // if User adjusts the scaling tracking index, ChordLab remembers it instantly
        saveSongLayoutPreference(m_currentFilePath);
        if (currentState == PlayAlong) {
            parseChordProToGrid(m_rawSongContent);
        }
    }
}

// (Keep your existing processLineContent, transposeChord, analyzeChordProMetaData, getThemeStyles and checkForCompanionAudio exactly as they were written!) 
void MainWindow::selectAudioTrack(QPushButton *clickedButton, const QString &trackPath, const QString &trackName) {
    // 1. If this button was already checked and is now being unchecked, stop playback
    if (!clickedButton->isChecked()) {
        m_mediaPlayer->stop();
        m_selectedAudioPath.clear();
        statusBar()->showMessage("Audio track deselected.");
        return;
    }

    // 2. Uncheck the other two audio buttons so only one can be active at a time
    m_btnTrackFull->setChecked(false);
    m_btnTrackBkg->setChecked(false);
    m_btnTrackSlow->setChecked(false);

    // 3. Keep our clicked button highlighted
    clickedButton->setChecked(true);

    // 4. Update the media player target path
    m_selectedAudioPath = trackPath;
    m_mediaPlayer->setSource(QUrl::fromLocalFile(m_selectedAudioPath));

    statusBar()->showMessage(QString("Selected %1: %2").arg(trackName, QFileInfo(trackPath).fileName()));
}
// Kept code below this line... and replaced code above...

void MainWindow::analyzeChordProMetaData(const QString &rawInput) {
    // Reset metrics before scanning
    m_currentSongMetrics.totalLines = 0;
    m_currentSongMetrics.maxLineCharacters = 0;
    m_currentSongMetrics.sectionCount = 0;
    m_currentSongMetrics.targetColumns = 1;

    QStringList lines = rawInput.split('\n');
    bool inSection = false;

    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith('{') && (line.contains("comment") || line.contains("c:"))) {
            m_currentSongMetrics.sectionCount++;
            inSection = true;
            continue;
        }

        if (line.startsWith('{')) continue;

        if (inSection) {
            m_currentSongMetrics.totalLines++;

            // Strip chords to isolate the true visual text footprint width
            QString lyricOnlyLine = line;
            lyricOnlyLine.remove(QRegularExpression("\\[.*?\\]"));

            int cleanLength = lyricOnlyLine.length();
            // Cap individual anomalous long lines at a reasonable layout floor (e.g. 50 characters)
            // so an accidental run-on line doesn't completely destroy column generation!
            if (cleanLength > m_currentSongMetrics.maxLineCharacters) {
                m_currentSongMetrics.maxLineCharacters = cleanLength;
            }
/*            if (cleanLength > m_currentSongMetrics.maxLineCharacters) {
                m_currentSongMetrics.maxLineCharacters = cleanLength;
            }*/
        }
    }

    // 🚀 ELASTIC LINK MULTI-LAYER ENGINE
    // Calculate display boundaries dynamically based on the current size of the UI widget
    int displayWidth = parsedEditor->width();
    if (displayWidth <= 0) {
        // Fallback calculation if window hasn't fully drawn yet on startup
        QScreen *screen = QGuiApplication::primaryScreen();
        displayWidth = screen ? screen->availableGeometry().size().width() * 0.6 : 800;
    }

    // Estimate character width in pixels for Consolas/Courier based on current zoom scale
    int baseFontSize = 10 + (m_zoomScaleLevel * 2);
    double approxCharWidthPixels = baseFontSize * 0.62;

    // Determine how many characters can physically fit across one column safely
    int maxCharsPerColumn = qMax(20, static_cast<int>(displayWidth / approxCharWidthPixels));

    if (m_currentSongMetrics.maxLineCharacters == 0) {
        m_currentSongMetrics.targetColumns = 1;
    } else {
        // Calculate how many columns can fit side-by-side without truncating text lines
        // We add an arbitrary 6-character safety buffer for column margins
//        int allocationWidthPerCol = m_currentSongMetrics.maxLineCharacters + 6;
        int allocationWidthPerCol = qMin(m_currentSongMetrics.maxLineCharacters, 48) + 2;
        int calculatedCols = displayWidth / (allocationWidthPerCol * approxCharWidthPixels);

        // Clamp layout structure to reasonable parameters (1 to 4 columns/quadrants)
        m_currentSongMetrics.targetColumns = qBound(1, calculatedCols, 4);
    }

    // Guard: If the song structure is very short, don't force unnecessary columns
    if (m_currentSongMetrics.sectionCount < m_currentSongMetrics.targetColumns) {
        m_currentSongMetrics.targetColumns = qMax(1, m_currentSongMetrics.sectionCount);
    }

    if (m_debugVerboseLevel) {
        qDebug() << "=== CHORDLAB ELASTIC GEOMETRY RADAR ===";
        qDebug() << "Available Widget Width:" << displayWidth << "px";
        qDebug() << "Current Font Visual Width:" << approxCharWidthPixels << "px/char";
        qDebug() << "Max Song Line Width:" << m_currentSongMetrics.maxLineCharacters << "chars";
        qDebug() << "Calculated Optimal Columns:" << m_currentSongMetrics.targetColumns;
    }
}

QString MainWindow::getThemeStyles() {
    if (m_currentTheme == Dark) {
        return "body { background-color: #121212; color: #E0E0E0; }"
               ".chord { color: #82AAFF; font-weight: bold; }"
               ".header-block { background-color: #1A1C2E; border-bottom: 2px solid #30364D; padding: 15px; margin-bottom: 20px; }"
               ".chorus-box { background-color: #1E2337; border-left: 5px solid #0047AB; padding: 10px; margin: 5px 0; }"
               ".verse-box { background-color: #121212; padding: 10px; margin: 2px 0; }"
               ".section-header { background-color: #333333; color: #82AAFF; font-weight: bold; }";
    }
    return "body { background-color: white; color: black; }"
           ".chord { color: darkblue; font-weight: bold; }"
           ".header-block { background-color: #f8f9fa; border-bottom: 2px solid #ccc; padding: 10px; }"
           ".chorus-box { background-color: #f0f7ff; border-left: 5px solid darkblue; padding: 10px; margin: 5px 0; }"
           ".verse-box { background-color: white; padding: 10px; margin: 2px 0; }"
           ".section-header { background-color: #eeeeee; color: black; font-weight: bold; }";
}

QString MainWindow::processLineContent(const QString &line) {
    QString output = "";

    if (m_currentMode == ChordDisplayMode::CIL) {
        int lastPos = 0;
        QRegularExpression chordRegex("\\[(.*?)\\]");
        auto matches = chordRegex.globalMatch(line);
        while (matches.hasNext()) {
            auto match = matches.next();
            output += line.mid(lastPos, match.capturedStart() - lastPos);
            QString transposed = transposeChord(match.captured(1), m_transposeShift);
            output += "<span class='chord'>[" + transposed + "]</span>";
            lastPos = match.capturedEnd();
        }
        output += line.mid(lastPos);
    }
    else {
        QString chordLine = "";
        QString lyricLine = "";
        int currentPos = 0;
        QRegularExpression chordRegex("\\[(.*?)\\]");
        auto matches = chordRegex.globalMatch(line);

        if (!matches.hasNext()) {
            output = line;
        } else {
            while (matches.hasNext()) {
                auto match = matches.next();
                int gapLength = match.capturedStart() - currentPos;
                QString gap = line.mid(currentPos, gapLength);
                lyricLine += gap;
                chordLine += QString(gap.length(), ' ');
                QString transposed = transposeChord(match.captured(1), m_transposeShift);
                chordLine += "<span class='chord'>" + transposed + "</span>";
                currentPos = match.capturedEnd();
            }
            lyricLine += line.mid(currentPos);
            output = chordLine + "<br>" + lyricLine;
        }
    }
    return output;
}

void MainWindow::checkForCompanionAudio(const QString &chordProPath) {
    m_audioTracks = AudioTracks();
    m_selectedAudioPath.clear();

    m_btnTrackFull->setChecked(false); m_btnTrackFull->setEnabled(false);
    m_btnTrackBkg->setChecked(false);  m_btnTrackBkg->setEnabled(false);
    m_btnTrackSlow->setChecked(false); m_btnTrackSlow->setEnabled(false);

    if (chordProPath.isEmpty()) return;

    QFileInfo info(chordProPath);
    QString cleanDir = QDir::cleanPath(info.path());
    QString basePrefix = cleanDir + QDir::separator() + info.completeBaseName();

    qDebug() << "=== ChordLab Audio Matrix Scan ===";
    qDebug() << "Normalized Base Prefix:" << basePrefix;

    QStringList audioExtensions = { ".wav", ".aiff", ".m4a", ".mp3" };

    for (const QString &ext : audioExtensions) {
        QString targetFile = basePrefix + ext;
        QFileInfo check(targetFile);
        if (check.exists() && check.isFile()) {
            m_audioTracks.fullPath = targetFile;
            m_btnTrackFull->setEnabled(true);
            break;
        }
    }

    for (const QString &ext : audioExtensions) {
        QString targetFile = basePrefix + "_backing" + ext;
        QFileInfo check(targetFile);
        if (check.exists() && check.isFile()) {
            m_audioTracks.backingPath = targetFile;
            m_btnTrackBkg->setEnabled(true);
            break;
        }
    }

    for (const QString &ext : audioExtensions) {
        QString targetFile = basePrefix + "_slow" + ext;
        QFileInfo check(targetFile);
        if (check.exists() && check.isFile()) {
            m_audioTracks.slowPath = targetFile;
            m_btnTrackSlow->setEnabled(true);
            break;
        }
    }

    if (m_btnTrackFull->isEnabled()) {
        m_btnTrackFull->setChecked(true);
        m_selectedAudioPath = m_audioTracks.fullPath;
    }
    else if (m_btnTrackBkg->isEnabled()) {
        m_btnTrackBkg->setChecked(true);
        m_selectedAudioPath = m_audioTracks.backingPath;
    }
    else if (m_btnTrackSlow->isEnabled()) {
        m_btnTrackSlow->setChecked(true);
        m_selectedAudioPath = m_audioTracks.slowPath;
    }

    if (!m_selectedAudioPath.isEmpty()) {
        m_mediaPlayer->setSource(QUrl::fromLocalFile(m_selectedAudioPath));
        statusBar()->showMessage(tr("Loaded: %1 | Armed: %2")
                                     .arg(info.fileName(), QFileInfo(m_selectedAudioPath).fileName()));
    } else {
        statusBar()->showMessage(tr("Loaded: %1 (No companion audio discovered)").arg(info.fileName()));
    }
}

QString MainWindow::transposeSingleNoteToken(const QString &noteToken, int semitones) {
    if (semitones == 0 || noteToken.isEmpty()) return noteToken;
    bool isLowercase = noteToken[0].isLower();
    QString cleanNote = noteToken.toUpper();

    QString baseNote = cleanNote.left(1);
    if (cleanNote.length() > 1 && (cleanNote[1] == '#' || cleanNote[1] == 'B')) {
        baseNote = cleanNote.left(2);
    }

    // 🚀 THE FIX: Pull modifiers from the ORIGINAL string so 'm7' doesn't become 'M7'
    QString trailingModifiers = noteToken.mid(baseNote.length());

    int index = NOTE_SCALE_SHARPS.indexOf(baseNote);
    if (index == -1) index = NOTE_SCALE_FLATS.indexOf(baseNote);
    if (index == -1) return noteToken;

    int targetIndex = (index + semitones) % 12;
    if (targetIndex < 0) targetIndex += 12;

    // Default to sharps in dark theme, flats in light theme for visual comfort
    QString transposedNote = (m_currentTheme == Dark) ?
        NOTE_SCALE_SHARPS[targetIndex] : NOTE_SCALE_FLATS[targetIndex];

    transposedNote += trailingModifiers;

    return isLowercase ? transposedNote.toLower() : transposedNote;
}

QString MainWindow::transposeChord(const QString &chordText, int semitones) {
    if (semitones == 0 || chordText.isEmpty()) return chordText;

    // 1. Slash Chord Router [C/B]
    if (chordText.contains('/')) {
        QStringList parts = chordText.split('/');
        if (parts.size() == 2) {
            return transposeChord(parts[0], semitones) + "/" + transposeSingleNoteToken(parts[1], semitones);
        }
    }

    // 2. Single Note Parenthetical Router (g)
    if (chordText.startsWith('(') && chordText.endsWith(')')) {
        QString innerNote = chordText.mid(1, chordText.length() - 2);
        return "(" + transposeSingleNoteToken(innerNote, semitones) + ")";
    }

    return transposeSingleNoteToken(chordText, semitones);
}

QString MainWindow::parseGridLine(const QString &line, int delta) {
    if (line.trimmed().isEmpty()) return line;

    QString result = "";
    QString currentToken = "";

    for (int i = 0; i < line.length(); ++i) {
        QChar ch = line[i];

        // Maintain standard structural dividers cleanly
        if (ch == '|' || ch.isSpace()) {
            if (!currentToken.isEmpty()) {
                // Look-ahead check: Only attempt transposition if token starts with a valid musical letter tone
                QChar firstChar = currentToken.trimmed().isEmpty() ? QChar(' ') : currentToken.trimmed()[0];
                char upperCheck = firstChar.toUpper().toLatin1();

                if (upperCheck >= 'A' && upperCheck <= 'G') {
                    result += transposeChord(currentToken, delta);
                } else {
                    result += currentToken; // Pass non-chord labels through safely!
                }
                currentToken = "";
            }
            result += ch;
        } else {
            currentToken += ch;
        }
    }

    if (!currentToken.isEmpty()) {
        QChar firstChar = currentToken.trimmed().isEmpty() ? QChar(' ') : currentToken.trimmed()[0];
        char upperCheck = firstChar.toUpper().toLatin1();
        if (upperCheck >= 'A' && upperCheck <= 'G') {
            result += transposeChord(currentToken, delta);
        } else {
            result += currentToken;
        }
    }

    return result;
}

QString MainWindow::parseTabLine(const QString &line, int chordDelta, int instrumentDelta) {
    if (line.trimmed().isEmpty()) return line;

    // Check if it's a structural string line (e.g., E|---, B|---, e|---)
    QRegularExpression tuningRegex("^([A-G][#b]?)\\|", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch tuningMatch = tuningRegex.match(line);

    if (tuningMatch.hasMatch()) {
        QString result = "";
        QString originalStringNote = tuningMatch.captured(1);

        QString transposedStringNote = transposeSingleNoteToken(originalStringNote, instrumentDelta);
        result += transposedStringNote + "|";

        int i = tuningMatch.capturedEnd();

        while (i < line.length()) {
            if (line[i].isDigit()) {
                QString fretStr = "";
                while (i < line.length() && line[i].isDigit()) {
                    fretStr += line[i++];
                }

                int originalFret = fretStr.toInt();
                int newFret = originalFret + instrumentDelta + chordDelta;

                QString outputVisual = "";
                int logicalVisualLength = 0;

                // 🚀 THE RED 'x' OUT-OF-BOUNDS GUARD
                if (newFret < 0) {
                    // We wrap the lower case x inside inline HTML color tags
                    outputVisual = "<span style='color:#FF3333; font-weight:bold;'>x</span>";
                    logicalVisualLength = 1; // It physically takes up only ONE character cell on screen!
                } else {
                    if (newFret > 24) newFret = 24; // Keep standard fret ceiling safety intact
                    outputVisual = QString::number(newFret);
                    logicalVisualLength = outputVisual.length();
                }

                result += outputVisual;

                // 📐 TRACK PRECISION DASHES USING LOGICAL LENGTH ONLY
                // This prevents the HTML span tag length from chewing up your structural tab lines!
                int sizeDifference = logicalVisualLength - fretStr.length();
                while (sizeDifference > 0 && i < line.length() && line[i] == '-') {
                    i++;
                    sizeDifference--;
                }
            } else {
                result += line[i++];
            }
        }
        return result;
    }
    else {
        // --- ROUTINE B: This is a floating chord line above tabs ---
        // Keeps spacing intervals intact while transposing musical chords cleanly
        QString result = "";
        QString currentToken = "";

        for (int i = 0; i < line.length(); ++i) {
            QChar ch = line[i];

            if (ch.isSpace()) {
                if (!currentToken.isEmpty()) {
                    result += transposeChord(currentToken, chordDelta);
                    currentToken = "";
                }
                result += ch;
            } else {
                currentToken += ch;
            }
        }
        if (!currentToken.isEmpty()) {
            result += transposeChord(currentToken, chordDelta);
        }
        return result;
    }
}

void MainWindow::saveLayoutPreference(const QString &filePath, const SongLayoutState &layout) {
    QSettings settings;
    QFileInfo fileInfo(filePath);
    QString uniqueSongKey = fileInfo.fileName(); // e.g., "HotelCalifornia.pro"

    // Creates a hierarchy: SongLayouts -> HotelCalifornia.pro -> [settings]
    settings.beginGroup("SongLayouts/" + uniqueSongKey);

    settings.setValue("hasOverride", true);
    settings.setValue("columns", layout.columnCount);
    settings.setValue("fontSize", layout.fontSize);
    settings.setValue("capoShift", m_capoShift);

    settings.endGroup();

    if (m_debugVerboseLevel) {
        qDebug() << "[Layout] Saved custom layout for:" << uniqueSongKey;
    }
}

MainWindow::SongLayoutState MainWindow::loadLayoutPreference(const QString &filePath) {
    QSettings settings;
    QFileInfo fileInfo(filePath);
    QString uniqueSongKey = fileInfo.fileName();

    SongLayoutState layout;
    layout.hasUserOverride = false; // Default assumption

    settings.beginGroup("SongLayouts/" + uniqueSongKey);

    if (settings.contains("hasOverride")) {
        layout.hasUserOverride = settings.value("hasOverride").toBool();
        layout.columnCount = settings.value("columns").toInt();
        layout.fontSize = settings.value("fontSize").toInt();

        // Optionally load capo/transpose data here too
    }

    settings.endGroup();
    return layout;
}


void MainWindow::saveSongLayoutPreference(const QString &filePath) {
    if (filePath.isEmpty()) return;
    QSettings settings;
    QString songKey = QFileInfo(filePath).fileName();

    settings.beginGroup("SongLayouts/" + songKey);
    settings.setValue("hasOverride", true);
    settings.setValue("zoomScale", m_zoomScaleLevel);
    settings.endGroup();
}

void MainWindow::loadSongLayoutPreference(const QString &filePath) {
    if (filePath.isEmpty()) return;
    QSettings settings;
    QString songKey = QFileInfo(filePath).fileName();

    settings.beginGroup("SongLayouts/" + songKey);
    if (settings.value("hasOverride", false).toBool()) {
        m_zoomScaleLevel = settings.value("zoomScale", 0).toInt();
    } else {
        m_zoomScaleLevel = 0; // Reset back to default scaling index if unconfigured
    }
    settings.endGroup();
}
