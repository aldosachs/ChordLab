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
    resize(size.width() * 0.75, size.height() * 0.6);

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
    fileMenu->addAction("&Open...", this, &MainWindow::handleFileOpen, QKeySequence::Open);
    fileMenu->addAction("&Save Standardized", this, &MainWindow::handleFileSave, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close);
}

void MainWindow::setupToolBar() {
    QToolBar *settingsToolBar = addToolBar("Critical Settings");
    m_btnTransposeUp = new QPushButton("Transpose +st");
    m_btnTransposeDown = new QPushButton("Transpose -st");
    m_btnTheme = new QPushButton("Theme: Light");
    m_btnTheme->setStyleSheet("QPushButton { background-color: #0047AB; color: white; padding: 5px; min-width: 100px; }");

    // Create a container for our buttons
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(10);

    // Apply your signature blue style
    QString style = "QPushButton { background-color: #0047AB; color: white; padding: 5px; min-width: 80px; }";
    m_btnTransposeUp->setStyleSheet(style);
    m_btnTransposeDown->setStyleSheet(style);

    // Add "Placeholder" Blue Buttons
    QString buttonStyle = "QPushButton { background-color: #0047AB; color: white; border-radius: 4px; padding: 5px; min-width: 60px; }";
    for(int i = 0; i < 4; ++i) {
        QPushButton *btn = new QPushButton(QString("Opt %1").arg(i + 1));
        btn->setStyleSheet(buttonStyle);
        layout->addWidget(btn);
    }

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


    layout->addWidget(m_btnTransposeDown);
    layout->addWidget(m_btnTransposeUp);
    layout->addWidget(m_btnTheme);
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
void MainWindow::handleFileSave() {
    statusBar()->showMessage("Save triggered - awaiting logic.");
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
    QRegularExpression titleRegex("\\{title:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression keyRegex("\\{key:\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);

    QString title = titleRegex.match(rawInput).captured(1).trimmed();
    QString key = keyRegex.match(rawInput).captured(1).trimmed();

    if (!title.isEmpty()) {
        result += "<div class='header-block'>" // Use the CSS class instead of inline styles
                  "<h2 style='margin:0;'>" + title + "</h2>";
        if (!key.isEmpty()) result += "Key: <b>" + key + "</b>";
        result += "</div>";
    }

    QStringList lines = rawInput.split('\n');
    bool inProtectedBlock = false;
    bool lastLineWasEmpty = false;
    bool inChorus = false;
    bool inVerse = false;

    for (QString line : lines) {
        line.remove('\r');
        QString trimmedLine = line.trimmed();

        // 1. Skip Metadata (handled by header pass) & Comments
        if (trimmedLine.startsWith("{title:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{key:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{tempo:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("{capo:", Qt::CaseInsensitive) ||
            trimmedLine.startsWith("#")) {
            continue;
        }

        // 2. Handle Empty Lines (Spacing)
        if (trimmedLine.isEmpty()) {
            if (lastLineWasEmpty) continue;
            result += "<br>";
            lastLineWasEmpty = true;
            continue;
        }
        lastLineWasEmpty = false;

        // 3. Handle Directives (Chorus / Tabs)
        if (trimmedLine.startsWith("{start_of_chorus}") || trimmedLine.startsWith("{soc")) {
            // #############
            if(inVerse) { result += "</div>"; inVerse = false; }
            // #############
            inChorus = true;
            result += "<div class='chorus-box'><span class='section-header'>Chorus</span><br>";
            continue;
        }
        if (trimmedLine.startsWith("{end_of_chorus}") || trimmedLine.startsWith("{eoc")) {
            inChorus = false;
            result += "</div>";
            continue;
        }
        if (trimmedLine.startsWith("{sot")) { inProtectedBlock = true; continue; }
        if (trimmedLine.startsWith("{eot")) { inProtectedBlock = false; continue; }

        // --- NEW LOGIC FOR {c: Verse} etc. ---
        if (trimmedLine.startsWith("{c:") || trimmedLine.startsWith("{comment:")) {
            // #############
            if(inVerse) { result += "</div>"; inVerse = false; }
            // #############

            // Extract text between ":" and "}"
            QString commentText = trimmedLine.section(':', 1).chopped(1).trimmed();
            inVerse = true;
            // Close the current verse box if one was open, then start a new one with a header
            result += "<div class='verse-box'><span class='section-header'>" + commentText + "</span><br>";
            continue;
        }
        // -------------------------------------

        if (trimmedLine.startsWith("{sot")) { inProtectedBlock = true; continue; }
        if (trimmedLine.startsWith("{eot")) { inProtectedBlock = false; continue; }

        // Inside the loop, after all the directive checks:
        if (!inChorus && !inProtectedBlock && !trimmedLine.isEmpty()) {
            if (!inVerse) {
                // Auto-start a verse box for lyrics that don't have a {c:} tag
                inVerse = true;
                result += "<div class='verse-box'>";
            }
            result += processLineContent(line) + "<br>";
        }
        else if (inChorus) {
            result += processLineContent(line) + "<br>";
        }

        // 4. THE FIX: Process and Wrap the line ONCE
        if (inProtectedBlock) {
            result += line + "<br>"; // Raw text for tabs
        }
        else if (inChorus) {
            // Just process content, the <div> is already open
            result += processLineContent(line) + "<br>";
        }
        else {
            // Not in chorus or tab? Wrap it in a verse box
            result += "<div class='verse-box'>" + processLineContent(line) + "</div>";
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
    statusBar()->showMessage(QString("Transposition: %1 semitones").arg(m_transposeShift));

    // Re-run the parser to reflect the new keys
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
               ".header-block { background-color: #1A1C2E; border-bottom: 2px solid #30364D; padding: 10px; }"
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

    m_btnTheme->setText(m_currentTheme == Light ? "Theme: Light" : "Theme: Dark");

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
        }
    }
    return output;
}