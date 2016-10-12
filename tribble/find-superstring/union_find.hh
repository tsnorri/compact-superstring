/*
 Copyright (c) 2016 Jarno Alanko
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see http://www.gnu.org/licenses/ .
 */

#include <sdsl/int_vector.hpp>

#ifndef TRIBBLE_UNIONFIND_HH
#define TRIBBLE_UNIONFIND_HH

class UnionFind {
    public:
        typedef std::size_t size_type;
    
    private:
        size_type n_elements; // Number of elements in structure
        sdsl::int_vector<> sizes; // Subtree sizes
        sdsl::int_vector<> parents; // Parent pointers
        UnionFind& operator = (const UnionFind&); // Assignment operator not implemented
        UnionFind (const UnionFind&); // Copy constructor not implemented

        sdsl::int_vector<> path; // Reusable space for path compression

        static inline std::size_t bits_for_n(size_type const n)
        {
            return 1 + sdsl::bits::hi(n);
        }
    
        UnionFind(size_type const n, size_type const bits_for_n):
            sizes(n, 0, bits_for_n),
            parents(n, 0, bits_for_n),
            path(n, 0, bits_for_n)
        {
            initialize(n);
        }

    public:
        size_type find(size_type id); // Returns the identifier of the set that contains the parameter id
        void doUnion(size_type id_1, size_type id_2); // Merges the two sets given by the identifiers
        size_type getSize(size_type id); // Returns the size of the set with identified id
        void initialize(size_type n_elements); // Re-initializes the structure with n elements.

        UnionFind(size_type const n):
            UnionFind(n, bits_for_n(n))
        {
        }
};

#endif
