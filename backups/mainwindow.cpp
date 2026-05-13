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

    // Create a container for our buttons
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(10);

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

    // Inside setupToolBar()
//    QPushButton *viewToggleBtn = new QPushButton("View: CIL/CAL");
//    viewToggleBtn->setStyleSheet("QPushButton { background-color: #0047AB; color: white; font-weight: bold; padding: 5px; }");

    // Connect the button to the logic
//    connect(viewToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleDisplayMode);
//    layout->addWidget(viewToggleBtn);
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
    // For now, we'll call a helper function to populate the right-hand side
//    QString testHtml = "<html><body><b style='color:red;'>If this is red and bold, HTML is working!</b></body></html>";
//    parsedEditor->setHtml(testHtml);

    QString expertVersion = runInitialParse(content);
    parsedEditor->setHtml(expertVersion);
    statusBar()->showMessage(tr("Loaded: ") + fileName);
}

QString MainWindow::runInitialParse(const QString &rawInput) {
    QString colorChord = "darkblue";
    QString colorLyrics = "#1A1A1A";

    // --- 1. SMART HEADER PRE-PASS ---
    auto getTag = [&](const QString &tag) {
        QRegularExpression re("\\{(?:" + tag + "):\\s*(.*)\\}", QRegularExpression::CaseInsensitiveOption);
        auto match = re.match(rawInput);
        return match.hasMatch() ? match.captured(1).trimmed() : QString();
    };

    QString title = getTag("title|t");
    QString key = getTag("key|k");
    QString tempo = getTag("tempo");
    QString capo = getTag("capo");

    QString result = "<html><head><style>"
                     "body { font-family: 'Consolas', monospace; white-space: pre; background-color: white; color: " + colorLyrics + "; }"
                                     ".chord { color: " + colorChord + "; font-weight: bold; }"
                                    ".comment { color: #444; background: #eee; padding: 2px 5px; font-weight: bold; }"
                                    "</style></head><body>";

    if (!title.isEmpty() || !key.isEmpty()) {
        result += "<div style='background-color: #f8f9fa; padding: 10px; border-bottom: 2px solid #ccc; margin-bottom: 10px;'>";
        if (!title.isEmpty()) result += "<h2 style='margin:0;'>" + title + "</h2>";
        if (!key.isEmpty()) result += "Key: <b>" + key + "</b> ";
        if (!tempo.isEmpty()) result += "| Tempo: <b>" + tempo + "</b> ";
        if (!capo.isEmpty()) result += "| Capo: <b>" + capo + "</b>";
        result += "</div>";
    }

    // --- 2. MAIN LOOP ---
    QStringList lines = rawInput.split('\n');
    bool inProtectedBlock = false;
    bool lastLineWasEmpty = false;

    for (QString line : lines) {
        line = line.trimmed();

        // Perfection 5: Ignore redundant empty lines
        if (line.isEmpty()) {
            if (lastLineWasEmpty) continue;
            result += "<br>";
            lastLineWasEmpty = true;
            continue;
        }
        lastLineWasEmpty = false;

        // Perfection 7: Tab/Grid Protection
        if (line.startsWith("{sot") || line.startsWith("{sog")) { inProtectedBlock = true; continue; }
        if (line.startsWith("{eot") || line.startsWith("{eog")) { inProtectedBlock = false; continue; }

        // Perfection 6: Handle Directives
        if (line.startsWith("{")) {
            if (line.contains("chorus", Qt::CaseInsensitive)) {
                if (line.contains("start")) result += "<div style='border-left: 5px solid darkblue; padding-left: 15px; background-color: #f0f7ff;'>";
                else result += "</div>";
                continue;
            }
            // Perfection 6: Show comments as pills
            if (line.contains("{c:") || line.contains("{comment:")) {
                QString cText = line.section(':', 1).chopped(1).trimmed();
                result += "<span class='comment'>" + cText + "</span><br>";
                continue;
            }
            continue; // Hide other metadata tags already in header
        }

        // --- 3. RENDERING MODES ---
        if (inProtectedBlock) {
            result += line + "<br>"; // Raw monospaced for tabs
        }
        else if (m_currentMode == ChordDisplayMode::CIL) {
            line.replace(QRegularExpression("\\[(.*?)\\]"), "<span class='chord'>[\\1]</span>");
            result += line + "<br>";
        }

            else {
            // ... (Insert your existing CAL "Double-Line" logic here) ...
            // Be sure to add <br> at the end of the stacked lines!
            // --- CAL MODE LOGIC ---
            QString chordLine = "";
            QString lyricLine = "";
            int currentPos = 0;

            QRegularExpression chordRegex("\\[(.*?)\\]");
            auto i = chordRegex.globalMatch(line);

            // If there are NO chords on this line, just treat it as a normal lyric line
            if (!i.hasNext()) {
                result += line + "<br>";
            } else {
                while (i.hasNext()) {
                    auto match = i.next();
                    int leadingLength = match.capturedStart() - currentPos;
                    QString gap = line.mid(currentPos, leadingLength);

                    lyricLine += gap;
                    chordLine += QString(gap.length(), ' ');

                    QString chordName = match.captured(1);
                    chordLine += "<span class='chord'>" + chordName + "</span>";
                    currentPos = match.capturedEnd();
                }
                lyricLine += line.mid(currentPos);

                // Perfection: Ensure both lines are added to the HTML result
                result += chordLine + "<br>" + lyricLine + "<br>";
            }
        }
    }
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

/* void MainWindow::toggleDisplayMode() {
    // 1. Toggle the mode
    m_currentMode = (m_currentMode == ChordDisplayMode::CIL) ?
                        ChordDisplayMode::CAL : ChordDisplayMode::CIL;

    // 2. Update the status bar so the user knows what happened
    QString modeName = (m_currentMode == ChordDisplayMode::CIL) ? "ChordPro (CIL)" : "LeadSheet (CAL)";
    m_viewBtn->setText(m_currentMode == ChordDisplayMode::CIL ? "Switch to LeadSheet (CAL)" : "Switch to ChordPro (CIL)");
    statusBar()->showMessage("Display Mode Switched to: " + modeName);

    // 3. Refresh the "Expert" view immediately
    QString currentContent = originalEditor->toPlainText();
    parsedEditor->setHtml(runInitialParse(currentContent));
} */