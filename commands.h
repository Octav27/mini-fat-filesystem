
#ifndef COMMANDS_H

#define COMMANDS_H
#include <iostream>


constexpr size_t CLUSTER_SIZE = 1024;
constexpr size_t CLUSTER_COUNT = 1024;
constexpr size_t DISK_SIZE = CLUSTER_COUNT * CLUSTER_SIZE;

constexpr uint32_t SUPERBLOCK_CLUSTER = 0;
constexpr uint32_t FAT_CLUSTER_START = 1;
constexpr uint32_t FAT_CLUSTER_COUNT = 4;
constexpr uint32_t ROOT_DIRECTORY_CLUSTER = FAT_CLUSTER_START + FAT_CLUSTER_COUNT;
constexpr uint32_t CONTENT_DIRECTORY_CLUSTER_START = ROOT_DIRECTORY_CLUSTER;


class DirEntry {
public:
    char dir_name[11]; // 11 bytes
    uint8_t dir_attr; // 0x00 = false = fisier; 0x01 = true = directoriu... 1 byte
    uint8_t dir_empty[12];// 12 bytes 
    uint32_t dir_firstcluster; // 4 bytes
    uint32_t size; // 4 bytes
    // 11+1+12+4+4=32
};

DirEntry* get_directory(uint8_t* disk, int directory_cluster);

uint32_t* get_fat(uint8_t* disk);

void initializeStack();

void create_file(uint8_t* disk, std::string& name);

void list_files(uint8_t* disk);

void show_current_directory(uint8_t* disk, uint32_t directory_cluster);

void change_directory(uint8_t* disk, std::string& to_directory_name);


void list_tree(uint8_t* disk, uint32_t directory_cluster, uint32_t number_tabs);

void list_fat(uint8_t* disk);


void delete_directory(uint8_t* disk, uint32_t current_cluster);

void delete_file(uint8_t* disk, uint32_t directory_cluster, std::string& nume);

void format_filesystem(uint8_t* disk);


void start_command_interface(uint8_t* disk);

#endif
