/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2(+), Libre 3(+), Dexcom G7/ONE+,        */
/*      Sibionics GS1Sb and GS3, Accu-Chek SmartGuide, CareSens Air and              */
/*      Aidex X sensors.                                                             */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Sun Aug 30 10:21:11 CEST 2026                                                */

#include "p256_scalar_multiply.h"

#include "p256_private_scalar_codec.h"

#include <stddef.h>
#include <string.h>

enum { F3F3C248_LIMBS = 8 };

typedef struct {
    uint32_t w[F3F3C248_LIMBS];
} f3f3c248_fe;

typedef struct {
    f3f3c248_fe x;
    f3f3c248_fe y;
    f3f3c248_fe z;
    int infinity;
} f3f3c248_jacobian;

static uint32_t load28_le_bits(const uint8_t in[35], unsigned lane) {
    const unsigned bit = lane * 28u;
    const unsigned byte = bit >> 3;
    const unsigned shift = bit & 7u;
    uint64_t acc = 0u;
    for (unsigned i = 0; i < 5u && byte + i < 35u; ++i) {
        acc |= (uint64_t)in[byte + i] << (8u * i);
    }
    return (uint32_t)(acc >> shift) & 0x0fffffffu;
}

static void decode_coordinate_35_to_10_lanes(
    const uint8_t in35[35],
    uint32_t out10[10]) {
    uint32_t x[10];
    for (unsigned i = 0; i < 10u; ++i) x[i] = load28_le_bits(in35, i);
    out10[0] = x[0] * 0x76a1775du + 0x76cba8dbu;
    out10[1] = x[1] * 0x414f921bu + 0x7e35f794u;
    out10[2] = x[2] * 0x0bc8c375u + 0x890be324u;
    out10[3] = x[3] * 0x6e86ec5fu + 0x74f9345fu;
    out10[4] = x[4] * 0x6ca29251u + 0xb12a6ae2u;
    out10[5] = x[5] * 0x2ce4237fu + 0x6932e193u;
    out10[6] = x[6] * 0x7a015a79u - 0x07e77a96u;
    out10[7] = x[7] * (uint32_t)-0x39a2e1c5 + (uint32_t)-0x365aac8f;
    out10[8] = x[8] * 0x76a1775du + 0x76cba8dbu;
    out10[9] = x[9] * 0x414f921bu + 0x7e35f794u;
}

/* P-256 in little-endian 32-bit limbs.  The app path only invokes this
 * module for P-256: both the fixed Libre base point and the recovered parent
 * context use this modulus. */
static const f3f3c248_fe k_p256_modulus = {{
    0xffffffffu, 0xffffffffu, 0xffffffffu, 0x00000000u,
    0x00000000u, 0x00000000u, 0x00000001u, 0xffffffffu
}};

/* Montgomery R and R^2 for R = 2^256. */
static const f3f3c248_fe k_p256_montgomery_one = {{
    0x00000001u, 0x00000000u, 0x00000000u, 0xffffffffu,
    0xffffffffu, 0xffffffffu, 0xfffffffeu, 0x00000000u
}};
static const f3f3c248_fe k_p256_montgomery_r2 = {{
    0x00000003u, 0x00000000u, 0xffffffffu, 0xfffffffbu,
    0xfffffffeu, 0xffffffffu, 0xfffffffdu, 0x00000004u
}};
/* R^3 lets the 24 high bits of the recovered 35-byte format enter
 * Montgomery form in one multiply: high * R^3 / R = high * R^2. */
static const f3f3c248_fe k_p256_montgomery_r3 = {{
    0x0000000au, 0xfffffffdu, 0xfffffff7u, 0xffffffedu,
    0xfffffffcu, 0x00000005u, 0x00000001u, 0x00000018u
}};

static void fe_zero(f3f3c248_fe *r) {
    memset(r, 0, sizeof(*r));
}

static void fe_one(f3f3c248_fe *r) {
    *r = k_p256_montgomery_one;
}

static int fe_cmp(const f3f3c248_fe *a, const f3f3c248_fe *b) {
    for (unsigned i = F3F3C248_LIMBS; i-- > 0u;) {
        if (a->w[i] < b->w[i]) return -1;
        if (a->w[i] > b->w[i]) return 1;
    }
    return 0;
}

