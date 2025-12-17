#include "volume4d.h"

inline float &Volume4D::at(int c, int x, int y, int z) {
    return data[((c * X + x) * Y + y) * Z + z];
}
