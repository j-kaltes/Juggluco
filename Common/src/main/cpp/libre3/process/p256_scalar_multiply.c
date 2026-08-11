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
/*      Tue Aug 11 16:33:40 CEST 2026                                                */
#include "p256_scalar_multiply.h"

#include "p256_coordinate_extract.h"
#include "p256_coordinate_codec.h"

#include <stddef.h>
#include <string.h>

enum { F3F3C248_LIMBS = 9 };

typedef struct {
    uint32_t w[F3F3C248_LIMBS];
} f3f3c248_fe;

typedef struct {
    f3f3c248_fe p;
    f3f3c248_fe a;
} f3f3c248_field;

typedef struct {
    f3f3c248_fe x;
    f3f3c248_fe y;
    f3f3c248_fe z;
    int infinity;
} f3f3c248_jacobian;

static void fe_zero(f3f3c248_fe *r) {
    memset(r, 0, sizeof(*r));
}

static void fe_one(f3f3c248_fe *r) {
    fe_zero(r);
    r->w[0] = 1u;
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
    const f3f3c248_field *f,
    f3f3c248_fe *r,
    const f3f3c248_fe *a,
    const f3f3c248_fe *b) {
    f3f3c248_fe t;
    uint32_t carry = fe_add_raw(&t, a, b);
    /* Native values are at most 0x118 bits, so a+b fits in nine limbs. */
    if (carry || fe_cmp(&t, &f->p) >= 0) fe_sub_raw(&t, &t, &f->p);
    *r = t;
}

static void fe_sub(
    const f3f3c248_field *f,
    f3f3c248_fe *r,
    const f3f3c248_fe *a,
    const f3f3c248_fe *b) {
    f3f3c248_fe t;
    if (fe_cmp(a, b) >= 0) {
        fe_sub_raw(&t, a, b);
    } else {
        f3f3c248_fe d;
        fe_sub_raw(&d, b, a);
        fe_sub_raw(&t, &f->p, &d);
    }
    *r = t;
}

static void fe_double(
    const f3f3c248_field *f,
    f3f3c248_fe *r,
    const f3f3c248_fe *a) {
    fe_add(f, r, a, a);
}

static void fe_mul(
    const f3f3c248_field *f,
    f3f3c248_fe *r,
    const f3f3c248_fe *a,
    const f3f3c248_fe *b) {
    f3f3c248_fe acc, cur;
    fe_zero(&acc);
    cur = *a;
    for (unsigned limb = 0; limb < F3F3C248_LIMBS; ++limb) {
        uint32_t bits = b->w[limb];
        for (unsigned bit = 0; bit < 32u; ++bit) {
            if (bits & 1u) fe_add(f, &acc, &acc, &cur);
            bits >>= 1;
            fe_double(f, &cur, &cur);
        }
    }
    *r = acc;
}

static void fe_square(
    const f3f3c248_field *f,
    f3f3c248_fe *r,
    const f3f3c248_fe *a) {
    fe_mul(f, r, a, a);
}

static void fe_mul_small(
    const f3f3c248_field *f,
    f3f3c248_fe *r,
    const f3f3c248_fe *a,
    unsigned n) {
    f3f3c248_fe acc;
    fe_zero(&acc);
    for (unsigned i = 0; i < n; ++i) fe_add(f, &acc, &acc, a);
    *r = acc;
}

static void fe_import_raw35(f3f3c248_fe *r, const uint8_t in[35]) {
    fe_zero(r);
    for (unsigned i = 0; i < 35u; ++i) {
        r->w[i >> 2] |= (uint32_t)in[i] << ((i & 3u) * 8u);
    }
}

static void fe_import_mod35(
    const f3f3c248_field *f,
    f3f3c248_fe *r,
    const uint8_t in[35]) {
    f3f3c248_fe acc, one;
    fe_zero(&acc);
    fe_one(&one);
    for (unsigned byte = 35u; byte-- > 0u;) {
        for (unsigned bit = 8u; bit-- > 0u;) {
            fe_double(f, &acc, &acc);
            if ((in[byte] >> bit) & 1u) fe_add(f, &acc, &acc, &one);
        }
    }
    *r = acc;
}

static void fe_export35(uint8_t out[35], const f3f3c248_fe *a) {
    for (unsigned i = 0; i < 35u; ++i) {
        out[i] = (uint8_t)(a->w[i >> 2] >> ((i & 3u) * 8u));
    }
}

