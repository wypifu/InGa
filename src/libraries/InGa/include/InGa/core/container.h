#ifndef INGA_CONTAINER_H
#define INGA_CONTAINER_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <deque>
#include <string>
#include <limits>
#include "allocator.h"

namespace Inga {

/**
 * Adaptateur STL pour l'allocateur InGa.
 * Permet aux containers standards d'utiliser ta gestion de pages et de groupes.
 */
template<typename T>
struct StlAllocator {
    typedef T value_type;

    StlAllocator() noexcept = default;
    template<typename U> constexpr StlAllocator(const StlAllocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();

        // On redirige vers ton système de pages. 
        // Par défaut, on utilise le groupe 0 (Général) pour la STL.
        void* p = Inga::Allocator::alloc(n * sizeof(T), alignof(T), 0, "STL_Internal", 0);
        
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        (void)n;
        Inga::Allocator::free(p);
    }

    // Compatibilité
    bool operator==(const StlAllocator&) const noexcept { return true; }
    bool operator!=(const StlAllocator&) const noexcept { return false; }
};

// --- ALIAS DE TYPES (Pour un code plus court et typé InGa) ---

template<typename T> 
using Vector = std::vector<T, StlAllocator<T>>;

template<typename K, typename V>
using UnorderedMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, StlAllocator<std::pair<const K, V>>>;

template<typename T>
using UnorderedSet = std::unordered_set<T, std::hash<T>, std::equal_to<T>, StlAllocator<T>>;

template<typename T>
using List = std::list<T, StlAllocator<T>>;

template<typename T>
using Deque = std::deque<T, StlAllocator<T>>;

// Utilisation d'IngaAllocator pour les chaînes de caractères
using String = std::basic_string<char, std::char_traits<char>, StlAllocator<char>>;

/**
 * Utilitaire de formatage de chaîne de caractères.
 * Alloue directement la mémoire finale via InGa pour éviter les copies inutiles.
 */
template<typename ... Args>
String StringFormat(const char* format, Args ... args) {
    // 1. On demande à snprintf la taille nécessaire (en passant nullptr)
    int size_s = std::snprintf(nullptr, 0, format, args...) + 1; 
    if (size_s <= 0) return String("");

    size_t size = static_cast<size_t>(size_s);
    
    // 2. On pré-alloue le String InGa (appelle ton allocateur en interne)
    String s;
    s.resize(size - 1); 

    // 3. On écrit directement dans le buffer du String
    std::snprintf(&s[0], size, format, args...);
    
    return s;
}

} // namespace Inga

#endif // INGA_CONTAINER_H
