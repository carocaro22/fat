#include "../fat.h"
#include "write.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Search the FAT table to find the first free cluster (value 0x0000 ); 
// Write the number of the found cluster back to the directory structure and 
// mark this cluster as EOC ( 0xFFFF ).
int findFatStart(FILE* in, long *fat_start, long *fat_size, bool verbose) {
    if (in == NULL) {
        if (verbose) {
            printf("could not find file\n");
        }
        return -1;
    }
    PartitionTable pt[4];
    Fat16BootSector bs;

    fseek(in, 0x1BE, SEEK_SET); // start of partitions
    fread(pt, sizeof(PartitionTable), 4, in);  
    fseek(in, 0x200 * pt[0].start_sector, SEEK_SET); // 0x200 is sector size
    fread(&bs, sizeof(Fat16BootSector), 1, in); // boot sector is in start of every partition

    unsigned int cluster_size = bs.sectors_per_cluster * bs.sector_size;
    long boot_sector_start = bs.sector_size * pt[0].start_sector;
    *fat_start = boot_sector_start + cluster_size; // boot block has one sector
    *fat_size = bs.fat_size_sectors * bs.sector_size;
}

int findFreeCluster(FILE* in, long *current_cluster_address, bool verbose) {
    long fat_start;
    long fat_size; 
    findFatStart(in, &fat_start, &fat_size, true);

    short tmp;
    short cmp = 0;
    int start;
    if (verbose) {
        printf("fat_start: %lX\n", fat_start);
    }

    for (int offset = 0; offset <= (fat_size - 1); offset++) {
        fseek(in, fat_start + (2 * offset), SEEK_SET);
        start = ftell(in);
        // printf("fat_entry_address: %lX\n", start);
        fread(&tmp, sizeof(short), 1, in);
        if (tmp == cmp) {
            fseek(in, start, SEEK_SET);
            *current_cluster_address = start;
            char data[2] = {0xFF, 0xFF}; // generalize this
            if (fwrite(&data, sizeof(char), 2, in) < 2) {
                if (verbose) {
                    printf("Error writing to file\n");
                }
                return offset;
            }
            // printf("fat_address: %X\n", start);
            return offset;
        }
    }
}

void writeToClusters(int starting_cluster, char* data, int filesize, FILE* in, bool verbose) {
    if (in == NULL) {
        if (verbose) {
            printf("could not find file\n");
        }
    }
    unsigned short current_cluster = starting_cluster;
    long current_cluster_addr; // how to know this? 
    int chunk_size = 0x800;
    char* ptr = data;
    
    printf ("-----------writing to cluster--------\n");
    for (int i = 0; i < filesize; i += chunk_size) {
        PartitionTable pt[4];
        Fat16BootSector bs;

        fseek(in, 0x1BE, SEEK_SET); // partitions start in this offset
        fread(pt, sizeof(PartitionTable), 4, in); // read 4 partitions
        // TODO: consider writing to different partition
        fseek(in, 0x200 * pt[0].start_sector, SEEK_SET); // read boot sector of first partition (1 sector length)
        fread(&bs, sizeof(Fat16BootSector), 1, in); 

        long root_addr = findRoot(in, &bs);
        long blocks_dir = bs.root_dir_entries * 32;
        unsigned int cluster_size = bs.sectors_per_cluster * bs.sector_size;
        long data_area_start = root_addr + blocks_dir;
        long start_pos = data_area_start + (current_cluster - 2) * cluster_size;
        printf("current_cluster: %hd\n", current_cluster);
        fseek(in, start_pos, SEEK_SET);
    
        if (filesize - i < chunk_size) 
        {
            chunk_size = filesize - i; // lastchunk
            if (fwrite(ptr, sizeof(char), chunk_size, in) < chunk_size) 
            {
                if (verbose) {
                    printf("Error writing to file\n");
                }
            } 
            return; // avoid calling findFreeCluster again
        }
        if (fwrite(ptr, sizeof(char), chunk_size, in) < chunk_size) 
        {
            if (verbose) {
                printf("Error writing to file\n");
            }
        } 
        fseek(in, current_cluster_addr, SEEK_SET);
        fwrite(&current_cluster, sizeof(short), 1, in);
        current_cluster = findFreeCluster(in, &current_cluster_addr, false);
        ptr += chunk_size; 
    }
    printf("--------finished writing---------\n");
}

char* readLocalFile(char* filename, int* filesize) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error opening file\n");
    }
    fseek(file, 0, SEEK_END);
    *filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(*filesize + 1);
    if (buffer == NULL) {
        printf("Error alocationg memory\n");
    }
    size_t read = fread(buffer, 1, *filesize, file);
    if (read < *filesize) {
        printf("Error reading file\n");
    }
    fclose(file);
    return buffer;
}

void write(char filename[13], long directory_address, FILE* in) {
    char filename_copy[13];
    strcpy(filename_copy, filename);
    char *name = strtok(filename_copy, ".");
    char *ext = strtok(NULL, ".");

    int filesize;
    char* data = readLocalFile(filename, &filesize);
    printf("filesize: %X\n", filesize);

    Fat16Entry* new_entry = malloc(sizeof(Fat16Entry));
    memset(new_entry, 0x20, sizeof(Fat16Entry));
    memset(new_entry->filename, ' ', 8);
    strncpy(new_entry->filename, name, strlen(name));

    memset(new_entry->ext, ' ', 3);
    strncpy(new_entry->ext, ext, strlen(ext)); 

    new_entry->file_size = filesize;
    long cluster_addr;
    int offset = findFreeCluster(in, &cluster_addr, true); 

    new_entry->starting_cluster = offset;
    writeToClusters(offset, data, filesize, in, true);

    fseek(in, directory_address, SEEK_SET);
    printf("directory_address %lX\n", directory_address);

    Fat16Entry* entry = malloc(sizeof(Fat16Entry));
    while (entry->filename[0] != 0x00)
    {
        fread(entry, sizeof(Fat16Entry), 1, in);
    }

    long pos = ftell(in);
    pos -= sizeof(Fat16Entry);
    fseek(in, pos, SEEK_SET);
    fwrite(new_entry, sizeof(Fat16Entry), 1, in);

    fseek(in, pos, SEEK_SET);
    fread(entry, sizeof(Fat16Entry), 1, in);
    printf("filename: %.8s\n", entry->filename);
    // printf("starting_cluster: %hd", entry->starting_cluster);

    free(data);
    free(entry);
    free(new_entry);
}

