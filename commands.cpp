#include "commands.h"
#include <stack>
#include <iostream>

uint32_t CURRENT_DIRECTORY_CLUSTER = ROOT_DIRECTORY_CLUSTER;
std::stack<uint32_t> current_cluster_stack;

std::string DIRECTORY_PATH = "root\\";

DirEntry* get_directory(uint8_t* disk, int directory_cluster) {
    return (DirEntry*)(disk + directory_cluster * CLUSTER_SIZE);

}

uint32_t* get_fat(uint8_t* disk) {
    return (uint32_t*)(disk + FAT_CLUSTER_START * CLUSTER_SIZE);
}

uint32_t get_parent_directory() {
    current_cluster_stack.pop();
	return current_cluster_stack.top();
}



uint32_t find_free_cluster(uint32_t* fat) {
    for (uint32_t i = CONTENT_DIRECTORY_CLUSTER_START; i < CLUSTER_COUNT; i++) {
        if (fat[i] == 0)
            return i;
    }

    std::cerr << "All clusters are occupied!\n";
    //Daca nu exista niciun cluster liber, atunci returnam -1
    return 0xFFFFFFFF;
}


int get_next_dir_entry_location(uint8_t* disk, int directory_cluster) {

    DirEntry* directory = (DirEntry*)(disk + directory_cluster * CLUSTER_SIZE);

    for (int i = 0; i < CLUSTER_SIZE / sizeof(DirEntry); i++) {
        if (directory[i].dir_name[0] == 0x00) {
            return i;
        }
    }

    return -1;
}

void to_83_name(std::string& initial_name, char out[11]) {
    std::memset(out, ' ', 11);

    if (initial_name.find('.') != std::string::npos) {
        size_t dot = initial_name.find('.');
        std::string name = initial_name.substr(0, dot);
        std::string  extension = initial_name.substr(dot + 1);

        for (size_t i = 0; i < name.size() && i < 8; i++) {
            out[i] = std::toupper(name[i]);
        }
        for (size_t i = 0; i < extension.size() && i < 3; i++) {
            out[11 - extension.size() + i] = std::toupper(extension[i]);
        }
        return;
    }

    for (size_t i = 0; i < initial_name.size() && i < 11; i++) {
        out[i] = std::toupper(initial_name[i]);
    }
    return;
}

void create_file(uint8_t* disk, std::string& name) {
    DirEntry* directory = get_directory(disk, CURRENT_DIRECTORY_CLUSTER);
    uint32_t* fat = get_fat(disk);
    const int max_entries = CLUSTER_SIZE / sizeof(DirEntry);

    //Setam numele fisierului in format 8.3
    char file_name[11];
    to_83_name(name, file_name);

    int index_free_cluster = find_free_cluster(fat); //Gasim un cluster in care vom scrie continutul DirEntry nou.
    int dir_entry_location = get_next_dir_entry_location(disk, CURRENT_DIRECTORY_CLUSTER); //Gasim locatia in care vom scrie noul DirEntry in cadrul directorului curent.

    if (fat[CURRENT_DIRECTORY_CLUSTER] == 0x00000000 || fat[CURRENT_DIRECTORY_CLUSTER] == 0xFFFFFFFF) {
        fat[CURRENT_DIRECTORY_CLUSTER] = index_free_cluster;
        directory->dir_firstcluster = index_free_cluster;
    }
    if (index_free_cluster != 0xFFFFFFFF) {

        std::memcpy(directory[dir_entry_location].dir_name, file_name, 11);

        bool isDirectory = name.find('.') == std::string::npos;
        isDirectory ? directory[dir_entry_location].dir_attr = 0x01 : directory[dir_entry_location].dir_attr = 0x00;

        if (isDirectory) {
            fat[index_free_cluster] = 0xFFFFFFFF;
            directory[dir_entry_location].dir_firstcluster = index_free_cluster;
            directory[dir_entry_location].size = 0;
        }
        else {
            fat[index_free_cluster] = 0xFFFFFFFF;
            directory[dir_entry_location].dir_firstcluster = -1;
            directory[dir_entry_location].size = 0;
        }

        std::string showName;
        showName.assign(file_name, 11);

        isDirectory ? std::cout << "Directory " << showName << " has been created!\n" : std::cout << "File " << showName << " has been created!\n";
    }
    else {
        std::cerr << "No free cluster available to create file!\n";
    }
}

void list_files(uint8_t* disk) {
    DirEntry* root = get_directory(disk, CURRENT_DIRECTORY_CLUSTER);
    const int max_entries = CLUSTER_SIZE / sizeof(DirEntry);

    for (int i = 0; i < max_entries; i++) {
        std::cout << " Pointer: " << &root[i] << ':' << '\n';

        if (root[i].dir_name[0] == 0x00)
            continue;

        if (root[i].dir_attr == 0x00) {
            std::cout << "<FILE>" << root[i].dir_name << "</FILE>\n";
        }
        else {
            std::cout << "<DIR>" << root[i].dir_name << "</DIR>\n";

        }
    }
}


