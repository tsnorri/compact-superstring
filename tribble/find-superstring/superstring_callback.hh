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
#include "merge.hh"
#include "union_find.hh"


namespace tribble {
class Superstring_callback : public find_superstring_match_callback {
  
public:
    
	Superstring_callback();
	
	// Returns true if merge was successful, or if no more merges can be done
	bool callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end) override;
	void set_substring_count(std::size_t count) override;
	void set_alphabet(alphabet_type const &alphabet) override;
	void set_strings_stream(std::istream &strings_stream) override;
	
	// Prints the final superstring to the given output stream
	void build_final_superstring(std::ostream& out); // Call after all prefix-suffix overlaps have been considered

private:
	
	// Returns n_strings if not found, else the index of the next one-bit in rightavailable to the right of index
	// Parameter index can be zero. In that case returns the first one-bit in the entire rightavailable array.
	std::size_t get_next_right_available(std::size_t index);
	
	// Sets rightavailable[index] = 0, and updates the union-find structure
	void make_not_right_available(std::size_t index);
	
	// Merges two strings if the merge would not create a cycle
	bool try_merge(std::size_t left_string, std::size_t right_string, std::size_t overlap_length);
	
	// Writes the string starting at index 'string_start' in the concatenation of strings 
	// with #-separators to 'out', skipping the first 'skip' characters
	void write_string(int64_t string_start, int64_t skip, std::ostream& out, sdsl::int_vector<0>& concatenation);

	UnionFind UF;
	
	std::size_t merges_done;
	
	int64_t n_strings;
	sdsl::int_vector <> leftend;
	sdsl::int_vector <> rightend;
	sdsl::int_vector <> next;
	sdsl::bit_vector rightavailable;
	
	detail::merge_array merges;
	
	alphabet_type alphabet;
	std::istream* strings_stream;
};
}

#endif
