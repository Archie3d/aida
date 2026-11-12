#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
    if (argc != 2 || strcmp(argv[1], "interrupt") != 0) { return 1; }
    const char* mode = getenv("AIDA_TEST_PRIORITY_PROBE");
    if (mode == NULL) { return 1; }
    printf("maximum native task priority unavailable: controlled fixture\n");
    if (strcmp(mode, "unavailable") == 0) { return 77; }
    if (strcmp(mode, "noisy") == 0) { return 0; }
    return 1;
}
