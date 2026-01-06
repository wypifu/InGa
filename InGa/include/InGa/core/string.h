#ifndef INGA_STRING_H
#define INGA_STRING_H

#include "stl_allocator.h"
#include <string>

namespace Inga 
{

class String 
{
public:
        using InternalString = std::basic_string<char, std::char_traits<char>, StlAllocator<char>>;

        // --- Constructeurs ---
        String() = default;

        String(const char* str) 
             : m_data(str ? str : "", StlAllocator<char>())
        {
        }

        // Constructeur de copie
        String(const String& other) 
            : m_data(other.m_data) 
        {
        }

        // Constructeur de déplacement (très important pour la performance)
        String(String&& other) noexcept 
            : m_data(std::move(other.m_data)) 
        {
        }

        // --- Méthodes de gestion de taille ---
        void resize(size_t n) 
        { 
            m_data.resize(n); 
        }

        size_t length() const 
        { 
            return m_data.length(); 
        }

        bool empty() const 
        { 
            return m_data.empty(); 
        }

        const char* c_str() const 
        { 
            return m_data.c_str(); 
        }

        // --- Accès et Opérateurs ---
        char& operator[](size_t index) 
        { 
            return m_data[index]; 
        }

        const char& operator[](size_t index) const 
        { 
            return m_data[index]; 
        }

        String& operator=(const String& other) 
        { 
            m_data = other.m_data; 
            return *this; 
        }

        String& operator=(String&& other) noexcept 
        { 
            m_data = std::move(other.m_data); 
            return *this; 
        }

        // --- Concaténation ---
        String& operator+=(const String& other)
        {
            m_data += other.m_data;
            return *this;
        }

        friend String operator+(String lhs, const String& rhs)
        {
            lhs += rhs;
            return lhs;
        }

    private:
        InternalString m_data;
    };
}
#endif

