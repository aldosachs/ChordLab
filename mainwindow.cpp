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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // 1. Scale to 60% of available screen
    QScreen *screen = QGuiApplication::primaryScreen();
    QSize size = screen->availableGeometry().size();
    resize(size.width() * 0.8, size.height() * 0.8);

    m_currentFilePath = ""; // Initialize 'raw ChoPro' file path var. as empty string, first up.

    setupMenus();
    setupToolBar();
    setupLayout();
    setAppState(Idle); // Start in "Neutral" mode

    // Reserved Toolbar Space
    //    addToolBar("Transport & Controls");
    // --> btn->setIcon(QIcon(":/icons/gig-list.png"));
}

void MainWindow::setupMenus() {
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Open...", QKeySequence::Open, this, &MainWindow::handleFileOpen);
    fileMenu->addAction("&Save Standardized", QKeySequence::Save, this, &MainWindow::handleFileSave);
//    fileMenu->addAction("&Open...", this, &MainWindow::handleFileOpen, QKeySequence::Open);
//    fileMenu->addAction("&Save Standardized", this, &MainWindow::handleFileSave, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close);
}

void MainWindow::setupToolBar() {
    QToolBar *settingsToolBar = addToolBar("Critical Settings");
    m_btnTransposeUp = new QPushButton("Key Up");
    m_btnTransposeDown = new QPushButton("Key Down");
    m_btnTheme = new QPushButton("Light theme...");
    m_btnTheme->setStyleSheet("QPushButton { background-color: #0047AB; color: white; padding: 5px; min-width: 80px; }");

    // Create a container for our buttons
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(10);

    // Apply your signature blue style
    QString style = "QPushButton { background-color: #004060; color: white; padding: 5px; min-width: 80px; }";
    m_btnTransposeUp->setStyleSheet(style);
    m_btnTransposeDown->setStyleSheet(style);

    // Add "Placeholder" Grey Function Key Buttons as class members
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

//    // Add "Placeholder" Grey Function Key Buttons
//    QString buttonStyle = "QPushButton { background-color: #808080; color: white; border-radius: 4px; padding: 5px; min-width: 40px; }";
//    for(int i = 0; i < 4; ++i) {
//        QPushButton *btn = new QPushButton(QString("Fn-%1").arg(i + 1));
//        btn->setStyleSheet(buttonStyle);
//        layout->addWidget(btn);
//    }


    QPushButton *btnReset = new QPushButton("Reset Key");
    btnReset->setStyleSheet("QPushButton { background-color: #0047ff; color: white; padding: 5px; }"); // Reset back to original key!
    connect(btnReset, &QPushButton::clicked, this, [=](){
        m_transposeShift = 0;
        statusBar()->showMessage("Key Reset to Original");
        parsedEditor->setHtml(runInitialParse(originalEditor->toPlainText()));
    });

    settingsToolBar->addWidget(container);

    // Initialize the member variable
    m_viewToggleBtn = new QPushButton("View: ChordPro (CIL)");
    m_viewToggleBtn->setStyleSheet("QPushButton { background-color: #0047AB; color: white; }");

    connect(m_viewToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleDisplayMode);
    layout->addWidget(m_viewToggleBtn);
    // Connect to the logic
    connect(m_btnTransposeUp, &QPushButton::clicked, this, [=](){ shiftTransposition(1); });
    connect(m_btnTransposeDown, &QPushButton::clicked, this, [=](){ shiftTransposition(-1); });
    connect(m_btnTheme, &QPushButton::clicked, this, &MainWindow::toggleTheme);

    layout->addWidget(m_btnTheme);
    layout->addWidget(m_btnTransposeDown);
    layout->addWidget(btnReset);
    layout->addWidget(m_btnTransposeUp);

}

