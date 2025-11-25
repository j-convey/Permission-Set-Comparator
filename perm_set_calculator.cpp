// C++ Qt6 version of the Salesforce Permission Set Comparator
// Mirrors functionality of perm_set_calculator.py
// Build with CMake (see accompanying CMakeLists.txt)

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidgetItem>
#include <QtGui/QFont>
#include <QtGui/QIcon>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

// Regex equivalents
static const QRegularExpression DATE_RE(QStringLiteral("^\\d{1,2}/\\d{1,2}/\\d{2,4}$"));
static const QRegularExpression ACTION_DATE_RE(
    QStringLiteral("^(?:add|del|delete|remove)\\s+\\d{1,2}/\\d{1,2}/\\d{2,4}$"),
    QRegularExpression::CaseInsensitiveOption
);

static QStringList tokenizeLine(const QString &rawLine) {
    if (rawLine.contains('\t')) {
        QStringList parts = rawLine.split('\t');
        QStringList tokens;
        for (const QString &p : parts) {
            QString trimmed = p.trimmed();
            if (!trimmed.isEmpty()) tokens << trimmed;
        }
        return tokens;
    }

    // Split by 2+ spaces
    QRegularExpression multiSpace("\\s{2,}");
    QStringList parts = rawLine.trimmed().split(multiSpace);
    QStringList tokens;
    for (const QString &p : parts) {
        if (!p.isEmpty()) tokens << p;
    }
    if (tokens.size() > 1) return tokens;

    // Fallback: comma separated
    if (rawLine.contains(',')) {
        parts = rawLine.split(',');
        tokens.clear();
        for (const QString &p : parts) {
            QString trimmed = p.trimmed();
            if (!trimmed.isEmpty()) tokens << trimmed;
        }
        return tokens;
    }
    return QStringList{ rawLine.trimmed() };
}

static QString extractPermissionName(const QString &rawLine) {
    QString line = rawLine.trimmed();
    if (line.isEmpty()) return QString();

    QString lowered = line.toLower();
    if (lowered.contains("permission set name") && lowered.contains("action")) return QString();
    if (ACTION_DATE_RE.match(line).hasMatch()) return QString();

    QStringList tokens = tokenizeLine(rawLine);
    if (tokens.isEmpty()) return QString();

    for (const QString &token : tokens) {
        QString trimmed = token.trimmed();
        QString loweredToken = trimmed.toLower();
        if (trimmed.isEmpty()) continue;
        if (loweredToken == "add" || loweredToken == "del" || loweredToken == "delete" || loweredToken == "remove") continue;
        if (DATE_RE.match(trimmed).hasMatch()) continue;
        if (loweredToken.contains("expires on") || loweredToken.contains("date assigned")) continue;
        if (loweredToken.contains("permission set name")) return QString();
        return trimmed; // first valid token
    }

    QString fallback = tokens.first().trimmed();
    QString fallbackLower = fallback.toLower();
    if (fallbackLower == "add" || fallbackLower == "del" || fallbackLower == "delete" || fallbackLower == "remove") return QString();
    if (DATE_RE.match(fallback).hasMatch()) return QString();
    return fallback;
}

static QStringList extractPermissionNames(const QString &raw) {
    QStringList lines = raw.split('\n');
    QStringList names;
    QSet<QString> seen;
    for (const QString &l : lines) {
        QString candidate = extractPermissionName(l);
        if (!candidate.isEmpty() && !seen.contains(candidate)) {
            names << candidate;
            seen.insert(candidate);
        }
    }
    return names;
}

static QSet<QString> parsePermissions(const QString &raw) {
    const QStringList names = extractPermissionNames(raw);
    return QSet<QString>(names.cbegin(), names.cend());
}

class PermissionInputArea : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit PermissionInputArea(const QString &placeholder, QWidget *parent = nullptr)
        : QPlainTextEdit(parent) {
        setPlaceholderText(placeholder);
        setLineWrapMode(QPlainTextEdit::NoWrap);
        connect(this, &QPlainTextEdit::textChanged, this, &PermissionInputArea::sanitizeText);
    }

private slots:
    void sanitizeText() {
        QString text = toPlainText();
        QStringList sanitizedLines = extractPermissionNames(text);
        QString sanitized = sanitizedLines.join('\n');
        if (text == sanitized) return;
        QSignalBlocker blocker(this); // RAII blocks signals
        setPlainText(sanitized);
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::End);
        setTextCursor(c);
    }
};

static QString resourcePath(const QString &name) {
    // Resolve relative to application dir.
    QDir base(QCoreApplication::applicationDirPath());
    return base.filePath(name);
}

