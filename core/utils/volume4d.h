#include <vector>
#include <array>

struct Volume4D {
    int C, X, Y, Z;                     // Current shape (Channels, Width, Height, Depth)
    std::array<float,3> spacing;        // Current spacing in mm (sx, sy, sz) 
    std::vector<float> data;            // Input array of shape (C, X, Y, Z)


    // Access element at (c, x, y, z)
    inline float &at(int c, int x, int y, int z);
};