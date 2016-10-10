/*
 Copyright (c) 2016 Tuukka Norri
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

#include "union_find.hh"
#include <utility>

using namespace std;

UnionFind::UnionFind(int64_t n_elements){
    initialize(n_elements);
}

void UnionFind::initialize(int64_t n_elements){
    this-> n_elements = n_elements;
    sizes.resize(n_elements);
    parents.resize(n_elements);
	for(int64_t i = 0; i < n_elements; i++){
		sizes[i] = 1;
		parents[i] = i;
	}
}

int64_t UnionFind::find(int64_t id){
    while(parents[id] != id) id = parents[id];
    return id;
}

void UnionFind::doUnion(int64_t id_1, int64_t id_2){
    int64_t x1 = find(id_1);
    int64_t x2 = find(id_2);
    if(x1 == x2) return; // Already in same set
    if(sizes[x1] <= sizes[x2]){
        parents[x1] = x2;
        sizes[x2] += sizes[x1];
    } else {
        parents[x2] = x1;
        sizes[x1] += sizes[x2];
    }
}

int64_t UnionFind::getSize(int64_t id){
    return sizes[find(id)];
}