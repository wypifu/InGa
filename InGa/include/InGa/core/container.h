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
#include "stl_allocator.h"
#include "string.h"

namespace Inga {

/**
 * Adaptateur STL pour l'allocateur InGa.
 * Permet aux containers standards d'utiliser ta gestion de pages et de groupes.
 */

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


/**
 * Utilitaire de formatage de chaîne de caractères.
 * Alloue directement la mémoire finale via InGa pour éviter les copies inutiles.
 */
template<typename ... Args>
Inga::String StringFormat(const char* format, Args ... args) {
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
