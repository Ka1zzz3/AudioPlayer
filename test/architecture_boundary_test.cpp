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
    void widgetsViewHasZeroViewModelProtocolDependency();
    void widgetsSelectionDoesNotDrivePlaybackQueueDirectly();
    void playlistAndRepositoryDetailsDoNotLeakIntoWidgetsView();
    void viewFacingProtocolsDoNotExposeModelOrBackendTypes();
    void playbackViewModelObservesUseCaseNotBackendService();
    void appCompositionRootOnlyComposesAndBinds();
    void productionViewModelsRequireInjectedUseCases();
    void audioEffectsDoNotLeakIntoStorageOrProcessingSchemas();
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

void ArchitectureBoundaryTest::widgetsViewHasZeroViewModelProtocolDependency()
{
    const QString mainWindowHeader = readTextFile(QStringLiteral("src/View/MainWindow.h"));
    const QString mainWindowSource = readTextFile(QStringLiteral("src/View/MainWindow.cpp"));
    const QString viewText = mainWindowHeader + QLatin1Char('\n') + mainWindowSource;

    const QStringList forbiddenTokens{
        QStringLiteral("#include \"Model/"),
        QStringLiteral("#include \"ViewModel/"),
        QStringLiteral("ViewModel::"),
        QStringLiteral("ViewModelProtocol"),
        QStringLiteral("scanCommand()"),
        QStringLiteral("loadCommand()"),
        QStringLiteral("saveCommand()"),
        QStringLiteral("pauseCommand()"),
        QStringLiteral("resetPlaybackRateCommand()"),
        QStringLiteral("enqueueCommand()"),
        QStringLiteral("seekTo("),
        QStringLiteral("setVolumePercent("),
        QStringLiteral("setPlaybackRate("),
        QStringLiteral("setEqualizerEnabled("),
        QStringLiteral("setEqualizerBandGain("),
        QStringLiteral("setEqualizerPreset("),
        QStringLiteral("setInputFilePaths("),
        QStringLiteral("setOutputDirectory("),
        QStringLiteral("setSelectedTaskRow("),
    };

    for (const QString &token : forbiddenTokens) {
        QVERIFY2(!viewText.contains(token),
                 qPrintable(QStringLiteral("View must expose binding ports/signals only; forbidden token %1").arg(token)));
    }

    QVERIFY2(viewText.contains(QStringLiteral("setLibraryCommands")), "View should expose command injection ports.");
    QVERIFY2(viewText.contains(QStringLiteral("seekRequested")), "View should expose user-intent signals.");
    QVERIFY2(viewText.contains(QStringLiteral("ViewCommand")), "View should execute injected ViewCommand objects.");
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
        QStringLiteral("ProcessingUseCase"),
        QStringLiteral("TranscodedPlaylistService"),
    };

    for (const QString &token : forbiddenTokens) {
        QVERIFY2(!mainWindowText.contains(token),
                 qPrintable(QStringLiteral("Widgets View must not expose Model/repository/usecase token %1").arg(token)));
    }
}

void ArchitectureBoundaryTest::viewFacingProtocolsDoNotExposeModelOrBackendTypes()
{
    const QStringList filesToCheck{
        QStringLiteral("src/ViewModel/LibraryViewModelProtocol.h"),
        QStringLiteral("src/ViewModel/PlaybackViewModelProtocol.h"),
        QStringLiteral("src/ViewModel/AudioEffectsViewModelProtocol.h"),
        QStringLiteral("src/ViewModel/PlaylistCollectionViewModelProtocol.h"),
        QStringLiteral("src/ViewModel/ProcessingViewModelProtocol.h"),
    };
    const QStringList forbiddenTokens{
        QStringLiteral("#include \"Model/"),
        QStringLiteral("Model::"),
        QStringLiteral("AudioFile"),
        QStringLiteral("LibraryDocument"),
        QStringLiteral("LibraryPlaylist"),
        QStringLiteral("UseCase"),
        QStringLiteral("IPlaybackService"),
        QStringLiteral("ITranscodingBackend"),
        QStringLiteral("FfmpegTranscodingBackend"),
        QStringLiteral("QProcess"),
        QStringLiteral("QMediaPlayer"),
        QStringLiteral("QAudioOutput"),
        QStringLiteral("GStreamer"),
        QStringLiteral("GstElement"),
        QStringLiteral("gst_"),
    };

    for (const QString &relativePath : filesToCheck) {
        const QString text = readTextFile(relativePath);
        for (const QString &token : forbiddenTokens) {
            QVERIFY2(!text.contains(token),
                     qPrintable(QStringLiteral("%1 must not expose Model/backend token %2").arg(relativePath, token)));
        }
    }
}

