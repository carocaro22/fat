#include "../fat.h"
#include "list_dos.h"

void list_dos(FILE *in)
{
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
    for (i = 0; i < bs.root_dir_entries; i++)
    {
        fread(&entry, sizeof(entry), 1, in);
        if (entry.filename[0] != 0x00)
        {
            // Convert the last write time to a date and time
            int year = ((entry.modify_date & 0xFE00) >> 9) + 1980;
            int month = (entry.modify_date & 0x01E0) >> 5;
            int day = entry.modify_date & 0x001F;
            int hour = (entry.modify_time & 0xF800) >> 11;
            int minute = (entry.modify_time & 0x07E0) >> 5;

            char *mode = (entry.attributes & 0x01) ? "-r----" : "-a----";

            char filename[9];
            int j = 0;
            for (int i = 0; i < 8; i++)
            {
                if (entry.filename[i] != ' ')
                {
                    filename[j++] = entry.filename[i];
                }
            }
            filename[j] = '\0';

            if (entry.ext[0] != ' ')
            {
                printf("%s   %d/%d/%d   %d:%02d AM   %10d %s.%.3s\n", mode, month, day, year, hour, minute, entry.file_size, filename, entry.ext);
            }
            else
            {
                printf("%s   %d/%d/%d   %d:%02d AM   %10d %s\n", mode, month, day, year, hour, minute, entry.file_size, filename);
            }
        }
    }
}
