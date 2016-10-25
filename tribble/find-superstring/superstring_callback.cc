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

#include "superstring_callback.hh"
#include <iostream>
#include <cassert>
#include <algorithm>


using namespace tribble;

Superstring_callback::Superstring_callback() 
:  UF(0), merges_done(0), n_strings(-1), n_unique_strings(-1) {}

bool Superstring_callback::try_merge(std::size_t left_string, std::size_t right_string, std::size_t overlap_length){

	assert(left_string < leftend.size());
	assert(right_string < leftend.size());
	assert(right_string < rightavailable.size());
	assert(rightavailable[right_string]);

	if(leftend[left_string] != right_string){
		string_successor[left_string] = right_string;
		overlap_lengths[left_string] = overlap_length; 
		make_not_right_available(right_string);
		leftend[rightend[right_string]] = leftend[left_string];
		rightend[leftend[left_string]] = rightend[right_string];
		merges_done++;
		
		if (DEBUGGING_OUTPUT)
		{
			std::cout << "Merged " << left_string << " " << right_string << std::endl;
		}
		return true;
	}
	return false;
}


void Superstring_callback::set_substring_count(std::size_t count){
	n_strings = count;
	std::size_t const bits_for_count(1 + sdsl::bits::hi(count));
	std::size_t const bits_for_next(1 + sdsl::bits::hi(1 + count));
	
	overlap_lengths.width(40); // TODO: max string length
	string_successor.width(bits_for_count + 1);
	leftend.width(bits_for_count);
	rightend.width(bits_for_count);
	next.width(bits_for_next);
	
	overlap_lengths.resize(n_strings);
	string_successor.resize(n_strings);
	leftend.resize(n_strings);
	rightend.resize(n_strings);
	next.resize(n_strings);
	
	rightavailable.resize(n_strings);
	
	for(std::size_t i = 0; i < n_strings; i++){
		overlap_lengths[i] = 0;
		string_successor[i] = n_strings;
		leftend[i] = i;
		rightend[i] = i;
		next[i] = i+1;
		rightavailable[i] = true;
		
	}
	UF.initialize(n_strings);
}

void Superstring_callback::set_strings_stream(std::istream &stream){
	strings_stream = &stream;
}

void Superstring_callback::set_is_unique_vector(sdsl::bit_vector const &vec){
	
	this->is_unique = &vec;
	this->n_unique_strings = 0;
	for(int64_t i = 0; i < vec.size(); i++){
		if(vec[i] == 0) make_not_right_available(i);
		else n_unique_strings++;
	}
	
}

/*  alphabet_type:
		public:
		typedef int_vector<>::size_type size_type;
		typedef int_vector<8>		   char2comp_type;
		typedef int_vector<8>		   comp2char_type;
		typedef int_vector<64>		  C_type;
		typedef uint16_t				sigma_type;
		typedef uint8_t				 char_type;
		typedef uint8_t				 comp_char_type;
		typedef std::string			 string_type;
		enum { int_width = 8 };

		typedef byte_alphabet_tag	   alphabet_category;
	private:
		char2comp_type m_char2comp; // Mapping from a character into the compact alphabet.
		comp2char_type m_comp2char; // Inverse mapping of m_char2comp.
		C_type		 m_C;		 // Cumulative counts for the compact alphabet [0..sigma].
		sigma_type	 m_sigma;	 // Effective size of the alphabet.

		void copy(const byte_alphabet&);
	public:

		const char2comp_type& char2comp;
		const comp2char_type& comp2char;
		const C_type&		 C;
		const sigma_type&	 sigma;
*/

void Superstring_callback::set_alphabet(alphabet_type const &alphabet){
	this->alphabet = alphabet;
}


bool Superstring_callback::callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end){

	// Change to 0-based indexing
	read_lex_rank -= 2; 
	match_sa_begin -= 2;
	match_sa_end -= 2;
		
	// Check that n_strings and u_unique_string have been initialized
	assert(n_strings != -1);
	assert(n_unique_strings != -1);
	
	// Sanity check
	assert(read_lex_rank >= 0 && read_lex_rank < n_strings);
	
	if((*is_unique)[read_lex_rank] == 0) return false;
	if(merges_done >= n_unique_strings - 1) return true; // No more merges can be done
	
	// Find two indices in the match_sa range that are right-available, and
	// try to merge read_lex_rank to one of them
	
	std::size_t k; // Position of the next one-bit in right-available
	if(match_sa_begin > 0) k = get_next_right_available(match_sa_begin-1);
	else {
		if(rightavailable[0] == 1) k = 0;
		else k = get_next_right_available(match_sa_begin);
	}
	
	if(k > match_sa_end) {
		// Next one is outside of the suffix array interval, or not found at all
		return false;
	}
	
	if(try_merge(read_lex_rank, k, match_length)) return true;
	
	// Failed, try again a second time
	k = get_next_right_available(k);
	
	if(k > match_sa_end) {
		// Next one is outside of the suffix array interval, or not found at all
		return false;
	}
	
	if(try_merge(read_lex_rank, k, match_length)) return true;
	
	return false; // Should never come here, because the second try should always be succesful
	
}


