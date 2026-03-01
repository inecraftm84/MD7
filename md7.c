#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

typedef unsigned long long u64;
#define M_PI  0x243F6A8885A308D3ULL
#define M_G   0x9E3779B97F4A7C15ULL
#define M_SQ2 0x6A09E667F3BCC908ULL

static u64 crunch_x(u64 h) {
    h ^= (h >> 31); h *= 0x7FB5D329728EA1E1ULL;
    h ^= (h >> 27); h *= 0xAB435E0DA5F710ULL;
    h ^= (h >> 33); return h;
}

static u64 rotl_x(u64 v, int s) { return (v << (s & 63)) | (v >> ((-s) & 63)); }

void process_file(const char *fn, u64 st[4]) {
    FILE *f = fopen(fn, "rb");
    if (!f) return;
    static u64 buf[8192];
    size_t r;
    while ((r = fread(buf, 8, 8192, f)) > 0) {
        for (size_t i = 0; i < r; i++) {
            u64 v = buf[i];
            st[0] = (st[0] + v) * M_G;
            st[1] = rotl_x(st[1] ^ st[0], 17) ^ v;
            st[2] = (st[2] * st[2]) + v + M_PI;
            st[3] = rotl_x(st[3] ^ st[2], (int)(v & 63));
        }
    }
    fclose(f);
}

void scan_dir(const char *dn, u64 st[4]) {
    DIR *d = opendir(dn);
    if (!d) { process_file(dn, st); return; }
    struct dirent *e;
    char path[1024];
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s", dn, e->d_name);
        struct stat s;
        if (stat(path, &s) == 0) {
            if (S_ISDIR(s.st_mode)) scan_dir(path, st);
            else process_file(path, st);
        }
    }
    closedir(d);
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    clock_t t = clock();
    u64 st[4] = { M_PI, M_G, M_SQ2, 0xFEEDFACECAFEBEEFULL };
    
    scan_dir(argv[1], st);

    for (int i = 0; i < 512; i++) {
        st[0] += st[1]; st[2] += st[3];
        st[1] = rotl_x(st[1], 13) ^ st[2];
        st[3] = rotl_x(st[3], 45) ^ st[0];
        st[i & 3] ^= crunch_x(st[(i+1) & 3] + i);
    }

    printf("%016llX%016llX%016llX%016llX\n%.4f ms\n", 
           crunch_x(st[0]), crunch_x(st[1]), crunch_x(st[2]), crunch_x(st[3]), 
           (double)(clock()-t)*1000.0/CLOCKS_PER_SEC);
    return 0;
}