static uint32_t fe_sub_raw(
    f3f3c248_fe *r,
    const f3f3c248_fe *a,
    const f3f3c248_fe *b) {
    f3f3c248_fe t;
    uint64_t borrow = 0u;
    for (unsigned i = 0; i < F3F3C248_LIMBS; ++i) {
        uint64_t sub = (uint64_t)b->w[i] + borrow;
        t.w[i] = (uint32_t)((uint64_t)a->w[i] - sub);
        borrow = (uint64_t)a->w[i] < sub;
    }
    *r = t;
    return (uint32_t)borrow;
}

static uint32_t fe_add_raw(
    f3f3c248_fe *r,
    const f3f3c248_fe *a,
    const f3f3c248_fe *b) {
    f3f3c248_fe t;
    uint64_t carry = 0u;
    for (unsigned i = 0; i < F3F3C248_LIMBS; ++i) {
        uint64_t sum = (uint64_t)a->w[i] + b->w[i] + carry;
        t.w[i] = (uint32_t)sum;
        carry = sum >> 32;
    }
    *r = t;
    return (uint32_t)carry;
}

static int fe_is_zero(const f3f3c248_fe *a) {
    uint32_t any = 0u;
    for (unsigned i = 0; i < F3F3C248_LIMBS; ++i) any |= a->w[i];
    return any == 0u;
}

static void fe_add(
    f3f3c248_fe *r,
    const f3f3c248_fe *a,
    const f3f3c248_fe *b) {
    f3f3c248_fe t;
    uint32_t carry = fe_add_raw(&t, a, b);
    if (carry || fe_cmp(&t, &k_p256_modulus) >= 0) {
        fe_sub_raw(&t, &t, &k_p256_modulus);
    }
    *r = t;
}

static void fe_sub(
    f3f3c248_fe *r,
    const f3f3c248_fe *a,
    const f3f3c248_fe *b) {
    f3f3c248_fe t;
    if (fe_cmp(a, b) >= 0) {
        fe_sub_raw(&t, a, b);
    } else {
        f3f3c248_fe d;
        fe_sub_raw(&d, b, a);
        fe_sub_raw(&t, &k_p256_modulus, &d);
    }
    *r = t;
}

static void fe_double(
    f3f3c248_fe *r,
    const f3f3c248_fe *a) {
    fe_add(r, a, a);
}

static void fe_mul(
    f3f3c248_fe *r,
    const f3f3c248_fe *a,
    const f3f3c248_fe *b) {
    uint32_t product[18] = {0};

    /* Schoolbook 8x8 product.  A 64-bit accumulator is sufficient because
     * each step contains one 32x32 product, one existing limb and carry. */
    for (unsigned i = 0; i < F3F3C248_LIMBS; ++i) {
        uint64_t carry = 0u;
        for (unsigned j = 0; j < F3F3C248_LIMBS; ++j) {
            const unsigned pos = i + j;
            const uint64_t value = (uint64_t)a->w[i] * b->w[j] +
                                   product[pos] + carry;
            product[pos] = (uint32_t)value;
            carry = value >> 32;
        }
        unsigned pos = i + F3F3C248_LIMBS;
        while (carry) {
            const uint64_t value = (uint64_t)product[pos] + carry;
            product[pos++] = (uint32_t)value;
            carry = value >> 32;
        }
    }

    /* Montgomery REDC.  P-256's low limb is -1, therefore
     * -p^-1 mod 2^32 is exactly 1 and m is simply product[i]. */
    for (unsigned i = 0; i < F3F3C248_LIMBS; ++i) {
        const uint32_t m = product[i];
        uint64_t carry = 0u;
        for (unsigned j = 0; j < F3F3C248_LIMBS; ++j) {
            const unsigned pos = i + j;
            const uint64_t value = (uint64_t)m * k_p256_modulus.w[j] +
                                   product[pos] + carry;
            product[pos] = (uint32_t)value;
            carry = value >> 32;
        }
        unsigned pos = i + F3F3C248_LIMBS;
        while (carry) {
            const uint64_t value = (uint64_t)product[pos] + carry;
            product[pos++] = (uint32_t)value;
            carry = value >> 32;
        }
    }

    f3f3c248_fe reduced;
    for (unsigned i = 0; i < F3F3C248_LIMBS; ++i) {
        reduced.w[i] = product[i + F3F3C248_LIMBS];
    }
    uint32_t high = product[2u * F3F3C248_LIMBS];
    if (high || fe_cmp(&reduced, &k_p256_modulus) >= 0) {
        const uint32_t borrow = fe_sub_raw(&reduced, &reduced,
                                           &k_p256_modulus);
        high -= borrow;
    }
    /* REDC is below 2p, so one subtraction always clears the high limb. */
    if (high || fe_cmp(&reduced, &k_p256_modulus) >= 0) {
        fe_sub_raw(&reduced, &reduced, &k_p256_modulus);
    }
    *r = reduced;
}

