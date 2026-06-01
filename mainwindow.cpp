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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // 1. Core Hardware & Audio Engine Instantiation First
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8f);

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::handlePlaybackStateChanged);

    // 2. Clear Internal State Variables Before UI Draws
    m_currentFilePath = "";
    m_parsedSongContentGrid = "";
    m_zoomScaleLevel = 0;
    m_transposeShift = 0;
    m_isLoadingFile = false;
    m_currentMode = ChordDisplayMode::CIL;
    m_currentTheme = Theme::Light;

    // 3. Sequential UI Component Construction
    setupMenus();
    setupLayout(); // Prepares central widgets and splitters
    setupToolBar();

    // 4. Set App Window Metric Policy (Consolidated to a stable 75% display footprint)
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QSize size = screen->availableGeometry().size();
        resize(size.width() * 0.75, size.height() * 0.75);
    }

    // 5. Final State Machine Launch
    setAppState(Idle);
}

void MainWindow::setupMenus() {
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Open...", QKeySequence::Open, this, &MainWindow::handleFileOpen);
    fileMenu->addAction("&Save Standardized", QKeySequence::Save, this, &MainWindow::handleFileSave);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close);
}

void MainWindow::setupToolBar() {
    QToolBar *settingsToolBar = addToolBar("Critical Settings");
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
        mainSplitter->setSizes(QList<int>({400, 400}));

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

void MainWindow::updateFunctionKeys() {
    disconnect(m_btnFn1, &QPushButton::clicked, nullptr, nullptr);
    disconnect(m_btnFn2, &QPushButton::clicked, nullptr, nullptr);
    disconnect(m_btnFn3, &QPushButton::clicked, nullptr, nullptr);
    disconnect(m_btnFn4, &QPushButton::clicked, nullptr, nullptr);

    if (currentState == OpenEdit || currentState == Idle) {
        m_btnFn1->setText("📂 Open");
        m_btnFn2->setText("💾 Save");
        m_btnFn3->setText("💾 As...");
        m_btnFn4->setText("🔄 Parse");

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
/*    } else if (currentState == PlayAlong) {
        m_btnFn1->setText("⏮ Rewind");
        m_btnFn2->setText("▶ Play");
        m_btnFn3->setText("⏸ Pause");
        m_btnFn4->setText("⏭ End");

        connect(m_btnFn1, &QPushButton::clicked, this, [=]() {
            m_mediaPlayer->setPosition(0);
            statusBar()->showMessage("Rewound to beginning.");
        });
        connect(m_btnFn2, &QPushButton::clicked, this, [=]() {
            if (m_selectedAudioPath.isEmpty()) {
                statusBar()->showMessage("No audio track selected or available for playback.");
                return;
            }
            m_mediaPlayer->play();
        });
        connect(m_btnFn3, &QPushButton::clicked, this, [=]() {
            m_mediaPlayer->pause();
        });
        connect(m_btnFn4, &QPushButton::clicked, this, [=]() {
            m_mediaPlayer->setPosition(m_mediaPlayer->duration());
        });
    }
*/
    } else if (currentState == PlayAlong) {
        m_btnFn1->setText("⏮ Rewind");

        // Dynamic Play/Pause Button State Checks
        if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            // Try loading a native resource icon first, fallback to text emoji if empty
            m_btnFn2->setIcon(QIcon(":/resources/icons/pause.png"));
            m_btnFn2->setText("⏸ Pause");
        } else {
            m_btnFn2->setIcon(QIcon(":/resources/icons/play.png"));
            m_btnFn2->setText("▶ Play");
        }

        m_btnFn3->setIcon(QIcon(":/resources/icons/stop.png"));
        m_btnFn3->setText("■ Stop");
        m_btnFn4->setText("⏭ End");

        connect(m_btnFn1, &QPushButton::clicked, this, [=]() {
            m_mediaPlayer->setPosition(0);
            statusBar()->showMessage("Rewound to beginning.");
        });

        // Unified Play/Pause Action Router
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
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file: ") + file.errorString());
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    m_rawSongContent = content;

    m_isLoadingFile = true; // Block signal loops safely
    originalEditor->setPlainText(content);
    m_isLoadingFile = false; // RE-ENABLE safely (fixed bug here)

    // Set state explicitly handles rendering separation cleanly
    setAppState(OpenEdit);

    checkForCompanionAudio(m_currentFilePath);
    statusBar()->showMessage(tr("Loaded: ") + fileName);
}

void MainWindow::handleFileSave() {
    if (m_currentFilePath.isEmpty()) {
        QString saveName = QFileDialog::getSaveFileName(this,
                                                        tr("Save ChordPro File"), "", tr("ChordPro Files (*.chopro *.cho *.pro *.txt *.crd)"));
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

    statusBar()->showMessage(tr("Saved successfully: ") + m_currentFilePath, 3000);
}

QString MainWindow::runInitialParse(const QString &rawInput) {
    QString result = "<html><head><style>"
                     "body { font-family: 'Consolas', monospace; white-space: pre; font-size: 10pt; }"
                     + getThemeStyles() +
                     "</style></head><body>";

    QRegularExpression titleRegex(R"(\{(?:title|t):\s*(.*?)\})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression artistRegex("\\{artist:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression keyRegex("\\{key:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression tempoRegex("\\{tempo:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression capoRegex("\\{capo:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);

    QString title = titleRegex.match(rawInput).captured(1).trimmed();
    QString artist = artistRegex.match(rawInput).captured(1).trimmed();
    QString key = keyRegex.match(rawInput).captured(1).trimmed();
    QString tempo = tempoRegex.match(rawInput).captured(1).trimmed();
    QString capo = capoRegex.match(rawInput).captured(1).trimmed();

    if (!title.isEmpty()) {
        result += "<div class='header-block'>";
        result += "<h1 style='margin:0; font-size: 16pt;'>" + title + "</h1>";
        if (!artist.isEmpty())   result += "Artist: <b>" + artist + "</b><br>";
        if (!key.isEmpty())   result += "Key: <b>" + key + "</b><br>";
        if (!tempo.isEmpty()) result += "Tempo: <b>" + tempo + "</b><br>";
        if (!capo.isEmpty())  result += "Capo: <b>" + capo + "</b>";
        result += "</div>";
    }

    QStringList lines = rawInput.split('\n');
    bool inProtectedBlock = false;
    bool lastLineWasEmpty = false;
    bool inChorus = false;
    bool inVerse = false;

    for (const QString &line : lines) {
        QString workingLine = line;
        workingLine.remove('\r');
        QString trimmedLine = workingLine.trimmed();
        bool lineHandled = false;

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

        if (isChorusStart) {
            if (inVerse) { result += "</div>"; inVerse = false; }
            inChorus = true;
            result += "<div class='chorus-box'><span class='section-header'>Chorus</span><br>";
            lineHandled = true;
        }
        else if (trimmedLine.startsWith("{end_of_chorus}") || trimmedLine.startsWith("{eoc")) {
            inChorus = false;
            result += "</div>";
            lineHandled = true;
        }
        else if (trimmedLine.startsWith("{c:") || trimmedLine.startsWith("{comment:")) {
            if (inVerse) result += "</div>";
            if (inChorus) { result += "</div>"; inChorus = false; }

            QString commentText = trimmedLine.section(':', 1).chopped(1).trimmed();
            inVerse = true;
            result += "<div class='verse-box'><span class='section-header'>" + commentText + "</span><br>";
            lineHandled = true;
        }
        else if (trimmedLine.startsWith("{sot")) { inProtectedBlock = true; lineHandled = true; }
        else if (trimmedLine.startsWith("{eot")) { inProtectedBlock = false; lineHandled = true; }

        if (!lineHandled) {
            if (inProtectedBlock) {
                result += line + "<br>";
            }
            else if (inChorus) {
                result += processLineContent(line) + "<br>";
            }
            else {
                if (!inVerse) {
                    inVerse = true;
                    result += "<div class='verse-box'>";
                }
                result += processLineContent(line) + "<br>";
            }
        }
    }
    if (inVerse || inChorus) result += "</div>";
    return result + "</body></html>";
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
        parsedEditor->setHtml(runInitialParse(m_rawSongContent));
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

    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith('{') && (line.contains("comment") || line.contains("c:"))) {
            if (insideSectionBlock) {
                currentSectionHtml += "</div>";
                gatheredSections.append(currentSectionHtml);
                currentSectionHtml = "";
            }

            QString sectionName = line;
            sectionName.remove(QRegularExpression("^\\{c(omment)?:?\\s*"));
            sectionName.remove('}');

            currentSectionHtml += "<div class='song-section'>";
            currentSectionHtml += QString("<div class='section-heading'>%1</div>").arg(sectionName);
            insideSectionBlock = true;
            continue;
        }

        if (line.startsWith('{')) continue;

        if (insideSectionBlock) {
            QString chordLine = "";
            QString lyricLine = "";
            int linePos = 0;

            QRegularExpression chordRegex("\\[(.*?)\\]");
            auto matches = chordRegex.globalMatch(line);
            if (!matches.hasNext()) {
                for (int i = 0; i < line.length(); ++i) {
                    QChar ch = line[i];
                    if (ch == ' ') lyricLine += "&nbsp;"; else lyricLine += ch;
                }
            } else {
                while (matches.hasNext()) {
                    auto match = matches.next();
                    int matchStart = match.capturedStart();

                    while (linePos < matchStart) {
                        QChar ch = line[linePos++];
                        if (ch == ' ') lyricLine += "&nbsp;"; else lyricLine += ch;
                        chordLine += "&nbsp;";
                    }

                    QString chordText = match.captured(1);
                    QString transposed = transposeChord(chordText, m_transposeShift);
                    int chordLen = transposed.length();

                    chordLine += QString("<span class='chord-line'>%1</span>").arg(transposed);

                    int closePos = match.capturedEnd();
                    int nextMatchStart = matches.hasNext() ? line.indexOf('[', closePos) : line.length();
                    int availableLyricGap = nextMatchStart - closePos;
                    if (chordLen > availableLyricGap) {
                        for (int i = 0; i < (chordLen - availableLyricGap); ++i) {
                            lyricLine += "&nbsp;";
                        }
                    }
                    linePos = closePos;
                }

                while (linePos < line.length()) {
                    QChar ch = line[linePos++];
                    if (ch == ' ') lyricLine += "&nbsp;"; else lyricLine += ch;
                    chordLine += "&nbsp;";
                }
            }

            QString lineBlockHtml = "<div style='white-space: nowrap; line-height: 1.2;'>";
            if (!chordLine.trimmed().isEmpty()) {
                lineBlockHtml += QString("<p style='margin: 0; padding: 0;' class='chord-line'>%1</p>").arg(chordLine);
            } else {
                lineBlockHtml += "<p style='margin: 0; padding: 0;' class='chord-line'>&nbsp;</p>";
            }

            lineBlockHtml += QString("<p style='margin: 0 0 6px 0; padding: 0;' class='lyric-text'>%1</p>").arg(lyricLine.isEmpty() ? "&nbsp;" : lyricLine);
            lineBlockHtml += "</div>";
            currentSectionHtml += lineBlockHtml;
        }
    }

    if (insideSectionBlock) {
        currentSectionHtml += "</div>";
        gatheredSections.append(currentSectionHtml);
    }

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
            QString rightPadding = (col < numCols - 1) ? "padding-right: 30px;" : "";
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
    updatePlayAlongLayoutDensity(); // Legitimate execution path strictly tied to Play mode initialization
}

void MainWindow::updatePlayAlongLayoutDensity() {
    // Standardizing baseline scale settings (Dropped to 10pt base down from 12pt to fix overflow issues)
    int baseFontSize = 10 + (m_zoomScaleLevel * 2);
    int headerMarginBottom = 4 + m_zoomScaleLevel;
    int sectionMarginBottom = 12 + (m_zoomScaleLevel * 3);
    double activeLineHeight = 1.15 + (m_zoomScaleLevel * 0.04);

    parsedEditor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    parsedEditor->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Apply color palettes dynamically from current system theme setup
    QString bgColor = (m_currentTheme == Dark) ? "#121212" : "#ffffff";
    QString txtColor = (m_currentTheme == Dark) ? "#E0E0E0" : "#222222";
    QString chordColor = (m_currentTheme == Dark) ? "#82AAFF" : "#b22222";

    QString baseHtml = "<html><head><style>"
                       "body {"
                       "  background-color: " + bgColor + ";"
                       "  color: " + txtColor + ";"
                       "  margin: 10px; padding: 0;"
                       "}"
                       "h1 { font-size: " + QString::number(baseFontSize + 4) + "pt; font-weight: bold; font-family: sans-serif; margin: 0 0 4px 0; }"
                       ".section-heading {"
                       "  font-size: " + QString::number(baseFontSize + 1) + "pt;"
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
                       "  font-weight: bold; color: " + chordColor + ";"
                       "  font-family: 'Consolas', 'Courier New', monospace !important;"
                       "  font-size: " + QString::number(baseFontSize) + "pt;"
                       "}"
                       ".lyric-text {"
                       "  color: " + txtColor + ";"
                       "  font-family: 'Consolas', 'Courier New', monospace !important;"
                       "  font-size: " + QString::number(baseFontSize) + "pt;"
                       "}"
                       "</style></head><body>"
                       "<div class='song-canvas' style='line-height: " + QString::number(activeLineHeight) + ";'>"
                       + m_parsedSongContentGrid +
                       "</div></body></html>";
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
        if (currentState == PlayAlong) {
            parseChordProToGrid(m_rawSongContent); // Re-calculate target columns dynamically when zooming!
        }
    }
}

void MainWindow::onZoomOutTriggered() {
    if (m_zoomScaleLevel > -4) {
        m_zoomScaleLevel--;
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

        // 1. Detect structural comment section headers
        if (line.startsWith('{') && (line.contains("comment") || line.contains("c:"))) {
            m_currentSongMetrics.sectionCount++;
            inSection = true;
            continue;
        }

        // Skip metadata lines like title, artist, key
        if (line.startsWith('{')) continue;

        // 2. Compute visual line lengths (cil_wll) and count lines (cil_lof)
        if (inSection) {
            m_currentSongMetrics.totalLines++; // Increment our cil_lof count

            // To find the true visual text footprint, look at how long the line is WITHOUT chord brackets
            QString lyricOnlyLine = line;
            lyricOnlyLine.remove(QRegularExpression("\\[.*?\\]"));

            int cleanLength = lyricOnlyLine.length();
            if (cleanLength > m_currentSongMetrics.maxLineCharacters) {
                m_currentSongMetrics.maxLineCharacters = cleanLength; // Lock in the new cil_wll peak
            }
        }
    }

    // 3. BRUTE FORCE LAYOUT DECISION (Initial setup for Play mode display)
    // If a song has long lines, force 1 column to avoid horizontal truncation.
    if (m_currentSongMetrics.maxLineCharacters > 68) {
        m_currentSongMetrics.targetColumns = 1;
    }
    // If the song has multiple distinct sections, try to give each section its own column space!
    else if (m_currentSongMetrics.sectionCount >= 3 || m_currentSongMetrics.totalLines > 24) {
        m_currentSongMetrics.targetColumns = 3; // Perfect fit for side-by-side song structure maps
    } else if (m_currentSongMetrics.sectionCount == 2 || m_currentSongMetrics.totalLines > 12) {
        m_currentSongMetrics.targetColumns = 2;
    } else {
        m_currentSongMetrics.targetColumns = 1; // Default layout for ultra-short single verse snippets
    }

    qDebug() << "==================================================";
    qDebug() << "===  CHORDLAB PRE-ANALYSIS COMPLETE           ===";
    qDebug() << "===  Total Line Count (cil_lof):" << m_currentSongMetrics.totalLines;
    qDebug() << "===  Max Line Width   (cil_wll):" << m_currentSongMetrics.maxLineCharacters << "chars";
    qDebug() << "===  Detected Sections:        " << m_currentSongMetrics.sectionCount;
    qDebug() << "===  Calculated Target Columns:" << m_currentSongMetrics.targetColumns;
    qDebug() << "==================================================";
}

QString MainWindow::transposeChord(const QString &chord, int semitones) {
    static const QStringList notes = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};

    QRegularExpression re("^([A-G][b#]?)");
    auto match = re.match(chord);
    if (!match.hasMatch()) return chord;

    QString root = match.captured(1);
    QString suffix = chord.mid(root.length());

    int idx = notes.indexOf(root);
    if (idx == -1) return chord;

    int newIdx = (idx + semitones) % 12;
    if (newIdx < 0) newIdx += 12;

    return notes[newIdx] + suffix;
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