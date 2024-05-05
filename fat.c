#include "fat.h"
#include <stdlib.h>
#include <string.h>
#include "functions/list_dos.h"
#include "functions/directoryTree.h"
#include "functions/read.h"
#include "functions/write.h"
#include "functions/delete.h"

long findRoot(FILE *in, Fat16BootSector *bs)
{
    PartitionTable pt[4];
    fseek(in, 0x1BE, SEEK_SET);
    fread(pt, sizeof(PartitionTable), 4, in);
    fseek(in, 512 * pt[0].start_sector, SEEK_SET);
    fread(bs, sizeof(Fat16BootSector), 1, in);
    long begin_of_root = (bs->reserved_sectors - 1 + bs->fat_size_sectors * bs->number_of_fats) * bs->sector_size;
    fseek(in, begin_of_root, SEEK_CUR);
    long begin_of_root_abs = ftell(in);

    return begin_of_root_abs;
}

long calculateStartingPosition(FILE *in, int starting_cluster, long root_address, Fat16BootSector* bs) {
    long data_area_start = root_address + (bs->root_dir_entries * sizeof(Fat16Entry));
    unsigned int cluster_size = bs->sectors_per_cluster * bs->sector_size;
    
    long start_position = data_area_start + (starting_cluster - 2) * cluster_size; 
    return start_position;
}

int main()
{
    FILE *in = fopen("sd.img", "r+b");
    Directory* root = printTree(in);
    
    //printf(root->name);
    Directory* dir = changeDir(root, "/ADR1");
    printf("%lX\n\n", dir->address);
    //write("data.txt", dir->address, in);

    //printf("-----READ-----\n");
    read("KOCKA.JPG", dir->address, in);
    //mydelete("data.txt", dir->address, in);

    freeDirectoryTree(root);
    fclose(in);
    return 0;
}
