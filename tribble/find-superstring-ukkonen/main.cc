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


#include <iostream>
#include <tribble/io.hh>
#include <tribble/timer.hh>
#include "cmdline.h"
#include "find_superstring_ukkonen.hh"
#include "read_input.hh"
#include "process_strings.hh"


void output_superstring(
	std::ostream &stream,
	tribble::string_vector_type const &strings,
	tribble::index_vector_type const &start_positions,
	tribble::next_string_map_type const &links,
	tribble::char_map_type const &comp2char
)
{
	// Output the superstring.
	for (auto const idx : start_positions)
	{
		for (auto const c : strings[idx])
			stream << comp2char[c];
		
		if (DEBUGGING_OUTPUT)
			stream << ' ';

		auto next_idx(idx);
		while (true)
		{
			auto const &ns(links[next_idx]);
			next_idx = ns.index;
			if (tribble::next_string::max_index == next_idx)
				break;
			
			auto const &str(strings[next_idx]);
			for (auto it(str.cbegin() + ns.overlap_length), end(str.cend()); it != end; ++it)
				stream << comp2char[*it];
			
			if (DEBUGGING_OUTPUT)
				stream << ' ';
		}
	}
	stream << std::endl;
}
		

int main(int argc, char **argv)
{
	std::cerr << "Assertions have been " << (TRIBBLE_ASSERTIONS_ENABLED ? "enabled" : "disabled") << '.' << std::endl;

	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	std::cin.tie(nullptr);					// We don't require any input from the user.

	tribble::file_ostream os;
	if (args_info.output_file_given)
		tribble::open_file_for_writing(args_info.output_file_arg, os);

	tribble::trie_type trie;
	trie.remove_substrings();
	trie.store_states_in_bfs_order();
	
	tribble::string_vector_type strings;
	tribble::state_map_type states_by_string;
	
	tribble::file_istream source_stream;
	
	tribble::char_map_type char2comp;
	tribble::char_map_type comp2char;
	
	{
		tribble::timer timer;
		std::cerr << "Reading the sequences…" << std::flush;

		tribble::open_file_for_reading(args_info.source_file_arg, source_stream);
		
		tribble::read_input(
			source_stream,
			args_info.source_format_arg,
			strings,
			char2comp,
			comp2char,
			trie,
			states_by_string
		);
		
		timer.stop();
		std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
	}
	
	std::size_t const string_count(trie.num_keywords());
	
	tribble::next_string_map_type links(string_count);
	tribble::index_vector_type start_positions;
	
	{
		tribble::timer timer;
		std::cerr << "Processing the strings…" << std::flush;

		tribble::process_strings(trie, states_by_string, links, start_positions);
		
		timer.stop();
		std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
	}

	if (0 == start_positions.size())
	{
		std::cerr << "No start positions." << std::endl;
		abort();
	}
	
	if (DEBUGGING_OUTPUT)
	{
		for (auto const idx : start_positions)
		{
			std::cerr << "sp: " << idx << std::endl;
			auto next_idx(idx);
			while (true)
			{
				auto const &ns(links[next_idx]);
				next_idx = ns.index;
				if (tribble::next_string::max_index == next_idx)
					break;
				std::cerr << "  " << next_idx << " " << ns.overlap_length << std::endl;
			}
		}
		
		std::size_t i(0);
		for (auto const &str : strings)
		{
			std::cerr << i++ << ": ";
			for (auto const c : str)
				std::cerr << comp2char[c];
			std::cerr << std::endl;
		}
	}

	output_superstring(args_info.output_file_given ? os : std::cout, strings, start_positions, links, comp2char);
	
	return 0;
}
