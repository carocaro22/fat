#include "list.h"
#include "../fat.h"

void listAllFiles(FILE *in)
{
    int i;
    PartitionTable pt[4];
    Fat16BootSector bs;
    Fat16Entry entry;

    // This function can take 3 values. SEEK_SET, SEEK_CUR or SEEK_END.
    // They say from where the offset id added: begin, current or enf of the file respectively
    fseek(in, 0x1BE, SEEK_SET); // go to partition table start, partitions start at offset 0x1BE, see http://www.cse.scu.edu/~tschwarz/coen252_07Fall/Lectures/HDPartitions.html
    // size of a single entry in the partition table is 16 bytes.
    // total size of partition table is 64 bytes
    fread(pt, sizeof(PartitionTable), 4, in); // read all entries (4)

    printf("Partition table\n-----------------------\n");
    for (i = 0; i < 4; i++)
    { // for all partition entries print basic info
        printf("Partition %d, type %02X, ", i, pt[i].partition_type);
        printf("start sector %8X, length %8X sectors\n", pt[i].start_sector, pt[i].length_sectors);
    }

    printf("\nSeeking to first partition by %d sectors\n", pt[0].start_sector);
    // a sector is the smallest addressable unit on the disk
    // when we want to seek to a particular sector, we need to convert that to a byte offset
    fseek(in, 512 * pt[0].start_sector, SEEK_SET); // Boot sector starts here (seek in bytes)
    fread(&bs, sizeof(Fat16BootSector), 1, in);    // Read boot sector content, see http://www.tavi.co.uk/phobos/fat.html#boot_block
    printf("Volume_label %.11s, %d sectors size, root dir entries %d\n", bs.volume_label, bs.sector_size, bs.root_dir_entries);

    // Seek to the beginning of root directory, it's position is fixed
    long begin_of_root = (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size;
    fseek(in, begin_of_root, SEEK_CUR);
    long begin_of_root_abs = ftell(in);

    // Read all entries of root directory
    printf("\nFilesystem root directory listing\n-----------------------\n");
    for (i = 0; i < bs.root_dir_entries; i++)
    {
        fread(&entry, sizeof(entry), 1, in);
        // Skip if filename was never used, see http://www.tavi.co.uk/phobos/fat.html#file_attributes
        if (entry.filename[0] != 0x00)
        {
            printf("%.8s.%.3s attributes 0x%02X starting cluster %8d len %8d B\n", entry.filename, entry.ext, entry.attributes, entry.starting_cluster, entry.file_size);
        }
    }
}