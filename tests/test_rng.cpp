#include "firescope/DeterministicRNG.hpp"
#include <iostream>
#include <stdexcept>
using namespace firescope;
int main() {
    try {
        if (hashCell(42,0,0,0) != 3752303144u || hashCell(42,127,127,100) != 1397797364u ||
            hashCell(0,0,0,0) != 2323667865u || hashCell(123,17,29,501) != 3873780843u)
            throw std::runtime_error("PRNG golden vectors changed");
        if (ignitionSucceeds(0,0) || ignitionSucceeds(UINT32_MAX,0) ||
            !ignitionSucceeds(UINT32_MAX,1) || !ignitionSucceeds(0,1) ||
            !ignitionSucceeds(2147483647u,0.5f) || ignitionSucceeds(2147483648u,0.5f))
            throw std::runtime_error("Probability threshold boundary failed");
        std::cout << "PASS: fixed hash vectors and probability 0/0.5/1 boundaries\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