static void fe_inverse(
    const f3f3c248_field *f,
    f3f3c248_fe *r,
    const f3f3c248_fe *a) {
    f3f3c248_fe exponent = f->p;
    f3f3c248_fe two, result;
    fe_zero(&two);
    two.w[0] = 2u;
    fe_sub_raw(&exponent, &exponent, &two);
    fe_one(&result);
    for (unsigned limb = F3F3C248_LIMBS; limb-- > 0u;) {
        for (unsigned bit = 32u; bit-- > 0u;) {
            fe_square(f, &result, &result);
            if ((exponent.w[limb] >> bit) & 1u) fe_mul(f, &result, &result, a);
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
    const f3f3c248_field *f,
    f3f3c248_jacobian *r,
    const f3f3c248_jacobian *p) {
    if (p->infinity || fe_is_zero(&p->y)) {
        jacobian_set_infinity(r);
        return;
    }

    f3f3c248_fe xx, yy, yyyy, zz, zz2;
    f3f3c248_fe x_plus_yy, s, m, t, tmp, ax;
    f3f3c248_jacobian out;
    fe_square(f, &xx, &p->x);
    fe_square(f, &yy, &p->y);
    fe_square(f, &yyyy, &yy);
    fe_square(f, &zz, &p->z);

    fe_add(f, &x_plus_yy, &p->x, &yy);
    fe_square(f, &s, &x_plus_yy);
    fe_sub(f, &s, &s, &xx);
    fe_sub(f, &s, &s, &yyyy);
    fe_double(f, &s, &s);

    fe_mul_small(f, &m, &xx, 3u);
    if (!fe_is_zero(&f->a)) {
        fe_square(f, &zz2, &zz);
        fe_mul(f, &ax, &f->a, &zz2);
        fe_add(f, &m, &m, &ax);
    }

    fe_square(f, &t, &m);
    fe_double(f, &tmp, &s);
    fe_sub(f, &out.x, &t, &tmp);
    fe_sub(f, &tmp, &s, &out.x);
    fe_mul(f, &tmp, &m, &tmp);
    fe_mul_small(f, &yyyy, &yyyy, 8u);
    fe_sub(f, &out.y, &tmp, &yyyy);

    fe_add(f, &tmp, &p->y, &p->z);
    fe_square(f, &out.z, &tmp);
    fe_sub(f, &out.z, &out.z, &yy);
    fe_sub(f, &out.z, &out.z, &zz);
    out.infinity = 0;
    *r = out;
}

static void jacobian_add(
    const f3f3c248_field *f,
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
    fe_square(f, &z1z1, &p->z);
    fe_square(f, &z2z2, &q->z);
    fe_mul(f, &u1, &p->x, &z2z2);
    fe_mul(f, &u2, &q->x, &z1z1);
    fe_mul(f, &s1, &p->y, &q->z);
    fe_mul(f, &s1, &s1, &z2z2);
    fe_mul(f, &s2, &q->y, &p->z);
    fe_mul(f, &s2, &s2, &z1z1);
    fe_sub(f, &h, &u2, &u1);
    fe_sub(f, &rr, &s2, &s1);
    if (fe_is_zero(&h)) {
        if (fe_is_zero(&rr)) jacobian_double(f, r, p);
        else jacobian_set_infinity(r);
        return;
    }

    fe_square(f, &hh, &h);
    fe_mul(f, &hhh, &h, &hh);
    fe_mul(f, &v, &u1, &hh);
    fe_square(f, &out.x, &rr);
    fe_sub(f, &out.x, &out.x, &hhh);
    fe_double(f, &tmp, &v);
    fe_sub(f, &out.x, &out.x, &tmp);
    fe_sub(f, &tmp, &v, &out.x);
    fe_mul(f, &tmp, &rr, &tmp);
    fe_mul(f, &hhh, &s1, &hhh);
    fe_sub(f, &out.y, &tmp, &hhh);
    fe_mul(f, &zprod, &p->z, &q->z);
    fe_mul(f, &out.z, &zprod, &h);
    out.infinity = 0;
    *r = out;
}

static int field_init(
    f3f3c248_field *f,
    const uint8_t modulus35[35],
    const uint8_t curve_a35[35],
    int use_curve_a) {
    fe_import_raw35(&f->p, modulus35);
    f3f3c248_fe three;
    fe_zero(&three);
    three.w[0] = 3u;
    if ((f->p.w[0] & 1u) == 0u || fe_cmp(&f->p, &three) <= 0 ||
        (f->p.w[8] & 0xff000000u) != 0u) {
        return -1;
    }
    if (use_curve_a) {
        fe_import_mod35(f, &f->a, curve_a35);
    } else {
        /* FUN_f3f3cf70's null-a branch is the standard a == -3 Jacobian
         * shortcut M = 3 * (X + Z^2) * (X - Z^2), not an a == 0 curve. */
        f3f3c248_fe three;
        fe_zero(&three);
        three.w[0] = 3u;
        fe_sub_raw(&f->a, &f->p, &three);
    }
    return 0;
}

int l3_p256_scalar_multiply_point(
    const uint8_t modulus35[L3_EC_FIELD_BYTES],
    const uint8_t curve_a35[L3_EC_FIELD_BYTES],
    int use_curve_a,
    const uint8_t x35[L3_EC_FIELD_BYTES],
    const uint8_t y35[L3_EC_FIELD_BYTES],
    const uint8_t scalar35[L3_EC_FIELD_BYTES],
    uint8_t out_x35[L3_EC_FIELD_BYTES],
    uint8_t out_y35[L3_EC_FIELD_BYTES]) {
    if (!modulus35 || (use_curve_a && !curve_a35) || !x35 || !y35 ||
        !scalar35 || !out_x35 || !out_y35) {
        return -1;
    }

    f3f3c248_field field;
    if (field_init(&field, modulus35, curve_a35, use_curve_a) != 0) return -1;
    f3f3c248_fe x, y;
    fe_import_mod35(&field, &x, x35);
    fe_import_mod35(&field, &y, y35);

    /* FUN_f3f3c480 first builds the exact 1P..15P four-bit table: even
     * entries double entry n/2, odd entries add P to the preceding even
     * entry.  Matching that evaluation order matters because native callers
     * do not require the input pair to be validated as a curve point. */
    f3f3c248_jacobian table[15];
    jacobian_set_affine(&table[0], &x, &y);
    for (unsigned n = 2u; n <= 15u; ++n) {
        if ((n & 1u) == 0u) {
            jacobian_double(&field, &table[n - 1u], &table[n / 2u - 1u]);
        } else {
            jacobian_add(&field, &table[n - 1u], &table[n - 2u], &table[0]);
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
                jacobian_double(&field, &acc, &acc);
            }
            if (digit != 0u) {
                jacobian_add(&field, &acc, &acc, &table[digit - 1u]);
            }
        }
    }

    if (acc.infinity || fe_is_zero(&acc.z)) {
        memset(out_x35, 0, L3_EC_FIELD_BYTES);
        memset(out_y35, 0, L3_EC_FIELD_BYTES);
        return 0;
    }

    f3f3c248_fe zinv, z2, z3, affine_x, affine_y;
    fe_inverse(&field, &zinv, &acc.z);
    fe_square(&field, &z2, &zinv);
    fe_mul(&field, &z3, &z2, &zinv);
    fe_mul(&field, &affine_x, &acc.x, &z2);
    fe_mul(&field, &affine_y, &acc.y, &z3);
    fe_export35(out_x35, &affine_x);
    fe_export35(out_y35, &affine_y);
    return 0;
}

int l3_authorization_core_initial_lanes10(
    const uint8_t *full_parent_ctx,
    const uint8_t left66[0x42],
    const uint8_t right66[0x42],
    const uint8_t scalar35[L3_EC_FIELD_BYTES],
    int32_t out_first_lanes10[10],
    int32_t out_second_lanes10[10]) {
    if (!full_parent_ctx || !left66 || !right66 || !scalar35 ||
        !out_first_lanes10 || !out_second_lanes10) {
        return -1;
    }

    uint8_t x35[35], y35[35], out_x35[35], out_y35[35];
    l3_authorization_core_copy_inputs35(left66, right66, x35, y35);
    const int use_a = full_parent_ctx[0x1d96u] != 0u;
    if (l3_p256_scalar_multiply_point(
            full_parent_ctx + 0x1d50u,
            full_parent_ctx + 0x1d73u,
            use_a,
            x35, y35, scalar35, out_x35, out_y35) != 0) {
        return -1;
    }
    uint32_t first[10], second[10];
    l3_p256_decode_coordinate_35_to_10_lanes(out_x35, first);
    l3_p256_decode_coordinate_35_to_10_lanes(out_y35, second);
    for (unsigned i = 0; i < 10u; ++i) {
        out_first_lanes10[i] = (int32_t)first[i];
        out_second_lanes10[i] = (int32_t)second[i];
    }
    return 0;
}
