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
#include <QTreeView>
#include <QHeaderView>
#include <QInputDialog>
#include <qapplication.h>
#include <QJsonDocument>
#include <QJsonObject>

static const QStringList NOTE_SCALE_SHARPS = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const QStringList NOTE_SCALE_FLATS  = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};

const QStringList MainWindow::AVAILABLE_FONTS = {
    "Cascadia Mono",
    "Courier Prime",
    "JetBrainsMono",
    "Libertinus Mono",
    "Red Hat Mono",
    "SixtyfourConvergence"
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // Core Hardware & Audio Engine Instantiation First
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8f);

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::handlePlaybackStateChanged);

    setWindowTitle("ChordLab V0.3beta");
    QIcon icon(":/resources/icons/CL-V003A.ico");
    setWindowIcon(icon);

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QCoreApplication::setOrganizationName("AldoMusic");
    QCoreApplication::setApplicationName("ChordLab");

    QSettings settings;
    QString savedStyle = settings.value("theme/lastStyle").toString();

    // Clear Internal State Variables Before UI Draws
    m_currentFilePath = "";
    m_parsedSongContentGrid = "";
    m_zoomCoarse = 0;
    m_zoomFine   = 0;
    m_columnOverride = 0;
    m_transposeShift = 0;
    m_capoShift = 0;                  // default to open neck
    m_instrumentTuningOffset = 0;     // default to standard guitar tuning
    m_debugTelemetryEnabled = false;   // enable/disable tracking telemetry out to Qt App Output window
    m_debugVerboseLevel = true;
    m_debug_Setlist = true;
    m_isLoadingFile = false;
    m_currentMode = ChordDisplayMode::CIL;
    m_currentTheme = Theme::Light;

    // 1. Initialize the Manager
    m_setlistManager = new SetlistManager(this);

    // 2. Initialize the View
    m_setlistView = new QTreeView(this); // 🚀 Now a TreeView!
    m_setlistView->setModel(m_setlistManager);
    m_setlistView->setHeaderHidden(true); // Hides the column headers
    m_setlistView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_setlistView->setDragDropMode(QAbstractItemView::InternalMove);
    m_setlistView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_setlistView->hide();

    onLoadSetlistTriggered();

    // Make expansions smooth/animated
    m_setlistView->setAnimated(true);

    // Auto-collapse other setlists when one is opened
    connect(m_setlistView, &QTreeView::expanded, this, [this](const QModelIndex &expandedIndex) {
        for (int i = 0; i < m_setlistManager->rowCount(); ++i) {
            QModelIndex idx = m_setlistManager->index(i, 0);
            if (idx != expandedIndex) {
                m_setlistView->collapse(idx);
            }
        }
    });

    // Enable drag-and-drop on the View
    m_setlistView->setDragEnabled(true);
    m_setlistView->setAcceptDrops(true);
    m_setlistView->setDropIndicatorShown(true);
    m_setlistView->setDragDropMode(QAbstractItemView::InternalMove); // The magic flag...

    connect(m_setlistView, &QTreeView::clicked, this, &MainWindow::handleSetlistItemClicked);
    connect(m_setlistView, &QTreeView::doubleClicked, this, &MainWindow::handleSetlistItemDoubleClicked);

    connect(m_setlistManager, &QStandardItemModel::rowsInserted, this, [this]() { m_isSetlistDirty = true; });
    connect(m_setlistManager, &QStandardItemModel::rowsRemoved, this, [this]() { m_isSetlistDirty = true; });

//    QSettings settings;
    m_currentFont = settings.value("display/font", "Courier Prime").toString();

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

    // Explicitly make sure NO hard minimum size constraints can lock the widget layout
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

        // Defer loading song structures by 50ms!
        // This lets the OS window manager completely finish sizing the central splitter
        // and central widgets, ensuring the Radar reads the true physical layout width!
        QTimer::singleShot(50, this, [=]() {
            QSettings delayedSettings;
            QString lastFile = delayedSettings.value("app/lastOpenedFile").toString();
            int lastMode = delayedSettings.value("app/lastMode", static_cast<int>(PlayAlong)).toInt();

            if (!lastFile.isEmpty() && QFile::exists(lastFile)) {
                loadSongQuietly(lastFile);
                // need to also check & reload companion audio files????
                setAppState(static_cast<AppState>(lastMode));
            } else {
                setAppState(Idle);
            }
        });
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);

    // LIVE LAYOUT LINK: If the user maximizes or manually resizes the frame,
    // re-trigger the engine calculations instantly to adjust columns live!
    if (currentState == PlayAlong && !m_rawSongContent.isEmpty() && !m_isLoadingFile) {
        parseChordProToGrid(m_rawSongContent);
    }
}

QString MainWindow::getResourcesPath() const {
#ifdef Q_OS_MACOS
    return QCoreApplication::applicationDirPath() + "/../Resources";
#else
    return QCoreApplication::applicationDirPath() + "/resources";
#endif
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

    QMenu *fontMenu = prefMenu->addMenu("Display Font");
    QActionGroup *fontGroup = new QActionGroup(this);
    fontGroup->setExclusive(true);

    QSettings settings;
    QString savedFont = settings.value("display/font", "Courier Prime").toString();
    m_currentFont = savedFont;

    for (const QString &fontName : AVAILABLE_FONTS) {
        QAction *action = fontMenu->addAction(fontName);
        action->setCheckable(true);
        action->setActionGroup(fontGroup);
        if (fontName == savedFont) action->setChecked(true);

        connect(action, &QAction::triggered, this, [=]() {
            m_currentFont = fontName;
            QSettings s;
            s.setValue("display/font", fontName);

            // Update both editors immediately
            QFont f(fontName, 10);
            f.setStyleHint(QFont::TypeWriter);
            originalEditor->setFont(f);
            parsedEditor->setFont(f);

            // Re-render with new font
            if (currentState == PlayAlong) {
                parseChordProToGrid(m_rawSongContent);
            } else if (currentState == OpenEdit) {
                parsedEditor->setHtml(runInitialParse(m_rawSongContent));
            }

            statusBar()->showMessage("Font changed to: " + fontName, 3000);
        });
    }

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
        QMessageBox::about(this, "About QPlayer", "ChordLab\nVersion 0.3beta\n11-Jun-2026...\nA chordpro multimedia app \nfor musicians.");
    });

