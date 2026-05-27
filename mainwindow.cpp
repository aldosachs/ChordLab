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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    m_mediaPlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.8f);

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::handlePlaybackStateChanged);

    QScreen *screen = QGuiApplication::primaryScreen();
    QSize size = screen->availableGeometry().size();
    resize(size.width() * 0.8, size.height() * 0.8);

    m_currentFilePath = "";
    m_parsedSongContentGrid = "";
    m_zoomScaleLevel = 0;

    setupMenus();
    setupToolBar();
    setupLayout();
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
        parsedEditor->setHtml(runInitialParse(originalEditor->toPlainText()));
    });

    QPushButton *btnModeToggle = new QPushButton("Mode: Edit 📝");
    btnModeToggle->setStyleSheet("QPushButton { background-color: #004060; color: white; padding: 5px; min-width: 100px; }");

    connect(btnModeToggle, &QPushButton::clicked, this, &MainWindow::togglePlaybackMode);

    layout->addWidget(btnModeToggle);
    layout->addWidget(m_btnTheme);
    layout->addWidget(m_btnTransposeDown);
    layout->addWidget(btnReset);
    layout->addWidget(m_btnTransposeUp);

    settingsToolBar->addWidget(container);

    m_viewToggleBtn = new QPushButton("View: ChordPro (CIL)");
    m_viewToggleBtn->setStyleSheet("QPushButton { background-color: #0047AB; color: white; }");

    connect(m_viewToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleDisplayMode);
    layout->addWidget(m_viewToggleBtn);

    connect(m_btnTransposeUp, &QPushButton::clicked, this, [=](){ shiftTransposition(1); });
    connect(m_btnTransposeDown, &QPushButton::clicked, this, [=](){ shiftTransposition(-1); });
    connect(m_btnTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);
}

void MainWindow::togglePlaybackMode() {
    if (currentState == Idle) {
        statusBar()->showMessage("Please open a song file before entering Play-Along mode.");
        return;
    }

    QPushButton *btn = qobject_cast<QPushButton*>(sender());

    if (currentState == OpenEdit) {
        if (btn) btn->setText("Mode: Play 🎤");
        setAppState(PlayAlong);
    } else {
        if (btn) btn->setText("Mode: Edit 📝");
        setAppState(OpenEdit);
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

    } else if (currentState == PlayAlong) {
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
}

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
        m_rawSongContent = originalEditor->toPlainText();
        parsedEditor->setHtml(runInitialParse(m_rawSongContent));
        parseChordProToGrid(m_rawSongContent);
    });

    setCentralWidget(mainSplitter);
}

void MainWindow::setAppState(AppState state) {
    currentState = state;
    updateFunctionKeys();

    switch(state) {
    case Idle:
        statusBar()->showMessage("Ready. Open a ChordPro file to begin.");
        mainSplitter->hide();
        break;

    case OpenEdit:
        statusBar()->showMessage("Editing Mode: Analyzing ChordPro syntax...");
        mainSplitter->show();

        parsedEditor->document()->clear(); {
            QFont normalFont = parsedEditor->document()->defaultFont();
            normalFont.setFamily("Consolas");
            parsedEditor->document()->setDefaultFont(normalFont);
        }

        mainSplitter->setSizes(QList<int>({400, 400}));
        originalEditor->show();
        parsedEditor->show();
        parsedEditor->setHtml(runInitialParse(m_rawSongContent));
        break;

    case PlayAlong:
        statusBar()->showMessage("Play-along Mode Active. Space: Toggle Play | Ctrl +/-: Zoom");
        originalEditor->hide();
        parsedEditor->show();

        mainSplitter->setSizes(QList<int>({0, this->width()}));
        updatePlayAlongLayoutDensity();
        break;
    }
}