static void fe_square(
    f3f3c248_fe *r,
    const f3f3c248_fe *a) {
    fe_mul(r, a, a);
}

static void fe_import_raw32(f3f3c248_fe *r, const uint8_t in[32]) {
    fe_zero(r);
    for (unsigned i = 0; i < 32u; ++i) {
        r->w[i >> 2] |= (uint32_t)in[i] << ((i & 3u) * 8u);
    }
}

static void fe_import_mod35(
    f3f3c248_fe *r,
    const uint8_t in[35]) {
    f3f3c248_fe low, high, low_montgomery, high_montgomery;
    fe_import_raw32(&low, in);
    fe_mul(&low_montgomery, &low, &k_p256_montgomery_r2);

    fe_zero(&high);
    high.w[0] = (uint32_t)in[32] |
                (uint32_t)in[33] << 8 |
                (uint32_t)in[34] << 16;
    fe_mul(&high_montgomery, &high, &k_p256_montgomery_r3);
    fe_add(r, &low_montgomery, &high_montgomery);
}

static void fe_export35(uint8_t out[35], const f3f3c248_fe *a) {
    f3f3c248_fe normal_one, normal;
    fe_zero(&normal_one);
    normal_one.w[0] = 1u;
    fe_mul(&normal, a, &normal_one);
    for (unsigned i = 0; i < 32u; ++i) {
        out[i] = (uint8_t)(normal.w[i >> 2] >> ((i & 3u) * 8u));
    }
    out[32] = out[33] = out[34] = 0u;
}

static void fe_inverse(
    f3f3c248_fe *r,
    const f3f3c248_fe *a) {
    f3f3c248_fe exponent = k_p256_modulus;
    f3f3c248_fe two, result;
    fe_zero(&two);
    two.w[0] = 2u;
    fe_sub_raw(&exponent, &exponent, &two);

    fe_one(&result);
    for (unsigned limb = F3F3C248_LIMBS; limb-- > 0u;) {
        for (unsigned bit = 32u; bit-- > 0u;) {
            fe_square(&result, &result);
            if ((exponent.w[limb] >> bit) & 1u) {
                fe_mul(&result, &result, a);
            }
        }
    }
    *r = result;
}

static void jacobian_set_infinity(f3f3c248_jacobian *p) {
    fe_zero(&p->x);
    fe_zero(&p->y);
    fe_zero(&p->z);
    p->infinity = 1;
}

static void jacobian_set_affine(
    f3f3c248_jacobian *p,
    const f3f3c248_fe *x,
    const f3f3c248_fe *y) {
    p->x = *x;
    p->y = *y;
    fe_one(&p->z);
    p->infinity = 0;
}

static void jacobian_double(
    f3f3c248_jacobian *r,
    const f3f3c248_jacobian *p) {
    if (p->infinity || fe_is_zero(&p->y)) {
        jacobian_set_infinity(r);
        return;
    }

    f3f3c248_fe xx, yy, yyyy, zz;
    f3f3c248_fe x_plus_yy, s, m, t, tmp;
    f3f3c248_jacobian out;
    fe_square(&xx, &p->x);
    fe_square(&yy, &p->y);
    fe_square(&yyyy, &yy);
    fe_square(&zz, &p->z);

    fe_add(&x_plus_yy, &p->x, &yy);
    fe_square(&s, &x_plus_yy);
    fe_sub(&s, &s, &xx);
    fe_sub(&s, &s, &yyyy);
    fe_double(&s, &s);

    /* Standard P-256 has a == -3, so
     * 3*X^2 + a*Z^4 == 3*(X + Z^2)*(X - Z^2). */
    fe_add(&m, &p->x, &zz);
    fe_sub(&tmp, &p->x, &zz);
    fe_mul(&m, &m, &tmp);
    fe_double(&tmp, &m);
    fe_add(&m, &tmp, &m);

    fe_square(&t, &m);
    fe_double(&tmp, &s);
    fe_sub(&out.x, &t, &tmp);
    fe_sub(&tmp, &s, &out.x);
    fe_mul(&tmp, &m, &tmp);
    fe_double(&yyyy, &yyyy);
    fe_double(&yyyy, &yyyy);
    fe_double(&yyyy, &yyyy);
    fe_sub(&out.y, &tmp, &yyyy);

    fe_add(&tmp, &p->y, &p->z);
    fe_square(&out.z, &tmp);
    fe_sub(&out.z, &out.z, &yy);
    fe_sub(&out.z, &out.z, &zz);
    out.infinity = 0;
    *r = out;
}

