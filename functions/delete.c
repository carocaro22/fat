#include "delete.h"
#include "../fat.h"
#include <string.h>

void mydelete(char* filename, long directory_address, FILE* in) {
    int i;
    PartitionTable pt[4];
    Fat16BootSector bs;
    Fat16Entry entry;

    char *name;
    char *extension;
    char *filename_copy = strdup(filename);
    name = strtok(filename_copy, ".");
    extension = strtok(NULL, ".");

    char name_padded[9];
    for (i = 0; i < 8; i++)
    {
        name_padded[i] = ' ';
    }
    name_padded[8] = '\0'; // Null-terminate the string
    for (i = 0; i < 8 && name[i] != '\0'; i++)
    {
        name_padded[i] = name[i];
    }
    fseek(in, 0x1BE, SEEK_SET);               // go to partition table start, partitions start at offset 0x1BE, see http://www.cse.scu.edu/~tschwarz/coen252_07Fall/Lectures/HDPartitions.html
    fread(pt, sizeof(PartitionTable), 4, in); // read all entries (4)

    long root_addr = findRoot(in, &bs);

    fseek(in, directory_address, SEEK_SET); // go to begin of directory
    do
    {
        long curr_addr = ftell(in);
        fread(&entry, sizeof(entry), 1, in);
        if (entry.filename[0] != 0x00)
        {
            char temp[9];
            memcpy(temp, entry.filename, 8);
            temp[8] = '\0';

            if (strcmp(temp, name_padded) == 0)
            {
                fseek(in, curr_addr, SEEK_SET);
                char deleted_sign = 0xE5;
                fwrite(&deleted_sign, 1, 1, in);

                unsigned int cluster_size = bs.sectors_per_cluster * bs.sector_size;
                long boot_sector_start = bs.sector_size * pt[0].start_sector;
                long fat_start = boot_sector_start + cluster_size; // boot block has one sector
                long entry_block_in_fat = entry.starting_cluster / 200;
                long entry_in_fat = fat_start + (entry_block_in_fat * cluster_size) + (entry.starting_cluster * 2);

                unsigned short current_cluster = entry.starting_cluster;          // actual cluster

                while (current_cluster != 0xFFFF)
                {
                    long entry_in_fat = fat_start + (current_cluster / 200 * cluster_size) + (current_cluster * 2);
                    fseek(in, entry_in_fat, SEEK_SET);
                    unsigned short zero = 0;
                    fread(&current_cluster, 2, 1, in);
                    fseek(in, entry_in_fat, SEEK_SET);
                    fwrite(&zero, sizeof(short), 1, in);

                    // printf("entry_in_fat: %X\n", entry_in_fat);
                    printf("current_cluster: %X\n", current_cluster);
                    // printf("next_cluster: %X\n", next_cluster);   
                } 
            }
        }
    } while (entry.filename[0] != 0x00);
}