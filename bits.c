#include "bits.h"

uint32_t BinaryMirror(uint32_t x) {
    uint32_t out = 0;

    for (int i = 0; i < 32; ++i) {
        uint32_t bit = x & 1;
        out = (out << 1) | bit;
        x >>= 1;
    }

    return out;
}

uint32_t CountSequence(uint32_t x) {
    uint32_t count = 0;

    for (int k = 0; k <= 29; ++k) {
        if (((x >> k) & 0x7u) == 0x2u) {
            ++count;
        }
    }
    return count;
}
