#include <QtTest/QtTest>

class AudioPlayerSkeletonTest : public QObject
{
    Q_OBJECT

private slots:
    void projectSkeletonIsTestable()
    {
        QVERIFY(true);
    }
};

QTEST_MAIN(AudioPlayerSkeletonTest)

#include "skeleton_test.moc"
