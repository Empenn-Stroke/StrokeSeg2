#include <QtTest/QTest>
#include <QFileInfo>

#include <utils/animawrapper.h>
#include <utils/path.h>

class TestAnimaWrapper : public QObject {
    Q_OBJECT

private slots:

    void initTestCase() {
        // Vérification minimale : le dossier ANIMA existe
        QVERIFY2(QFileInfo(anima_root_path).exists(),
                 "anima_root_path does not exist");
    }

    void testRun_NoArguments() {
        AnimaWrapper wrapper;

        int ret = wrapper.run({});

        QCOMPARE(ret, -1);
        QVERIFY(!wrapper.lastStderr().isEmpty());
    }

    void testRun_InvalidProgram() {
        AnimaWrapper wrapper;

        int ret = wrapper.run({ "this_program_does_not_exist" });

        QCOMPARE(ret, -1);
        QVERIFY(!wrapper.lastStderr().isEmpty());
    }

    void testRun_ValidProgram_Version() {
        AnimaWrapper wrapper;

        // Programme simple, sans entrée/sortie fichiers
        // adapte si besoin selon ton install ANIMA
        int ret = wrapper.run({
            "animaN4BiasCorrection",
            "--help"
        });

        QCOMPARE(ret, 0);

        QVERIFY(wrapper.lastStdout().contains("Usage") || wrapper.lastStdout().contains("help"));
    }
};

QTEST_MAIN(TestAnimaWrapper)
#include "test_animawrapper.moc"