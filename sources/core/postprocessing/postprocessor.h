// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "managers/configmanager.h"
#include "utils/animawrapper.h"
#include "utils/niftiVolume.h"

#include "preprocessing/preprocVolume.h"
#include "preprocessing/resampling.h"

namespace postprocessing 
{

    /**
     * @class Postprocessor
     * @brief Class responsible for applying the postprocessing pipeline to the output of the
     * inference step. The postprocessing pipeline includes the following steps:
     * - Convert the pmap to segmentation data. The pmap can also be returned as is.
     * - Remove padding.
     * - Uncrop.
     * - Resample to the original spacing.
     */
    class Postprocessor 
    {
      public:
        /**
         * @brief Constructor for the Postprocessor class.
         *
         * @param wr A pointer to the AnimaWrapper object to be used for postprocessing.
         */
        Postprocessor(AnimaWrapper *wr) : m_wrapper(wr) {}

        /**
         * @brief Destructor for the Postprocessor class.
         */
        ~Postprocessor() = default;

        /**
         * @brief Applies the postprocessing pipeline to the inference output.
         *
         * This function processes the inference output data through a series of steps,
         * including converting the pmap to segmentation data, removing padding, uncropping,
         * and resampling to the original spacing.
         *
         * @param data The inference output data as a 4D tensor.
         * @param preproc_volume The preprocessed volume associated with the inference output.
         * @param bbox The bounding box for the processed region.
         * @param segmentation_threshold The threshold for converting the pmap to segmentation.
         * @param save_pmap A boolean indicating whether to save the pmap.
         * @param dir The directory where the output files should be saved.
         * @param trsfPath The path to the transformation file.
         * @param mni A boolean indicating whether to use MNI space.
         * @param saveInterSteps A boolean indicating whether to save intermediate processing steps.
         * @return The processed NIfTI volume as a NiftiVolume object.
         */
        NiftiVolume postprocess(const NiftiVolume::Tensor4f &data,
                                 const PreprocessedVolume &preproc_volume,
                                 const std::array<std::array<int, 2>, 3> &bbox,
                                 float segmentation_threshold, bool save_pmap, QString dir,
                                 QString trsfPath, bool mni, bool saveInterSteps);

      private:
        ConfigManager &m_config = ConfigManager::instance();
        AnimaWrapper *m_wrapper;
        preprocessing::Resampler m_resampler;
        
    };

} // namespace postprocessing
