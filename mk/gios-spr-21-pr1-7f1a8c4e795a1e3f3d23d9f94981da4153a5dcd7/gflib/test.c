#include "content.h"
#include <stdio.h>

int main() {
    int test = content_get("courses/ud923/filecorpus/road.jpg");
    printf("%d", test);
}