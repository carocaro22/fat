// FAT16 structures
// see http://www.tavi.co.uk/phobos/fat.html

typedef struct {
    unsigned char first_byte; //1
    unsigned char start_chs[3]; //1 + 3 = 4
    unsigned char partition_type; // 4 + 1 = 5
    unsigned char end_chs[3]; // 5 + 3 = 8
    unsigned int start_sector; //8 + 4 = 12
    unsigned int length_sectors; // 12 + 4 = 16
} __attribute((packed)) PartitionTable;

// type: char, short, int
// size: 1   , 2    , 4
typedef struct {
    unsigned char jmp[3];
    char oem[8];
    unsigned short sector_size; 
    unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char number_of_fats;
    unsigned short root_dir_entries;
    unsigned short total_sectors_short; // if zero, later field is used
    unsigned char media_descriptor;
    unsigned short fat_size_sectors;
    unsigned short sectors_per_track;
    unsigned short number_of_heads;
    unsigned int hidden_sectors;
    unsigned int total_sectors_int;
    unsigned char drive_number;
    unsigned char current_head;
    unsigned char boot_signature;
    unsigned int volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute((packed)) Fat16BootSector;

typedef struct {
    unsigned char filename[8]; // 8
    unsigned char ext[3]; //11
    unsigned char attributes; //12
    unsigned char reserved[10]; //22
    unsigned short modify_time; // 24
    unsigned short modify_date; // 26
    unsigned short starting_cluster; //28
    unsigned int file_size; //32
} __attribute((packed)) Fat16Entry;