//    QSettings settings;
    QString currentPath = settings.value("theme/lastStyle").toString();
    qDebug() << "Available resources at :/resources/styles:" << themeDir.entryList();
    for (const QString &themeFile : themes) {
        QString fullPath = ":/resources/styles/" + themeFile;
        QString menuName = themeFile;
        menuName.remove(".qss");

        // ADD TO themeMenu
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

    m_settingsToolBar = addToolBar("Critical Settings");
    m_settingsToolBar->setObjectName("criticalSettingsToolBar");

    // Initialize your Setlist Actions
    // --- Add this button creation block ---
    m_btnToggleSetlist = new QPushButton("≡ Sets");
    m_btnToggleSetlist->setStyleSheet("QPushButton { background-color: #333333; color: white; padding: 5px; width: 20px; font-weight: bold; }");

    // Connect to your existing slot
    connect(m_btnToggleSetlist, &QPushButton::clicked, this, &MainWindow::onHamburgerClicked);

    // Add it to the layout (or directly to the toolbar)
    m_settingsToolBar->addWidget(m_btnToggleSetlist);

    // --- Setlist companion buttons (enable/disable approach) ---
    m_btnAddSong     = new QPushButton(QIcon(":/resources/icons/add_sg.png"), "Add", this);
    m_btnRemoveSong  = new QPushButton(QIcon(":/resources/icons/remove_sg.png"), "Remove", this);
    m_btnSaveSetlist = new QPushButton(QIcon(":/resources/icons/save_sl.png"), "Save", this);

    QString setlistBtnStyle = "QPushButton { background-color: #4A4A4A; color: #FFFFFF; "
                              "padding: 4px 8px; font-size: 9pt; border: 1px solid #666; "
                              "min-width: 12px; border-radius: 3px; }"
                              "QPushButton:disabled { background-color: #2A2A2A; "
                              "color: #555555; border-color: #444; }";

    m_btnAddSong->setStyleSheet(setlistBtnStyle);
    m_btnRemoveSong->setStyleSheet(setlistBtnStyle);
    m_btnSaveSetlist->setStyleSheet(setlistBtnStyle);

    m_btnAddSong->setEnabled(false);
    m_btnRemoveSong->setEnabled(false);
    m_btnSaveSetlist->setEnabled(false);

    connect(m_btnAddSong,     &QPushButton::clicked, this, &MainWindow::handleAddSongToSetlist);
    connect(m_btnRemoveSong,  &QPushButton::clicked, this, &MainWindow::handleRemoveSongFromSetlist);
    connect(m_btnSaveSetlist, &QPushButton::clicked, this, &MainWindow::handleSaveSetlist);

    // Add DIRECTLY to toolbar — no wrapper widget!
    m_settingsToolBar->addWidget(m_btnAddSong);
    m_settingsToolBar->addWidget(m_btnRemoveSong);
    m_settingsToolBar->addWidget(m_btnSaveSetlist);

    m_settingsToolBar->addSeparator();

    m_btnTransposeUp = new QPushButton("Key Up");
    m_btnTransposeDown = new QPushButton("Key Down");
    m_btnTheme = new QPushButton("Light theme...");
    m_btnTheme->setStyleSheet("QPushButton { background-color: #0047AB; color: white; padding: 5px; min-width: 80px; }");

    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(10);
                                                                                                // was 80px
    QString style = "QPushButton { background-color: #004060; color: white; padding: 5px; min-width: 60px; }";
    m_btnTransposeUp->setStyleSheet(style);
    m_btnTransposeDown->setStyleSheet(style);
                                                                                                                        // was 65px
    QString buttonStyle = "QPushButton { background-color: #555555; color: white; border-radius: 4px; padding: 5px; min-width: 40px; font-weight: bold; }";
    m_btnFn1 = new QPushButton("Fn-1");
    m_btnFn2 = new QPushButton("Fn-2");
    m_btnFn3 = new QPushButton("Fn-3");
    m_btnFn4 = new QPushButton("Fn-4");

    m_btnFn1->setStyleSheet(buttonStyle);
    m_btnFn2->setStyleSheet(buttonStyle);
    m_btnFn3->setStyleSheet(buttonStyle);
    m_btnFn4->setStyleSheet(buttonStyle);

    layout->addWidget(m_btnFn1);  // 🚀 reassignable, multi-function buttons...
    layout->addWidget(m_btnFn2);
    layout->addWidget(m_btnFn3);
    layout->addWidget(m_btnFn4);
                                                                                                                                                         // was 50px
    QString audioBtnStyle = "QPushButton { background-color: #2D2D2D; color: #777777; border: 1px solid #444; border-radius: 4px; padding: 5px; min-width: 30px; font-weight: bold; }"
                            "QPushButton:enabled { color: #E0E0E0; border-color: #555; }"
                            "QPushButton:checked { background-color: #006644; color: white; border-color: #00FF88; }";
    m_btnTrackFull = new QPushButton("Full");
    m_btnTrackBkg  = new QPushButton("Bkg");
    m_btnTrackSplit  = new QPushButton("Split");
    m_btnTrackSlow = new QPushButton("Slow");

    m_btnTrackFull->setCheckable(true);
    m_btnTrackBkg->setCheckable(true);
    m_btnTrackSplit->setCheckable(true);
    m_btnTrackSlow->setCheckable(true);

    m_btnTrackFull->setStyleSheet(audioBtnStyle);
    m_btnTrackBkg->setStyleSheet(audioBtnStyle);
    m_btnTrackSplit->setStyleSheet(audioBtnStyle);
    m_btnTrackSlow->setStyleSheet(audioBtnStyle);

    m_btnTrackFull->setEnabled(false);
    m_btnTrackBkg->setEnabled(false);
    m_btnTrackSplit->setEnabled(false);
    m_btnTrackSlow->setEnabled(false);

    layout->addWidget(m_btnTrackFull);
    layout->addWidget(m_btnTrackBkg);
    layout->addWidget(m_btnTrackSplit);
    layout->addWidget(m_btnTrackSlow);

    connect(m_btnTrackFull, &QPushButton::clicked, this, [=]() { selectAudioTrack(m_btnTrackFull, m_audioTracks.fullPath, "Full Track"); });
    connect(m_btnTrackBkg,  &QPushButton::clicked, this, [=]() { selectAudioTrack(m_btnTrackBkg, m_audioTracks.backingPath, "Backing Track"); });
    connect(m_btnTrackSplit, &QPushButton::clicked, this, [=]() { selectAudioTrack(m_btnTrackSplit, m_audioTracks.splitPath, "Split Stereo Track"); });
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

    m_viewToggleBtn = new QPushButton("ChordPro-CIL");
    m_viewToggleBtn->setStyleSheet("QPushButton { background-color: #0047AB; color: white; }");
    connect(m_viewToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleDisplayMode);
    layout->addWidget(m_viewToggleBtn);

    connect(m_btnTransposeUp, &QPushButton::clicked, this, [=](){ shiftTransposition(1); });
    connect(m_btnTransposeDown, &QPushButton::clicked, this, [=](){ shiftTransposition(-1); });
    connect(m_btnTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);

    m_settingsToolBar->addWidget(container);
}

void MainWindow::onHamburgerClicked() {
    if (m_setlistView->isHidden()) {
        m_setlistView->show();
        mainSplitter->setSizes({250, this->width() - 250});
        m_btnToggleSetlist->setText("× Hide");
        m_btnAddSong->setEnabled(true);
        m_btnRemoveSong->setEnabled(true);
        m_btnSaveSetlist->setEnabled(true);
        qDebug() << ">>> Hamburger OPEN: setlistActionsWidget visible ="
                 << m_setlistView->isVisible()
                 << "| parent visible =" << m_setlistView->parentWidget()->isVisible();
    } else {
        m_setlistView->hide();
        m_btnToggleSetlist->setText("≡ Sets");
        m_btnAddSong->setEnabled(false);
        m_btnRemoveSong->setEnabled(false);
        m_btnSaveSetlist->setEnabled(false);
    }
}