void MainWindow::updateFunctionKeys() {
    // Safely disconnect any previous lambda actions so they don't stack up
    m_btnFn1->disconnect();
    m_btnFn2->disconnect();
    m_btnFn3->disconnect();
    m_btnFn4->disconnect();

    if (currentState == OpenEdit || currentState == Idle) {
        // --- EDIT MODE CONFIGURATION ---
        m_btnFn1->setText("📂 Open");
        m_btnFn2->setText("💾 Save");
        m_btnFn3->setText("💾 As...");
        m_btnFn4->setText("🔄 Parse"); // Back-up explicit parse button!

        connect(m_btnFn1, &QPushButton::clicked, this, &MainWindow::handleFileOpen);
        connect(m_btnFn2, &QPushButton::clicked, this, &MainWindow::handleFileSave);
        connect(m_btnFn3, &QPushButton::clicked, this, [=]() {
            QString oldPath = m_currentFilePath;
            m_currentFilePath.clear(); // Force getSaveFileName prompt
            handleFileSave();
            if (m_currentFilePath.isEmpty()) m_currentFilePath = oldPath; // Restore if canceled
        });
        connect(m_btnFn4, &QPushButton::clicked, this, [=]() {
            parsedEditor->setHtml(runInitialParse(originalEditor->toPlainText()));
        });

    } else if (currentState == PlayAlong) {
        // --- PLAYBACK MODE CONFIGURATION ---
        m_btnFn1->setText("⏮ Rewind");
        m_btnFn2->setText("▶ Play");
        m_btnFn3->setText("⏸ Pause");
        m_btnFn4->setText("⏭ End");

        // Stubs for future MP3 / MIDI engine integrations!
        connect(m_btnFn1, &QPushButton::clicked, this, [=]() { statusBar()->showMessage("Rewinding track..."); });
        connect(m_btnFn2, &QPushButton::clicked, this, [=]() { statusBar()->showMessage("Playing track..."); });
        connect(m_btnFn3, &QPushButton::clicked, this, [=]() { statusBar()->showMessage("Track Paused."); });
        connect(m_btnFn4, &QPushButton::clicked, this, [=]() { statusBar()->showMessage("Skipped to end."); });
    }
}

void MainWindow::setupLayout() {
    mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Left Side: Original/Raw ChoPro
    originalEditor = new QPlainTextEdit(this);
    originalEditor->setPlaceholderText("Original File Content...");
    // Use Monospaced font for alignment of [|] [/] [.]
    QFont monoFont("Consolas", 10);
    originalEditor->setFont(monoFont);

    // Right Side: Expert/Parsed Content
    parsedEditor = new QTextEdit(this);
    parsedEditor->setPlaceholderText("Parsed & Standardized Output...");
    parsedEditor->setFont(monoFont);
    parsedEditor->setReadOnly(true); // Expert view starts as read-only

    mainSplitter->addWidget(originalEditor);
    mainSplitter->addWidget(parsedEditor);

    // --- THE LIVE REFRESH LINK ---
    // Whenever the text on the left changes, instantly re-parse and display on the right
    connect(originalEditor, &QPlainTextEdit::textChanged, this, [=]() {
        parsedEditor->setHtml(runInitialParse(originalEditor->toPlainText()));
    });

    setCentralWidget(mainSplitter);
}

void MainWindow::setAppState(AppState state) {
    currentState = state;
    switch(state) {
    case Idle:
        statusBar()->showMessage("Ready. Open a ChordPro file to begin.");
        mainSplitter->hide();
        break;
    case OpenEdit:
        statusBar()->showMessage("Editing Mode: Analyzing ChordPro syntax...");
        mainSplitter->show();
        break;
    case PlayAlong:
        statusBar()->showMessage("Play-along Mode Active.");
        // Future: Hide originalEditor, maximize parsed view or renderer
        break;
    }
}

// This is the implementation for the Save action
//void MainWindow::handleFileSave() {
//    statusBar()->showMessage("Save triggered - awaiting logic.");
//    }
void MainWindow::handleFileSave() {
    // 1. If no file is currently open, act like a "Save As" dialog
    if (m_currentFilePath.isEmpty()) {
        QString saveName = QFileDialog::getSaveFileName(this,
                                                        tr("Save ChordPro File"), "", tr("ChordPro Files (*.chopro *.pro *.txt *.crd)"));

        if (saveName.isEmpty()) {
            return; // User canceled the save dialog
        }
        m_currentFilePath = saveName;
    }

    // 2. Open the file for writing
    QFile file(m_currentFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Save Error"), tr("Could not write to file: ") + file.errorString());
        return;
    }

    // 3. Stream the raw data directly from the left editor
    QTextStream out(&file);
    out << originalEditor->toPlainText();
    file.close();

    // 4. Update the status bar to show it worked perfectly
    statusBar()->showMessage(tr("Saved successfully: ") + m_currentFilePath, 3000); // Display for 3 seconds
}

