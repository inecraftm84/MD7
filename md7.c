#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

typedef unsigned long long u64;

u64 crunch(u64 h) {
    h ^= (h >> 31);
    h *= 0x7FB5D329728EA1E1ULL;
    h ^= (h >> 27);
    h *= 0xAB435E0DA5F710ULL;
    h ^= (h >> 33);
    return h;
}

u64 rotl64(u64 v, int s) {
    return (v << (s & 63)) | (v >> ((-s) & 63));
}

void md7_phantom_unicode(const char *p) {
    struct stat st_f;
    if (stat(p, &st_f) != 0) return;

    clock_t t = clock();
    u64 s_b = (u64)st_f.st_size;
    char *n = strrchr(p, '/') ? strrchr(p, '/') + 1 : (char *)p;
    int n_l = strlen(n);

    u64 st[4] = {
        0xCBF29CE484222325ULL ^ s_b,
        0x517CC1B727220A95ULL ^ n_l,
        0x13198A2E03707344ULL ^ crunch(s_b),
        0xBE5466CF34E90C6CULL ^ 0xFEEDFACECAFEBEEFULL
    };

    FILE *f = fopen(p, "rb");
    if (f) {
        unsigned char b[4096];
        size_t r; u64 total = 0;
        u64 seed = crunch(s_b ^ 0xDEADBEEF);
        
        while ((r = fread(b, 1, sizeof(b), f)) > 0) {
            for (size_t i = 0; i < r; i++) {
                st[(total + i) % 4] = crunch(st[(total + i) % 4] ^ b[i]);
                st[(total + i + 1) % 4] ^= rotl64(st[(total + i) % 4], b[i] % 61);
                
                if ((i & 0x3F) == 0) {
                    seed = crunch(seed ^ st[0]);
                    u64 fake_unicode = (seed % 0x2BAF) + 0xAC00; 
                    st[i % 4] ^= (fake_unicode << (i % 32));
                    st[0] /= ((b[i] & 0x7) + 1);
                }
            }
            total += r;
        }
        fclose(f);
    }

    u64 m_x = crunch(st[0] ^ st[3]);
    int rounds = 2048 + (n_l * 32); 
    for (int i = 0; i < rounds; i++) {
        int a = i % 4, b = (i + 1) % 4, c = (i + 2) % 4, d = (i + 3) % 4;
        
        u64 dyn_uni = (crunch(m_x ^ i) % 0x07FF) + 0x0800;
        st[a] ^= dyn_uni;

        st[a] = rotl64(st[a], (int)((st[b] ^ m_x) % 63) + 1);
        st[a] ^= crunch(st[c] + i);
        
        u64 d_p = ((st[d] ^ dyn_uni) & 0x1F) + 1;
        st[b] = (st[b] / d_p) ^ crunch(st[a]);
        
        st[c] = st[c] - (m_x ^ st[d]);
        m_x = rotl64(m_x ^ st[a], 19) ^ dyn_uni;
    }

    double d_e = (double)(clock() - t) * 1000.0 / CLOCKS_PER_SEC;
    printf("%016llX%016llX%016llX%016llX\n%.4f ms\n", 
           crunch(st[0]), crunch(st[1]), crunch(st[2]), crunch(st[3]), d_e);
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    md7_phantom_unicode(argv[1]);
    return 0;
}