class PermissionSetCalculator : public QMainWindow {
    Q_OBJECT
public:
    PermissionSetCalculator() {
        setWindowTitle("Permission Set Comparator");
        // Prefer ICO then PNG
        const QStringList icons{ "Salesforce_perm_Calc_icon.ico", "Salesforce_perm_Calc_icon.png" };
        for (const QString &ic : icons) {
            QString p = resourcePath(ic);
            if (QFileInfo::exists(p)) { setWindowIcon(QIcon(p)); break; }
        }
        resize(900, 800);
        loadDescriptionsFromCsv();
        buildUi();
        applyStyles();
    }

private:
    PermissionInputArea *userInput{nullptr};
    PermissionInputArea *mirrorInput{nullptr};
    QTableWidget *outputArea{nullptr};
    QPushButton *compareButton{nullptr};
    QMap<QString, QString> permDescriptions;

    void loadDescriptionsFromCsv() {
        QString path = resourcePath("Permission Sets.csv");
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QTextStream in(&file);
        // Skip header
        if (!in.atEnd()) in.readLine();

        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts;
            QString current;
            bool inQuote = false;
            for (int i = 0; i < line.length(); ++i) {
                QChar c = line[i];
                if (c == '"') {
                    inQuote = !inQuote;
                } else if (c == ',' && !inQuote) {
                    parts << current;
                    current.clear();
                } else {
                    current += c;
                }
            }
            parts << current;

            if (parts.size() >= 4) {
                QString name = parts[2].trimmed();
                QString desc = parts[3].trimmed();
                if (desc.startsWith('"') && desc.endsWith('"')) {
                    desc = desc.mid(1, desc.length() - 2);
                    desc.replace("\"\"", "\"");
                }
                if (!name.isEmpty()) {
                    permDescriptions.insert(name.toLower(), desc);
                }
            }
        }
        file.close();
    }

    void buildUi() {
        QWidget *central = new QWidget(this);
        setCentralWidget(central);
        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->setSpacing(24);
        mainLayout->setContentsMargins(40, 40, 40, 40);
        central->setLayout(mainLayout);

        QLabel *header = new QLabel("Permission Set Comparator");
        header->setObjectName("HeaderLabel");
        header->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(header);

        QHBoxLayout *inputsLayout = new QHBoxLayout;
        inputsLayout->setSpacing(24);
        mainLayout->addLayout(inputsLayout);

        userInput = new PermissionInputArea("Paste primary user's permissions here...");
        QGroupBox *userGroup = new QGroupBox("Primary User");
        QVBoxLayout *userGroupLayout = new QVBoxLayout;
        userGroupLayout->setContentsMargins(16, 24, 16, 16);
        userGroupLayout->addWidget(userInput);
        userGroup->setLayout(userGroupLayout);
        inputsLayout->addWidget(userGroup);

        mirrorInput = new PermissionInputArea("Paste mirror user's permissions here...");
        QGroupBox *mirrorGroup = new QGroupBox("Mirror User");
        QVBoxLayout *mirrorGroupLayout = new QVBoxLayout;
        mirrorGroupLayout->setContentsMargins(16, 24, 16, 16);
        mirrorGroupLayout->addWidget(mirrorInput);
        mirrorGroup->setLayout(mirrorGroupLayout);
        inputsLayout->addWidget(mirrorGroup);

        compareButton = new QPushButton("Compare Permissions");
        compareButton->setCursor(Qt::PointingHandCursor);
        compareButton->setFixedHeight(50);
        connect(compareButton, &QPushButton::clicked, this, &PermissionSetCalculator::comparePermissions);
        mainLayout->addWidget(compareButton);

        QGroupBox *outputGroup = new QGroupBox("Missing Permissions (Mirror has, User needs)");
        QVBoxLayout *outputGroupLayout = new QVBoxLayout;
        outputGroupLayout->setContentsMargins(16, 24, 16, 16);
        
        outputArea = new QTableWidget;
        outputArea->setMinimumHeight(300);
        outputArea->setColumnCount(2);
        outputArea->setHorizontalHeaderLabels({"Permission Set", "Description"});
        outputArea->verticalHeader()->setVisible(false);
        outputArea->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        outputArea->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
        outputArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        outputArea->setWordWrap(true);
        outputArea->setShowGrid(false);
        outputArea->setSelectionBehavior(QAbstractItemView::SelectRows);
        outputArea->setEditTriggers(QAbstractItemView::NoEditTriggers);
        outputArea->setAlternatingRowColors(true);
        outputArea->setFocusPolicy(Qt::NoFocus);
        outputArea->setSelectionMode(QAbstractItemView::NoSelection);
        
        outputGroupLayout->addWidget(outputArea);
        outputGroup->setLayout(outputGroupLayout);
        mainLayout->addWidget(outputGroup);
    }

    void applyStyles() {
        setStyleSheet(R"(
            QMainWindow {
                background-color: #f0f2f5;
            }
            QWidget {
                font-family: "Segoe UI", sans-serif;
                font-size: 14px;
                color: #1c1e21;
            }
            QLabel#HeaderLabel {
                font-size: 28px;
                font-weight: 700;
                color: #1c1e21;
                margin-bottom: 10px;
            }
            QGroupBox {
                background-color: #ffffff;
                border: 1px solid #dddfe2;
                border-radius: 8px;
                margin-top: 24px;
                font-size: 14px;
                font-weight: 600;
                color: #4b4f56;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                left: 12px;
                padding: 0 8px;
            }
            QPlainTextEdit {
                border: 1px solid #ccd0d5;
                border-radius: 6px;
                padding: 10px;
                background-color: #f5f6f7;
                font-family: "Consolas", "Courier New", monospace;
                font-size: 13px;
            }
            QPlainTextEdit:focus {
                background-color: #ffffff;
                border: 1px solid #1877f2;
            }
            QPushButton {
                background-color: #1877f2;
                color: #ffffff;
                border: none;
                border-radius: 6px;
                font-size: 16px;
                font-weight: 600;
                padding: 12px;
            }
            QPushButton:hover {
                background-color: #166fe5;
            }
            QPushButton:pressed {
                background-color: #155db5;
            }
            QTableWidget {
                border: 1px solid #ccd0d5;
                border-radius: 6px;
                background-color: #ffffff;
                alternate-background-color: #f9fafb;
                font-family: "Segoe UI", sans-serif;
                font-size: 13px;
                color: #1c1e21;
                outline: none;
            }
            QHeaderView::section {
                background-color: #f0f2f5;
                padding: 8px;
                border: none;
                border-bottom: 1px solid #dddfe2;
                font-weight: 600;
                color: #4b4f56;
            }
            QTableWidget::item {
                padding: 8px;
                border-bottom: 1px solid #f0f2f5;
            }
        )");
    }

