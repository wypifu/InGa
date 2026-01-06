#ifndef INGA_STL_ALLOCATOR_H
#define INGA_STL_ALLOCATOR_H

#include "allocator.h"
#include <limits>
#include <new>
#include <type_traits>

namespace Inga {

template<typename T>
    struct StlAllocator
    {
        typedef T value_type;

        StlAllocator() noexcept = default;
        template<typename U> constexpr StlAllocator(const StlAllocator<U>&) noexcept {}

        // --- AJOUT : Opérateur d'affectation pour satisfaire la STL ---
        StlAllocator& operator=(const StlAllocator&) = default;

        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap            = std::true_type;

        [[nodiscard]] T* allocate(std::size_t n)
        {
            if (n == 0) return nullptr;
            void* p = Inga::Allocator::alloc(n * sizeof(T), alignof(T), 0, "STL_Internal", 0);
            if (!p) throw std::bad_alloc();
            return static_cast<T*>(p);
        }

        void deallocate(T* p, std::size_t n) noexcept
        {
            (void)n;
            Inga::Allocator::free(p);
        }

        bool operator==(const StlAllocator&) const noexcept { return true; }
        bool operator!=(const StlAllocator&) const noexcept { return false; }
    };
}
#endif
