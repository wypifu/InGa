#ifndef INGA_INTERNAL_ALLOCATOR_H
#define INGA_INTERNAL_ALLOCATOR_H

#include <InGa/core/inga_platform.h>

#define INGA_MAX_PAGES_PER_GROUP 64

namespace Inga
{
    /* * BlockHeader : Métadonnées placées avant chaque allocation.
     * On garde la logique de double-double liste chaînée.
     */
struct BlockHeader
{
    U32 canary;
    U64 size;
    U64 payloadSize;
    
    // On change U8* en BlockHeader*
    BlockHeader* next;      
    BlockHeader* previous;  
    
    BlockHeader* lFree;     
    BlockHeader* rFree;     

    U16 groupId;
    U16 pageId;
    B8  used;

#ifdef INGA_DEBUG
    const char* file;
    U32 line;
#endif
};

    /*
     * MemoryPage : Une page de mémoire brute segmentée en blocs.
     */
    struct MemoryPage
    {
        U8* data;               // Pointeur vers le début de la zone mémoire brute
        U8* freeBlock;          // Tête de la liste des blocs libres
        U64 freeSize;           // Espace total libre sur la page
        U64 biggestFree;        // Taille du plus grand bloc libre (pour aller vite)
        U16 pageId;
        U16 groupId;
        U8* firstFreeBlock;
        // pthread_mutex_t mutex; // On l'ajoutera quand on fera le module Thread
    };

    /*
     * MemoryGroup : Un ensemble de pages avec la même configuration.
     */
    struct MemoryGroup
    {
        const char* name;
        MemoryPage* pages;      // Tableau de pages
        U64 pageSize;           // Taille fixe de chaque page du groupe
        U32 pageCount;          // Nombre actuel de pages
        U16 id;
        IngaMutex mutex; // Un verrou par groupe
    };
}

#endif // INGA_INTERNAL_ALLOCATOR_H
