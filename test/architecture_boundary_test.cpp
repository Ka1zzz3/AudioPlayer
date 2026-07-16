#include <QtTest/QtTest>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

class ArchitectureBoundaryTest : public QObject
{
    Q_OBJECT

private slots:
    void widgetsViewBindsOnlyToViewModelProtocol();
    void widgetsSelectionDoesNotDrivePlaybackQueueDirectly();
    void playlistAndRepositoryDetailsDoNotLeakIntoWidgetsView();
    void playbackBackendDoesNotLeakIntoViewOrProtocols();
    void processingBackendDoesNotLeakIntoViewOrProtocols();
    void productionBuildDoesNotReferenceQmlOrQtQuick();
    void vcpkgManifestUsesWidgetsWithoutQmlDependencies();
};

namespace {

QString sourceRoot()
{
#ifdef AUDIOPLAYER_SOURCE_ROOT
    return QString::fromUtf8(AUDIOPLAYER_SOURCE_ROOT);
#else
    return QDir::currentPath();
#endif
}

QString readTextFile(const QString &relativePath)
{
    QFile file(QDir(sourceRoot()).filePath(relativePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTest::qFail(qPrintable(QStringLiteral("Unable to read %1").arg(relativePath)), __FILE__, __LINE__);
        return {};
    }

    return QString::fromUtf8(file.readAll());
}

QStringList productionFiles()
{
    QStringList files{QStringLiteral("CMakeLists.txt"), QStringLiteral("vcpkg.json")};
    const QString srcPath = QDir(sourceRoot()).filePath(QStringLiteral("src"));
    QDirIterator iterator(srcPath,
                          {QStringLiteral("*.cpp"), QStringLiteral("*.h"), QStringLiteral("*.qml")},
                          QDir::Files,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        files.append(QDir(sourceRoot()).relativeFilePath(iterator.next()));
    }
    return files;
}

} // namespace

void ArchitectureBoundaryTest::widgetsViewBindsOnlyToViewModelProtocol()
{
    const QString mainWindowHeader = readTextFile(QStringLiteral("src/View/MainWindow.h"));
    const QString mainWindowSource = readTextFile(QStringLiteral("src/View/MainWindow.cpp"));
    const QString viewText = mainWindowHeader + QLatin1Char('\n') + mainWindowSource;

    QVERIFY2(!viewText.contains(QStringLiteral("#include \"Model/")), "View must not include Model headers.");
    const QStringList forbiddenConcreteViewModels{
        QStringLiteral("#include \"ViewModel/LibraryViewModel.h\""),
        QStringLiteral("#include \"ViewModel/PlaybackViewModel.h\""),
        QStringLiteral("#include \"ViewModel/PlaylistCollectionViewModel.h\""),
    };
    for (const QString &token : forbiddenConcreteViewModels) {
        QVERIFY2(!viewText.contains(token),
                 qPrintable(QStringLiteral("View must depend on protocols, not concrete ViewModel token %1").arg(token)));
    }
    QVERIFY2(viewText.contains(QStringLiteral("LibraryViewModelProtocol")),
             "View should bind through the library ViewModel protocol.");
    QVERIFY2(viewText.contains(QStringLiteral("PlaylistCollectionViewModelProtocol")),
             "View should bind through the playlist ViewModel protocol.");
    QVERIFY2(viewText.contains(QStringLiteral("PlaybackViewModelProtocol")),
             "View should bind through the playback ViewModel protocol.");
    QVERIFY2(viewText.contains(QStringLiteral("ViewCommand")), "View should bind button intents through ViewCommand.");

    const QRegularExpression directBusinessCall(QStringLiteral(R"(\.\s*(load|save|refresh|scanDirectory)\s*\()"));
    QVERIFY2(!directBusinessCall.match(viewText).hasMatch(),
             "View must not directly call load/save/refresh/scanDirectory business methods.");
}

void ArchitectureBoundaryTest::widgetsSelectionDoesNotDrivePlaybackQueueDirectly()
{
    const QString mainWindowText = readTextFile(QStringLiteral("src/View/MainWindow.h"))
                                 + QLatin1Char('\n')
                                 + readTextFile(QStringLiteral("src/View/MainWindow.cpp"));
    const QStringList forbiddenTokens{
        QStringLiteral("setCurrentPlaybackIndex"),
        QStringLiteral("replaceQueue"),
        QStringLiteral("currentPlaybackIndexChanged"),
        QStringLiteral("syncPlaybackSelection"),
    };

    for (const QString &token : forbiddenTokens) {
        QVERIFY2(!mainWindowText.contains(token),
                 qPrintable(QStringLiteral("Widgets View selection must not directly drive playback token %1").arg(token)));
    }
}

void ArchitectureBoundaryTest::playlistAndRepositoryDetailsDoNotLeakIntoWidgetsView()
{
    const QString mainWindowText = readTextFile(QStringLiteral("src/View/MainWindow.h"))
                                 + QLatin1Char('\n')
                                 + readTextFile(QStringLiteral("src/View/MainWindow.cpp"));
    const QStringList forbiddenTokens{
        QStringLiteral("LibraryDocument"),
        QStringLiteral("LibraryPlaylist"),
        QStringLiteral("JsonLibraryDocumentRepository"),
        QStringLiteral("JsonSongRepository"),
        QStringLiteral("PlaylistCollectionUseCase"),
        QStringLiteral("LibraryUseCase"),
        QStringLiteral("PlaybackUseCase"),
    };

    for (const QString &token : forbiddenTokens) {
        QVERIFY2(!mainWindowText.contains(token),
                 qPrintable(QStringLiteral("Widgets View must not expose Model/repository/usecase token %1").arg(token)));
    }
}

void ArchitectureBoundaryTest::playbackBackendDoesNotLeakIntoViewOrProtocols()
{
    const QStringList filesToCheck{
        QStringLiteral("src/View/MainWindow.h"),
        QStringLiteral("src/View/MainWindow.cpp"),
        QStringLiteral("src/ViewModel/LibraryViewModelProtocol.h"),
        QStringLiteral("src/ViewModel/PlaybackViewModelProtocol.h"),
        QStringLiteral("src/ViewModel/PlaylistCollectionViewModelProtocol.h"),
    };
    const QStringList forbiddenTokens{
        QStringLiteral("QMediaPlayer"),
        QStringLiteral("QAudioOutput"),
        QStringLiteral("QMediaPlaylist"),
        QStringLiteral("#include <QMedia"),
        QStringLiteral("#include <QAudioOutput"),
        QStringLiteral("#include \"Model/Service/QtMultimediaPlaybackService.h\""),
    };

    for (const QString &relativePath : filesToCheck) {
        const QString text = readTextFile(relativePath);
        for (const QString &token : forbiddenTokens) {
            QVERIFY2(!text.contains(token),
                     qPrintable(QStringLiteral("%1 must not expose playback backend token %2")
                                     .arg(relativePath, token)));
        }
    }
}


void ArchitectureBoundaryTest::processingBackendDoesNotLeakIntoViewOrProtocols()
{
    const QStringList filesToCheck{
        QStringLiteral("src/View/MainWindow.h"),
        QStringLiteral("src/View/MainWindow.cpp"),
        QStringLiteral("src/ViewModel/LibraryViewModelProtocol.h"),
        QStringLiteral("src/ViewModel/PlaybackViewModelProtocol.h"),
        QStringLiteral("src/ViewModel/PlaylistCollectionViewModelProtocol.h"),
        QStringLiteral("src/ViewModel/ProcessingViewModelProtocol.h"),
    };
    const QStringList forbiddenTokens{
        QStringLiteral("QProcess"),
        QStringLiteral("ffmpeg"),
        QStringLiteral("ffprobe"),
        QStringLiteral("FfmpegTranscodingBackend"),
        QStringLiteral("ITranscodingBackend"),
        QStringLiteral("ProcessingUseCase"),
    };

    for (const QString &relativePath : filesToCheck) {
        const QString text = readTextFile(relativePath);
        for (const QString &token : forbiddenTokens) {
            QVERIFY2(!text.contains(token),
                     qPrintable(QStringLiteral("%1 must not expose processing backend token %2")
                                     .arg(relativePath, token)));
        }
    }
}

void ArchitectureBoundaryTest::productionBuildDoesNotReferenceQmlOrQtQuick()
{
    const QStringList forbiddenTokens{
        QStringLiteral("qt_add_qml_module"),
        QStringLiteral("Qt6::Qml"),
        QStringLiteral("Qt6::Quick"),
        QStringLiteral("QQml"),
        QStringLiteral("QGuiApplication"),
        QStringLiteral("Main.qml"),
        QStringLiteral("QML_")};

    for (const QString &relativePath : productionFiles()) {
        const QString text = readTextFile(relativePath);
        for (const QString &token : forbiddenTokens) {
            QVERIFY2(!text.contains(token), qPrintable(QStringLiteral("%1 must not contain %2").arg(relativePath, token)));
        }
    }
}

void ArchitectureBoundaryTest::vcpkgManifestUsesWidgetsWithoutQmlDependencies()
{
    const QString manifest = readTextFile(QStringLiteral("vcpkg.json"));
    QVERIFY2(manifest.contains(QStringLiteral("\"widgets\"")), "vcpkg qtbase features must include Widgets.");
    QVERIFY2(manifest.contains(QStringLiteral("\"qtmultimedia\"")), "vcpkg manifest must include Qt Multimedia.");
    QVERIFY2(!manifest.contains(QStringLiteral("qtdeclarative")), "vcpkg manifest must not depend on Qt Declarative/QML.");
    QVERIFY2(!manifest.contains(QStringLiteral("\"qml\"")), "vcpkg manifest must not enable Qt Multimedia QML imports.");
    QVERIFY2(!manifest.contains(QStringLiteral("qtshadertools")), "vcpkg manifest must not retain Qt Quick shader tooling override.");
    QVERIFY2(!manifest.contains(QStringLiteral("qtlanguageserver")), "vcpkg manifest must not retain QML language server override.");
}

QTEST_MAIN(ArchitectureBoundaryTest)

#include "architecture_boundary_test.moc"