QString MainWindow::generateFullScreenHtml(const QString& parsedSongContent) {
    QString html = "<html><head><style>"
                   "body {"
                   "  background-color: #ffffff;"
                   "  font-family: sans-serif;"
                   "  margin: 20px;"
                   "  padding: 0;"
                   "}"
                   ".song-canvas {"
                   "  display: flex;"
                   "  flex-direction: column;"
                   "  flex-wrap: wrap;"
                   "  height: 85vh;"
                   "  align-content: flex-start;"
                   "  gap: 25px;"
                   "}"
                   ".song-section {"
                   "  width: 380px;"
                   "  break-inside: avoid;"
                   "  page-break-inside: avoid;"
                   "}"
                   ".chord-line { font-weight: bold; color: #b22222; font-family: sans-serif; min-height: 1.2em; }"
                   ".tab-block {"
                   "  display: block;"
                   "  clear: both;"
                   "  font-family: 'Consolas', 'Courier New', monospace !important;"
                   "  white-space: pre !important;"
                   "  background-color: #f4f6f9;"
                   "  padding: 12px;"
                   "  border-left: 4px solid #007acc;"
                   "  margin: 8px 0;"
                   "  line-height: 1.3;"
                   "}"
                   "</style></head><body>";

    html += "<div class='song-canvas'>";
    html += parsedSongContent;
    html += "</div></body></html>";

    return html;
}