void MainWindow::handleSetlistItemClicked(const QModelIndex &index) {
    // Safety check: If the user clicked a top-level Setlist file container, do nothing
    if (!index.parent().isValid()) return;

    // Extract the relative path we stored inside the item's custom data role
    QString relativePath = index.data(Qt::UserRole + 1).toString();

    if (relativePath.isEmpty()) {
        qDebug() << "relativePath.isEmpty is true, no file";
        return;
    }

    // Reconstruct the base path matching your directory structure
//    QString fullPath = QCoreApplication::applicationDirPath() + "/resources/pieces/" + relativePath;
    QString fullPath = getResourcesPath() + "/pieces/" + relativePath;

    // --- THE SMART RESOLVER ---
    QFileInfo fileInfo(fullPath);

    if (fileInfo.isDir()) {
        // It's a folder! Let's find the master chord file inside
        QDir dir(fullPath);
        QStringList filters = {"*.cho", "*.pro", "*.txt", "*.crd"};
        QStringList files = dir.entryList(filters, QDir::Files);

        if (!files.isEmpty()) {
            // Grab the first valid chord file inside the folder
            fullPath = dir.absoluteFilePath(files.first());
            qDebug() << "After !files.isEmpty, just fullPath = " << "|" << fullPath << "|";
        } else {
            qDebug() << "❌ Folder contains no valid chord files:" << fullPath;
            return;
        }
    } else if (!fileInfo.exists()) {
        // It's not a folder, but the exact string doesn't exist. Probably missing an extension!
        qDebug() << "After !fileInfo.exists, just fullPath = " << "|" << fullPath << "|";
        if (QFile::exists(fullPath + ".cho")) {
            fullPath += ".cho";
        } else if (QFile::exists(fullPath + ".pro")) {
            fullPath += ".pro";
        } else if (QFile::exists(fullPath + ".txt")) {
            fullPath += ".txt";
        } else {
            qDebug() << "❌ File completely missing from hard drive:" << fullPath;
            return;
        }
    }

    // Fire your existing parser/loader function!
    qDebug() << "✅ Loading verified song -->" << fullPath;
    this->loadSongQuietly(fullPath);
}

void MainWindow::handleSetlistItemDoubleClicked(const QModelIndex &index) {
    if (!index.parent().isValid()) return;

    if (!m_setlistView->isHidden()) {
        m_setlistView->hide();
        m_btnToggleSetlist->setText("≡ Sets");

        // deselect the setlist buttons
        m_btnAddSong->setEnabled(false);
        m_btnRemoveSong->setEnabled(false);
        m_btnSaveSetlist->setEnabled(false);
    }

    setAppState(PlayAlong);
    this->setFocus();
}

QStringList MainWindow::getAvailableSetlists() {

    QDir dir(":/resources/setlists/");
    qDebug() << "Path exists?" << dir.exists();

    QStringList files = dir.entryList(QStringList() << "*.set", QDir::Files);
    qDebug() << "Files found:" << files;

    return files;
}

void MainWindow::onLoadSetlistTriggered() {
    qDebug() << "--> top of onLoadSetlistTriggered()";
    qDebug() << "[Platform] Resources path:" << getResourcesPath();
    QStringList setlists = getAvailableSetlists();

//    QString setlistDir = QCoreApplication::applicationDirPath() + "/resources/setlists";
    QString setlistDir = getResourcesPath() + "/setlists";

    QDir dir(setlistDir);

    // 2. Find the files (which your logs show is already working!)
    QStringList filters;
    filters << "*.set";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

    // 3. Clear the model first so you don't get duplicates if this is called twice
    m_setlistManager->clear();

    // 4. Loop through the files and feed them to the Manager
    for (const QFileInfo &fileInfo : fileList) {
        QString fullPath = fileInfo.absoluteFilePath();
        m_setlistManager->loadSetFile(fullPath); // this builds out the tree list with the setlist file line-items!
    }
    if (setlists.isEmpty()) {
        statusBar()->showMessage("No setlists found in resources!");
        return;
    }
}

void MainWindow::handleRemoveSongFromSetlist() {
    QModelIndex currentIndex = m_setlistView->currentIndex();
    if (!currentIndex.isValid()) return;

    QStandardItem *item = m_setlistManager->itemFromIndex(currentIndex);

    // Safety check: Ensure we are deleting a song (child), not the entire setlist (parent)
    if (item && item->parent() != nullptr) {
        item->parent()->removeRow(item->row()); // Qt deletes it from memory and the UI instantly!
    }
}

void MainWindow::handleAddSongToSetlist() {
    QModelIndex currentIndex = m_setlistView->currentIndex();
    if (!currentIndex.isValid()) return;

    QStandardItem *currentItem = m_setlistManager->itemFromIndex(currentIndex);

    // Find the parent setlist (if they clicked a song, grab its parent instead)
    QStandardItem *parentSetlist = currentItem->parent() ? currentItem->parent() : currentItem;

    // Open file dialog
    QString filePath = QFileDialog::getOpenFileName(this, "Add Song to Setlist", "", "ChordPro Files (*.cho *.pro *.txt);;All Files (*)");
    if (filePath.isEmpty()) return;

    // Create the new item
    QFileInfo fileInfo(filePath);
    QStandardItem *newSong = new QStandardItem(fileInfo.fileName());
    newSong->setEditable(false);

    // Convert absolute path to a relative path based on your resources/pieces structure
//    QDir piecesDir(QCoreApplication::applicationDirPath() + "/resources/pieces");
    QDir piecesDir(getResourcesPath() + "/pieces");
    QString relativePath = piecesDir.relativeFilePath(filePath);

    // Assuming FilePathRole is Qt::UserRole + 1 as per standard practices
    newSong->setData(relativePath, Qt::UserRole + 1);

    // Append to the setlist
    parentSetlist->appendRow(newSong);

    // Optional: Expand the tree so the user sees their new addition
    m_setlistView->expand(parentSetlist->index());
}

void MainWindow::handleSaveSetlist() {
    QModelIndex currentIndex = m_setlistView->currentIndex();
    if (!currentIndex.isValid()) return;

    QStandardItem *currentItem = m_setlistManager->itemFromIndex(currentIndex);
    QStandardItem *parentSetlist = currentItem->parent() ? currentItem->parent() : currentItem;

    // We need to know where to save this.
    // Pro-tip: When you load the setlist, store the .set file path in the parent item's data!
//    QString setlistPath = QCoreApplication::applicationDirPath() + "/resources/setlists/" + parentSetlist->text();
    QString setlistPath = getResourcesPath() + "/setlists/" + parentSetlist->text();

    QFile file(setlistPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to save setlist!";
        return;
    }

    QTextStream out(&file);
    out << "## Setlist = " << parentSetlist->text() << "\n";

    // Iterate through all child songs and write their stored paths back to the text file
    for (int i = 0; i < parentSetlist->rowCount(); ++i) {
        QStandardItem *songItem = parentSetlist->child(i);
        QString songPath = songItem->data(Qt::UserRole + 1).toString(); // Get the hidden path
        out << "\"" << songPath << "\"\n"; // Format it back into quotes!
    }

    file.close();
    m_isSetlistDirty = false; // The changes are safely on disk now!
    statusBar()->showMessage("Setlist saved successfully!", 3000);
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
    // 1. Create your widgets FIRST (don't add to splitters yet)
    originalEditor = new QPlainTextEdit(this);
    originalEditor->setPlaceholderText("Original File Content...");

    parsedEditor = new QTextEdit(this);
    parsedEditor->setPlaceholderText("Parsed & Standardized Output...");
    parsedEditor->setReadOnly(true);

    QFont monoFont(m_currentFont, 10);
    monoFont.setStyleHint(QFont::TypeWriter);
    originalEditor->setFont(monoFont);
    parsedEditor->setFont(monoFont);

    // 2. Now create the inner splitter and add the existing widgets
    editorSplitter = new QSplitter(Qt::Horizontal);
    editorSplitter->addWidget(originalEditor);
    editorSplitter->addWidget(parsedEditor);

    // 3. Create the main splitter and add the Setlist + the editorSplitter
    mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(m_setlistView);  // Setlist manager
    mainSplitter->addWidget(editorSplitter); // The editor block

    originalEditor->setMinimumWidth(100);
    parsedEditor->setMinimumWidth(100);

    // 4. Connect the signal (this looks perfect)
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

        mainSplitter->show();
        // This now sets: [Setlist Size, EditorSplitter Size]
        mainSplitter->setSizes(QList<int>({200, this->width() - 200}));

        // This sets the inner splitter: [Original Size, Parsed Size]
        editorSplitter->setSizes(QList<int>({(this->width()-200)/2, (this->width()-200)/2}));

        originalEditor->show();
        parsedEditor->show();
        parsedEditor->setHtml(runInitialParse(m_rawSongContent));

        parsedEditor->setFocusPolicy(Qt::StrongFocus);  // ← restore for editing

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
        parsedEditor->setFocusPolicy(Qt::NoFocus);  // ← add this
        this->setFocus();                            // ← ensure MainWindow has focus
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings;

    if (m_isSetlistDirty) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Unsaved Changes",
            "Your setlist has unsaved modifications. Would you like to save before exiting?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );

        if (reply == QMessageBox::Yes) {
            handleSaveSetlist();
            event->accept(); // Safe to close now
        } else if (reply == QMessageBox::No) {
            event->accept(); // User explicitly discards changes, proceed to close
        } else {
            event->ignore(); // Cancel button clicked! Halt exit and stay in ChordLab
        }
    } else {
        event->accept(); // Nothing was changed, close immediately and cleanly
    }

    // Save the window size and desktop location
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());

    // Save current song's layout to JSON on close
    if (!m_currentFilePath.isEmpty()) {
        saveSongLayoutToJson(m_currentFilePath);
    }

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
//    loadSongLayoutPreference(filePath);
    loadSongLayoutFromJson(filePath);

    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        m_rawSongContent = in.readAll();
        originalEditor->setPlainText(m_rawSongContent);
        file.close();
    }

    // Reset transpositions (the logic you had in handleFileOpen)
    m_transposeShift = 0;
    m_capoShift = 0;
    m_instrumentTuningOffset = 0;

    // Apply layout preference
