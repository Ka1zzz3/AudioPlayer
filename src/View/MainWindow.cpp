#include "View/MainWindow.h"

#include "Common/ViewCommand.h"
#include "ViewModel/LibraryViewModelProtocol.h"
#include "ViewModel/PlaybackViewModelProtocol.h"

#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QStringList>
#include <QWidget>

namespace AudioPlayer::View {

namespace {
constexpr auto defaultStoragePath = "library.json";
}

MainWindow::MainWindow(ViewModel::LibraryViewModelProtocol &libraryViewModel,
                       ViewModel::PlaybackViewModelProtocol &playbackViewModel,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_viewModel(libraryViewModel)
    , m_playbackViewModel(playbackViewModel)
{
    Q_UNUSED(m_playbackViewModel)
    buildUi();
    bindViewModel();
}

void MainWindow::buildUi()
{
    setWindowTitle(tr("AudioPlayer"));
    resize(900, 600);

    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget);

    auto *titleLabel = new QLabel(tr("Audio Library"), centralWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    auto *inputLayout = new QHBoxLayout();
    auto *storageLayout = new QFormLayout();
    auto *scanDirectoryLayout = new QFormLayout();

    m_storagePathInput = new QLineEdit(QString::fromLatin1(defaultStoragePath), centralWidget);
    m_scanDirectoryPathInput = new QLineEdit(centralWidget);
    storageLayout->addRow(tr("Storage path"), m_storagePathInput);
    scanDirectoryLayout->addRow(tr("Scan directory"), m_scanDirectoryPathInput);

    inputLayout->addLayout(storageLayout, 1);
    inputLayout->addLayout(scanDirectoryLayout, 1);

    m_scanButton = new QPushButton(tr("Scan"), centralWidget);
    m_saveButton = new QPushButton(tr("Save"), centralWidget);
    m_loadButton = new QPushButton(tr("Load"), centralWidget);
    m_refreshButton = new QPushButton(tr("Refresh"), centralWidget);
    inputLayout->addWidget(m_scanButton);
    inputLayout->addWidget(m_saveButton);
    inputLayout->addWidget(m_loadButton);
    inputLayout->addWidget(m_refreshButton);
    mainLayout->addLayout(inputLayout);

    m_countLabel = new QLabel(centralWidget);
    m_statusLabel = new QLabel(centralWidget);
    m_errorLabel = new QLabel(centralWidget);
    m_warningsLabel = new QLabel(centralWidget);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #a00000;"));
    m_warningsLabel->setStyleSheet(QStringLiteral("color: #9a6600;"));
    m_warningsLabel->setWordWrap(true);
    mainLayout->addWidget(m_countLabel);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_errorLabel);
    mainLayout->addWidget(m_warningsLabel);

    m_songListView = new QListView(centralWidget);
    m_songListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_songListView->setModel(m_viewModel.songs());
    mainLayout->addWidget(m_songListView, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::bindViewModel()
{
    m_viewModel.setStoragePath(m_storagePathInput->text());
    m_viewModel.setScanDirectoryPath(m_scanDirectoryPathInput->text());

    bindLineEdit(*m_storagePathInput,
                 &ViewModel::LibraryViewModelProtocol::storagePath,
                 &ViewModel::LibraryViewModelProtocol::setStoragePath,
                 &ViewModel::LibraryViewModelProtocol::storagePathChanged);
    bindLineEdit(*m_scanDirectoryPathInput,
                 &ViewModel::LibraryViewModelProtocol::scanDirectoryPath,
                 &ViewModel::LibraryViewModelProtocol::setScanDirectoryPath,
                 &ViewModel::LibraryViewModelProtocol::scanDirectoryPathChanged);

    bindButton(*m_scanButton, m_viewModel.scanCommand());
    bindButton(*m_saveButton, m_viewModel.saveCommand());
    bindButton(*m_loadButton, m_viewModel.loadCommand());
    bindButton(*m_refreshButton, m_viewModel.refreshCommand());

    connect(&m_viewModel, &ViewModel::LibraryViewModelProtocol::countChanged, this, &MainWindow::updateCount);
    connect(&m_viewModel,
            &ViewModel::LibraryViewModelProtocol::statusMessageChanged,
            this,
            &MainWindow::updateStatusMessage);
    connect(&m_viewModel, &ViewModel::LibraryViewModelProtocol::lastErrorChanged, this, &MainWindow::updateLastError);
    connect(&m_viewModel, &ViewModel::LibraryViewModelProtocol::warningsChanged, this, &MainWindow::updateWarnings);

    updateCount();
    updateStatusMessage();
    updateLastError();
    updateWarnings();
}

void MainWindow::bindLineEdit(QLineEdit &lineEdit,
                              const QString &(ViewModel::LibraryViewModelProtocol::*getter)() const noexcept,
                              void (ViewModel::LibraryViewModelProtocol::*setter)(QString),
                              void (ViewModel::LibraryViewModelProtocol::*changedSignal)())
{
    connect(&lineEdit, &QLineEdit::textChanged, this, [this, setter](const QString &text) {
        (m_viewModel.*setter)(text);
    });

    connect(&m_viewModel, changedSignal, this, [this, &lineEdit, getter]() {
        const QString &value = (m_viewModel.*getter)();
        if (lineEdit.text() != value) {
            lineEdit.setText(value);
        }
    });
}

void MainWindow::bindButton(QPushButton &button, Common::ViewCommand *command)
{
    if (command == nullptr) {
        button.setEnabled(false);
        return;
    }

    button.setEnabled(command->isEnabled());
    connect(&button, &QPushButton::clicked, command, &Common::ViewCommand::execute);
    connect(command, &Common::ViewCommand::enabledChanged, &button, [&button, command]() {
        button.setEnabled(command->isEnabled());
    });
}

void MainWindow::updateCount()
{
    m_countLabel->setText(tr("%n song(s)", nullptr, m_viewModel.count()));
}

void MainWindow::updateStatusMessage()
{
    setLabelVisibleText(*m_statusLabel, m_viewModel.statusMessage());
}

void MainWindow::updateLastError()
{
    setLabelVisibleText(*m_errorLabel, m_viewModel.lastError());
}

void MainWindow::updateWarnings()
{
    const QStringList warnings = m_viewModel.warnings();
    QStringList lines;
    lines.reserve(warnings.size());
    for (const QString &warning : warnings) {
        lines.append(tr("Warning: %1").arg(warning));
    }

    setLabelVisibleText(*m_warningsLabel, lines.join(QLatin1Char('\n')));
}

void MainWindow::setLabelVisibleText(QLabel &label, const QString &text)
{
    label.setText(text);
    label.setVisible(!text.isEmpty());
}

} // namespace AudioPlayer::View
