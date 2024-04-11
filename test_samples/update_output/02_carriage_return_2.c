#include <stdio.h>
#include <unistd.h>

int main() {
    int i;
    for (i = 0; i <= 100; i++) {
        printf("Progress: %d%%\r", i);
        fflush(stdout);
        usleep(100000); // Sleep for 100ms (0.1 second)
    }
    printf("\n");
    return 0;
}

