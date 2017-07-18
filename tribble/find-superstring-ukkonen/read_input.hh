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


#ifndef TRIBBLE_SUPERSTRING_READ_INPUT_HH
#define TRIBBLE_SUPERSTRING_READ_INPUT_HH


#include <tribble/io.hh>
#include "cmdline.h" // For enum_source_format
#include "find_superstring_ukkonen.hh"


namespace tribble {
	
	void read_input(
		tribble::file_istream &source_stream,
		enum_source_format const source_format,
		tribble::string_vector_type &strings,
		tribble::trie_type &trie,
		tribble::string_map_type &strings_by_state,
		tribble::state_map_type &states_by_string
	);
}

#endif