//    loadSongLayoutPreference(filePath);

    // Trigger UI state
    setAppState(OpenEdit);

    // Trigger Audio check
    checkForCompanionAudio(filePath);

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
    // 1. Get the path
//    QString appDir = QCoreApplication::applicationDirPath();
//    QString initialPath = QDir(appDir + "/resources/pieces").exists() ?
//                              appDir + "/resources/pieces" : QDir::homePath();
    QString initialPath = QDir(getResourcesPath() + "/pieces").exists() ?
                              getResourcesPath() + "/pieces" : QDir::homePath();

    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Open ChordPro File"), initialPath,
        tr("ChordPro Files (*.chopro *.cho *.pro *.txt *.crd)")
        );

    if (fileName.isEmpty()) return;

    // 2. Delegate EVERYTHING to your master loader
    // This now handles text loading, layout, audio checking, and state setting!
    loadSongQuietly(fileName);

    // 3. Provide user feedback
    statusBar()->showMessage(tr("Loaded: ") + fileName);
}

void MainWindow::handleFileSave() {
    if (m_currentFilePath.isEmpty()) {
        // 1. Determine the path to the running executable
//        QString appDir = QCoreApplication::applicationDirPath();
        // 2. Point to our standard user workspace folder
//        QString initialPath = appDir + "/resources/pieces";
        QString initialPath = getResourcesPath() + "/pieces";
        if (!QDir(initialPath).exists()) {
            initialPath = QDir::homePath();
        }

        // 3. Fallback safely if the workspace directory isn't deployed yet
        if (!QDir(initialPath).exists()) {
            initialPath = QDir::homePath();
        }

        // synchronized: Opens the Save Dialog right in the same folder as Open Dialog!
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
                     "body { font-family: '" + m_currentFont + "', 'Courier New', monospace; white-space: pre; font-size: 10pt; }"
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

        // 1. use empty lines as formal paragraph breaks for unstructured text
        if (line.isEmpty()) {
            if (insideSectionBlock && !inGridBlock && !inTabBlock) {
                currentSectionHtml += "</div>";
                gatheredSections.append(currentSectionHtml);
                currentSectionHtml = "";
                insideSectionBlock = false;
            }
            continue;
        }

        // 2. Grid & Tab State Toggles (Keep your existing {sog}/{sot} blocks here)
        if (line.startsWith("{start_of_grid}") || line.startsWith("{sog}")) {
            inGridBlock = true;
            if (!insideSectionBlock) {
                insideSectionBlock = true;  // ← add this
                qDebug() << "New sog GRID insideSectionBlock = true";
            }
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
            if (!insideSectionBlock) {
                insideSectionBlock = true;  // ← add this
                qDebug() << "New sot TAB insideSectionBlock = true";
            }
            currentSectionHtml += "<div class='song-section'><div style='font-family: Consolas; white-space: pre; background: rgba(0, 136, 0, 0.1); padding: 5px; border-radius: 4px;'>";
            continue;
        }
        if (line.startsWith("{end_of_tab}") || line.startsWith("{eot}")) {
            inTabBlock = false;
            currentSectionHtml += "</div></div>";
            continue;
        }

        // 3. explicit Section Headers ({c:}, {soc}, and #)
        bool isChorusStart = line.startsWith("{soc", Qt::CaseInsensitive) || line.startsWith("{start_of_chorus", Qt::CaseInsensitive);
        bool isCommentStart = line.startsWith("{c:", Qt::CaseInsensitive) || line.startsWith("{comment:", Qt::CaseInsensitive);
        bool isInformalHeader = line.startsWith("#");

        if (isChorusStart || isCommentStart || isInformalHeader) {
            if (insideSectionBlock) {
                currentSectionHtml += "</div>";
                gatheredSections.append(currentSectionHtml);
                currentSectionHtml = "";
            }

            QString sectionName = "";
            if (isChorusStart) {
                sectionName = "Chorus";
            } else if (isInformalHeader) {
                sectionName = line.mid(1).trimmed(); // Grab text safely after '#'
            } else {
                QRegularExpression rx("^\\{c(?:omment)?:?\\s*([^}]*)\\}?", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch match = rx.match(line);
                if (match.hasMatch() && !match.captured(1).trimmed().isEmpty()) sectionName = match.captured(1).trimmed();
                else sectionName = "...";
            }

            currentSectionHtml += "<div class='song-section'>";
            currentSectionHtml += QString("<div class='section-heading'>%1</div>").arg(sectionName);
            insideSectionBlock = true;
            continue;
        }

        // 4. Handle end of chorus explicitly
        if (line.startsWith("{eoc", Qt::CaseInsensitive) || line.startsWith("{end_of_chorus", Qt::CaseInsensitive)) {
            if (insideSectionBlock) {
                currentSectionHtml += "</div>";
                gatheredSections.append(currentSectionHtml);
                currentSectionHtml = "";
                insideSectionBlock = false;
            }
            continue;
        }

        // 5. Ignore Metadata tags in the main body (Title handles them!)
        if (line.startsWith("{t:") || line.startsWith("{title:") || line.startsWith("{st:") || line.startsWith("{subtitle:") || line.startsWith("{key:") || line.startsWith("{tempo:") || line.startsWith("{capo:")) {
            continue;
        }

        // 6. 🚀 THE UNSTRUCTURED AUTOLOADER
        // If we hit standard text/chords but aren't in a section block, start one automatically!
        if (!insideSectionBlock && !inGridBlock && !inTabBlock && !line.startsWith("{")) {
            currentSectionHtml += "<div class='song-section'>";
            // We deliberately omit the <div class='section-heading'> so it acts as continuous anonymous text!
            insideSectionBlock = true;
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

            // fallback: If they type an empty {c:} or are half-way through typing {c:V
            if (sectionName.isEmpty()) {
                sectionName = "..."; // Keeps the UI stable while typing
            }
            currentSectionHtml += "<div class='song-section'>";
            currentSectionHtml += QString("<div class='section-heading'>%1</div>").arg(sectionName);
            insideSectionBlock = true;
            continue;
        }

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
            }
//            else {     // No else — if there are no chords, don't waste the vertical space!
//                lineBlockHtml += "<p style='margin: 0; padding: 0;' class='chord-line'>&nbsp;</p>";
//            }

            if (showDiagnosticCheckline && !chordLine.trimmed().isEmpty()) {
                lineBlockHtml += QString("<p style='margin: 0; padding: 0; color: #555555; font-size: 8pt;'>%1</p>").arg(dashLine);
            }

            lineBlockHtml += QString("<p style='margin: 0 0 3px 0; padding: 0;' class='lyric-text'>%1</p>").arg(lyricLine.isEmpty() ? "&nbsp;" : lyricLine);
            // This line added 8px below every lyric line --> relaxed versus tight spacing:
//            "<p style='margin: 0 0 8px 0; padding: 0;' class='lyric-text'>"
//            "<p style='margin: 0 0 3px 0; padding: 0;' class='lyric-text'>"
            lineBlockHtml += "</div>";
            currentSectionHtml += lineBlockHtml;
        }
    }

    if (insideSectionBlock) {
        currentSectionHtml += "</div>";
        gatheredSections.append(currentSectionHtml);
    }

    int numCols = (m_columnOverride > 0) ? m_columnOverride
                                         : m_currentSongMetrics.targetColumns;
    if (m_columnOverride == 0) {
        if (m_zoomCoarse == 1 && numCols > 2) numCols = 2;
        else if (m_zoomCoarse >= 2) numCols = 1;
    }
    if (numCols < 1) numCols = 1;

    qDebug() << "Column decision: override=" << m_columnOverride
             << "radar=" << m_currentSongMetrics.targetColumns
             << "final=" << numCols;

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
    double baseFontSize       = 10.0 + (m_zoomCoarse * 2.0) + (m_zoomFine * 0.5);
    // could be a future User-pref, or a song by song judgement?? ==> relax or tight line spacing
    // Tighter — closer to SBPro density
    double activeLineHeight  = 1.05 + (m_zoomCoarse * 0.03) + (m_zoomFine * 0.008);

    //    int sectionMarginBottom  = 5    + (m_zoomCoarse * 2)    + m_zoomFine;
    int sectionMarginBottom = 5 + (m_zoomCoarse * 2) + m_zoomFine + m_sectionSpacingBonus;

    int headerMarginBottom   = 2    + (m_zoomCoarse / 2);

    parsedEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    parsedEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QString bgColor    = (m_currentTheme == Dark) ? "#20204f" : "#e0F0ff";
    QString txtColor   = (m_currentTheme == Dark) ? "#E0E0E0" : "#222222";
    QString chordColor = (m_currentTheme == Dark) ? "#ffb060" : "#c22222";
    QString headColor  = (m_currentTheme == Dark) ? "#6699ff" : "#0055aa";

    QRegularExpression titleRx(R"(\{(?:title|t):\s*([^}]*)\})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression artistRx(R"(\{(?:artist|st|subtitle):\s*([^}]*)\})", QRegularExpression::CaseInsensitiveOption);
    QString title  = titleRx.match(m_rawSongContent).captured(1).trimmed();
    QString artist = artistRx.match(m_rawSongContent).captured(1).trimmed();

    QString headerHtml = "";
    if (!title.isEmpty()) {
        headerHtml += "<div style='text-align: center; margin-bottom: 20px; padding-bottom: 10px; border-bottom: 2px solid " + chordColor + ";'>";
        headerHtml += "<h1 style='margin: 0; font-size: " + QString::number(baseFontSize + 8, 'f', 1) + "pt; color: " + txtColor + ";'>" + title + "</h1>";
        if (!artist.isEmpty()) {
            headerHtml += "<h3 style='margin: 5px 0 0 0; font-size: " + QString::number(baseFontSize + 2, 'f', 1) + "pt; color: #777; font-weight: normal;'>" + artist + "</h3>";
        }
        headerHtml += "</div>";
    }

    QString baseHtml = "<html><head><style>"
                       "body { background-color: " + bgColor + "; color: " + txtColor + "; margin: 8px 10px; padding: 0;"
