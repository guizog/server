//
// Created by guizo on 16/11/2025.
//
#include "common.h"

char *str_replace(char *testInput, char *replaceChar, char *replaceWith) {
    char* temp;
    char* insertPoint;
    char* result;
    int replaceCount;
    int lenReplace = strlen(replaceChar);
    int lenWith = strlen(replaceWith);
    int lenFront;

    insertPoint = testInput;
    for (replaceCount = 0; (temp = strstr(insertPoint, "/")); replaceCount++) {
        printf("replace count: %d\n", replaceCount);
        insertPoint = temp + lenReplace;
    }

    int diffCharSize = lenWith - lenReplace;
    temp = result = malloc(strlen(testInput) + 1 + diffCharSize * replaceCount);

    while (replaceCount--) {
        insertPoint = strstr(testInput, replaceChar);
        lenFront = insertPoint - testInput;
        temp = strncpy(temp, testInput, lenFront) + lenFront;
        temp = strcpy(temp, replaceWith) + lenWith;
        testInput += lenFront + lenReplace;
    }
    strcpy(temp, testInput);

    for (int i = 0; result[i] != '\0'; i++) {
        printf("char[%d] = '%c' (ASCII %d)\n", i, result[i], result[i]);
    }

    return result;
}
