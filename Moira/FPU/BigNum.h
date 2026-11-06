// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "../MoiraTypes.h"
#include <algorithm>
#include <bit>
#include <vector>

namespace MOIRA_NAMESPACE {

/* A minimal arbitrary-precision unsigned integer. It is only used for the
 * conversion from and to packed decimal numbers, which have to be exact. The
 * numbers involved are up to a few thousand bits wide, so the simple
 * algorithms used here are fast enough.
 */
struct Big {

    // Limbs in little-endian order without leading zeros
    std::vector<u32> v;

    Big() { }
    explicit Big(u64 x) { if (x) v.push_back((u32)x); if (x >> 32) v.push_back((u32)(x >> 32)); }

    bool isZero() const { return v.empty(); }

    int bitLength() const
    {
        return v.empty() ? 0 : int(v.size() - 1) * 32 + (32 - std::countl_zero(v.back()));
    }

    bool bit(int i) const
    {
        return i >= 0 && size_t(i / 32) < v.size() && ((v[i / 32] >> (i % 32)) & 1);
    }

    void trim() { while (!v.empty() && v.back() == 0) v.pop_back(); }

    void mulSmall(u32 m)
    {
        u64 carry = 0;
        for (auto &limb : v) { u64 t = u64(limb) * m + carry; limb = (u32)t; carry = t >> 32; }
        if (carry) v.push_back((u32)carry);
        trim();
    }

    // Divides by a small number and returns the remainder
    u32 divSmall(u32 d)
    {
        u64 rem = 0;
        for (size_t i = v.size(); i-- > 0; ) { u64 cur = (rem << 32) | v[i]; v[i] = (u32)(cur / d); rem = cur % d; }
        trim();
        return (u32)rem;
    }

    // Adds a number
    void add(const Big &o)
    {
        u64 carry = 0;
        for (size_t i = 0; i < std::max(v.size(), o.v.size()) || carry; i++) {
            if (i == v.size()) v.push_back(0);
            u64 t = u64(v[i]) + (i < o.v.size() ? o.v[i] : 0) + carry;
            v[i] = (u32)t; carry = t >> 32;
        }
    }

    void addSmall(u32 a)
    {
        u64 carry = a;
        for (size_t i = 0; carry && i < v.size(); i++) { u64 t = u64(v[i]) + carry; v[i] = (u32)t; carry = t >> 32; }
        if (carry) v.push_back((u32)carry);
    }

    void shl(int n)
    {
        if (isZero() || n == 0) return;
        int limbs = n / 32, bits = n % 32;
        if (bits) {
            u32 carry = 0;
            for (auto &limb : v) { u32 next = limb >> (32 - bits); limb = (limb << bits) | carry; carry = next; }
            if (carry) v.push_back(carry);
        }
        v.insert(v.begin(), limbs, 0);
    }

    // Shifts left by one bit and appends 'in' as the new least significant bit
    void shl1(bool in)
    {
        u32 carry = in;
        for (auto &limb : v) { u32 next = limb >> 31; limb = (limb << 1) | carry; carry = next; }
        if (carry) v.push_back(carry);
    }

    int cmp(const Big &o) const
    {
        if (v.size() != o.v.size()) return v.size() < o.v.size() ? -1 : 1;
        for (size_t i = v.size(); i-- > 0; ) if (v[i] != o.v[i]) return v[i] < o.v[i] ? -1 : 1;
        return 0;
    }

    // Subtracts o (which must not be greater than this)
    void sub(const Big &o)
    {
        i64 borrow = 0;
        for (size_t i = 0; i < v.size(); i++) {
            i64 t = i64(v[i]) - borrow - (i < o.v.size() ? i64(o.v[i]) : 0);
            borrow = t < 0; v[i] = (u32)(t + (borrow ? (1LL << 32) : 0));
        }
        trim();
    }

    Big mul(const Big &o) const
    {
        Big r;
        if (isZero() || o.isZero()) return r;
        r.v.assign(v.size() + o.v.size(), 0);
        for (size_t i = 0; i < v.size(); i++) {
            u64 carry = 0;
            for (size_t j = 0; j < o.v.size(); j++) {
                u64 t = u64(v[i]) * o.v[j] + r.v[i + j] + carry;
                r.v[i + j] = (u32)t; carry = t >> 32;
            }
            r.v[i + o.v.size()] += (u32)carry;
        }
        r.trim();
        return r;
    }

    // Checks whether any bit below position n is set
    bool anyBitBelow(int n) const
    {
        for (int i = 0; i < n / 32 && size_t(i) < v.size(); i++) if (v[i]) return true;
        if (size_t(n / 32) < v.size() && (n % 32) && (v[n / 32] & ((1u << (n % 32)) - 1))) return true;
        return false;
    }

    static Big pow10(int n)
    {
        Big result(1);
        for (; n >= 9; n -= 9) result.mulSmall(1000000000);
        for (; n > 0; n--) result.mulSmall(10);
        return result;
    }

    // Computes quotient and remainder of this / d
    Big divmod(const Big &d, Big &rem) const
    {
        Big q; rem = Big();
        q.v.assign(v.size(), 0);
        for (int i = bitLength() - 1; i >= 0; i--) {
            rem.shl1(bit(i));
            if (rem.cmp(d) >= 0) { rem.sub(d); q.v[i / 32] |= 1u << (i % 32); }
        }
        q.trim();
        return q;
    }

    // Returns the decimal digits (most significant first, at least one)
    std::vector<int> digits() const
    {
        std::vector<int> r;
        Big t = *this;
        while (!t.isZero()) {
            u64 rem = 0;
            for (size_t i = t.v.size(); i-- > 0; ) { u64 cur = (rem << 32) | t.v[i]; t.v[i] = (u32)(cur / 10); rem = cur % 10; }
            t.trim(); r.push_back((int)rem);
        }
        if (r.empty()) r.push_back(0);
        std::reverse(r.begin(), r.end());
        return r;
    }
};

}
