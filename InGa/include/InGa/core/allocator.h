#ifndef INGA_ALLOCATOR_H
#define INGA_ALLOCATOR_H

#include <new>
#include <InGa/core/export.h>
#include <InGa/core/inga_platform.h>

namespace Inga
{
    // Structure simple pour la configuration des groupes
    struct AllocationGroupInfo
    {
        const char* Name;
        U64 PageSize;
    };

    class INGA_API Allocator
    {
    public:
        // Gestion du cycle de vie du moteur de mémoire
        static B8 start(U32 maxGroups, U64 defaultPageSize);
        static void stop();

        // Gestion des groupes
        static U16 addGroup(const AllocationGroupInfo& info);
        static U16 setGroupIdByName(const char* name);

        // Fonctions d'allocation de base (Le moteur utilise celles-ci)
        static void* alloc(U64 size, U32 align, U16 groupId, const char* file, I32 line);
        static void* realloc(void* ptr, U64 size, U32 align, const char* file, I32 line);
        static void  free(void* ptr);

        // Monitoring et Debug
        static void printStats();
    };
}

extern "C++"{
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;

// --- Signatures Spécifiques au Debug ---
#ifdef INGA_DEBUG
    // Permet le tracking précis : new(__FILE__, __LINE__) MyClass
    void* operator new(size_t size, const char* file, int line);
    void* operator new[](size_t size, const char* file, int line);
    void operator delete(void* ptr, const char* file, int line) noexcept;
#endif
}

// --- Macros Utilisateur (La façade "facile") ---
#define INGA_ALLOC(size) \
    Inga::Allocator::alloc(size, 16, 0, __FILE__, __LINE__)

#define INGA_ALLOC_ALIGN(size, align) \
    Inga::Allocator::alloc(size, align, 0, __FILE__, __LINE__)

#define INGA_ALLOC_FROM(size, groupId) \
    Inga::Allocator::alloc(size, 16, groupId, __FILE__, __LINE__)

#define INGA_ALLOC_FROM_ALIGN(size, align, groupId) \
    Inga::Allocator::alloc(size, align, groupId, __FILE__, __LINE__)

#define INGA_REALLOC(ptr, size) \
    Inga::Allocator::realloc(ptr, size, 16, __FILE__, __LINE__)

#define INGA_FREE(ptr) \
    Inga::Allocator::free(ptr)

#ifdef INGA_DEBUG
    // En Debug, INGA_NEW utilise la version avec tracking
    #define INGA_NEW new(__FILE__, __LINE__)
#else
    // En Release, INGA_NEW devient un simple new standard
    #define INGA_NEW new
#endif

#endif // INGA_ALLOCATOR_H
