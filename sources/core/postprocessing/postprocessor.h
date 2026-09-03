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
        Postprocessor(AnimaWrapper *wr) : m_wrapper(wr) {}
        ~Postprocessor() = default;

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