private slots:
    void comparePermissions() {
        QSet<QString> userPerms = parsePermissions(userInput->toPlainText());
        QSet<QString> mirrorPerms = parsePermissions(mirrorInput->toPlainText());
        // Difference: mirror - user
        QStringList missing;
        for (const QString &m : mirrorPerms) {
            if (!userPerms.contains(m)) missing << m;
        }
        // Case-insensitive sort
        std::sort(missing.begin(), missing.end(), [](const QString &a, const QString &b) {
            return a.toCaseFolded() < b.toCaseFolded();
        });

        outputArea->setRowCount(0);
        if (!missing.isEmpty()) {
            outputArea->setRowCount(missing.size());
            for (int i = 0; i < missing.size(); ++i) {
                QString perm = missing[i];
                QString lower = perm.toLower();
                QString desc = permDescriptions.value(lower, "");

                QTableWidgetItem *nameItem = new QTableWidgetItem(perm);
                // Make the name bold and dark red to highlight it
                QFont boldFont;
                boldFont.setBold(true);
                nameItem->setFont(boldFont);
                nameItem->setForeground(QBrush(QColor("#c53030")));

                QTableWidgetItem *descItem = new QTableWidgetItem(desc);
                descItem->setForeground(QBrush(QColor("#4b4f56"))); // Dark gray for description

                outputArea->setItem(i, 0, nameItem);
                outputArea->setItem(i, 1, descItem);
            }
            outputArea->resizeRowsToContents();
        } else {
            outputArea->setRowCount(1);
            QTableWidgetItem *msgItem = new QTableWidgetItem("No missing permissions.");
            outputArea->setItem(0, 0, msgItem);
            outputArea->setItem(0, 1, new QTableWidgetItem("The user already has all permission sets listed for the mirror user."));
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Global app icon preference (ICO then PNG)
    const QStringList icons{ "Salesforce_perm_Calc_icon.ico", "Salesforce_perm_Calc_icon.png" };
    for (const QString &ic : icons) {
        QString p = resourcePath(ic);
        if (QFileInfo::exists(p)) { app.setWindowIcon(QIcon(p)); break; }
    }

#ifdef _WIN32
    // Set AppUserModelID for proper taskbar grouping/icon usage
    const wchar_t *appId = L"com.vivint.salesforce.permcalc";
    // Errors are non-critical; ignore return value
    SetCurrentProcessExplicitAppUserModelID(appId);
#endif

    PermissionSetCalculator window;
    window.show();

    return app.exec();
}

#include "perm_set_calculator.moc"

