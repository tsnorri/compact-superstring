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


#ifndef TRIBBLE_SUPERSTRING_CALLBACK_HH
#define TRIBBLE_SUPERSTRING_CALLBACK_HH

#include <cstddef> // std::size_t
#include <vector>
#include <string>
#include <tuple>
#include "find_superstring.hh"

class Superstring_callback : public find_superstring_match_callback {
  
public:
    
	Superstring_callback();
	
	// Returns true if merge was successful, or if no more merges can be done
	bool callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end) override;
	void set_substring_count(std::size_t count) override;
	
	std::string build_final_superstring(std::vector<std::string> strings); // Call after all prefix-suffix overlaps have been considered
	
	~Superstring_callback(){
		std::cout << "destructor" << std::endl;
	}
    
private:
	
	// Returns n_strings if not found, else the index of the next one-bit in rightavailable to the right of index
	// Parameter index can be zero. In that case returns the first one-bit in the entire rightavailable array.
	std::size_t get_next_right_available(std::size_t index);
	
	// Merges two strings if the merge would not create a cycle
	bool try_merge(std::size_t left_string, std::size_t right_string, std::size_t overlap_length);
	
	std::size_t merges_done;
	
	int64_t n_strings;
	// todo: use sdsl containers
	std::vector<std::size_t> leftend;
	std::vector<std::size_t> rightend;
	std::vector<std::size_t> next;
	std::vector<bool> rightavailable;
	std::vector<std::tuple<std::size_t,std::size_t,std::size_t> > merges; // triples (left read, right read, overlap length)
	
};

#endif