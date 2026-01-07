#include "volume4d.h"

inline float &Volume4D::at(int c, int x, int y, int z) {
    return data[((c * X + x) * Y + y) * Z + z];
}

inline const float &Volume4D::at(int c, int x, int y, int z) const {
    return data[((c * X + x) * Y + y) * Z + z];
}



size_t Volume4D::count(const Volume4D *mask) const {
    if (!mask) {
        return data.size();
    }
    size_t cnt = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        if (mask->data[i] >= 0) {
            ++cnt;
        }
    }
    return cnt;
}

double Volume4D::sum(const Volume4D *mask = nullptr) const {
    double s = 0.0;
    for (size_t i = 0; i < count(); ++i) {
        if (mask && mask->data[i] < 0) {
            continue;
        }
        s += data[i];
    }
    return s;
}

double Volume4D::mean(const Volume4D *mask) const {
    double s = 0.0;
    size_t cnt = 0;
    for (size_t i = 0; i < count(); ++i) {
        if (mask && mask->data[i] < 0) {
            continue;
        }
        s += data[i];
        cnt++
    }
    return s / cnt;

}

double Volume4D::variance(const Volume4D *mask) const {
    double sum = 0.0;
    double sq_sum = 0.0;
    size_t count = 0;

    for (size_t i = 0; i < count(); ++i) {
        if (mask && mask->data[i] < 0) {
            continue;
        }
        double v = data[i];
        sum += v;
        sq_sum += v * v;
        count++;
    }
    double mean = (count > 0) ? sum / count : 0.0;
    return (count > 0) ? sq_sum / count - mean * mean : 0.0;
}

double Volume4D::stddev(const Volume4D *mask) const {
    double var = variance(mask);
    return std::sqrt(std::max(var, 1e-8));
}
