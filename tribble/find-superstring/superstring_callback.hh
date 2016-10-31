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
	void set_is_unique_vector(sdsl::bit_vector const &vec) override;
	void finish_matching() override;
	
	// Prints the final superstring 'out'. Call only after all prefix-suffix overlaps have been considered
	// and set_alphabet, set_substring_count and set_strings_stream has been called
	void build_final_superstring(std::ostream& out) override;

private:
	
	// Returns n_strings if not found, else the index of the next one-bit in rightavailable to the right of index
	// Parameter index can be zero. In that case returns the first one-bit in the entire rightavailable array.
	std::size_t get_next_right_available(std::size_t index);
	
	// Sets rightavailable[index] = 0, and updates the union-find structure
	void make_not_right_available(std::size_t index);
	
	// Sets 'right_string' as the successor of 'left_string' if this does not create a cycle
	bool try_merge(std::size_t left_string, std::size_t right_string, std::size_t overlap_length);
	
	// Writes the string starting at index 'string_start' in the concatenation of strings 
	// with #-separators to 'out', skipping the first 'skip' characters
	void write_string(int64_t string_start, int64_t skip, std::ostream& out, sdsl::int_vector<0>& concatenation);
	
	// Writes to 'out' the chained merge of all reads on the path starting from 'start_string'
	void do_path(int64_t start_string, std::ostream& out, sdsl::int_vector<0>& concatenation, sdsl::int_vector<0>& string_start_points);
	
	UnionFind UF; // For finding the next one-bit in rightavailable quickly. See paper.
	
	std::size_t merges_done; // Number of merges done by try_merge
	int64_t n_strings; // Total number of input strings
	int64_t n_unique_strings; // The number of distinct strings that are not a substring of another
	
	sdsl::int_vector <> leftend; // See paper
	sdsl::int_vector <> rightend; // See paper
	sdsl::int_vector <> next; // See paper
	sdsl::bit_vector rightavailable; // See paper
	
	sdsl::int_vector <> string_successor; // read_successor[i] = the next read from i in the directed read graph
	// if read_successor[i] = n_string, it means the read has no successor
	
	sdsl::int_vector <> overlap_lengths; // overlap_lengths[i] = the length of the overlap of read i to the successor of read i
	
	alphabet_type alphabet;
	std::istream* strings_stream; // Raw pointer bad I know, I know
	const sdsl::bit_vector *is_unique;
};
}

#endif