//                        "body { background-color: " + bgColor + "; color: " + txtColor + "; margin: 12px 18px; padding: 0;"
                                                            "  font-family: '" + m_currentFont + "', 'Courier New', monospace; }"
                                    "h1 { font-size: " + QString::number(baseFontSize + 4, 'f', 1) + "pt; font-weight: bold; font-family: sans-serif; margin: 0 0 4px 0; }"
                                                                     ".section-heading {"
                                                                     "  font-size: " + QString::number(baseFontSize + 1, 'f', 1) + "pt;"
                                                                     "  color: " + headColor + ";"
                                     "  font-weight: bold;"
                                     "  font-family: '" + m_currentFont + "', 'Courier New', monospace;"
                                         "  margin: 0 0 " + QString::number(headerMarginBottom) + "px 0;"
                                                               "  padding: 0;"
                                                               "  border-bottom: 1px solid " + headColor + "44;"
                                     "}"
                                     ".song-section { margin-bottom: " + QString::number(sectionMarginBottom) + "px; }"
                                                                ".chord-line {"
                                                                "  font-weight: bold;"
                                                                "  color: " + chordColor + ";"
                                      "  font-family: '" + m_currentFont + "', 'Courier New', monospace;"
                                         "  font-size: " + QString::number(baseFontSize, 'f', 1) + "pt;"
                                                                 "  margin: 0; padding: 0;"
                                                                 "}"
                                                                 ".lyric-text {"
                                                                 "  color: " + txtColor + ";"
                                    "  font-family: '" + m_currentFont + "', 'Courier New', monospace;"
                                         "  font-size: " + QString::number(baseFontSize, 'f', 1) + "pt;"
                                                                 "  margin: 0; padding: 0;"           // tighter lyric margin