void ArchitectureBoundaryTest::playbackViewModelObservesUseCaseNotBackendService()
{
    const QString playbackViewModelText = readTextFile(QStringLiteral("src/ViewModel/PlaybackViewModel.h"))
                                      + QLatin1Char('\n')
                                      + readTextFile(QStringLiteral("src/ViewModel/PlaybackViewModel.cpp"));
    QVERIFY2(!playbackViewModelText.contains(QStringLiteral("IPlaybackService")),
             "PlaybackViewModel must observe PlaybackUseCase, not backend service.");
}

void ArchitectureBoundaryTest::appCompositionRootOnlyComposesAndBinds()
{
    const QString appText = readTextFile(QStringLiteral("src/App/AppCompositionRoot.cpp"));
    const QStringList forbiddenTokens{
        QStringLiteral("taskUpdated"),
        QStringLiteral("ProcessingTaskStatus::Succeeded"),
        QStringLiteral("replaceQueue"),
        QStringLiteral("setCurrentPlaybackIndex"),
        QStringLiteral("currentVisibleSongsChanged"),
        QStringLiteral("playRequested"),
        QStringLiteral("reportIntegrationWarning"),
        QStringLiteral("AudioFile("),
        QStringLiteral("QSet"),
    };
    for (const QString &token : forbiddenTokens) {
        QVERIFY2(!appText.contains(token),
                 qPrintable(QStringLiteral("AppCompositionRoot must not own workflow token %1").arg(token)));
    }
}



void ArchitectureBoundaryTest::productionViewModelsRequireInjectedUseCases()
{
    const QStringList filesToCheck{
        QStringLiteral("src/ViewModel/LibraryViewModel.h"),
        QStringLiteral("src/ViewModel/LibraryViewModel.cpp"),
        QStringLiteral("src/ViewModel/PlaylistCollectionViewModel.h"),
        QStringLiteral("src/ViewModel/PlaylistCollectionViewModel.cpp"),
    };
    const QStringList forbiddenTokens{
        QStringLiteral("LibraryViewModel(QObject"),
        QStringLiteral("PlaylistCollectionViewModel(QObject"),
        QStringLiteral("std::make_shared<ModelService::LibraryUseCase>"),
        QStringLiteral("std::make_shared<ModelService::PlaylistCollectionUseCase>"),
    };

    for (const QString &relativePath : filesToCheck) {
        const QString text = readTextFile(relativePath);
        for (const QString &token : forbiddenTokens) {
            QVERIFY2(!text.contains(token),
                     qPrintable(QStringLiteral("%1 must require App/test injection; forbidden token %2").arg(relativePath, token)));
        }
    }
}

void ArchitectureBoundaryTest::audioEffectsDoNotLeakIntoStorageOrProcessingSchemas()
{
    const QStringList filesToCheck{
        QStringLiteral("src/Model/JsonLibraryDocumentRepository.h"),
        QStringLiteral("src/Model/JsonLibraryDocumentRepository.cpp"),
        QStringLiteral("src/Model/LibraryDocument.h"),
        QStringLiteral("src/Model/LibraryDocument.cpp"),
        QStringLiteral("src/Model/LibraryPlaylist.h"),
        QStringLiteral("src/Model/LibraryPlaylist.cpp"),
        QStringLiteral("src/Model/ProcessingTask.h"),
        QStringLiteral("src/Model/ProcessingTask.cpp"),
        QStringLiteral("src/Model/ProcessingTypes.h"),
        QStringLiteral("src/Model/ProcessingTypes.cpp"),
        QStringLiteral("src/Model/Service/FfmpegTranscodingBackend.h"),
        QStringLiteral("src/Model/Service/FfmpegTranscodingBackend.cpp"),
    };
    const QStringList forbiddenTokens{
        QStringLiteral("AudioEffects"),
        QStringLiteral("Equalizer"),
        QStringLiteral("playbackRate"),
        QStringLiteral("equalizer"),
        QStringLiteral("presetGains"),
    };

    for (const QString &relativePath : filesToCheck) {
        const QString text = readTextFile(relativePath);
        for (const QString &token : forbiddenTokens) {
            QVERIFY2(!text.contains(token),
                     qPrintable(QStringLiteral("%1 must not persist or process session-only audio effects token %2")
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
