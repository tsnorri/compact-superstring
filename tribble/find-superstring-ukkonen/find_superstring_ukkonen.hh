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


#include <aho_corasick/aho_corasick.hpp>
#include <deque>
#include <list>
#include <string>
#include <tribble/linked_list.hh>
#include <vector>
#include <unordered_map>


#ifndef TRIBBLE_SUPERSTRING_FIND_SUPERSTRING_UKKONEN_HH
#define TRIBBLE_SUPERSTRING_FIND_SUPERSTRING_UKKONEN_HH

#ifdef NDEBUG
#	define TRIBBLE_ASSERTIONS_ENABLED	(0)
#else
#	define TRIBBLE_ASSERTIONS_ENABLED	(1)
#endif

#ifndef DEBUGGING_OUTPUT
#	define DEBUGGING_OUTPUT 0
#endif


namespace tribble {
	
	struct next_string
	{
		static constexpr std::size_t const max_index{std::numeric_limits <std::size_t>::max()};
		
		std::size_t index{max_index};
		std::size_t overlap_length{0};
		
		next_string() = default;
		next_string(std::size_t index_, std::size_t overlap_length_):
			index(index_),
			overlap_length(overlap_length_)
		{
		}
	};
	
	
	typedef aho_corasick::basic_trie <char>								trie_type;
	typedef std::string													string_type;
	typedef std::vector <string_type>									string_vector_type;
	typedef std::unordered_map <trie_type::state_ptr_type, std::size_t>	string_map_type;
	typedef std::vector <trie_type::state_ptr_type>						state_map_type;
	typedef std::vector <std::size_t>									index_vector_type;
	typedef std::vector <index_vector_type>								index_vector_map_type;
	
	struct index_list
	{
		typedef std::size_t value_type;
		
		std::vector <value_type>	values;
		linked_list					indices;
		
		index_list():
			values(),
			indices(0, CHAR_BIT * sizeof(value_type))
		{
		}
		
		index_list(std::size_t const count):
			values(count),
			indices(count, CHAR_BIT * sizeof(value_type))
		{
		}
	};
	typedef std::vector <std::list <index_list>>						index_ll_map_type;
			
	typedef std::deque <trie_type::state_ptr_type>						state_ptr_queue_type;
	typedef std::vector <trie_type::state_ptr_type>						state_ptr_vector_type;
	typedef std::vector <next_string>									next_string_map_type;
	
	
	// Insert a string into the trie.
	bool insert(
		trie_type &trie,
		string_map_type &strings_by_state,
		string_type const &string,
		std::size_t const idx
	);
}

#endif
