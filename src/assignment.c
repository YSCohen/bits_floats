#include "assignment.h"

#include <math.h>
#include <stddef.h>

// Problem 1

unsigned bitset_symmetric_difference_count(uint32_t a, uint32_t b)
{
    uint32_t x = a ^ b;
    unsigned sum = 0;
    while (x) {
        sum += x & 1;
        x >>= 1;
    }
    return sum;
}

// Problem 2

uint32_t bit_range_mask(unsigned low, unsigned high)
{
    uint32_t mask = 0xFFFFFFFF;
    mask >>= (31 + low - high);
    mask <<= low;
    return mask;
}

uint32_t splice_bits(
    uint32_t destination,
    uint32_t source,
    unsigned low,
    unsigned high)
{
    uint32_t dest_mask = ~bit_range_mask(low, high); // ~ flipped
    uint32_t src_mask = bit_range_mask(0, (high - low));
    return (destination & dest_mask) | ((source & src_mask) << low);
}

// Problem 3

int32_t decode_twos_complement(uint32_t bits, unsigned width)
{
    unsigned width_minus_1 = width - 1;
    int32_t dec = bits & bit_range_mask(0, width_minus_1);
    if (width < 32 && dec >> (width - 1)) {
        dec |= bit_range_mask(width, 31);
    }
    return dec;
}

// Problem 4

bool add_signed_width(
    uint32_t a_bits,
    uint32_t b_bits,
    unsigned width,
    uint32_t* sum_bits)
{
    unsigned width_minus_1 = width - 1;
    uint32_t mask = bit_range_mask(0, width_minus_1);
    a_bits &= mask;
    b_bits &= mask;

    *sum_bits = (a_bits + b_bits) & mask;

    if (!((a_bits >> width_minus_1) ^ (b_bits >> width_minus_1))) {
        // ^ = if a and b had the same sign,
        // v = and the sum has the other sign, return true (overflow)
        return (a_bits >> width_minus_1) ^ (*sum_bits >> width_minus_1);

    } else {
        // if they have different signs, overflow is impossible
        return false;
    }
}

// Problem 5

int compare_signed_unsigned_math(
    int32_t signed_value,
    uint32_t unsigned_value)
{
    if (signed_value < 0) {
        return -1;
    } else {
        uint32_t us = (uint32_t)signed_value;

        if (us < unsigned_value) {
            return -1;
        } else {
            return us > unsigned_value;
        }
    }
}

// Problem 6

uint32_t multiply_by_24(uint32_t x)
{
    return (x << 5) - (x << 3);
}

// Problem 7

fp32_info_t fp32_decode(uint32_t bits)
{
    uint8_t exp = (bits >> 23) & bit_range_mask(0, 7);
    uint32_t frac = bits & bit_range_mask(0, 22);

    fp32_kind_t kind = FP32_NORMAL;
    if (exp == 0) {
        if (frac == 0) {
            kind = FP32_ZERO;
        } else {
            kind = FP32_SUBNORMAL;
        }
    } else if (exp == 255) {
        if (frac == 0) {
            kind = FP32_INFINITY;
        } else {
            kind = FP32_NAN;
        }
    }

    int ube = exp - 127;
    if (kind == FP32_ZERO || kind == FP32_SUBNORMAL) {
        ube = -126;
    } else if (kind == FP32_INFINITY || kind == FP32_NAN) {
        ube = 0;
    }

    fp32_info_t info = {
        .negative = bits & bit_range_mask(31, 31),
        .kind = kind,
        .exponent_bits = exp,
        .fraction_bits = frac,
        .unbiased_exponent = ube,
    };
    return info;
}

static double two_to_the_n(int n)
{
    double ans = 1.0;
    for (int i = n; i > 0; i--) {
        ans *= 2.0;
    }
    for (int i = n; i < 0; i++) {
        ans *= 0.5;
    }
    return ans;
}

double fp32_exact_value(uint32_t bits)
{
    fp32_info_t info = fp32_decode(bits);
    double sign = info.negative ? -1.0 : 1.0;

    if (info.kind == FP32_NORMAL) {
        double mantissa = 1.0 + info.fraction_bits * two_to_the_n(-23);
        return sign * mantissa * two_to_the_n(info.unbiased_exponent);
    } else if (info.kind == FP32_SUBNORMAL) {
        double mantissa = info.fraction_bits * two_to_the_n(-23);
        return sign * mantissa * two_to_the_n(-126);
    } else if (info.kind == FP32_ZERO) {
        return sign * 0.0;
    } else if (info.kind == FP32_INFINITY) {
        return sign * INFINITY;
    } else {
        return sign * NAN;
    }
}

// Problem 8

uint32_t fp32_next_up_bits(uint32_t bits)
{
    fp32_info_t info = fp32_decode(bits);

    if (info.kind == FP32_NAN || (info.kind == FP32_INFINITY && !info.negative)) {
        return bits;
    } else if (info.kind == FP32_ZERO) {
        return 1;
    } else if (info.negative) {
        return bits - 1;
    } else {
        return bits + 1;
    }
}

// Problem 9

bool fp32_truncate_to_int(uint32_t bits, int32_t* out)
{
    fp32_info_t info = fp32_decode(bits);

    if (info.kind == FP32_INFINITY || info.kind == FP32_NAN) {
        return false;
    }

    if (info.kind == FP32_ZERO
        || info.kind == FP32_SUBNORMAL
        || info.unbiased_exponent < 0) {

        *out = 0;
        return true;
    }

    if (info.unbiased_exponent >= 31) {
        if (info.unbiased_exponent == 31 && info.negative && info.fraction_bits == 0) {
            *out = INT32_MIN;
            return true;
        }

        return false;
    }

    uint32_t unscaled = info.fraction_bits | bit_range_mask(23, 23);

    if (info.unbiased_exponent >= 23) {
        *out = unscaled << (info.unbiased_exponent - 23);
    } else {
        *out = unscaled >> (23 - info.unbiased_exponent);
    }

    if (info.negative) {
        *out = -(*out);
    }

    return true;
}
