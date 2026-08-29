#pragma once

#include <array>
#include <vector>

namespace bakuon::test
{
class ReferenceTruePeakMeter
{
public:
    static double measureLinear(const std::vector<std::array<double, 2>>& signal,
                                int channels);
};
}
