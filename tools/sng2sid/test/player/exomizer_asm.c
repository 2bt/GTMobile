#include "membuf.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    struct membuf src;
    struct membuf dest;
    membuf_init(&src);
    membuf_init(&dest);

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, stdin)) > 0) {
        membuf_append(&src, buf, n);
    }

    if (assemble(&src, &dest) != 0) {
        fprintf(stderr, "assemble failed\n");
        return 1;
    }
    fwrite(membuf_get(&dest), 1, membuf_memlen(&dest), stdout);
    membuf_free(&src);
    membuf_free(&dest);
    return 0;
}
