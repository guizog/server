//
// Created by guizo on 16/11/2025.
//
#include "common.h"

char *str_replace(const char *testInput, const char *replaceChar, const char *replaceWith){
    if (!testInput || !replaceChar || !replaceWith)
        return NULL;

    size_t lenInput = strlen(testInput);
    size_t lenReplace = strlen(replaceChar);
    size_t lenWith = strlen(replaceWith);

    if (lenReplace == 0)
        return NULL;

    size_t replaceCount = 0;
    const char *insertPoint = testInput;
    const char *temp;

    while ((temp = strstr(insertPoint, replaceChar)) != NULL) {
        replaceCount++;
        insertPoint = temp + lenReplace;
    }

    size_t newSize = lenInput + replaceCount * (lenWith - lenReplace) + 1;

    char *result = malloc(newSize);
    if (!result)
        return NULL;

    char *dest = result;
    const char *src = testInput;

    while ((insertPoint = strstr(src, replaceChar)) != NULL) {
        size_t lenFront = insertPoint - src;

        memcpy(dest, src, lenFront);
        dest += lenFront;

        memcpy(dest, replaceWith, lenWith);
        dest += lenWith;

        src = insertPoint + lenReplace;
    }

    strcpy(dest, src);

    return result;
}