void MainWindow::handleFileOpen() {
    // 1. Get the file path from the user
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open ChordPro File"), "", tr("ChordPro Files (*.chopro *.pro *.txt *.crd)"));

    if (fileName.isEmpty()) {
        return;
    }

    // 2. Attempt to open and read the file
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"), tr("Could not open file: ") + file.errorString());
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // 3. Populate the 'Original' editor and update State
    originalEditor->setPlainText(content);
    setAppState(OpenEdit);

    // 4. Run the initial 'Expert' Parser (The Meta-data Pass)
    // Call a helper function to populate the right-hand side
    QString expertVersion = runInitialParse(content);
    parsedEditor->setHtml(expertVersion);
    statusBar()->showMessage(tr("Loaded: ") + fileName);
}

QString MainWindow::runInitialParse(const QString &rawInput) {
    QString result = "<html><head><style>"
                     "body { font-family: 'Consolas', monospace; white-space: pre; }"
                     + getThemeStyles() + // <--- Inject our dynamic theme here!
                     "</style></head><body>";
    QString colorChord = "darkblue";
    QString chorusBg = "#f0f7ff";
    QString commentBg = "#eeeeee";

/*    QString result = "<html><head><style>"
                     "body { font-family: 'Consolas', monospace; white-space: pre; }"
                     ".chord { color: " + colorChord + "; font-weight: bold; }"
                                    ".comment { background-color: " + commentBg + "; padding: 2px 5px; border-radius: 3px; }"
                                   // Add these to your style block at the top of runInitialParse
                                   ".chorus-box { background-color: #f0f7ff; border-left: 5px solid darkblue; padding: 5px 10px; margin: 5px 0; }"
                                   ".section-header { background-color: #eeeeee; font-weight: bold; padding: 2px 5px; border-radius: 3px; display: inline-block; }"
                                   "</style></head><body>";
*/
    // 1. Pre-pass for Metadata Header
//    QRegularExpression titleRegex("\\{title:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);
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
        result += "<h1 style='margin:0;'>" + title + "</h1>"; // h1 makes it pop more!
        if (!artist.isEmpty())   result += "Artist: <b>" + artist + "</b><br>";
        if (!key.isEmpty())   result += "Key: <b>" + key + "</b><br>";
        if (!tempo.isEmpty()) result += "Tempo: <b>" + tempo + "</b><br>";
        if (!capo.isEmpty())  result += "Capo: <b>" + capo + "</b>"; // No extra div closures here

        result += "</div>"; // Just ONE closure at the end of the metadata
    }

    QStringList lines = rawInput.split('\n');
    bool inProtectedBlock = false;
    bool lastLineWasEmpty = false;
    bool inChorus = false;
    bool inVerse = false;

    for (const QString &line : lines) {
//    for (QString line : lines) {
//        line.remove('\r');
//        QString trimmedLine = line.trimmed();

    QString workingLine = line; // Make a local, changeable copy of just this line
    workingLine.remove('\r');   // This is good now!
    QString trimmedLine = workingLine.trimmed();
        bool lineHandled = false; // The gatekeeper

        // 1. Skip Metadata & Comments
        if (trimmedLine.startsWith("{title:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{t:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{artist:", Qt::CaseInsensitive) ||            
            trimmedLine.startsWith("{key:", Qt::CaseInsensitive) ||            
            trimmedLine.startsWith("{tempo:", Qt::CaseInsensitive) ||            
            trimmedLine.startsWith("{capo:", Qt::CaseInsensitive) ||

            trimmedLine.startsWith("#")) {
            continue;
        }

        // 2. Handle Empty Lines
        if (trimmedLine.isEmpty()) {
            if (lastLineWasEmpty) continue;
//            result += "<br>";
            lastLineWasEmpty = true;
            continue;
        }
        lastLineWasEmpty = false;

        // 3. Directives (Headers) - These close and open boxes
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
            // This now only handles non-chorus comments (Verse, Outro, Bridge)
            if (inVerse) result += "</div>";
            if (inChorus) { result += "</div>"; inChorus = false; }

            QString commentText = trimmedLine.section(':', 1).chopped(1).trimmed();
            inVerse = true;
            result += "<div class='verse-box'><span class='section-header'>" + commentText + "</span><br>";
            lineHandled = true;
        }
        else if (trimmedLine.startsWith("{sot")) { inProtectedBlock = true; lineHandled = true; }
        else if (trimmedLine.startsWith("{eot")) { inProtectedBlock = false; lineHandled = true; }

        // 4. Content Processing - Only runs if the line wasn't a Directive
        if (!lineHandled) {
            if (inProtectedBlock) {
                result += line + "<br>";
            }
            else if (inChorus) {
                result += processLineContent(line) + "<br>";
            }
            else {
                // Verse Logic: Auto-start if needed, then process
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

    // Change the button text based on the new state
    if (m_currentMode == ChordDisplayMode::CIL) {
        m_viewToggleBtn->setText("View: ChordPro (CIL)");
    } else {
        m_viewToggleBtn->setText("View: LeadSheet (CAL)");
    }

    // Refresh the view
    parsedEditor->setHtml(runInitialParse(originalEditor->toPlainText()));
}

void MainWindow::shiftTransposition(int delta) {
    m_transposeShift += delta;

    // The "Octave Wrap": Reset to 0 when we hit 12 or -12
    if (m_transposeShift >= 12 || m_transposeShift <= -12) {
        m_transposeShift = 0;
    }

    statusBar()->showMessage(QString("Transposition: %1 semitones").arg(m_transposeShift));
    parsedEditor->setHtml(runInitialParse(originalEditor->toPlainText()));
}

QString MainWindow::transposeChord(const QString &chord, int semitones) {
    static const QStringList notes = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};

    // Regex to split root (e.g., "F#") from suffix (e.g., "m7")
    QRegularExpression re("^([A-G][b#]?)");
    auto match = re.match(chord);
    if (!match.hasMatch()) return chord;

    QString root = match.captured(1);
    QString suffix = chord.mid(root.length());

    int idx = notes.indexOf(root);
    if (idx == -1) return chord; // Not in our basic list

    int newIdx = (idx + semitones) % 12;
    if (newIdx < 0) newIdx += 12; // Handle negative modulo

    return notes[newIdx] + suffix;
}

