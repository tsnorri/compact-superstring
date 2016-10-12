/*
 Copyright (c) 2016 Jarno Alanko, Tuukka Norri
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
#include <iostream>


void UnionFind::initialize(size_type n_elements){
	auto const bits(bits_for_n(n_elements));
	this->n_elements = n_elements;
	
	sizes.width(bits);
	parents.width(bits);
	sizes.resize(n_elements);
	parents.resize(n_elements);
	path.width(bits);
	path.resize(n_elements);
	
	for(size_type i = 0; i < n_elements; i++){
		sizes[i] = 1;
		parents[i] = i;
	}
}

auto UnionFind::find(size_type id) -> size_type {
	size_type count(0);
	while(parents[id] != id) { // while not at root
		path[count] = id;
		id = parents[id];
		++count;
	}
	
	// id is now root. Do path compression.
	for (std::size_t i(0); i < count; ++i) {
		auto const node(path[i]);
		parents[node] = id;
	}
	
	return id;
}

void UnionFind::doUnion(size_type id_1, size_type id_2){
	size_type x1 = find(id_1);
	size_type x2 = find(id_2);
	if(x1 == x2) return; // Already in same set
	if(sizes[x1] <= sizes[x2]){
		parents[x1] = x2;
		sizes[x2] += sizes[x1];
	} else {
		parents[x2] = x1;
		sizes[x1] += sizes[x2];
	}
}

auto UnionFind::getSize(size_type id) -> size_type {
	return sizes[find(id)];
}
