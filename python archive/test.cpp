#include <iostream>
#include <ctime>

int main() {
    // First loop
    clock_t start = clock();

    char a = 'a';
    for (int i = 0; i < 1000000; ++i) {
        for (int j = 0; j < 1000; ++j) {
            if (a == 'a') {
                // pass
            }
        }
    }

    clock_t end = clock();
    std::cout << "Time for first loop: " << double(end - start) / CLOCKS_PER_SEC << " seconds" << std::endl;

    // Second loop
    start = clock();

    bool b = true;
    for (int i = 0; i < 1000000; ++i) {
        for (int j = 0; j < 1000; ++j) {
            if (b == true) {
                // pass
            }
        }
    }

    end = clock();
    std::cout << "Time for second loop: " << double(end - start) / CLOCKS_PER_SEC << " seconds" << std::endl;

    return 0;
}
