#include "fat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

 void read(char* filename, FILE* in) {
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
  for (i = 0; i < 8; i++) {
    name_padded[i] = ' ';
  }
  name_padded[8] = '\0';  // Null-terminate the string
  for (i = 0; i < 8 && name[i] != '\0'; i++) {
    name_padded[i] = name[i];
  }
  fseek(in, 0x1BE, SEEK_SET);                // go to partition table start, partitions start at offset 0x1BE, see http://www.cse.scu.edu/~tschwarz/coen252_07Fall/Lectures/HDPartitions.html
  fread(pt, sizeof(PartitionTable), 4, in);  // read all entries (4)

  // A sector has 0x200 bytes = 512 bytes
  fseek(in, 512 * pt[0].start_sector, SEEK_SET); // Boot sector starts here (seek in bytes)
  fread(&bs, sizeof(Fat16BootSector), 1, in);    // Read boot sector content, see http://www.tavi.co.uk/phobos/fat.html#boot_block
  long begin_of_root = (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size;
  fseek(in, begin_of_root, SEEK_CUR);
  long begin_of_root_abs = ftell(in);
  for (i = 0; i < bs.root_dir_entries; i++) {
    fread(&entry, sizeof(entry), 1, in);
    if (entry.filename[0] != 0x00) {
        char temp[9]; 
        memcpy(temp, entry.filename, 8);
        temp[8] = '\0'; 
      if (strcmp(temp, name_padded) == 0) {
        long blocks_dir = bs.root_dir_entries * 32; 
        long data_area_start = begin_of_root_abs + blocks_dir;
        unsigned int cluster_size = bs.sectors_per_cluster * bs.sector_size;
        long startPos = data_area_start + (entry.starting_cluster - 2) * cluster_size;
        long boot_sector_start = bs.sector_size * pt[0].start_sector;
        long fat_start = boot_sector_start + cluster_size; // boot block has one sector
        long entry_block_in_fat = entry.starting_cluster / 200;
        long entry_in_fat = fat_start + (entry_block_in_fat * cluster_size) + (entry.starting_cluster * 2);

        char* data = malloc(entry.file_size);
        FILE* out = fopen(filename, "wb");
        
        if (entry.file_size <= cluster_size) {
          fseek(in, startPos, SEEK_SET); // start reading from start position
          fread(data, entry.file_size, 1, in);  // Read only bytes_to_write bytes
          size_t written = fwrite(data, 1, entry.file_size, out);
        }
        else {
        unsigned int fat_content_address = entry_in_fat; // address of first data entry in fat
        unsigned short fat_content = 0; // content of fat, also the position of next cluster of data
        int entry_offset = startPos; // offset to add to fat_content
        unsigned int total_bytes_written = 0;
        unsigned int filesize = entry.file_size;
        
        int bytes_to_write = cluster_size;
        while(fat_content != 0xFFFF) {
            fseek(in, fat_content_address, SEEK_SET);
            fread(&fat_content, 2, 1, in);
            
            // write to file block
            fseek(in, entry_offset, SEEK_SET); // start reading from start position
            fread(data, bytes_to_write, 1, in);  // Read only bytes_to_write bytes
            size_t written = fwrite(data, 1, bytes_to_write, out);
            // end of write to file block

            total_bytes_written += written;
            if (total_bytes_written + cluster_size > filesize) {
                bytes_to_write = filesize - total_bytes_written; // calculate remaining bytes
            } else {
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
  }
}

void listAllFiles(FILE* in ) {
  int i;
  PartitionTable pt[4];
  Fat16BootSector bs;
  Fat16Entry entry;

  // This function can take 3 values. SEEK_SET, SEEK_CUR or SEEK_END. 
  // They say from where the offset id added: begin, current or enf of the file respectively
  fseek(in, 0x1BE, SEEK_SET);                // go to partition table start, partitions start at offset 0x1BE, see http://www.cse.scu.edu/~tschwarz/coen252_07Fall/Lectures/HDPartitions.html
  // size of a single entry in the partition table is 16 bytes. 
  // total size of partition table is 64 bytes
  fread(pt, sizeof(PartitionTable), 4, in);  // read all entries (4)

  printf("Partition table\n-----------------------\n");
  for (i = 0; i < 4; i++) { // for all partition entries print basic info
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
  for (i = 0; i < bs.root_dir_entries; i++) {
    fread(&entry, sizeof(entry), 1, in);
    // Skip if filename was never used, see http://www.tavi.co.uk/phobos/fat.html#file_attributes
    if (entry.filename[0] != 0x00) {
      printf("%.8s.%.3s attributes 0x%02X starting cluster %8d len %8d B\n", entry.filename, entry.ext, entry.attributes, entry.starting_cluster, entry.file_size);
    }
  }
}

void list(FILE* in ) {
  int i;
  PartitionTable pt[4];
  Fat16BootSector bs;
  Fat16Entry entry;

  fseek(in, 0x1BE, SEEK_SET);                
  fread(pt, sizeof(PartitionTable), 4, in);  

  fseek(in, 512 * pt[0].start_sector, SEEK_SET); 
  fread(&bs, sizeof(Fat16BootSector), 1, in);    

  long begin_of_root = (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size;
  fseek(in, begin_of_root, SEEK_CUR);
  long begin_of_root_abs = ftell(in);

  printf("Mode     LastWriteTime             Length Name\n");
  printf("----     -------------             ------ ----\n");
  for (i = 0; i < bs.root_dir_entries; i++) {
    fread(&entry, sizeof(entry), 1, in);
    if (entry.filename[0] != 0x00) {
      // Convert the last write time to a date and time
      int year = ((entry.modify_date & 0xFE00) >> 9) + 1980;
      int month = (entry.modify_date & 0x01E0) >> 5;
      int day = entry.modify_date & 0x001F;
      int hour = (entry.modify_time & 0xF800) >> 11;
      int minute = (entry.modify_time & 0x07E0) >> 5;

      char* mode = (entry.attributes & 0x01) ? "-r----" : "-a----";

      char filename[9];
      int j = 0;
      for (int i = 0; i < 8; i++) {
        if (entry.filename[i] != ' ') {
          filename[j++] = entry.filename[i];
        }
      }
      filename[j] = '\0';

      if (entry.ext[0] != ' ') {
        printf("%s   %d/%d/%d   %d:%02d AM   %10d %s.%.3s\n", mode, month, day, year, hour, minute, entry.file_size, filename, entry.ext);
      } else {
        printf("%s   %d/%d/%d   %d:%02d AM   %10d %s\n", mode, month, day, year, hour, minute, entry.file_size, filename);
      }
  }
}

}

int main() {
  // rb stands for read binary
  FILE* in = fopen("sd.img", "rb");
  
  //listAllFiles(in); // Given code
  //list(in); // formatted string to DOS format
  read("FAT16.jpg", in);
  read("ABSTRAKT.txt", in);

  fclose(in);
  return 0;
}
