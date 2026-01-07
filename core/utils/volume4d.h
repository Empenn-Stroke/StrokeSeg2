#include <vector>
#include <array>

struct Volume4D {
    int C, X, Y, Z;                     // Current shape (Channels, Width, Height, Depth)
    std::array<float,3> spacing;        // Current spacing in mm (sx, sy, sz) 
    std::vector<float> data;            // Input array of shape (C, X, Y, Z)

    // Total number of valid elements
    size_t count(const Volume4D *mask = nullptr) const;


    // Access element at (c, x, y, z)
    inline float &at(int c, int x, int y, int z);

    inline const float &at(int c, int x, int y, int z) const;

    double sum(const Volume4D *mask = nullptr) const;
    double mean(const Volume4D *mask = nullptr) const;
    double variance(const Volume4D *mask = nullptr) const;
    double stddev(const Volume4D *mask = nullptr) const;
};