#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main()
{
    clock_t now = clock();
    for (int i = 0; i < 100; ++i) {
        int ticks = clock();
        printf("%d\n", ticks);
        sleep(1);
    }
}