void show_current_directory(uint8_t* disk, uint32_t directory_cluster) {
    DirEntry* directory = get_directory(disk, directory_cluster);

    std::cout << "-------CURRENT DIRECTORY--------\n";

    std::cout << "Name: " << directory->dir_name << ";\n";
    std::cout << "dir_attr: " << directory->dir_attr << ";\n";
    std::cout << "dir_empty: " << directory->dir_empty << ";\n";
    std::cout << "dir_firstcluster: " << directory->dir_firstcluster << ";\n";
    std::cout << "size: " << directory->size << ";\n";

    std::cout << "------------------\n\n";
}


void list_tree(uint8_t* disk, uint32_t directory_cluster, uint32_t number_tabs) {
    DirEntry* directory = get_directory(disk, directory_cluster);
    std::string padding = "";
    for (uint32_t i = 0; i < number_tabs; i++) {
        padding += "\t";
    }
    if (directory_cluster == CURRENT_DIRECTORY_CLUSTER) {
        padding += "*";
    }



    for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(DirEntry); i++) {

        if (directory[i].dir_name[0] == 0x00)
            continue;

        if (directory[i].dir_attr == 0x00) {
            std::cout << padding << "<FILE>" << directory[i].dir_name << "</FILE> "<<directory[i].dir_firstcluster<< "\n";
        }
        else {
            std::cout << padding << "<DIR>" << directory[i].dir_name << "</DIR> " << directory[i].dir_firstcluster << "\n";
            list_tree(disk, directory[i].dir_firstcluster, number_tabs + 1);
        }
    }

}

void change_directory(uint8_t* disk, std::string& to_directory_name) {
    //DirEntry* directory = get_directory(disk, to_directory_cluster);

    DirEntry* current_directory = get_directory(disk, CURRENT_DIRECTORY_CLUSTER);

    if (to_directory_name == "..") {
		CURRENT_DIRECTORY_CLUSTER = get_parent_directory();
		DIRECTORY_PATH = DIRECTORY_PATH.substr(0, DIRECTORY_PATH.find_last_of('\\', DIRECTORY_PATH.length() - 2) + 1);
        return;
    }

    char directory_name[11];
    to_83_name(to_directory_name, directory_name);

    for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(DirEntry); i++) {

        if (std::memcmp(current_directory[i].dir_name, directory_name, 11) == 0) {
             if (current_directory[i].dir_firstcluster != 0) {
                 current_cluster_stack.push(current_directory[i].dir_firstcluster);
                CURRENT_DIRECTORY_CLUSTER = current_directory[i].dir_firstcluster;
				DIRECTORY_PATH += to_directory_name + '\\';

            }
            return;
        }
    }
}


void list_fat(uint8_t* disk) {
    uint32_t* fat = get_fat(disk);

    std::cout << "---------FAT---------\n";


    for (uint32_t i = 0; i < 12; i++) {

        std::cout << i << ": " << (fat[i]) << ";\t" << '\n';

    }
    for (uint32_t i = 12; i < CLUSTER_COUNT; i++) {
        if (fat[i] != 0x00000000) {
            std::cout << i << ": " << (fat[i]) << ";\t" << '\n';
        }
    }
    std::cout << 1023 << ": " << (fat[1023]) << ";\t" << '\n';

}


void delete_directory(uint8_t* disk, uint32_t current_cluster) {
    DirEntry* directory = get_directory(disk, current_cluster);
    uint32_t* fat = get_fat(disk);
    if (directory->dir_firstcluster == 0xFFFFFFFF) {
        directory->dir_name[0] = 0x00;
        fat[directory->dir_firstcluster] = 0x00000000;
        std::cout << "Directory EMPTY: " << directory->dir_name << " has been deleted\n";
        return;

    }
    for (uint32_t i = 0; i < CLUSTER_SIZE / sizeof(DirEntry); i++) {
        //Daca e fisier atunci seteaza doar name=0 si fat->0
        if (directory[i].dir_attr == 0x00) {
            if (directory[i].dir_name == 0x00) {
                continue;
            }
            else {
                std::cout << "FILE from Directory: " << directory[i].dir_name << " has been deleted\n";

                directory[i].dir_name[0] = 0x00;
                fat[directory->dir_firstcluster + i] = 0x00000000;
            }
        }
        else {
            delete_directory(disk, directory[i].dir_firstcluster);
            std::cout << "Directory from Directory : " << directory[i].dir_name << " has been deleted\n";
        }
    }
}



 