QString MainWindow::getThemeStyles() {
    if (m_currentTheme == Dark) {
        return "body { background-color: #121212; color: #E0E0E0; }"
               ".chord { color: #82AAFF; font-weight: bold; }"
               // The Title/Header block: Blue-black-biased dark grey
               ".header-block { background-color: #1A1C2E; border-bottom: 2px solid #30364D; padding: 15px; margin-bottom: 20px; }"
               ".chorus-box { background-color: #1E2337; border-left: 5px solid #0047AB; padding: 10px; margin: 5px 0; }"
               ".verse-box { background-color: #121212; padding: 10px; margin: 2px 0; }"
               ".section-header { background-color: #333333; color: #82AAFF; font-weight: bold; }";
    }
    // Default Light Theme
    return "body { background-color: white; color: black; }"
           ".chord { color: darkblue; font-weight: bold; }"
           ".header-block { background-color: #f8f9fa; border-bottom: 2px solid #ccc; padding: 10px; }"
           ".chorus-box { background-color: #f0f7ff; border-left: 5px solid darkblue; padding: 10px; margin: 5px 0; }"
           ".verse-box { background-color: white; padding: 10px; margin: 2px 0; }"
           ".section-header { background-color: #eeeeee; color: black; font-weight: bold; }";
}

void MainWindow::toggleTheme() {
    // Cycle through themes: Light -> Dark -> Light
    m_currentTheme = (m_currentTheme == Light) ? Dark : Light;

    m_btnTheme->setText(m_currentTheme == Light ? "Light theme" : "Dark theme");

    // Refresh the view to apply the new CSS
    parsedEditor->setHtml(runInitialParse(originalEditor->toPlainText()));
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
    else { // CAL Mode
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
//            output = chordLine + lyricLine;
        }
    }
    return output;
}