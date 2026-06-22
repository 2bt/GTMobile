#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sha256 {
namespace {

inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t sig0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline uint32_t sig1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

inline uint32_t cs0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline uint32_t cs1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline uint32_t read_be32(uint8_t const* p) {
    return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]);
}

inline void write_be32(uint8_t* p, uint32_t v) {
    p[0] = uint8_t(v >> 24);
    p[1] = uint8_t(v >> 16);
    p[2] = uint8_t(v >> 8);
    p[3] = uint8_t(v);
}

inline uint32_t const* k_table() {
    static uint32_t const K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    };
    return K;
}

inline void sha256_blocks(uint8_t const* data, size_t blocks, uint32_t state[8]) {
    uint32_t const* K = k_table();
    for (size_t b = 0; b < blocks; ++b) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = read_be32(data + b * 64 + i * 4);
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = sig1(w[i - 2]) + w[i - 7] + sig0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = state[0];
        uint32_t b0 = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];
        uint32_t f = state[5];
        uint32_t g = state[6];
        uint32_t h = state[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = h + cs1(e) + ch(e, f, g) + K[i] + w[i];
            uint32_t t2 = cs0(a) + maj(a, b0, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b0;
            b0 = a;
            a = t1 + t2;
        }

        state[0] += a;
        state[1] += b0;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }
}

inline int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // namespace

inline void sha256(uint8_t const* data, size_t len, uint8_t digest[32]) {
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };

    size_t full_blocks = len / 64;
    sha256_blocks(data, full_blocks, state);

    uint8_t block[64] = {};
    size_t rem = len % 64;
    if (rem > 0) {
        memcpy(block, data + full_blocks * 64, rem);
    }
    block[rem] = 0x80;

    if (rem >= 56) {
        sha256_blocks(block, 1, state);
        memset(block, 0, 64);
    }

    uint64_t bit_len = uint64_t(len) * 8;
    for (int i = 0; i < 8; ++i) {
        block[63 - i] = uint8_t(bit_len >> (i * 8));
    }
    sha256_blocks(block, 1, state);

    for (int i = 0; i < 8; ++i) {
        write_be32(digest + i * 4, state[i]);
    }
}

inline bool sha256_matches_hex(uint8_t const* data, size_t len, char const* hex) {
    uint8_t digest[32];
    sha256(data, len, digest);
    for (int i = 0; i < 32; ++i) {
        int hi = hex_value(hex[i * 2]);
        int lo = hex_value(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        if (digest[i] != uint8_t((hi << 4) | lo)) return false;
    }
    return hex[64] == '\0';
}

} // namespace sha256