void delete_file(uint8_t* disk, uint32_t directory_cluster, std::string& nume) {
    DirEntry* directory = get_directory(disk, directory_cluster);
    uint32_t* fat = get_fat(disk);

    char file_name[11];
    to_83_name(nume, file_name); //aflam numele asa cum e in sistem

    const int max_entries = CLUSTER_SIZE / sizeof(DirEntry);


    //Daca numele contine caracterul '.', atunci e fisier
    if (nume.find('.') != std::string::npos) {

        for (int i = 0; i < max_entries; i++) {
            if (directory[i].dir_name[0] == 0x00)
                continue;

            std::string rootName = directory[i].dir_name;
            std::cout << "String rootName: " << rootName << '\n';
            std::string fileName;
            fileName.assign(file_name, 11);
            std::cout << "String fileName: " << fileName << '\n';

            if (rootName == fileName) {
                std::cout << "File " << directory[i].dir_name << " has been deleted!\n";
                directory[i].dir_name[0] = 0x00;
                fat[directory->dir_firstcluster + i] = 0x00000000;

                return;
            }
        }
    }
    else {
        for (int i = 0; i < max_entries; i++) {
            if (directory[i].dir_name[0] == 0x00)
                continue;

         //   std::string rootName = directory[i].dir_name;
            std::cout << "String rootName: " << directory[i].dir_name << '\n';
        //    std::string fileName;
         //   fileName.assign(file_name, 11);
            std::cout << "String fileName: " << file_name << '\n';

            std::cout << "Memcmp: " << std::memcmp(directory[i].dir_name, file_name, 11) << '\n';


            if (std::memcmp(directory[i].dir_name, file_name,11)==0) {
                delete_directory(disk, directory[i].dir_firstcluster);
                std::cout << "Directory " << directory[i].dir_name << " has been deleted!\n";


                return;
            }
        }
    }

}


void format_directory(uint8_t* disk, uint32_t directory_cluster) {
    DirEntry* directory = get_directory(disk, directory_cluster);


    if (directory_cluster == ROOT_DIRECTORY_CLUSTER) {
        if (directory->dir_firstcluster == 0x00000000) {

        }
    }
    else if (directory->dir_firstcluster == 0x00000000) {
        directory->dir_name[0] = 0x00;
    }
    for (int i = 0; i < CLUSTER_SIZE / sizeof(DirEntry); i++) {
        if (directory[i].dir_attr == 0x00) {
            directory[i].dir_name[0] = 0x00;
        }
        else {
            format_directory(disk, directory[i].dir_firstcluster);
            directory[i].dir_name[0] = 0x00;
        }
    }
}

void format_filesystem(uint8_t* disk) {
    //Formatare FAT
    uint32_t* fat = nullptr;

    if (disk != nullptr) {
        fat = get_fat(disk);
        for (int i = ROOT_DIRECTORY_CLUSTER; i < CLUSTER_COUNT; i++)
            fat[i] = 0;

        fat[SUPERBLOCK_CLUSTER] = 0xFFFFFFFF;

        for (int i = FAT_CLUSTER_START; i < FAT_CLUSTER_START + FAT_CLUSTER_COUNT; i++)
            fat[i] = 0x00000000;

        fat[ROOT_DIRECTORY_CLUSTER] = 0xFFFFFFFF;

        format_directory(disk, ROOT_DIRECTORY_CLUSTER);

    }
    else {
        std::cerr << "Disk or hMapFile is nullptr\n";
    }
}

void start_command_interface(uint8_t* disk) {
    std::string command;
    std::cout << DIRECTORY_PATH << ": ";

    while (std::cin >> command) {
        if (command == "create") {
            std::string file_name;
            std::cout << "Enter name of file/directory to create: \n";
            //   std::cout << "Format name of file: name.ext\n";
            std::cin >> file_name;
            create_file(disk, file_name);

        }
        else if (command == "ls") {
            std::cout << "\n ------- " << CURRENT_DIRECTORY_CLUSTER << " DIRECTORY-------\n";
            list_files(disk);
        }
        else if (command == "delete") {
            std::string file_name;
            std::cout << "Enter name of file/directory to delete: \n";
            //   std::cout << "Format name of file: name.ext\n";
            std::cin >> file_name;
            delete_file(disk, CURRENT_DIRECTORY_CLUSTER, file_name);

        }
        else if (command == "format") {
            format_filesystem(disk);
        }
        else if (command == "lstree") {
            list_tree(disk, ROOT_DIRECTORY_CLUSTER, 0);
        }
        else if (command == "help") {

        }
        else if (command == "exit") {
            std::cout << "You exited!!\n";
            break;
        }
        else if (command == "cd") {
            std::string directory_name;
            std::cout << "Enter name of directory to go into: \n";
            //  std::cout << "Format name of file: name.ext\n";
            std::cin >> directory_name;
            change_directory(disk, directory_name);
        }
        else if (command == "fat") {
            list_fat(disk);
        }


        std::cout << DIRECTORY_PATH<<": ";
    }
}