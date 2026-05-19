#include <Windows.h>
#include <fstream>
#include "commands.h"

//Tabelul FAT... Indexul reprezinta Cluster-ul, iar valoarea urmatorul cluster din lant, sau 0 daca e liber sau 0xFFFFFFFF(-1) daca e sfarsitul linked_list.
//uint32_t FAT[CLUSTER_COUNT];


//Un fisier sau directoriu din sistemul de fisiere


//Directoriul ROOT. Primul cluster dupa FAT.
//DirEntry entries[CLUSTER_SIZE/sizeof(DirEntry)];

static_assert(sizeof(DirEntry) == 32, "Size of DirEntry IS NOT 32");


int main()
{

    //Construim un pointer catre DISK-ul construit cu parametrii descrisi mai jos.
    HANDLE hFile = CreateFileA("minifat.img", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    //Construim din DISK un MAPPING OBJECT
    HANDLE hMapFile = CreateFileMappingA(hFile, nullptr, PAGE_READWRITE, 0, DISK_SIZE, 0);
    //MapViewOfFile returneaza inceputul acelui view al hMapFile
    //Vom folosi disk ca pointer la inceputul disk ului

    uint8_t* disk = nullptr;
    if (hMapFile != nullptr) {
        disk = (uint8_t*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, DISK_SIZE);
    }
    else {
        std::cerr << "hMapFile is nullptr, cannot map view of file.\n";

    }
    //current_cluster_stack.push(ROOT_DIRECTORY_CLUSTER);
    initializeStack();
    start_command_interface(disk);


    //Curatare
    if (disk != nullptr) {
        UnmapViewOfFile(disk);
    }
    else {
        std::cerr << "Can't UnMap disk: Disk is already nullptr";
    }
    if (hMapFile != nullptr) {
        CloseHandle(hMapFile);
    }
    else {
        std::cerr << "Can't close hMapFile: hMapFile is already nullptr";
    }
    CloseHandle(hFile);

    return 0;
}
