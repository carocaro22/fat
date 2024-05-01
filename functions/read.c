#include "../fat.h"
#include "read.h"
#include <string.h>
#include <stdlib.h>


void read(char *filename, long directory_address, FILE *in)
{
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
        fread(&entry, sizeof(entry), 1, in);
        if (entry.filename[0] != 0x00)
        {
            char temp[9];
            memcpy(temp, entry.filename, 8);
            temp[8] = '\0';

            if (strcmp(temp, name_padded) == 0)
            {
                long blocks_dir = bs.root_dir_entries * 32;
                long data_area_start = root_addr + blocks_dir;
                unsigned int cluster_size = bs.sectors_per_cluster * bs.sector_size;
                long startPos = data_area_start + (entry.starting_cluster - 2) * cluster_size;
                long boot_sector_start = bs.sector_size * pt[0].start_sector;
                long fat_start = boot_sector_start + cluster_size; // boot block has one sector
                long entry_block_in_fat = entry.starting_cluster / 200;
                long entry_in_fat = fat_start + (entry_block_in_fat * cluster_size) + (entry.starting_cluster * 2);

                char *data = malloc(entry.file_size);
                FILE *out = fopen(filename, "wb");

                if (entry.file_size <= cluster_size)
                {
                    fseek(in, startPos, SEEK_SET);       // start reading from start position
                    fread(data, entry.file_size, 1, in); // Read only bytes_to_write bytes
                    size_t written = fwrite(data, 1, entry.file_size, out);
                }
                else
                {
                    unsigned int fat_content_address = entry_in_fat; // address of first data entry in fat
                    unsigned short fat_content = 0;                  // content of fat, also the position of next cluster of data
                    int entry_offset = startPos;                     // offset to add to fat_content
                    unsigned int total_bytes_written = 0;
                    unsigned int filesize = entry.file_size;

                    int bytes_to_write = cluster_size;
                    while (fat_content != 0xFFFF)
                    {
                        fseek(in, fat_content_address, SEEK_SET);
                        fread(&fat_content, 2, 1, in);

                        // write to file block
                        fseek(in, entry_offset, SEEK_SET);  // start reading from start position
                        fread(data, bytes_to_write, 1, in); // Read only bytes_to_write bytes
                        size_t written = fwrite(data, 1, bytes_to_write, out);
                        // end of write to file block

                        total_bytes_written += written;
                        if (total_bytes_written + cluster_size > filesize)
                        {
                            bytes_to_write = filesize - total_bytes_written; // calculate remaining bytes
                        }
                        else
                        {
                            bytes_to_write = cluster_size;
                        }

                        // calculate next
                        fat_content_address += 2;
                        entry_offset = data_area_start + (fat_content * cluster_size) - (2 * cluster_size);
                    }
                }
                free(data);
            }
        }
    } while (entry.filename[0] != 0x00);
}