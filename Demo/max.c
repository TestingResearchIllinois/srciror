#include <assert.h>
#include <limits.h>
#include <stdio.h>

int max(int arr[], int size) {
    int i;
    int max = INT_MIN;
    for (i = 0; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

void testSingle() {
    int arr[1] = {-1};
    int expected = -1;
    int actual = max(arr, 1);
    assert(expected == actual);
}

void testMulti() {
    int arr[3] = {2, 3, 1};
    int expected = 3;
    int actual = max(arr, 3);
    assert(expected == actual);
}

int main() {
    testSingle();
    testMulti();
    printf("FINISHED\n");
}
