#include <stdio.h>

typedef struct Directory
{
    long address;
    char name[20];
    struct Directory **subdirectories;
    int subdirectoryCount;
} Directory;

Directory* printTree(FILE* in);
Directory* changeDir(Directory* root, char *path);
void freeDirectoryTree(Directory* root);