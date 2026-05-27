#include <QtTest/QTest>
#include <QSignalSpy>
#include <QDir>
#include <QTemporaryDir>

#include <preprocessing/brainextraction.h>

#include "mockanimawrapper.h"


class TestBrainExtraction : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Optionnel : créer un environnement de test
    }

    void test_FullPipelineSequence() {
        MockAnimaWrapper mock;
        QTemporaryDir tempDir;

        QDir dir(tempDir.path());
        dir.mkdir("atlas");
        QString simulatedAtlasDir = tempDir.path() + "/atlas";

        QString atlasPath = simulatedAtlasDir + "/atlas.nii.gz";
        QString iccPath = simulatedAtlasDir + "/BrainMask.nrrd";

        QFile(atlasPath).open(QIODevice::WriteOnly);
        QFile(iccPath).open(QIODevice::WriteOnly);

        BrainExtraction bet(&mock, atlasPath);

        QString inputImg = tempDir.path() + "/input.nii.gz";
        QString prefix = tempDir.path() + "/output";

        QString result = bet.run(inputImg, prefix);

        qDebug() << "Steps executed:" << mock.callCount;
        QVERIFY(mock.callCount >= 10);
    }

    void test_Cancellation() {
        MockAnimaWrapper mock;
        BrainExtraction bet(&mock, "atlas.nii.gz");

        QSignalSpy spyError(&bet, &BrainExtraction::error);

        bet.requestCancel();

        QString result = bet.run("in.nii.gz", "out");

        QVERIFY(result.isEmpty());
        QCOMPARE(spyError.count(), 1);
        QVERIFY(spyError.at(0).at(0).toString().contains("cancelled", Qt::CaseInsensitive));
    }

    void test_AnimaErrorHandling() {
        MockAnimaWrapper mock;
        mock.shouldFail = true;

        BrainExtraction bet(&mock, "atlas.nii.gz");
        QSignalSpy spyError(&bet, &BrainExtraction::error);

        // On lance (cela devrait lever une exception dans runCommand)
        QString result = bet.run("in.nii.gz", "out");

        // Vérifications
        QVERIFY(result.isEmpty()); // Devrait être VRAI maintenant
        QCOMPARE(spyError.count(), 1);

        // On vérifie que le message contient bien l'erreur
        QString errorMsg = spyError.at(0).at(0).toString();
        QVERIFY(errorMsg.contains("failed with exit code 1"));
    }

    void test_CommandContent() {
        MockAnimaWrapper mock;
        BrainExtraction bet(&mock, "my_atlas.nii.gz");

        QTemporaryDir tempDir;
        QString prefix = tempDir.path() + "/p";
        
        // On lance pour vérifier si les arguments sont bien formés (ex: le premier appel)
        bet.run("input.nii.gz", prefix);

        // On ne peut pas facilement tester TOUTES les commandes ici sans complexifier le mock, 
        // mais on peut vérifier si le wrapper a été appelé.
        QVERIFY(!mock.lastCommand.isEmpty());
        QCOMPARE(mock.lastCommand.first(), QString("animaConvertImage")); // Dernière commande du flux
    }
};

// On ajoute ceci pour que le Mock fonctionne si AnimaWrapper n'est pas purement virtuel
// ou pour s'assurer du lien lors des tests.
QTEST_MAIN(TestBrainExtraction)
#include "test_brainextraction.moc"