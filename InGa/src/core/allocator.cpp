#include <InGa/core/allocator.h>
#include "internal_allocator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <chrono> // On l'utilise ici seulement pour la mesure brute


// --- Statistiques de performance internes ---
#ifdef INGA_DEBUG
static U64 g_total_alloc_time_us = 0;
static U64 g_total_free_time_us  = 0;
static U64 g_alloc_calls         = 0;
static U64 g_free_calls          = 0;
#endif

// Petite fonction utilitaire interne (non exportée)
static inline U64 GetMicroseconds()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
}

#ifdef INGA_DEBUG
struct AllocPerformanceCounter
{
    U64 start;
    U64* targetTotalTime;
    U64* targetCallCount;

    AllocPerformanceCounter(U64* totalTime, U64* callCount)
        : targetTotalTime(totalTime), targetCallCount(callCount)
    {
        start = GetMicroseconds();
    }

    ~AllocPerformanceCounter()
    {
        *targetTotalTime += (GetMicroseconds() - start);
        (*targetCallCount)++;
    }
};
#define INGA_INSTRUMENT_ALLOC() AllocPerformanceCounter counter(&g_total_alloc_time_us, &g_alloc_calls)
#define INGA_INSTRUMENT_FREE()  AllocPerformanceCounter counter(&g_total_free_time_us, &g_free_calls)
#else
#define INGA_INSTRUMENT_ALLOC()
#define INGA_INSTRUMENT_FREE()
#endif

namespace Inga
{
    // L'état du moteur : statique pour rester privé à ce fichier
    static MemoryGroup* g_groups = nullptr;
    static U32 g_max_groups = 0;
    static U32 g_group_count = 0;
    static B8  g_is_initialized = INGA_FALSE;

    // Buffer de secours pour les allocations "trop précoces"
    static U8  g_bootstrap_buffer[1024 * 1024]; 
    static U64 g_bootstrap_offset = 0;
    static IngaMutex g_global_mutex;

    B8 Allocator::start(U32 maxGroups, U64 defaultPageSize)
    {
        if (g_is_initialized) return INGA_FALSE;

        g_max_groups = maxGroups;
        // Allocation "OS" pour le dictionnaire de nos groupes
        g_groups = (MemoryGroup*)malloc(sizeof(MemoryGroup) * g_max_groups);
        memset(g_groups, 0, sizeof(MemoryGroup) * g_max_groups);

  // Initialisation du verrou global
        INGA_MUTEX_INIT(&g_global_mutex);
        g_is_initialized = INGA_TRUE;

        // On crée immédiatement le groupe 0 (Général)
        AllocationGroupInfo general = { "General", defaultPageSize };
        addGroup(general);

      return INGA_TRUE;
    }

