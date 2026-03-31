#include <QtTest/QTest>
#include <QFileInfo>

#include <utils/animawrapper.h>
#include <utils/path.h>
#include <iostream>

class TestAnimaWrapper : public QObject {
    Q_OBJECT

private slots:

    void initTestCase() {
        // Vérification minimale : le dossier ANIMA existe
        qDebug() << Paths::animaRootPath();
        QVERIFY2(QFileInfo(Paths::animaRootPath()).exists(),
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

    void testRun_ValidProgram_ErrorExit() {
        AnimaWrapper wrapper;

        int ret = wrapper.run({"animaN4BiasCorrection", "--this-argument-does-not-exist"});

        QVERIFY(ret != 0);
        QVERIFY(!wrapper.lastStderr().isEmpty());
    }

    void testRun_OutputIsReset() {
        AnimaWrapper wrapper;

        wrapper.run({"this_program_does_not_exist"});
        QVERIFY(!wrapper.lastStderr().isEmpty());

        wrapper.run({"animaN4BiasCorrection", "--help"});
        QVERIFY(wrapper.lastStderr().isEmpty());
    }

};

QTEST_MAIN(TestAnimaWrapper)
#include "test_animawrapper.moc"