void MainWindow::handleFileSave() {
    if (m_currentFilePath.isEmpty()) {
        QString saveName = QFileDialog::getSaveFileName(this,
                                                        tr("Save ChordPro File"), "", tr("ChordPro Files (*.chopro *.pro *.txt *.crd)"));

        if (saveName.isEmpty()) {
            return;
        }
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

void MainWindow::handleFileOpen() {
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open ChordPro File"), "", tr("ChordPro Files (*.chopro *.pro *.txt *.crd)"));

    if (fileName.isEmpty()) {
        return;
    }

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

    originalEditor->setPlainText(content);
    setAppState(OpenEdit);

    QString expertVersion = runInitialParse(content);
    parsedEditor->setHtml(expertVersion);
    statusBar()->showMessage(tr("Loaded: ") + fileName);
    checkForCompanionAudio(m_currentFilePath);
    parseChordProToGrid(m_rawSongContent);
}

QString MainWindow::runInitialParse(const QString &rawInput) {
    QString result = "<html><head><style>"
                     "body { font-family: 'Consolas', monospace; white-space: pre; }"
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
        result += "<h1 style='margin:0;'>" + title + "</h1>";
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
    m_currentMode = (m_currentMode == ChordDisplayMode::CIL) ?
                        ChordDisplayMode::CAL : ChordDisplayMode::CIL;

    if (m_currentMode == ChordDisplayMode::CIL) {
        m_viewToggleBtn->setText("View: ChordPro (CIL)");
    } else {
        m_viewToggleBtn->setText("View: LeadSheet (CAL)");
    }

    parsedEditor->setHtml(runInitialParse(m_rawSongContent));
}

void MainWindow::shiftTransposition(int delta) {
    m_transposeShift += delta;

    if (m_transposeShift >= 12 || m_transposeShift <= -12) {
        m_transposeShift = 0;
    }

    statusBar()->showMessage(QString("Transposition: %1 semitones").arg(m_transposeShift));
    parsedEditor->setHtml(runInitialParse(m_rawSongContent));
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

void MainWindow::toggleTheme() {
    m_currentTheme = (m_currentTheme == Light) ? Dark : Light;
    m_btnTheme->setText(m_currentTheme == Light ? "Light theme" : "Dark theme");
    parsedEditor->setHtml(runInitialParse(m_rawSongContent));
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

void MainWindow::selectAudioTrack(QPushButton *clickedButton, const QString &filePath, const QString &trackType) {
    m_btnTrackFull->setChecked(false);
    m_btnTrackBkg->setChecked(false);
    m_btnTrackSlow->setChecked(false);

    clickedButton->setChecked(true);

    if (m_selectedAudioPath != filePath && !m_selectedAudioPath.isEmpty()) {
        m_mediaPlayer->stop();
        statusBar()->showMessage(tr("Stopped current playback. Loaded: %1").arg(trackType));
    }

    m_selectedAudioPath = filePath;
    m_mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
    statusBar()->showMessage(tr("Audio Ready [%1]: %2").arg(trackType, QFileInfo(filePath).fileName()));
}

void MainWindow::parseChordProToGrid(const QString &rawText) {
    m_parsedSongContentGrid.clear();

    QStringList lines = rawText.split('\n');
    QString currentSectionHtml;
    bool inSection = false;
    bool inMonospaceBlock = false;

    QRegularExpression commentRegex(R"(\{(?:c|comment):\s*([^}]+)\})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression metadataRegex(R"(\{(title|subtitle|tempo|time|key):\s*([^}]+)\})", QRegularExpression::CaseInsensitiveOption);

    QString metadataBlock = "";

    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() && !inMonospaceBlock) continue;

        QRegularExpressionMatch metaMatch = metadataRegex.match(line);
        if (metaMatch.hasMatch()) {
            QString tag = metaMatch.captured(1).toLower();
            QString value = metaMatch.captured(2);

            if (tag == "title") {
                metadataBlock += QString("<h1 style='margin: 0 0 5px 0; color: #111;'>%1</h1>").arg(value);
            } else if (tag == "subtitle") {
                metadataBlock += QString("<h3 style='margin: 0 0 10px 0; color: #666; font-style: italic;'>%1</h3>").arg(value);
            } else {
                tag[0] = tag[0].toUpper();
                metadataBlock += QString("<span style='font-size: 10pt; color: #555;'><b>%1:</b> %2</span>&nbsp;&nbsp;").arg(tag, value);
            }
            continue;
        }

        if (line.contains("{sog", Qt::CaseInsensitive) || line.contains("{sot", Qt::CaseInsensitive)) {
            if (!inSection) {
                currentSectionHtml += "<div class='song-section'>";
                inSection = true;
            }
            currentSectionHtml += "<div class='tab-block'>";
            inMonospaceBlock = true;
            continue;
        }
        if (line.contains("{eog", Qt::CaseInsensitive) || line.contains("{eot", Qt::CaseInsensitive)) {
            currentSectionHtml += "</div>";
            inMonospaceBlock = false;
            continue;
        }

        if (inMonospaceBlock) {
            QString escapedLine = line.toHtmlEscaped();
            escapedLine.replace(QRegularExpression(R"(\[([A-G][b#]?[mM]?[0-9]*)\])"), "<b>\\1</b>");
            currentSectionHtml += escapedLine + "\n";
            continue;
        }

        QRegularExpressionMatch commentMatch = commentRegex.match(line);
        if (commentMatch.hasMatch()) {
            if (inSection) {
                currentSectionHtml += "</div>";
            }

            QString sectionName = commentMatch.captured(1);
            currentSectionHtml += "<div class='song-section'>";

            if (!metadataBlock.isEmpty()) {
                currentSectionHtml += metadataBlock + "<br><br>";
                metadataBlock.clear();
            }

            currentSectionHtml += QString("<h2 style='color: #007acc; margin: 0 0 10px 0; font-family: sans-serif; border-bottom: 2px solid #eef;'>%1</h2>").arg(sectionName);
            inSection = true;
            continue;
        }

        if (inSection) {
            // Treat the whole line as an unbreakable visual horizontal row
            QString rowHtml = "<div style='margin-bottom: 6px; white-space: nowrap;'>";

            int pos = 0;
            QString currentChord = "";
            QString currentPhrase = "";

            while (pos < line.length()) {
                if (line[pos] == '[') {
                    if (!currentChord.isEmpty() || !currentPhrase.isEmpty()) {
                        rowHtml += QString("<div style='display: inline-block; vertical-align: bottom; text-align: left; margin-right: 2px;'>") +
                                   QString("<div class='chord-line'>%1</div>").arg(currentChord.isEmpty() ? "&nbsp;" : currentChord) +
                                   QString("<div class='lyric-text'>%1</div>").arg(currentPhrase.isEmpty() ? "&nbsp;" : currentPhrase) +
                                   QString("</div>");
                        currentPhrase.clear();
                    }

                    int closePos = line.indexOf(']', pos);
                    if (closePos != -1) {
                        currentChord = line.mid(pos + 1, closePos - pos - 1);
                        pos = closePos + 1;
                        continue;
                    }
                }

                if (line[pos] == ' ') {
                    currentPhrase += "&nbsp;";
                } else {
                    currentPhrase += line[pos];
                }
                pos++;
            }

            // Flush any remaining trailing words or spaces at the end of the line
            if (!currentChord.isEmpty() || !currentPhrase.isEmpty()) {
                rowHtml += QString("<div style='display: inline-block; vertical-align: bottom; text-align: left; margin-right: 2px;'>") +
                           QString("<div class='chord-line'>%1</div>").arg(currentChord.isEmpty() ? "&nbsp;" : currentChord) +
                           QString("<div class='lyric-text'>%1</div>").arg(currentPhrase.isEmpty() ? "&nbsp;" : currentPhrase) +
                           QString("</div>");
            }

            rowHtml += "</div>";
            currentSectionHtml += rowHtml;
        }
    }

    if (inSection) {
        currentSectionHtml += "</div>";
    }

    m_parsedSongContentGrid = currentSectionHtml;
}

void MainWindow::onZoomInTriggered() {
    if (m_zoomScaleLevel < 6) {
        m_zoomScaleLevel++;
        updatePlayAlongLayoutDensity();
    }
}

void MainWindow::onZoomOutTriggered() {
    if (m_zoomScaleLevel > -4) {
        m_zoomScaleLevel--;
        updatePlayAlongLayoutDensity();
    }
}

void MainWindow::updatePlayAlongLayoutDensity() {
    int baseFontSize = 12 + (m_zoomScaleLevel * 2);
    int columnWidth = 360 + (m_zoomScaleLevel * 40); // Increased step scale to help columns form

    // Add explicit diagnostics to track window properties and scale level changes
    qDebug() << "=== CHORDLAB ZOOM LAYOUT MONITOR ===";
    qDebug() << "Current Zoom Scale Level Steps:" << m_zoomScaleLevel;
    qDebug() << "Calculated Font Baseline Sizing:" << baseFontSize << "pt";
    qDebug() << "Target Column Bounds Width Constraint:" << columnWidth << "px";

    QString cssLayoutMode;

    if (m_zoomScaleLevel >= 3) {
        cssLayoutMode = ".song-canvas {"
                        "  display: block;"
                        "  height: auto;"
                        "  margin: 0 auto;"
                        "  max-width: 900px;"
                        "} "
                        ".song-section {"
                        "  width: 100%;"
                        "  margin-bottom: 30px;"
                        "  white-space: nowrap !important;"
                        "}";
    } else {
        // Multi-Column Mode: Setting max height forces the layout container to wrap side-by-side
        cssLayoutMode = QString(
                            ".song-canvas {"
                            "  display: flex;"
                            "  flex-direction: column;"
                            "  flex-wrap: wrap;"
                            "  height: 75vh;" // Slightly lowered container floor height to force column wrapping sooner
                            "  align-content: flex-start;"
                            "  gap: 35px;"
                            "} "
                            ".song-section {"
                            "  width: %1px;"
                            "  break-inside: avoid-column;"
                            "  page-break-inside: avoid;"
                            "  white-space: nowrap !important;"
                            "}").arg(columnWidth);
    }

    QString baseHtml = "<html><head><style>"
                       "body {"
                       "  background-color: #ffffff;"
                       "  font-family: sans-serif;"
                       "  margin: 25px; padding: 0;"
                       "}"
                       + cssLayoutMode +
                       "h1 { font-size: " + QString::number(baseFontSize + 6) + "pt; margin: 0 0 5px 0; }"
                                                                                "h2 { font-size: " + QString::number(baseFontSize + 3) + "pt; color: #007acc; margin: 0 0 10px 0; border-bottom: 2px solid #eef; }"
                                                             "h3 { font-size: " + QString::number(baseFontSize + 1) + "pt; margin: 0 0 10px 0; color: #666; }"
                                                             ".chord-line {"
                                                             "  font-weight: bold; color: #b22222;"
                                                             "  font-family: sans-serif;"
                                                             "  font-size: " + QString::number(baseFontSize + 1) + "pt;"
                                                             "  min-height: 1.2em;"
                                                             "}"
                                                             ".lyric-text {"
                                                             "  color: #222; font-family: sans-serif;"
                                                             "  font-size: " + QString::number(baseFontSize) + "pt;"
                                                         "}"
                                                         ".tab-block {"
                                                         "  display: block; clear: both;"
                                                         "  font-family: 'Consolas', 'Courier New', monospace !important;"
                                                         "  font-size: " + QString::number(baseFontSize - 1) + "pt;"
                                                             "  white-space: pre !important;"
                                                             "  background-color: #f4f6f9;"
                                                             "  padding: 12px; border-left: 4px solid #007acc; margin: 10px 0;"
                                                             "}"
                                                             "</style></head><body>"
                                                             "<div class='song-canvas'>"
                       + m_parsedSongContentGrid +
                       "</div></body></html>";

    parsedEditor->setHtml(baseHtml);
}

/* void MainWindow::updatePlayAlongLayoutDensity() {
    int baseFontSize = 12 + (m_zoomScaleLevel * 2);
    int columnWidth = 360 + (m_zoomScaleLevel * 40);   // was 30

    // explicit diagnostics to track window properties and scale level changes
    qDebug() << "=== CHORDLAB ZOOM LAYOUT MONITOR ===";
    qDebug() << "Current Zoom Scale Level Steps:" << m_zoomScaleLevel;
    qDebug() << "Calculated Font Baseline Sizing:" << baseFontSize << "pt";
    qDebug() << "Target Column Bounds Width Constraint:" << columnWidth << "px";

    QString cssLayoutMode;

    if (m_zoomScaleLevel >= 3) {
        // High-Zoom: Single Column scrolling sheet (SBP style)
        cssLayoutMode = ".song-canvas {"
                        "  display: block;"
                        "  height: auto;"
                        "  margin: 0 auto;"
                        "  max-width: 900px;"
                        "} "
                        ".song-section {"
                        "  width: 100%;"
                        "  margin-bottom: 30px;"
                        "  white-space: nowrap !important;" // Protects full lyric lines from breaking
                        "}";
    } else {
        // Multi-Column: Side-by-side fitting canvas
        cssLayoutMode = QString(
                            ".song-canvas {"
                            "  display: flex;"
                            "  flex-direction: column;"
                            "  flex-wrap: wrap;"
                            "  height: 75vh;" // was 82vh
                            "  align-content: flex-start;"
                            "  gap: 35px;"
                            "} "
                            ".song-section {"
                            "  width: %1px;"
                            "  break-inside: avoid;"
                            "  page-break-inside: avoid;"
                            "  white-space: nowrap !important;" // Keeps lines intact in column views
                            "}").arg(columnWidth);
    }

    QString baseHtml = "<html><head><style>"
                       "body {"
                       "  background-color: #ffffff;"
                       "  font-family: sans-serif;"
                       "  margin: 25px; padding: 0;"
                       "}"
                       + cssLayoutMode +
                       ".chord-line {"
                       "  font-weight: bold; color: #b22222;"
                       "  font-family: sans-serif;"
                       "  font-size: " + QString::number(baseFontSize + 1) + "pt;"
                                                             "  min-height: 1.2em;"
                                                             "}"
                                                             ".lyric-text {"
                                                             "  color: #222; font-family: sans-serif;"
                                                             "  font-size: " + QString::number(baseFontSize) + "pt;"
                                                         "}"
                                                         ".tab-block {"
                                                         "  display: block; clear: both;"
                                                         "  font-family: 'Consolas', 'Courier New', monospace !important;"
                                                         "  font-size: " + QString::number(baseFontSize - 1) + "pt;"
                                                             "  white-space: pre !important;"
                                                             "  background-color: #f4f6f9;"
                                                             "  padding: 12px; border-left: 4px solid #007acc; margin: 10px 0;"
                                                             "}"
                                                             "</style></head><body>"
                                                             "<div class='song-canvas'>"
                       + m_parsedSongContentGrid +
                       "</div></body></html>";

    parsedEditor->setHtml(baseHtml);
}
*/

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (currentState == PlayAlong) {

        // 1. Spacebar Toggle Hooked to your actual m_mediaPlayer instance variable!
        if (event->key() == Qt::Key_Space) {
            // Updated for Qt 6 multimedia architecture
            if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
                m_mediaPlayer->pause();
            } else {
                if (!m_selectedAudioPath.isEmpty()) {
                    m_mediaPlayer->play();
                }
            }
            event->accept();
            return;
        }

        // 2. Control Key Layout Zoom Modifiers
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