//                                                                 "  margin: 0 0 2px 0; padding: 0;"  // relaxed lyric margin
                                                                 "}"
                                                                 "</style></head><body>"
                                                                 "<div class='song-canvas' style='line-height: " + QString::number(activeLineHeight, 'f', 3) + ";'>"
                       + headerHtml + m_parsedSongContentGrid +
                       "</div></body></html>";

    parsedEditor->setHtml(baseHtml);

    // Live status bar feedback
    int activeCols = (m_columnOverride > 0) ? m_columnOverride : m_currentSongMetrics.targetColumns;
    statusBar()->showMessage(
        QString("Font: %1pt | Columns: %2%3 | Zoom: %4 coarse / %5 fine")
            .arg(baseFontSize, 0, 'f', 1)
            .arg(activeCols)
            .arg(m_columnOverride > 0 ? " (manual)" : " (auto)")
            .arg(m_zoomCoarse)
            .arg(m_zoomFine),
        5000
        );
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

    qDebug() << "keyPressEvent key=" << event->key()
    << "modifiers=" << event->modifiers()
    << "state=" << currentState;

    if (currentState == PlayAlong) {
        bool ctrl  = event->modifiers() & Qt::ControlModifier;
        bool shift = event->modifiers() & Qt::ShiftModifier;
        int  key   = event->key();

        // marks that the user has manually tuned layout for this song
        auto markManualOverride = [&]() {
            m_useAutoLayout = false;
            saveSongLayoutToJson(m_currentFilePath);
        };

        // Spacebar — play/pause
        if (key == Qt::Key_Space && !ctrl && !shift) {
            if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
                m_mediaPlayer->pause();
            } else {
                if (!m_selectedAudioPath.isEmpty()) m_mediaPlayer->play();
            }
            event->accept(); return;
        }

        if (ctrl && !shift) {
            // Coarse zoom in/out
            if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
                if (m_zoomCoarse < 6) m_zoomCoarse++;
//                saveSongLayoutPreference(m_currentFilePath);
                markManualOverride();   // ← ADD
                parseChordProToGrid(m_rawSongContent);
                event->accept(); return;
            }
            if (key == Qt::Key_Minus) {
                if (m_zoomCoarse > -4) m_zoomCoarse--;
//                saveSongLayoutPreference(m_currentFilePath);
                markManualOverride();   // ← ADD
                parseChordProToGrid(m_rawSongContent);
                event->accept(); return;
            }
            // Reset all zoom and columns
            if (key == Qt::Key_0) {
                m_zoomCoarse = 0;
                m_zoomFine   = 0;
                m_columnOverride = 0;
                m_sectionSpacingBonus = 0;
                m_useAutoLayout       = true;  // ← re-enable optimizer
                saveSongLayoutToJson(m_currentFilePath);
//                saveSongLayoutPreference(m_currentFilePath);
                parseChordProToGrid(m_rawSongContent);
                event->accept(); return;
            }
            // Column override — Ctrl+Right/Left
            if (key == Qt::Key_Right) {
                qDebug() << "In column override right";
                int current = (m_columnOverride > 0) ? m_columnOverride
                                                     : m_currentSongMetrics.targetColumns;
                if (current < 4) m_columnOverride = current + 1;
                markManualOverride();   // ← ADD
                parseChordProToGrid(m_rawSongContent);
                event->accept(); return;
            }
            if (key == Qt::Key_Left) {
                qDebug() << "In column override left";
                if (m_columnOverride > 1) m_columnOverride--;
                else m_columnOverride = 0;  // back to auto
                markManualOverride();   // ← ADD
                parseChordProToGrid(m_rawSongContent);
                event->accept(); return;
            }
        }

        if (ctrl && shift) {
            // Fine zoom in/out — re-render only, no column recalculation needed
            if (key == Qt::Key_Plus || key == Qt::Key_Equal) {
                if (m_zoomFine < 8) m_zoomFine++;
//                saveSongLayoutPreference(m_currentFilePath);
                markManualOverride();   // ← ADD
                updatePlayAlongLayoutDensity();
                event->accept(); return;
            }
            if (key == Qt::Key_Minus) {
                if (m_zoomFine > -8) m_zoomFine--;
//                saveSongLayoutPreference(m_currentFilePath);
                markManualOverride();   // ← ADD
                updatePlayAlongLayoutDensity();
                event->accept(); return;
            }

            // NEW: Section spacing (graphic design white-space control)
            if (key == Qt::Key_Up) {
                if (m_sectionSpacingBonus < 20) m_sectionSpacingBonus++;
                markManualOverride();
                updatePlayAlongLayoutDensity();
                statusBar()->showMessage(
                    QString("Section spacing: +%1px  (Ctrl+0 to reset)").arg(m_sectionSpacingBonus), 2000);
                event->accept(); return;
            }
            if (key == Qt::Key_Down) {
                if (m_sectionSpacingBonus > 0) m_sectionSpacingBonus--;
                markManualOverride();
                updatePlayAlongLayoutDensity();
                statusBar()->showMessage(
                    QString("Section spacing: +%1px  (Ctrl+0 to reset)").arg(m_sectionSpacingBonus), 2000);
                event->accept(); return;
            }
        }
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::selectAudioTrack(QPushButton *clickedButton, const QString &trackPath, const QString &trackName) {
    // 1. If this button was already checked and is now being unchecked, stop playback
    if (!clickedButton->isChecked()) {
        m_mediaPlayer->stop();
        m_selectedAudioPath.clear();
        statusBar()->showMessage("Audio track deselected.");
        return;
    }

    // 2. Uncheck the other three audio buttons so only one can be active at a time
    m_btnTrackFull->setChecked(false);
    m_btnTrackBkg->setChecked(false);
    m_btnTrackSplit->setChecked(false);
    m_btnTrackSlow->setChecked(false);

    // 3. Keep our clicked button highlighted
    clickedButton->setChecked(true);

    // 4. Update the media player target path
    m_selectedAudioPath = trackPath;
    m_mediaPlayer->setSource(QUrl::fromLocalFile(m_selectedAudioPath));

    statusBar()->showMessage(QString("Selected %1: %2").arg(trackName, QFileInfo(trackPath).fileName()));
}

void MainWindow::analyzeChordProMetaData(const QString &rawInput) {

    // --- Reset all metrics ---
    m_currentSongMetrics.totalLines        = 0;
    m_currentSongMetrics.maxLineCharacters = 0;
    m_currentSongMetrics.sectionCount      = 0;
    m_currentSongMetrics.targetColumns     = 1;
    m_currentSongMetrics.sections.clear();

    QStringList lines      = rawInput.split('\n');
    bool insideSectionBlock = false;
    bool inGridBlock        = false;
    bool inTabBlock         = false;

    SongLayoutMetrics::SectionInfo currentSection;

    // Lambda: close the in-progress section and bank it
    auto closeSection = [&]() {
        if (insideSectionBlock) {
            m_currentSongMetrics.sections.append(currentSection);
            m_currentSongMetrics.sectionCount++;
            insideSectionBlock = false;
            currentSection = {};
        }
    };

    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();

        // Empty line closes unstructured text sections (matches parseChordProToGrid behaviour)
        if (line.isEmpty()) {
            if (insideSectionBlock && !inGridBlock && !inTabBlock)
                closeSection();
            continue;
        }

        // --- Grid / Tab block toggles ---
        if (line.startsWith("{start_of_grid}") || line.startsWith("{sog}")) { inGridBlock = true;  continue; }
        if (line.startsWith("{end_of_grid}")   || line.startsWith("{eog}")) { inGridBlock = false; continue; }
        if (line.startsWith("{start_of_tab}")  || line.startsWith("{sot}")) { inTabBlock  = true;  continue; }
        if (line.startsWith("{end_of_tab}")    || line.startsWith("{eot}")) { inTabBlock  = false; continue; }

        // --- Named section headers (mirrors parseChordProToGrid logic exactly) ---
        bool isChorusStart  = line.startsWith("{soc",           Qt::CaseInsensitive)
                             || line.startsWith("{start_of_chorus", Qt::CaseInsensitive);
        bool isCommentStart = line.startsWith("{c:",            Qt::CaseInsensitive)
                              || line.startsWith("{comment:",      Qt::CaseInsensitive);
        bool isInformalHdr  = line.startsWith("#");

        if (isChorusStart || isCommentStart || isInformalHdr) {
            closeSection();
            QString name = "Section";
            if      (isChorusStart)  name = "Chorus";
            else if (isInformalHdr)  name = line.mid(1).trimmed();
            else {
                QRegularExpression rx(R"(\{c(?:omment)?:?\s*([^}]*)\}?)",
                                      QRegularExpression::CaseInsensitiveOption);
                auto m = rx.match(line);
                if (m.hasMatch() && !m.captured(1).trimmed().isEmpty())
                    name = m.captured(1).trimmed();
            }
            currentSection = {name, 0, true};
            insideSectionBlock = true;
            continue;
        }

        // Explicit chorus close
        if (line.startsWith("{eoc", Qt::CaseInsensitive) ||
            line.startsWith("{end_of_chorus", Qt::CaseInsensitive)) {
            closeSection();
            continue;
        }

        // Skip metadata directives
        if (line.startsWith("{t:")     || line.startsWith("{title:")    ||
            line.startsWith("{st:")    || line.startsWith("{subtitle:") ||
            line.startsWith("{key:")   || line.startsWith("{tempo:")    ||
            line.startsWith("{capo:")  || line.startsWith("{artist:"))   { continue; }

        // Other unrecognised directives
        if (line.startsWith("{")) continue;

        // --- Unstructured text → auto-start anonymous section (mirrors parseChordProToGrid) ---
        if (!insideSectionBlock) {
            currentSection = {"", 0, false};
            insideSectionBlock = true;
        }

        // Content line — accumulate metrics
        if (insideSectionBlock) {
            m_currentSongMetrics.totalLines++;
            currentSection.lineCount++;

            QString lyricOnly = line;
            lyricOnly.remove(QRegularExpression("\\[.*?\\]"));
            int len = lyricOnly.length();
            if (len > m_currentSongMetrics.maxLineCharacters)
                m_currentSongMetrics.maxLineCharacters = len;
        }
    }

    closeSection(); // Flush any trailing open section

    // --- Logging ---
    if (m_debugVerboseLevel) {
        qDebug() << "=== CHORDLAB ELASTIC GEOMETRY RADAR ===";
        qDebug() << "Sections parsed:" << m_currentSongMetrics.sections.size();
        for (const auto &s : m_currentSongMetrics.sections)
            qDebug() << "  [" << (s.name.isEmpty() ? "(anon)" : s.name)
                     << "] lines=" << s.lineCount << " header=" << s.hasHeader;
        qDebug() << "Max lyric line width:" << m_currentSongMetrics.maxLineCharacters << "chars";
    }

    // --- Column + zoom decision ---
    if (m_columnOverride == 0 && m_useAutoLayout) {
        // NEW: Two-axis optimizer (horizontal + vertical fit)
        autoOptimizeLayout();
    } else if (m_columnOverride == 0) {
        // Legacy Radar: used when user has manually set zoom but not columns
        int displayWidth = parsedEditor->viewport()->width();
        if (displayWidth <= 0) {
            QScreen *screen = QGuiApplication::primaryScreen();
            displayWidth = screen ? (int)(screen->availableGeometry().width() * 0.6) : 800;
        }
        double baseFontSize          = 10.0 + (m_zoomCoarse * 2.0) + (m_zoomFine * 0.5);
        double approxCharWidthPixels = baseFontSize * 0.62;
        int allocationWidthPerCol    = qMin(m_currentSongMetrics.maxLineCharacters, 48) + 2;
        int calculatedCols           = displayWidth / (allocationWidthPerCol * approxCharWidthPixels);
        m_currentSongMetrics.targetColumns = qBound(1, calculatedCols, 4);
        if (m_currentSongMetrics.sectionCount < m_currentSongMetrics.targetColumns)
            m_currentSongMetrics.targetColumns = qMax(1, m_currentSongMetrics.sectionCount);
    }
    // Note: if m_columnOverride > 0, targetColumns is not used — parseChordProToGrid reads
    // m_columnOverride directly, so we intentionally do nothing here in that case.
}

