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
        QString atlasPath = "dummy_atlas.nii.gz";
        BrainExtraction bet(&mock, atlasPath);

        QSignalSpy spyProgress(&bet, &BrainExtraction::progress);
        QSignalSpy spyFinished(&bet, &BrainExtraction::finished);

        QTemporaryDir tempDir;
        QString inputImg = tempDir.path() + "/input.nii.gz";
        QString prefix = tempDir.path() + "/test_output";

        // Exécution
        QString result = bet.run(inputImg, prefix);

        // Vérifications
        // 1. Nombre de commandes ANIMA appelées (Rigid, Affine, CreateImg, XML, Apply, Mask, Dense, XML, Apply, Mask, Convert)
        // D'après votre code, il y a environ 11 étapes.
        QVERIFY(mock.callCount >= 10);
        
        // 2. Vérification des signaux
        QVERIFY(spyProgress.count() > 0);
        QCOMPARE(spyFinished.count(), 1);
        
        // 3. Vérification du chemin de sortie
        QString expectedOutput = prefix + "_BET.nii.gz";
        QCOMPARE(result, expectedOutput);
        QCOMPARE(spyFinished.at(0).at(0).toString(), expectedOutput);
    }

    void test_Cancellation() {
        MockAnimaWrapper mock;
        BrainExtraction bet(&mock, "atlas.nii.gz");

        QSignalSpy spyError(&bet, &BrainExtraction::error);

        // On demande l'annulation immédiatement
        bet.requestCancel();

        QString result = bet.run("in.nii.gz", "out");

        // Vérifications
        QVERIFY(result.isEmpty());
        QCOMPARE(spyError.count(), 1);
        QVERIFY(spyError.at(0).at(0).toString().contains("cancelled", Qt::CaseInsensitive));
    }

    void test_AnimaErrorHandling() {
        MockAnimaWrapper mock;
        mock.shouldFail = true; // On force une erreur simulée
        
        BrainExtraction bet(&mock, "atlas.nii.gz");
        QSignalSpy spyError(&bet, &BrainExtraction::error);

        QString result = bet.run("in.nii.gz", "out");

        // Vérifications
        QVERIFY(result.isEmpty());
        QCOMPARE(spyError.count(), 1);
        QCOMPARE(spyError.at(0).at(0).toString(), QString("Simulated ANIMA error"));
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