void Superstring_callback::make_not_right_available(std::size_t index){

	
	// the position of the next one-bit in right-available
	std::size_t q = next[UF.find(index)];
	
	// Merge to the right if needed
	if(index < n_strings-1 && rightavailable[index+1] == 0)
		UF.doUnion(UF.find(index), UF.find(index+1));
	
	// Merge to the left
	if(index > 0)
		UF.doUnion(UF.find(index), UF.find(index-1));
	
	rightavailable[index] = 0;
	next[UF.find(index)] = q;
}



std::size_t Superstring_callback::get_next_right_available(std::size_t index){
	std::size_t k = UF.find(index);
	if(k == n_strings) return k;
	else return next[k];
}

/* Simple O(n^2) implementation for testing purposes
std::size_t Superstring_callback::get_next_right_available(std::size_t index){

	for(std::size_t i = index+1; i < n_strings; i++){
		assert(i < rightavailable.size());
		if(rightavailable[i]) 
			return i;
	}
	return n_strings;
}*/

void Superstring_callback::write_string(int64_t string_start, int64_t skip, std::ostream& out, sdsl::int_vector<0>& concatenation){
	int64_t k = string_start;
	char c = alphabet.comp2char[concatenation[k]];
	while(c != '#'){
		if(skip > 0) skip--;
		else out << c;
		
		k++;
		c = alphabet.comp2char[concatenation[k]];
	}
}

void Superstring_callback::do_path(int64_t start_string, std::ostream& out, sdsl::int_vector<0>& concatenation, sdsl::int_vector<0>& string_start_points){
	// Concatenate all strings putting in the overlapping region of adjacent strings only once
	// Write out the first string as a whole
	write_string(string_start_points[start_string], 0, out, concatenation);
	
	int64_t current_string_idx = start_string;
	while(string_successor[current_string_idx] != n_strings){ // While there exists a successor
		int64_t left_string = current_string_idx;
		int64_t right_string = string_successor[current_string_idx];
		int64_t overlap = overlap_lengths[left_string];
		
		// Write the right string skipping the first 'overlap' characters
		write_string(string_start_points[right_string], overlap, out, concatenation);
		current_string_idx = right_string;
	}
}

void Superstring_callback::build_final_superstring(std::ostream& out){

	// Assuming the stream 'out' contains the concatenation of all strings
	// separated by the '#' character i.e.
	// #s1#s2#s3#s3#s4#s5#s6#
	
	// Read the strings from the input stream given earlier with set_strings_stream
	sdsl::int_vector<0> concatenation; // Reading the contents of the input stream into here
	concatenation.width(1 + sdsl::bits::hi(alphabet.sigma));
	concatenation.resize(1); // Initial size. Will be doubled every time it becomes full
	
	sdsl::int_vector<0> string_start_points; // Starting point of each string in the concatenation
	string_start_points.width(40); // TODO: should be the total length of the concatenation
	string_start_points.resize(n_strings);
	
	int64_t concatenation_index = 0; // For appending characters to the concatenation
	int64_t n_strings_read = 0; // Strings read so far
	
	char c;
	while(*strings_stream >> c){

		// If out of space, double the length of the strings array
		if(concatenation_index == concatenation.size()){
			concatenation.resize(concatenation.size() * 2);
		}
		concatenation[concatenation_index] = alphabet.char2comp[c];
		
		// Record the starting points of strings
		if(n_strings_read < n_strings && c == '#'){
			string_start_points[n_strings_read] = concatenation_index+1;
			n_strings_read++;
		}
		concatenation_index++;
	}
	
	concatenation.resize(concatenation_index);
	
	assert(n_strings_read != 0);
	
	std::size_t current_string_idx = n_strings;
	
	// Concatenate all paths in the graph defined by the string_successor array
	for(std::size_t i = 0; i < n_strings; i++){
		if(rightavailable[i]){ // successor to none of the strings i.e. start of a path
			do_path(i, out, concatenation, string_start_points);
		}
	}
}