/* void MainWindow::analyzeChordProMetaData(const QString &rawInput) {
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
        }
    }

    // elastic LINK MULTI-LAYER ENGINE
    // Calculate display boundaries dynamically based on the current size of the UI widget
    int displayWidth = parsedEditor->width();
    if (displayWidth <= 0) {
        // Fallback calculation if window hasn't fully drawn yet on startup
        QScreen *screen = QGuiApplication::primaryScreen();
        displayWidth = screen ? screen->availableGeometry().size().width() * 0.6 : 800;
    }

    // Estimate character width in pixels for Consolas/Courier based on current zoom scale
    double baseFontSize = 10.0 + (m_zoomCoarse * 2.0) + (m_zoomFine * 0.5);
    double approxCharWidthPixels = baseFontSize * 0.62;

    // Determine how many characters can physically fit across one column safely
    int maxCharsPerColumn = qMax(20, static_cast<int>(displayWidth / approxCharWidthPixels));

    if (m_currentSongMetrics.maxLineCharacters == 0) {
        m_currentSongMetrics.targetColumns = 1;
    } else {
        // Calculate how many columns can fit side-by-side without truncating text lines

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
} */

void MainWindow::autoOptimizeLayout() {

    // Prefer viewport() dimensions — excludes scrollbars and gives the true drawable area
    int displayWidth  = parsedEditor->viewport()->width();
    int displayHeight = parsedEditor->viewport()->height();

    if (displayWidth <= 0 || displayHeight <= 0) {
        // Widget not yet fully rendered — fall back to screen geometry
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            displayWidth  = (int)(screen->availableGeometry().width()  * 0.6);
            displayHeight = (int)(screen->availableGeometry().height() * 0.6);
        } else {
            displayWidth = 1024; displayHeight = 768;
        }
    }

    // Layout constants (pixels)
    const int titleAreaPx   = 80;   // title + subtitle + divider
    const int bodyMarginPx  = 24;   // 12px top + 12px bottom body padding
    const int interColPadPx = 35;   // right-padding inserted between columns
    const int minFontSizePt = 8;    // absolute floor — below this is unreadable on stage

    int availableHeight = displayHeight - titleAreaPx - bodyMarginPx;
    if (availableHeight < 100) availableHeight = 100; // sanity floor

    const auto &sections = m_currentSongMetrics.sections;
    int numSections = sections.isEmpty()
                          ? qMax(1, m_currentSongMetrics.sectionCount)
                          : (int)sections.size();

    // -----------------------------------------------------------------------
    // Outer loop: fewest columns first  (1 → 4)
    // Inner loop: largest font first    (zoomCoarse 4 → -2  ==  18pt → 6pt)
    // Accept immediately on first passing combination → maximises font/minimises cols
    // -----------------------------------------------------------------------
    for (int cols = 1; cols <= 4; ++cols) {

        if (cols > numSections) break; // pointless to have more cols than sections

        int interColTotalPad = interColPadPx * (cols - 1);
        int availColWidth    = (displayWidth - interColTotalPad - bodyMarginPx) / cols;
        if (availColWidth < 80) continue;  // column too narrow to display anything useful

        int sectionsPerCol = (int)qCeil((double)numSections / cols);

        for (int zc = 4; zc >= -2; --zc) {

            double fontSize        = 10.0 + (zc * 2.0);
            if (fontSize < minFontSizePt) continue;

            double charWidthPx     = fontSize * 0.62;    // empirical monospace ratio
            double pairHeightPx    = fontSize * 2.35;    // chord row + lyric row (with line-height 1.05)
            double headerHeightPx  = (fontSize + 2.0) * 1.9; // section heading row
            double sectionGapPx    = 5.0 + qMax(0, zc) * 1.5 + (double)m_sectionSpacingBonus;

            // === Horizontal check ===
            // The widest lyric line must fit inside one column (5% tolerance for HTML)
            int maxCharsPerCol = (int)((double)availColWidth / charWidthPx);
            if (m_currentSongMetrics.maxLineCharacters > (int)(maxCharsPerCol * 1.05))
                continue;  // this font is too large for this column width → try smaller

            // === Vertical check ===
            // Estimate the pixel height of the tallest column
            int maxColHeightPx = 0;

            for (int col = 0; col < cols; ++col) {
                int startSec = col * sectionsPerCol;
                int endSec   = qMin(startSec + sectionsPerCol, numSections);
                double colHeight = 0.0;

                for (int s = startSec; s < endSec; ++s) {
                    if (s < (int)sections.size()) {
                        if (sections[s].hasHeader) colHeight += headerHeightPx;
                        colHeight += sections[s].lineCount * pairHeightPx;
                    } else {
                        // Fallback estimate when section detail not available
                        colHeight += headerHeightPx + 4.0 * pairHeightPx;
                    }
                    colHeight += sectionGapPx;
                }
                maxColHeightPx = qMax(maxColHeightPx, (int)colHeight);
            }

            if (maxColHeightPx <= availableHeight) {
                // ✅ Both checks pass — commit this layout
                m_currentSongMetrics.targetColumns = cols;
                m_zoomCoarse = zc;
                m_zoomFine   = 0;  // Fine zoom stays as whatever was loaded from JSON

                if (m_debugVerboseLevel) {
                    qDebug() << "[AutoLayout] ✅" << cols << "col(s) @"
                             << fontSize << "pt  (zc=" << zc << ")"
                             << " | col h:" << maxColHeightPx << "/" << availableHeight << "px"
                             << " | col w:" << availColWidth << "px"
                             << " | max chars/col:" << maxCharsPerCol;
                }
                return;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Total fallback: 4 columns at minimum zoom
    // Means the song is genuinely too large to fit — user should use fine zoom
    // -----------------------------------------------------------------------
    m_currentSongMetrics.targetColumns = qMin(4, numSections);
    m_zoomCoarse = -2;
    m_zoomFine   = 0;
    qDebug() << "[AutoLayout] ⚠️ Fallback: 4-col / minimum zoom — song may need scrolling";
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
    m_btnTrackSplit->setChecked(false);  m_btnTrackSplit->setEnabled(false);
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
        QString targetFile = basePrefix + "_split" + ext;
        QFileInfo check(targetFile);
        if (check.exists() && check.isFile()) {
            m_audioTracks.splitPath = targetFile;
            m_btnTrackSplit->setEnabled(true);
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
    else if (m_btnTrackSplit->isEnabled()) {
        m_btnTrackSplit->setChecked(true);
        m_selectedAudioPath = m_audioTracks.splitPath;
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
//    if (cleanNote.length() > 1 && (cleanNote[1] == '#' || cleanNote[1] == 'B')) {
//        baseNote = cleanNote.left(2);
//    }
    // FLAT CHORDS to transpose >>> Change here...
    if (noteToken.length() > 1 && (noteToken[1] == '#' || noteToken[1] == 'b')) {
        baseNote = cleanNote.left(1) + noteToken[1];  // e.g. "Bb", "Eb", "C#"
    }

    // remove modifiers from the ORIGINAL string so 'm7' doesn't become 'M7'
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

    // Handle [chord] bracket notation (e.g. || [D]/// | [Bm]/// ||)
    if (line.contains('[')) {
        QString result = "";
        int lastPos = 0;
        QRegularExpression bracketRegex("\\[([^\\]]+)\\]");
        auto matches = bracketRegex.globalMatch(line);
        while (matches.hasNext()) {
            auto match = matches.next();
            result += line.mid(lastPos, match.capturedStart() - lastPos);
            result += "[" + transposeChord(match.captured(1), delta) + "]";
            lastPos = match.capturedEnd();
        }
        result += line.mid(lastPos);
        return result;
    }

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

                // red 'x' out-of-bounds guard...
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
        // --- Routine B: floating chord line above tabs ---
        // Keeps spacing intervals intact while transposing musical chords cleanly
        QString result = "";
        QString currentToken = "";

        // Handle [chord] bracket notation
        if (line.contains('[')) {
            int lastPos = 0;
            QRegularExpression bracketRegex("\\[([^\\]]+)\\]");
            auto matches = bracketRegex.globalMatch(line);
            while (matches.hasNext()) {
                auto match = matches.next();
                result += line.mid(lastPos, match.capturedStart() - lastPos);
                result += "[" + transposeChord(match.captured(1), chordDelta) + "]";
                lastPos = match.capturedEnd();
            }
            result += line.mid(lastPos);
            return result;
        }

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

        // Can load capo/transpose data here too
    }

    settings.endGroup();
    return layout;
}

void MainWindow::saveSongLayoutPreference(const QString &filePath) {
    if (filePath.isEmpty()) return;
    QSettings settings;
    QString songKey = QFileInfo(filePath).fileName();
    settings.beginGroup("SongLayouts/" + songKey);
    settings.setValue("hasOverride",     true);
    settings.setValue("zoomCoarse",      m_zoomCoarse);
    settings.setValue("zoomFine",        m_zoomFine);
    settings.setValue("columnOverride",  m_columnOverride);
    settings.endGroup();
}

void MainWindow::loadSongLayoutPreference(const QString &filePath) {
    if (filePath.isEmpty()) return;
    QSettings settings;
    QString songKey = QFileInfo(filePath).fileName();
    settings.beginGroup("SongLayouts/" + songKey);
    if (settings.value("hasOverride", false).toBool()) {
        m_zoomCoarse     = settings.value("zoomCoarse",     0).toInt();
        m_zoomFine       = settings.value("zoomFine",       0).toInt();
        m_columnOverride = settings.value("columnOverride", 0).toInt();
    } else {
        m_zoomCoarse     = 0;
        m_zoomFine       = 0;
        m_columnOverride = 0;
    }
    settings.endGroup();
}

QString MainWindow::getSongLayoutJsonPath(const QString &chordProPath) const {
    // Stores alongside the .cho/.pro file
    // e.g. "resources/pieces/Yesterday.cho" → "resources/pieces/Yesterday.layout.json"
    // This means layout preferences travel with the song files (great for network shares).
    QFileInfo fi(chordProPath);
    return fi.dir().absoluteFilePath(fi.completeBaseName() + ".layout.json");
}

void MainWindow::saveSongLayoutToJson(const QString &chordProPath) {
    if (chordProPath.isEmpty()) return;
    QString jsonPath = getSongLayoutJsonPath(chordProPath);

    QJsonObject obj;
    obj["zoomCoarse"]          = m_zoomCoarse;
    obj["zoomFine"]            = m_zoomFine;
    obj["columnOverride"]      = m_columnOverride;
    obj["sectionSpacingBonus"] = m_sectionSpacingBonus;
    obj["useAutoLayout"]       = m_useAutoLayout;
    // Store schema version for future compatibility
    obj["schemaVersion"]       = 1;

    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        if (m_debugVerboseLevel)
            qDebug() << "[Layout JSON] Saved:" << jsonPath;
    } else {
        // Song may be in a read-only directory (e.g. app bundle on macOS)
        // Silently fall back to QSettings for this path
        saveSongLayoutPreference(chordProPath);
        qDebug() << "[Layout JSON] ⚠️ Write failed — falling back to QSettings for:" << jsonPath;
    }
}

void MainWindow::loadSongLayoutFromJson(const QString &chordProPath) {
    if (chordProPath.isEmpty()) return;
    QString jsonPath = getSongLayoutJsonPath(chordProPath);

    QFile file(jsonPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        // No JSON file — first time this song has been opened, let optimizer run fresh
        m_zoomCoarse          = 0;
        m_zoomFine            = 0;
        m_columnOverride      = 0;
        m_sectionSpacingBonus = 0;
        m_useAutoLayout       = true;
        if (m_debugVerboseLevel) qDebug() << "[Layout JSON] No layout file — auto-layout enabled";
        return;
    }

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    m_zoomCoarse          = obj.value("zoomCoarse").toInt(0);
    m_zoomFine            = obj.value("zoomFine").toInt(0);
    m_columnOverride      = obj.value("columnOverride").toInt(0);
    m_sectionSpacingBonus = obj.value("sectionSpacingBonus").toInt(0);
    m_useAutoLayout       = obj.value("useAutoLayout").toBool(true);

    if (m_debugVerboseLevel)
        qDebug() << "[Layout JSON] Loaded — zc=" << m_zoomCoarse
                 << "zf=" << m_zoomFine << "cols=" << m_columnOverride
                 << "spacing=" << m_sectionSpacingBonus
                 << "autoLayout=" << m_useAutoLayout;
}
