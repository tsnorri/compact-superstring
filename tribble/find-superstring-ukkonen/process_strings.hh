/*
 Copyright (c) 2017 Tuukka Norri
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


#ifndef TRIBBLE_SUPERSTRING_PROCESS_STRINGS_HH
#define TRIBBLE_SUPERSTRING_PROCESS_STRINGS_HH


#include "find_superstring_ukkonen.hh"


namespace tribble {
	
	void process_strings(
		trie_type &trie,
		string_map_type const &strings_by_state,
		state_map_type const &states_by_string,
		next_string_map_type &dst,
		index_vector_type &start_positions
	);
}

#endif