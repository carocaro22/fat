#include "../fat.h"
#include "directoryTree.h"
#include <stdlib.h>
#include <string.h>

Directory* createDirectoryAlloc(long address, char name[20])
{
    Directory *directory = malloc(sizeof(Directory));
    directory->subdirectoryCount = 0;
    strncpy(directory->name, name, 20);
    directory->address = address;
    directory->subdirectories = NULL;
    return directory;
}

Directory* addSubdirectoryAlloc(Directory *parent, char name[20], long address)
{
    Directory* child = createDirectoryAlloc(address, name);
    Directory parent_copy = *parent;
    parent->subdirectoryCount++;
    // start of "append"
    Directory** new_subdirectories = malloc(parent->subdirectoryCount * sizeof(Directory*));
    for ( int i = 0; i < parent->subdirectoryCount - 1; i++) {
        new_subdirectories[i] = parent->subdirectories[i];
    }
    new_subdirectories[parent->subdirectoryCount - 1] = child;
    free(parent->subdirectories);
    parent->subdirectories = new_subdirectories;
    // end of "append"
    return child;
}

void breadthFirstTransversal(Directory* directory, int depth, int currentDepth) { 
    if (directory == NULL) {
        return;
    }
    if (depth == currentDepth) {
        for (int i = 0; i < currentDepth; i++) {
            printf("----");
        }
        printf("%s\n", directory->name);
    }
    for (int i = 0; i < directory->subdirectoryCount; i++) {
        breadthFirstTransversal(directory->subdirectories[i], depth, currentDepth + 1);
    }
}

void makeTree(FILE *in, Directory* parent, long root_address, Fat16BootSector* bs)
{
    Fat16Entry *entry = malloc(sizeof(Fat16Entry));
    int index = 0;
    while(1) {
    fseek(in, parent->address + (index * sizeof(Fat16Entry)), SEEK_SET);
    index++;
    fread(entry, sizeof(Fat16Entry), 1, in);
        if (entry->filename[0] == 0x00) {
            break;
        } else if (entry->filename[0] == 0xE5) {
            continue;
        } else if (entry->filename[0] == '.') {
            continue;
        } else if (entry->attributes == 0x10){
            long starting_position = calculateStartingPosition(in, entry->starting_cluster, root_address, bs);
            char filename[20] = "";
            char *ptr = strchr(entry->filename, ' ');
            if (ptr) {
                *ptr = '\0';
            }
            strcat(filename, entry->filename);
            Directory* child = addSubdirectoryAlloc(parent, filename, starting_position);
            makeTree(in, child, root_address, bs);
        } else {
            long starting_position = calculateStartingPosition(in, entry->starting_cluster, root_address, bs);
            char filename[20] = "";
            strncpy(filename, entry->filename, 8);
            char *space = strchr(filename, ' ');
            if (space != NULL) {
                *space = '\0';
            }
            strcat(filename, ".");
            strncat(filename, entry->ext, 3);
            Directory* child = addSubdirectoryAlloc(parent, filename, starting_position);
        }
    }
    free(entry);
}

void _listDirs(Directory* dir, int level) {
    for(int j = 0; j < level; j++){
        printf("----");

    }
    printf("%s\n", dir->name);

    for(int i = 0; i<dir->subdirectoryCount; i++){
        _listDirs(dir->subdirectories[i], level+1);
    }
}

void listDirs(Directory* dir) {
    return _listDirs(dir, 0);
}

Directory* printTree(FILE* in) {
    Fat16BootSector bs;
    long root_address = findRoot(in, &bs);
    char root_name[20] = "ROOT";
    Directory *root = createDirectoryAlloc(root_address, root_name);
    makeTree(in, root, root_address, &bs);
    listDirs(root);
    return root;
}

void freeDirectoryTree(Directory* root) {
    if (root == NULL) {
        return;
    }
    for (int i = 0; i < root->subdirectoryCount; i++) {
        freeDirectoryTree(root->subdirectories[i]);
    }
    free(root->subdirectories);
    free(root);
}

// Directory* changeDir(Directory* root, char *path) {
//     //printf("hello");
//     char *token = strtok(path, "/");
//     Directory *current = root;
//     int i = 0;
//     while (token != NULL) {
//         Directory *subdir = current->subdirectories[i];
//         while (subdir != NULL) {
//             if (strcmp(subdir->name, token) == 0) {
//                 break;
//             }
//             subdir = subdir->subdirectories[++i];
//         }
//         if (subdir == NULL) {
//             printf("Directory not found!");
//             return NULL; // directory not found
//         }
//         current = subdir;
//         token = strtok(NULL, "/");
//     }
//     return current;
// }
Directory* changeDir(Directory* root, char *path) {
    if (root == NULL || path == NULL) {
        printf("root or path is NULL!\n");
        return NULL;
    }

    char *token = strtok(path, "/");
    Directory *current = root;
    while (token != NULL) {
        if (current->subdirectories == NULL) {
            printf("current->subdirectories is NULL!\n");
            return NULL;
        }

        Directory *subdir = NULL;
        for (int i = 0; i < current->subdirectoryCount; i++) {
            subdir = current->subdirectories[i];
            if (subdir->name == NULL) {
                printf("subdir->name is NULL!\n");
                return NULL;
            }

            if (strcmp(subdir->name, token) == 0) {
                break;
            }

            subdir = NULL;
        }

        if (subdir == NULL) {
            printf("Directory not found!\n");
            return NULL; // directory not found
        }

        current = subdir;
        token = strtok(NULL, "/");
    }

    return current;

    // char *token = strtok(path, "/");
    // Directory *current = root;
    // int i = 0;
    // while (token != NULL) {
    //     if (current->subdirectories == NULL) {
    //         printf("current->subdirectories is NULL!\n");
    //         return NULL;
    //     }

    //     Directory *subdir = current->subdirectories[i];
    //     while (subdir != NULL && subdir->subdirectories != NULL) {
    //         if (subdir->name == NULL) {printf("subdir->name is NULL!\n"); return NULL;}

    //         if (strcmp(subdir->name, token) == 0) {
    //             break;
    //         }

    //         if (subdir->subdirectories == NULL) {printf("subdir->subdirectories is NULL!\n");return NULL;}

    //         subdir = subdir->subdirectories[++i];
    //     }

    //     if (subdir == NULL) {
    //         printf("Directory not found!\n");
    //         return NULL; // directory not found
    //     }

    //     current = subdir;
    //     token = strtok(NULL, "/");
    // }

    // return current;
}