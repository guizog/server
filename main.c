#include "server.h"

extern char WORKING_DIR[1024];
extern char CONTENT_DIR[1024];

int main(void) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        char cwd_copy[1024];
        strcpy(cwd_copy, cwd);

        char *dir = dirname(cwd_copy);
        strcpy(WORKING_DIR, dir);
        snprintf(CONTENT_DIR, sizeof(CONTENT_DIR), "%s\\content", WORKING_DIR);

        printf("Root path: %s\nContent path: %s\n", WORKING_DIR, CONTENT_DIR);
    }

    startServer();

    return 0;
}