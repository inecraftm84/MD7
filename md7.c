#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <openssl/sha.h>

typedef struct {
    unsigned long state;
} MyRng;

void my_seed(MyRng *r, unsigned long seed) { r->state = seed; }
unsigned long my_rand(MyRng *r) {
    r->state = (r->state * 1103515245 + 12345) & 0x7FFFFFFF;
    return r->state;
}

unsigned int rotl(unsigned int value, int shift) {
    return (value << shift) | (value >> (32 - shift));
}

void md7_c(const char *path) {
    struct stat st_file;
    if (stat(path, &st_file) != 0) {
        printf("Error: Cannot access '%s'\n", path);
        return;
    }

    clock_t start = clock();
    long s_b = st_file.st_size;
    char *n = strrchr(path, '/') ? strrchr(path, '/') + 1 : (char *)path;
    int n_l = strlen(n);

    unsigned char h_head[1024] = {0}, h_tail[1024] = {0};
    FILE *f = fopen(path, "rb");
    if (!f) return;
    size_t head_sz = fread(h_head, 1, 1024, f);
    size_t tail_sz = 0;
    if (s_b > 1024) {
        fseek(f, -((s_b - 1024 < 1024) ? s_b - 1024 : 1024), SEEK_END);
        tail_sz = fread(h_tail, 1, 1024, f);
    }
    fclose(f);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, h_head, head_sz);
    SHA256_Update(&sha256, h_tail, tail_sz);
    SHA256_Update(&sha256, n, n_l);
    SHA256_Final(hash, &sha256);

    unsigned int st = (hash[0] << 24) | (hash[1] << 16) | (hash[2] << 8) | hash[3];
    MyRng rng;
    my_seed(&rng, st ^ (unsigned int)s_b);

    unsigned int bp[5] = {(s_b>>16)&0xFFFF, (s_b>>8)&0xFF, (s_b>>4)&0x0F, (s_b>>2)&0x03, s_b&0x01};
    
    for (int i = 0; i < 32 * n_l; i++) {
        for (int j = 4; j > 0; j--) {
            int r_idx = my_rand(&rng) % (j + 1);
            unsigned int tmp = bp[j]; bp[j] = bp[r_idx]; bp[r_idx] = tmp;
        }
        for (int j = 0; j < 5; j++) {
            int op = my_rand(&rng) % 4;
            unsigned int v = bp[j];
            if (op == 0) st += v;
            else if (op == 1) st -= v;
            else if (op == 2) st *= (v % 13 + 1);
            else if (op == 3) st ^= v;
            st &= 0xFFFFFFFF;
            st = rotl(st, (my_rand(&rng) % 31) + 1);
        }
    }

    char final_str[32];
    sprintf(final_str, "%u", st);
    SHA256((unsigned char*)final_str, strlen(final_str), hash);
    
    for(int i = 0; i < 8; i++) printf("%02X", hash[i]);
    printf("\n%.4f ms\n", (double)(clock() - start) * 1000.0 / CLOCKS_PER_SEC);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("MD7 Tool - A MD7!\n");
        printf("Usage: md7sum <filename>\n");
        return 1;
    }
    md7_c(argv[1]);
    return 0;
}

