#include "../packed16/packed-string.h"
#include "ps-robinhood.h"
#include "cs-robinhood.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 1000000
#define STR_MAX 20

static const char ALPHABET[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$";

static double now_seconds(void) {
    return (double)clock() / CLOCKS_PER_SEC;
}

static void random_string(char* out, const int len) {
    for (int i = 0; i < len; ++i)
        out[i] = ALPHABET[rand() & 63];
    out[len] = '\0';
}

int main(void) {
    srand(1234);

    char** strings = malloc(N * sizeof(char*));
    char** missing = malloc(N * sizeof(char*));

    for (int i = 0; i < N; ++i) {
        strings[i] = malloc(STR_MAX + 1);
        missing[i] = malloc(STR_MAX + 1);

        random_string(strings[i], rand() % STR_MAX + 1);
        random_string(missing[i], rand() % STR_MAX + 1);
    }

    printf("N = %d\n\n", N);

    // =========================
    // C STRING
    // =========================

    csrh_map ct;
    csrh_init(&ct, N * 2);

    double t0, t1, t2, t3, t4;

    volatile uint64_t sink = 0;

    t0 = now_seconds();
    for (int i = 0; i < N; ++i)
        csrh_set(&ct, strings[i], i);
    t1 = now_seconds();

    for (int i = 0; i < N; ++i)
        csrh_get(&ct, strings[i], (uint64_t*)&sink);
    t2 = now_seconds();

    for (int i = 0; i < N; ++i)
        csrh_contains(&ct, missing[i]);
    t3 = now_seconds();

    for (int i = 0; i < N; ++i)
        csrh_delete(&ct, strings[i]);
    t4 = now_seconds();

    printf("C String:\n");
    printf("  Insert:  %.3f s\n", t1 - t0);
    printf("  Lookup:  %.3f s\n", t2 - t1);
    printf("  Missing: %.3f s\n", t3 - t2);
    printf("  Delete:  %.3f s\n\n", t4 - t3);

    // =========================
    // PACKED STRING
    // =========================

    ps_t* pss = malloc(N * sizeof(ps_t));
    ps_t* pss_missing = malloc(N * sizeof(ps_t));

    for (int i = 0; i < N; ++i) {
        pss[i] = ps_pack(strings[i]);
        pss_missing[i] = ps_pack(missing[i]);
    }

    psrh_map ps_table;
    psrh_init(&ps_table, N * 2);

    t0 = now_seconds();
    for (int i = 0; i < N; ++i)
        psrh_set(&ps_table, pss[i], i);
    t1 = now_seconds();

    for (int i = 0; i < N; ++i)
        psrh_get(&ps_table, pss[i], (uint64_t*)&sink);
    t2 = now_seconds();

    for (int i = 0; i < N; ++i)
        psrh_contains(&ps_table, pss_missing[i]);
    t3 = now_seconds();

    for (int i = 0; i < N; ++i)
        psrh_delete(&ps_table, pss[i]);
    t4 = now_seconds();

    printf("PackedString:\n");
    printf("  Insert:  %.3f s\n", t1 - t0);
    printf("  Lookup:  %.3f s\n", t2 - t1);
    printf("  Missing: %.3f s\n", t3 - t2);
    printf("  Delete:  %.3f s\n", t4 - t3);

    free(strings);
    free(missing);
    free(pss);
    free(pss_missing);

    return 0;
}

/*
N = 1000000

C String:
  Insert:  0.139 s
  Lookup:  0.167 s
  Missing: 0.190 s
  Delete:  0.141 s

PackedString:
  Insert:  0.041 s
  Lookup:  0.031 s
  Missing: 0.052 s
  Delete:  0.042 s
*/