static void jacobian_add(
    f3f3c248_jacobian *r,
    const f3f3c248_jacobian *p,
    const f3f3c248_jacobian *q) {
    if (p->infinity) {
        *r = *q;
        return;
    }
    if (q->infinity) {
        *r = *p;
        return;
    }

    f3f3c248_fe z1z1, z2z2, u1, u2, s1, s2;
    f3f3c248_fe h, rr, hh, hhh, v, tmp, zprod;
    f3f3c248_jacobian out;
    fe_square(&z1z1, &p->z);
    fe_square(&z2z2, &q->z);
    fe_mul(&u1, &p->x, &z2z2);
    fe_mul(&u2, &q->x, &z1z1);
    fe_mul(&s1, &p->y, &q->z);
    fe_mul(&s1, &s1, &z2z2);
    fe_mul(&s2, &q->y, &p->z);
    fe_mul(&s2, &s2, &z1z1);
    fe_sub(&h, &u2, &u1);
    fe_sub(&rr, &s2, &s1);
    if (fe_is_zero(&h)) {
        if (fe_is_zero(&rr)) jacobian_double(r, p);
        else jacobian_set_infinity(r);
        return;
    }

    fe_square(&hh, &h);
    fe_mul(&hhh, &h, &hh);
    fe_mul(&v, &u1, &hh);
    fe_square(&out.x, &rr);
    fe_sub(&out.x, &out.x, &hhh);
    fe_double(&tmp, &v);
    fe_sub(&out.x, &out.x, &tmp);
    fe_sub(&tmp, &v, &out.x);
    fe_mul(&tmp, &rr, &tmp);
    fe_mul(&hhh, &s1, &hhh);
    fe_sub(&out.y, &tmp, &hhh);
    fe_mul(&zprod, &p->z, &q->z);
    fe_mul(&out.z, &zprod, &h);
    out.infinity = 0;
    *r = out;
}

int l3_p256_scalar_multiply_point(
    const uint8_t x35[L3_EC_FIELD_BYTES],
    const uint8_t y35[L3_EC_FIELD_BYTES],
    const uint8_t scalar35[L3_EC_FIELD_BYTES],
    uint8_t out_x35[L3_EC_FIELD_BYTES],
    uint8_t out_y35[L3_EC_FIELD_BYTES]) {
    if (!x35 || !y35 || !scalar35 || !out_x35 || !out_y35) {
        return -1;
    }

    f3f3c248_fe x, y;
    fe_import_mod35(&x, x35);
    fe_import_mod35(&y, y35);

    /* FUN_f3f3c480 first builds the exact 1P..15P four-bit table: even
     * entries double entry n/2, odd entries add P to the preceding even
     * entry.  Matching that evaluation order matters because native callers
     * do not require the input pair to be validated as a curve point. */
    f3f3c248_jacobian table[15];
    jacobian_set_affine(&table[0], &x, &y);
    for (unsigned n = 2u; n <= 15u; ++n) {
        if ((n & 1u) == 0u) {
            jacobian_double(&table[n - 1u], &table[n / 2u - 1u]);
        } else {
            jacobian_add(&table[n - 1u], &table[n - 2u], &table[0]);
        }
    }

    f3f3c248_jacobian acc;
    jacobian_set_infinity(&acc);
    int started = 0;
    for (unsigned byte = L3_EC_FIELD_BYTES; byte-- > 0u;) {
        const unsigned nibble[2] = {
            (unsigned)(scalar35[byte] >> 4),
            (unsigned)(scalar35[byte] & 0x0fu)
        };
        for (unsigned half = 0; half < 2u; ++half) {
            const unsigned digit = nibble[half];
            if (!started) {
                if (digit != 0u) {
                    acc = table[digit - 1u];
                    started = 1;
                }
                continue;
            }
            for (unsigned i = 0; i < 4u; ++i) {
                jacobian_double(&acc, &acc);
            }
            if (digit != 0u) {
                jacobian_add(&acc, &acc, &table[digit - 1u]);
            }
        }
    }

    if (acc.infinity || fe_is_zero(&acc.z)) {
        memset(out_x35, 0, L3_EC_FIELD_BYTES);
        memset(out_y35, 0, L3_EC_FIELD_BYTES);
        return 0;
    }

    f3f3c248_fe zinv, z2, z3, affine_x, affine_y;
    fe_inverse(&zinv, &acc.z);
    fe_square(&z2, &zinv);
    fe_mul(&z3, &z2, &zinv);
    fe_mul(&affine_x, &acc.x, &z2);
    fe_mul(&affine_y, &acc.y, &z3);
    fe_export35(out_x35, &affine_x);
    fe_export35(out_y35, &affine_y);
    return 0;
}

