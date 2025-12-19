#include "preprocessor.h"

preprocessing::Preprocessor::Preprocessor() {}

preprocessing::Preprocessor::~Preprocessor() {}

Volume4D preprocessing::Preprocessor::preprocess(const Volume4D &vol,
                                                 const std::array<float, 3> &target_spacing) {
    return Volume4D();
}