    U16 Allocator::addGroup(const AllocationGroupInfo& info)
    {
        INGA_MUTEX_LOCK(&g_global_mutex);
        if (g_group_count >= g_max_groups)
        {
              INGA_MUTEX_UNLOCK(&g_global_mutex);
              return 0xFFFF;
        }

        U16 id = static_cast<U16>(g_group_count++);
        MemoryGroup* group = &g_groups[id];

        INGA_MUTEX_UNLOCK(&g_global_mutex);

        INGA_MUTEX_INIT(&group->mutex);

        group->id = id;
        group->name = info.Name;
        group->pageSize = info.PageSize;
        group->pageCount = 1;

        // On alloue de quoi stocker la gestion des pages du groupe
        group->pages = (MemoryPage*)malloc(sizeof(MemoryPage) * INGA_MAX_PAGES_PER_GROUP);
        memset(group->pages, 0, sizeof(MemoryPage) * INGA_MAX_PAGES_PER_GROUP);

        // Initialisation de la Page 0 du groupe
        MemoryPage* page = &group->pages[0];
        page->groupId = id;
        page->pageId = 0;
        page->data = (U8*)malloc(group->pageSize);
        page->freeSize = group->pageSize;
        
        // On crée le tout premier bloc (vide) dans cette page
        BlockHeader* firstBlock = (BlockHeader*)page->data;
        firstBlock->canary = 0x494E4741; // "INGA"
        firstBlock->size = group->pageSize;
        firstBlock->used = INGA_FALSE;
        firstBlock->next = nullptr;
        firstBlock->previous = nullptr;

        page->freeBlock = (U8*)firstBlock;

        return id;
    }

U16 Allocator::setGroupIdByName(const char* name)
{
    if (!name || !g_is_initialized)
    {
        return 0xFFFF;
    }

    INGA_MUTEX_LOCK(&g_global_mutex);

    for (U32 i = 0; i < g_group_count; ++i)
    {
        // On compare les noms (strcmp est sûr ici car on est verrouillé)
        if (strcmp(g_groups[i].name, name) == 0)
        {
            INGA_MUTEX_UNLOCK(&g_global_mutex);
            return (U16)i;
        }
    }

    INGA_MUTEX_UNLOCK(&g_global_mutex);
    
    // Si on ne trouve pas, on loggue une erreur et on retourne un ID invalide
    printf("[InGa] Erreur : Groupe memoire '%s' non trouve.\n", name);
    return 0xFFFF; 
}

static bool createNewPage(MemoryGroup* group)
{
  if (group->pageCount >= INGA_MAX_PAGES_PER_GROUP) 
    {
        return false;
    }

    U32 newPageIndex = group->pageCount;
    MemoryPage* page = &group->pages[newPageIndex];

    page->data = (U8*)::malloc(group->pageSize);
    if (!page->data) 
    {
        return false;
    }

    page->groupId = group->id;
    page->pageId = static_cast<U16>(newPageIndex);

    BlockHeader* firstBlock = (BlockHeader*)page->data;
    firstBlock->canary = 0x494E4741;
    firstBlock->size = group->pageSize;
    firstBlock->used = INGA_FALSE;
    firstBlock->groupId = group->id;
    firstBlock->pageId = static_cast<U16>(newPageIndex);

    // Liens physiques (Ordre sur la RAM)
    firstBlock->next = nullptr;
    firstBlock->previous = nullptr;

    // Liens logiques (Ordre des blocs libres)
    firstBlock->lFree = nullptr;
    firstBlock->rFree = nullptr;

    // La page pointe sur ce premier bloc libre
    page->firstFreeBlock = (U8*)firstBlock;

    group->pageCount++;
    return true;
}

void* Allocator::alloc(U64 size, U32 align, U16 groupId, const char* file, I32 line)
{
    INGA_INSTRUMENT_ALLOC();
// 1. BOOTSTRAP
    if (!g_is_initialized) 
    {
        U64 padding = (align - (g_bootstrap_offset % align)) % align;
        if (g_bootstrap_offset + size + padding > sizeof(g_bootstrap_buffer)) 
        {
            return nullptr; 
        }
        
        void* ptr = &g_bootstrap_buffer[g_bootstrap_offset + padding];
        g_bootstrap_offset += (size + padding);
        return ptr;
    }

    // 2. SECURITÉ
    if (groupId >= g_group_count) 
    {
        return nullptr;
    }

    MemoryGroup* group = &g_groups[groupId];

    // --- ZONE CRITIQUE ---
    INGA_MUTEX_LOCK(&group->mutex);

    // 3. RECHERCHE DANS LES PAGES
    for (U32 p = 0; p < group->pageCount; ++p) 
    {
        MemoryPage* page = &group->pages[p];
        // MODIFICATION : On utilise rFree pour ne parcourir que les blocs libres
        BlockHeader* current = (BlockHeader*)page->firstFreeBlock;

        while (current) 
        {
            // Note : On sait qu'il est libre car on parcourt la liste lFree/rFree
            U64 headerEnd = (U64)((U8*)current + sizeof(BlockHeader));
            U64 padding = (align - (headerEnd % align)) % align;
            U64 totalNeeded = size + padding + sizeof(BlockHeader);

            if (current->size >= totalNeeded) 
            {
                // --- MISE À JOUR LISTE LIBRE ---
                // On débranche 'current' de la liste des blocs libres
                if (current->lFree) current->lFree->rFree = current->rFree;
                if (current->rFree) current->rFree->lFree = current->lFree;
                if ((U8*)current == page->firstFreeBlock) 
                {
                    page->firstFreeBlock = (U8*)current->rFree;
                }

                // --- SPLIT ---
                U64 remaining = current->size - totalNeeded;
                if (remaining > (sizeof(BlockHeader) + 32)) 
                {
                    BlockHeader* nextB = (BlockHeader*)((U8*)current + totalNeeded);
                    
                    nextB->canary = 0x494E4741;
                    nextB->size = remaining;
                    nextB->used = INGA_FALSE;
                    nextB->groupId = groupId;
                    nextB->pageId = static_cast<U16>(p);
                    
                    // Liens physiques
                    nextB->next = current->next;
                    nextB->previous = current;
                    if (current->next) 
                    {
                        current->next->previous = nextB;
                    }
                    current->next = nextB;

                    // --- MISE À JOUR LISTE LIBRE (nextB) ---
                    // On insère le nouveau bloc libre en tête de liste
                    nextB->lFree = nullptr;
                    nextB->rFree = (BlockHeader*)page->firstFreeBlock;
                    if (page->firstFreeBlock) 
                    {
                        ((BlockHeader*)page->firstFreeBlock)->lFree = nextB;
                    }
                    page->firstFreeBlock = (U8*)nextB;
                    
                    current->size = totalNeeded;
                }

                current->used = INGA_TRUE;
                current->payloadSize = size;
#ifdef INGA_DEBUG
                current->file = file;
                current->line = line;
#endif

                INGA_MUTEX_UNLOCK(&group->mutex); 
                return (void*)(headerEnd + padding);
            }
            // MODIFICATION : On passe au bloc LIBRE suivant
            current = current->rFree;
        }
    }

    // 4. EXTENSION (Nouvelle Page)
    if (createNewPage(group)) 
    {
        // Pour éviter de dupliquer la logique complexe de split/liens libres,
        // on appelle récursivement alloc. Le mutex sera repris proprement.
        INGA_MUTEX_UNLOCK(&group->mutex);
        return alloc(size, align, groupId, file, line);
    }

    INGA_MUTEX_UNLOCK(&group->mutex);
    return nullptr;
}

void Allocator::free(void* ptr)
{
  INGA_INSTRUMENT_FREE();
// 1. SÉCURITÉ
    if (!ptr) 
    {
        return;
    }

    // On récupère le header (Juste avant le payload)
    BlockHeader* header = (BlockHeader*)((U8*)ptr - sizeof(BlockHeader));
    
    // Vérification du Canary pour éviter de libérer n'importe quoi
    if (header->canary != 0x494E4741) 
    {
        INGA_ASSERT_RAW(header->canary == 0x494E4741, "Attempt to free invalid or corrupted pointer!");
        // Log d'erreur possible ici : "Tentative de libération d'un pointeur invalide"
        return;
    }

    INGA_ASSERT_RAW(header->used == INGA_TRUE, "Double free detected!");

    MemoryGroup* group = &g_groups[header->groupId];
    MemoryPage* page = &group->pages[header->pageId];

    // --- ZONE CRITIQUE ---
    INGA_MUTEX_LOCK(&group->mutex);

    header->used = INGA_FALSE;
    header->payloadSize = 0;

    // 2. FUSION PHYSIQUE (Coalescence)
    // On fusionne avec le voisin suivant s'il est libre
    if (header->next && !header->next->used) 
    {
        BlockHeader* nextB = header->next;

        // CRUCIAL : On débranche nextB de la liste LIBRE avant de le faire disparaître
        if (nextB->lFree) nextB->lFree->rFree = nextB->rFree;
        if (nextB->rFree) nextB->rFree->lFree = nextB->lFree;
        if ((U8*)nextB == page->firstFreeBlock) 
        {
            page->firstFreeBlock = (U8*)nextB->rFree;
        }

        // Fusion physique
        header->size += nextB->size;
        header->next = nextB->next;
        if (header->next) 
        {
            header->next->previous = header;
        }
    }

    // On fusionne avec le voisin précédent s'il est libre
    if (header->previous && !header->previous->used) 
    {
        BlockHeader* prevB = header->previous;

        // CRUCIAL : On débranche prevB de la liste LIBRE car il va être agrandi
        if (prevB->lFree) prevB->lFree->rFree = prevB->rFree;
        if (prevB->rFree) prevB->rFree->lFree = prevB->lFree;
        if ((U8*)prevB == page->firstFreeBlock) 
        {
            page->firstFreeBlock = (U8*)prevB->rFree;
        }

        // Fusion physique : prevB absorbe le bloc actuel (header)
        prevB->size += header->size;
        prevB->next = header->next;
        if (header->next) 
        {
            header->next->previous = prevB;
        }
        
        // Le bloc de référence devient le précédent
        header = prevB;
    }

    // 3. RÉINSERTION DANS LA LISTE LIBRE
    // Maintenant que le bloc est possiblement plus gros, on le remet en tête de liste libre
    header->lFree = nullptr;
    header->rFree = (BlockHeader*)page->firstFreeBlock;
    
    if (page->firstFreeBlock) 
    {
        ((BlockHeader*)page->firstFreeBlock)->lFree = header;
    }
    
    page->firstFreeBlock = (U8*)header;

    INGA_MUTEX_UNLOCK(&group->mutex);
}


void Allocator::printStats()
{
  if (!g_is_initialized)
    {
        printf("[InGa] Allocateur non initialise.\n");
        return;
    }

    INGA_MUTEX_LOCK(&g_global_mutex);

    printf("\n=== INGA MEMORY ENGINE STATS ===\n");
    printf("Groupes actifs : %u / %u\n", g_group_count, g_max_groups);

    for (U32 i = 0; i < g_group_count; ++i)
    {
        MemoryGroup* group = &g_groups[i];
        printf("\nGroupe [%u] : %s\n", group->id, group->name);
        printf("  Taille Page : %" PRIu64" octets | Pages allouees : %u\n", group->pageSize, group->pageCount);

        U64 totalUsed = 0;
        U64 totalFree = 0;
        U32 usedBlocks = 0;
        U32 freeBlocks = 0;

        for (U32 p = 0; p < group->pageCount; ++p)
        {
            MemoryPage* page = &group->pages[p];
            // On parcourt PHYSIQUEMENT la page via 'next'
            BlockHeader* current = (BlockHeader*)page->data;
            
            printf("  Page %u :\n", p);

            while (current)
            {
                if (current->used)
                {
                    totalUsed += current->payloadSize;
                    usedBlocks++;
                    #ifdef INGA_DEBUG
                    printf("    [OCCUPE] %" PRIu64 " octets (%s:%d)\n", 
                           current->payloadSize, current->file, current->line);
                    #else
                    printf("    [OCCUPE] %" PRIu64 " octets\n", current->payloadSize);
                    #endif
                }
                else
                {
                    totalFree += current->size;
                    freeBlocks++;
                    printf("    [LIBRE ] %" PRIu64 " octets (Total bloc avec header)\n", current->size);
                }
                current = current->next;
            }
        }

        printf("  >> Recap : %u blocs occupes (%" PRIu64 " B) | %u blocs libres (%" PRIu64 " B)\n", 
               usedBlocks, totalUsed, freeBlocks, totalFree);
    }
    printf("================================\n\n");
#ifdef INGA_DEBUG
    printf("\n=== PERFORMANCE DE L'ALLOCATEUR ===\n");
    
    if (g_alloc_calls > 0)
    {
        double avgAlloc = (double)g_total_alloc_time_us / (double)g_alloc_calls;
        printf("  Allocations : %" PRIu64 " appels | Moyenne : %.3f us\n", 
               g_alloc_calls, avgAlloc);
    }
    else 
    {
        printf("  Allocations : Aucun appel enregistré.\n");
    }

    if (g_free_calls > 0)
    {
        double avgFree = (double)g_total_free_time_us / (double)g_free_calls;
        printf("  Libérations : %" PRIu64 " appels | Moyenne : %.3f us (fusions incl.)\n", 
               g_free_calls, avgFree);
    }
    else
    {
        printf("  Libérations : Aucun appel enregistré.\n");
    }
    
    printf("==================================\n");
#endif

    INGA_MUTEX_UNLOCK(&g_global_mutex);
}

void Allocator::stop()
{
    if (!g_is_initialized) return;

    U64 totalLeaked = 0;
    U32 leakCount = 0;

    printf("\n[InGa] Verification des fuites memoire avant fermeture...\n");
    printStats();

    // 1. SCAN DE TOUS LES GROUPES
    for (U32 i = 0; i < g_group_count; ++i)
    {
        MemoryGroup* group = &g_groups[i];
        
        for (U32 p = 0; p < group->pageCount; ++p)
        {
            MemoryPage* page = &group->pages[p];
            BlockHeader* current = (BlockHeader*)page->data;

            // Parcours physique de la page
            while (current)
            {
                if (current->used)
                {
                    leakCount++;
                    totalLeaked += current->payloadSize;

                    #ifdef INGA_DEBUG
                    printf("  !! LEAK !! Bloc de %" PRIu64 " octets non libere | Alloue dans : %s:%d\n", 
                           current->payloadSize, current->file, current->line);
                    #else
                    printf("  !! LEAK !! Bloc de %" PRIu64 " octets non libere (Activez INGA_DEBUG pour la source)\n", 
                           current->payloadSize);
                    #endif
                }
                current = current->next;
            }
        }
    }

    // 2. LOGIQUE DE CRASH SI LEAKS
    if (leakCount > 0)
    {
        printf("\n[FATAL ERROR] L'allocateur a detecte %u fuites memoire (%" PRIu64 " octets) !\n", leakCount, totalLeaked);
        printf("[FATAL ERROR] Corrigez vos free() avant de pouvoir fermer le moteur.\n");
        
        // On force le crash pour "agacer" le programmeur
        // __builtin_trap() est excellent sur Clang/GCC, sinon un déréférencement nul
        #if defined(_MSC_VER)
            __debugbreak(); 
        #else
            __builtin_trap();
        #endif
    }

    // 3. NETTOYAGE (Si aucun leak, on arrive ici)
    for (U32 i = 0; i < g_group_count; ++i)
    {
        MemoryGroup* group = &g_groups[i];
        INGA_MUTEX_DESTROY(&group->mutex);

        for (U32 p = 0; p < group->pageCount; ++p)
        {
            ::free(group->pages[p].data);
        }
        ::free(group->pages);
    }

    ::free(g_groups);
    g_is_initialized = INGA_FALSE;

    INGA_MUTEX_DESTROY(&g_global_mutex);
    
    printf("[InGa] Allocateur arrete proprement. Aucune fuite detectee.\n");
}

void* Allocator::realloc(void* ptr, U64 newSize, U32 align, const char* file, I32 line)
{
  // 1. CAS PARTICULIERS STANDARDS
    if (!ptr) return alloc(newSize, align, 0, file, line); 
    if (newSize == 0) 
    {
        free(ptr);
        return nullptr;
    }

    // 2. RÉCUPÉRATION DU HEADER ACTUEL
    BlockHeader* header = (BlockHeader*)((U8*)ptr - sizeof(BlockHeader));
    
    // Sécurité : on vérifie que c'est bien un bloc à nous
    INGA_ASSERT_RAW(header->canary == 0x494E4741, "Realloc sur un pointeur invalide !");

    MemoryGroup* group = &g_groups[header->groupId];
    INGA_MUTEX_LOCK(&group->mutex);

    // Calcul de l'espace actuel
    U64 currentTotalSize = header->size;

    // 3. OPTIMISATION : AGRANDISSEMENT SUR PLACE
    // On regarde si le voisin physique suivant est libre
    if (header->next && !header->next->used)
    {
        U64 totalPotentialSize = currentTotalSize + header->next->size;
        
        // Calcul du besoin réel avec alignement
        U64 headerEnd = (U64)((U8*)header + sizeof(BlockHeader));
        U64 padding = (align - (headerEnd % align)) % align;
        U64 totalNeeded = newSize + padding + sizeof(BlockHeader);

        if (totalPotentialSize >= totalNeeded)
        {
            // --- MISE À JOUR LISTE LIBRE ---
            // On débranche le voisin suivant car il va être absorbé
            MemoryPage* page = &group->pages[header->pageId];
            BlockHeader* nextB = header->next;

            if (nextB->lFree) nextB->lFree->rFree = nextB->rFree;
            if (nextB->rFree) nextB->rFree->lFree = nextB->lFree;
            if ((U8*)nextB == page->firstFreeBlock) 
            {
                page->firstFreeBlock = (U8*)nextB->rFree;
            }

            // --- ABSORPTION PHYSIQUE ---
            header->size = totalPotentialSize;
            header->next = nextB->next;
            if (header->next) 
            {
                header->next->previous = header;
            }

            // Mise à jour des infos de debug et taille
            header->payloadSize = newSize;
#ifdef INGA_DEBUG
            header->file = file;
            header->line = line;
#endif

            INGA_MUTEX_UNLOCK(&group->mutex);
            return ptr; // L'adresse ne change pas, pas besoin de memcpy !
        }
    }

    // 4. DÉPLACEMENT OBLIGATOIRE
    // Si on arrive ici, on ne peut pas agrandir sur place
    INGA_MUTEX_UNLOCK(&group->mutex); 
    
    // On alloue un nouveau bloc
    void* newPtr = alloc(newSize, align, header->groupId, file, line);
    if (newPtr)
    {
        // On copie l'ancienne donnée vers la nouvelle destination
        U64 copySize = (header->payloadSize < newSize) ? header->payloadSize : newSize;
        ::memcpy(newPtr, ptr, copySize);
        
        // On libère l'ancien bloc (qui gérera ses propres fusions)
        free(ptr);
    }

    return newPtr;
}

}


