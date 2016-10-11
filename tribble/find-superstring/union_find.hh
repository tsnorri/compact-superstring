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

#include <vector>
#include <map>

#ifndef TRIBBLE_UNIONFIND_HH
#define TRIBBLE_UNIONFIND_HH

class UnionFind {
    private:
        int64_t n_elements; // Number of elements in structure
        std::vector<int64_t> sizes; // Subtree sizes
        std::vector<int64_t> parents; // Parent pointers
        UnionFind& operator = (const UnionFind&); // Assignment operator not implemented
        UnionFind (const UnionFind&); // Copy constructor not implemented

        std::vector<int64_t> path; // Reusable space for path compression

    public:
        int64_t find(int64_t id); // Returns the identifier of the set that contains the parameter id
        void doUnion(int64_t id_1, int64_t id_2); // Merges the two sets given by the identifiers
        int64_t getSize(int64_t id); // Returns the size of the set with identified id
        void initialize(int64_t n_elements); // Re-initializes the structure with n elements.

        UnionFind(int64_t n_elements);

};

#endif