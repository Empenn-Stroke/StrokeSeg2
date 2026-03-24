#pragma once

#include <QString>
#include <QStringList>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "brainextraction.h"
#include "preprocvolume.h"
#include "resampling.h"

#include <utils/animawrapper.h>
#include <utils/niftiVolume.h>

#include <managers/configmanager.h>

namespace preprocessing {

    /**
     * @class Preprocessor
     * @brief Manage the complet pipeline for 4D volume preprocessing.
     *
     * This class centralise the main preprocessing steps applied to the input volumes before inference. 
     * It is designed to be modular and extensible, allowing for easy integration of
     * additional steps or alternative algorithms as needed. The pipeline is optimized for typical
     * neuroimaging workflows, particularly in the context of stroke lesion segmentation, but can be
     * adapted for other applications with similar requirements.
     *
     * Utility :
     * - Biais correction to correct for intensity inhomogeneities.
     * - Registration to a reference atlas (MNI).
     * - Skull stripping.
     * - Statistic normalization (z-score).
     * - Cropping and padding to fit inference input dimension.
     */
    class Preprocessor {
      public:
        /**
         * @brief Builder for the Preprocessor class.
         * @param res Pointer to instance of the Resampling class.
         * @param br Pointer to instance of the BrainExtraction class.
         * @param wr Pointer to instance of the AnimaWrapper class for executing Anima commands.
         * @param save Indicates if intermediary results should be saved for
         * debugging purposes.
         */
        Preprocessor(Resampling *res, BrainExtraction *br, AnimaWrapper *wr, bool save)
            : resampler(*res), brainExtraction(br), wrapper(wr), save_intermediary_steps(save) {}

        ~Preprocessor() = default;

        /**
         * @brief Exécute le pipeline complet sur une paire de modalités (T1 et FLAIR).
         * * @param t1_path Chemin vers l'image T1.
         * @param flair_path Chemin vers l'image FLAIR.
         * @param temp_dir Répertoire temporaire pour les fichiers intermédiaires.
         * @param bet_only Si vrai, arrête le traitement après l'extraction du cerveau.
         * @return PreprocessedVolume Objet contenant les volumes finaux et leurs métadonnées.
         */
        PreprocessedVolume preprocess(const QString &t1_path, const QString &flair_path,
                                      const QString &temp_dir, bool bet_only);

      private:
        Resampling resampler;
        BrainExtraction *brainExtraction;
        AnimaWrapper *wrapper;
        bool save_intermediary_steps;
        ConfigManager &config = ConfigManager::instance();
        QString atlasImage = atlas_dir + "/Reference_T1.nrrd";

        /**
         * @brief Applique une normalisation Z-Score au volume.
         * * Calcule la moyenne et l'écart-type (éventuellement restreints à un masque de
         * segmentation) pour transformer les intensités : $z = \frac{x - \mu}{\sigma}$.
         * * @param vol Volume à normaliser (modifié en place).
         * @param seg Segmentation optionnelle pour définir la zone de calcul (ex: masque cérébral).
         */
        void zScoreNormalize(NiftiVolume &vol, const NiftiVolume *seg = nullptr);

        /**
         * @brief Identifie les zones contenant des données non nulles.
         * @param vol Volume d'entrée.
         * @return Vecteur de booléens représentant le masque d'activité.
         */
        std::vector<bool> computeNonZeroMask(const NiftiVolume &vol);

        /**
         * @brief Réduit le volume à sa boîte englobante (Bounding Box) non vide.
         * * @param vol Volume source à recadrer.
         * @param seg Segmentation associée à recadrer de façon identique.
         * @param nonzero_label Label considéré comme significatif pour le recadrage.
         * @param bbox_out Pointeur optionnel pour récupérer les coordonnées de la bounding box.
         * @return Paire contenant le volume et la segmentation recadrés.
         */
        std::pair<NiftiVolume, NiftiVolume>
        cropToNonZero(const NiftiVolume &vol, const NiftiVolume *seg = nullptr,
                      int nonzero_label = 1, std::array<std::array<int, 2>, 3> *bbox_out = nullptr);

        /**
         * @brief Ajoute des bordures au volume pour atteindre une taille spécifique ou un multiple.
         * * Utile pour garantir que les dimensions sont compatibles avec les architectures UNet
         * (multiples de $2^n$).
         * * @param vol Volume d'entrée.
         * @param min_size Taille minimale requise.
         * @param div Facteur de divisibilité requis pour les dimensions.
         * @return Paire contenant le volume "paddé" et les offsets appliqués.
         */
        std::pair<NiftiVolume, std::vector<std::array<int, 2>>> padVolume(const NiftiVolume &vol,
                                                                          int min_size, int div);

        /**
         * @brief Réoriente le fichier NIfTI vers l'orientation standard RAS
         * (Right-Anterior-Superior).
         * @param input_path Chemin du fichier source.
         * @param prefix Préfixe pour le fichier de sortie.
         * @return Chemin vers le fichier réorienté.
         */
        QString reorientToRAS(const QString &input_path, const QString &prefix);

        /**
         * @brief Corrige les artéfacts d'illumination (champ d'inhomogénéité).
         * @param input_path Chemin du fichier source.
         * @param prefix Préfixe pour le fichier de sortie.
         * @return Chemin vers le fichier corrigé.
         */
        QString biasCorrect(const QString &input_path, const QString &prefix);

        /**
         * @brief Aligne spatialement le volume sur un atlas de référence (MNI152).
         * * @param input_path Image à recaler.
         * @param mni_image_path Image cible (atlas).
         * @param prefix_label Label pour identifier la sortie.
         * @param base_path_prefix Répertoire de travail.
         * @return Paire contenant [Chemin de l'image recalée, Chemin de la matrice de
         * transformation].
         */
        std::pair<QString, QString> registerToReference(const QString &input_path,
                                                        const QString &mni_image_path,
                                                        const QString &prefix_label,
                                                        const QString &base_path_prefix);

        /**
         * @brief Sous-routine traitant une modalité spécifique.
         * @param modality_path Chemin du fichier de la modalité.
         * @param is_MNI Définit si l'image doit être traitée en espace MNI ou natif.
         * @param bbox_ptr Coordonnées de recadrage à appliquer/récupérer.
         * @return PreprocessedVolume partiel pour cette modalité.
         */
        PreprocessedVolume
        preprocessModality(const QString &modality_path, bool is_MNI,
                           std::array<std::array<int, 2>, 3> *bbox_ptr = nullptr);

        /**
         * @brief Utilitaire de log pour afficher l'étape en cours.
         * @param actionName Nom de l'action de prétraitement.
         */
        static void printAction(const QString &actionName);

        /**
         * @brief Déplace le fichier final vers le répertoire de sortie définitif.
         * @param img_path Chemin actuel du fichier.
         * @return Nouveau chemin du fichier.
         */
        QString moveToOutput(const QString &img_path);
    };

} // namespace preprocessing