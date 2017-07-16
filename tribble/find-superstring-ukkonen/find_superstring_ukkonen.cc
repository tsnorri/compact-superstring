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


#include "find_superstring_ukkonen.hh"


namespace tribble {

	bool insert(
		trie_type &trie,
		string_map_type &strings_by_state,
		string_type const &string,
		std::size_t const idx
	)
	{
		auto const state_ptr(trie.insert(string));
		if (state_ptr)
		{
			assert(strings_by_state.cend() == strings_by_state.find(state_ptr));
			strings_by_state[state_ptr] = idx;
			return true;
		}
		
		return false;
	}
}
