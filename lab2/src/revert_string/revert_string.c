#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
	if (str == NULL) {
        return;
    }

    size_t len = strlen(str);
    if (len < 2) {
        return;
    }

    size_t left = 0;
    size_t right = len - 1;

    while (left < right) {
        char tmp = str[left];
        str[left] = str[right];
        str[right] = tmp;

        left++;
        right--;
    }
}