int l3_p256_public64_from_private32_libre_basepoint(
    const uint8_t private32_be[32],
    uint8_t public64_be[64]) {
    uint8_t base_x35[L3_EC_FIELD_BYTES];
    uint8_t base_y35[L3_EC_FIELD_BYTES];
    uint8_t scalar35[L3_EC_FIELD_BYTES];
    uint8_t out_x35[L3_EC_FIELD_BYTES];
    uint8_t out_y35[L3_EC_FIELD_BYTES];
    uint8_t checked_native_scalar35[L3_EC_FIELD_BYTES];

    if (!private32_be || !public64_be ||
        l3_p256_encode_private_scalar_to_native35(
            private32_be, checked_native_scalar35) != 0) {
        return -1;
    }
    for (unsigned i = 0; i < 32u; ++i) {
        base_x35[i] = l3_ephemeral_basepoint_public_xy64[31u - i];
        base_y35[i] = l3_ephemeral_basepoint_public_xy64[63u - i];
        scalar35[i] = private32_be[31u - i];
    }
    base_x35[32] = base_x35[33] = base_x35[34] = 0u;
    base_y35[32] = base_y35[33] = base_y35[34] = 0u;
    scalar35[32] = scalar35[33] = scalar35[34] = 0u;

    if (l3_p256_scalar_multiply_point(base_x35, base_y35, scalar35,
                                      out_x35, out_y35) != 0 ||
        out_x35[32] || out_x35[33] || out_x35[34] ||
        out_y35[32] || out_y35[33] || out_y35[34]) {
        memset(public64_be, 0, 64u);
        return -1;
    }
    for (unsigned i = 0; i < 32u; ++i) {
        public64_be[i] = out_x35[31u - i];
        public64_be[32u + i] = out_y35[31u - i];
    }
    return 0;
}

int l3_authorization_core_initial_lanes10(
    const uint8_t left66[0x42],
    const uint8_t right66[0x42],
    const uint8_t scalar35[L3_EC_FIELD_BYTES],
    int32_t out_first_lanes10[10],
    int32_t out_second_lanes10[10]) {
    if (!left66 || !right66 || !scalar35 ||
        !out_first_lanes10 || !out_second_lanes10) {
        return -1;
    }

    uint8_t x35[35], y35[35], out_x35[35], out_y35[35];
    memcpy(x35, left66, sizeof(x35));
    memcpy(y35, right66, sizeof(y35));
    if (l3_p256_scalar_multiply_point(
            x35, y35, scalar35, out_x35, out_y35) != 0) {
        return -1;
    }
    uint32_t first[10], second[10];
    decode_coordinate_35_to_10_lanes(out_x35, first);
    decode_coordinate_35_to_10_lanes(out_y35, second);
    for (unsigned i = 0; i < 10u; ++i) {
        out_first_lanes10[i] = (int32_t)first[i];
        out_second_lanes10[i] = (int32_t)second[i];
    }
    return 0;
}
