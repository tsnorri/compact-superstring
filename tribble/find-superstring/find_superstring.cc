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

#include "find_superstring.hh"


void find_superstring_match_dummy_callback::set_substring_count(std::size_t set_substring_count)
{
	// No-op.
}


void find_superstring_match_dummy_callback::set_alphabet(alphabet_type const &alphabet)
{
	// No-op.
}


void find_superstring_match_dummy_callback::set_strings_stream(std::istream &strings_stream)
{
	// No-op.
}


void find_superstring_match_dummy_callback::set_is_unique_vector(sdsl::bit_vector const &vec)
{
	// No-op.
}


bool find_superstring_match_dummy_callback::callback(
	std::size_t const read_lex_rank,
	std::size_t const match_length,
	std::size_t const match_sa_begin,
	std::size_t const match_sa_end
)
{
	std::cerr
	<< "*** Found match for substring " << read_lex_rank << " of length " << match_length
	<< ": [" << match_sa_begin << ", " << match_sa_end << "]" << std::endl;
	
	return false;
}


void find_superstring_match_dummy_callback::finish_matching()
{
	// No-op.
}


void find_superstring_match_dummy_callback::build_final_superstring(std::ostream &)
{
	// No-op.
}