void* operator new(size_t size)
{
    #ifdef INGA_DEBUG
        return Inga::Allocator::alloc((U64)size, 16, 0, "Global New", 0);
    #else
        return Inga::Allocator::alloc((U64)size, 16, 0, nullptr, 0);
    #endif
}

void* operator new[](size_t size)
{
    #ifdef INGA_DEBUG
        return Inga::Allocator::alloc((U64)size, 16, 0, "Global New[]", 0);
    #else
        return Inga::Allocator::alloc((U64)size, 16, 0, nullptr, 0);
    #endif
}

void operator delete(void* ptr) noexcept
{
    Inga::Allocator::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    Inga::Allocator::free(ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept {
    (void)size; // On ignore la taille car notre header la connaît déjà
    Inga::Allocator::free(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept {
    (void)size;
    Inga::Allocator::free(ptr);
}

#include <cstddef>

// --- Implémentations Debug ---
#ifdef INGA_DEBUG
void* operator new(size_t size, const char* file, int line)
{
    return Inga::Allocator::alloc((U64)size, 16, 0, file, line);
}

void* operator new[](size_t size, const char* file, int line)
{
    return Inga::Allocator::alloc((U64)size, 16, 0, file, line);
}

void operator delete(void* ptr, [[maybe_unused]] const char* file, [[maybe_unused]] int line) noexcept
{
  
    Inga::Allocator::free(ptr);
}
#endif
