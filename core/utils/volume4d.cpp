#include "volume4d.h"

inline float &Volume4D::at(int c, int x, int y, int z) {
    return data[((c * X + x) * Y + y) * Z + z];
}

inline const float &Volume4D::at(int c, int x, int y, int z) const {
    return data[((c * X + x) * Y + y) * Z + z];
}

double Volume4D::mean(const Volume4D *mask) const {
    double sum = 0.0;
    size_t count = 0;

    for (int c = 0; c < C; ++c)
        for (int x = 0; x < X; ++x)
            for (int y = 0; y < Y; ++y)
                for (int z = 0; z < Z; ++z) {

                    if (mask && mask->at(0, x, y, z) < 0)
                        continue;

                    sum += at(c, x, y, z);
                    ++count;
                }

    return (count > 0) ? sum / count : 0.0;
}

double Volume4D::variance(const Volume4D *mask) const {
    double mean_val = mean(mask);
    double sq_sum = 0.0;
    size_t count = 0;

    for (int c = 0; c < C; ++c)
        for (int x = 0; x < X; ++x)
            for (int y = 0; y < Y; ++y)
                for (int z = 0; z < Z; ++z) {

                    if (mask && mask->at(0, x, y, z) < 0)
                        continue;

                    double v = at(c, x, y, z) - mean_val;
                    sq_sum += v * v;
                    ++count;
                }

    return (count > 0) ? sq_sum / count : 0.0;
}

double Volume4D::stddev(const Volume4D *mask) const {
    double var = variance(mask);
    return std::sqrt(std::max(var, 1e-8));
}
