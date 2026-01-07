#pragma once
#include <utils/volume4d.h>
#include <array>
#include <string>
#include <vector>
#include <tuple>

struct PreprocessedVolume {
    Volume4D data;
    std::array<float, 16> affine; // Flattened 4x4 matrix
    std::array<int, 3> original_shape;
    std::string trsf_path;
    std::array<float, 3> spacing;
    std::vector<std::array<int, 2>> padding;
    std::string MNI_base_image;
};