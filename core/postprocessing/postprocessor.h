#pragma once
#include "managers/configmanager.h"
#include "utils/animawrapper.h"
#include "utils/niftiVolume.h"

#include "preprocessing/preprocVolume.h"
#include "preprocessing/resampling.h"

namespace postprocessing {

    /**
     * @class Postprocessor
     * @brief TODO:
     */
    class Postprocessor {
      public:
        Postprocessor() = default;
        ~Postprocessor() = default;

        /**
         * @brief Apply postprocessing pipeline on the data produced by the inference step:
         *
         * - Convert the pmap to segmentation data. The pmap can also be returned as is
         * - Remove padding
         * - Uncrop
         * - Resample to the original spacing
         * - Save image
         * - Register to reference only if the inverse transformation was applied during
         *   preprocessing
         */
        void postprocess(const NiftiVolume::Tensor4f &data,
                         const PreprocessedVolume &preproc_volume,
                         const std::array<std::array<int, 2>, 3> &bbox,
                         float segmentation_threshold, bool save_pmap, QString dir,
                         QString trsf_path);

      private:
        ConfigManager &config = ConfigManager::instance();
        AnimaWrapper wrapper;
        preprocessing::Resampling resampler;
    };

} // namespace postprocessing