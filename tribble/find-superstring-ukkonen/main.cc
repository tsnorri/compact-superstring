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


int main(int argc, char **argv)
{
	std::cerr << "Assertions have been " << (TRIBBLE_ASSERTIONS_ENABLED ? "enabled" : "disabled") << '.' << std::endl;

	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);
	
	bool const output_is_terminal(isatty(fileno(stdin)));
	
	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	std::cin.tie(nullptr);					// We don't require any input from the user.

	tribble::trie_type trie;
	trie.remove_substrings();
	
	tribble::string_list_type strings;
	tribble::string_map_type strings_by_state;
	tribble::state_map_type states_by_string;
	
	tribble::file_istream source_stream;
	
	{
		tribble::timer timer;
		std::cerr << "Reading the sequences…" << std::flush;

		tribble::open_file_for_reading(args_info.source_file_arg, source_stream);
		
		tribble::read_input(
			source_stream,
			args_info.source_format_arg,
			strings,
			trie,
			strings_by_state,
			states_by_string
		);
		
		timer.stop();
		std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
	}
	
	std::size_t const string_count(trie.num_keywords());
	
	tribble::next_string_map_type links(string_count);
	tribble::index_list_type start_positions;
	
	{
		tribble::timer timer;
		std::cerr << "Processing the strings…" << std::flush;

		tribble::process_strings(trie, strings_by_state, states_by_string, links, start_positions);
		
		timer.stop();
		std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
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
			std::cerr << i++ << ": " << str << std::endl;
	}

	// Output the superstring.
	for (auto const idx : start_positions)
	{
		std::cout << strings[idx];
		if (DEBUGGING_OUTPUT)
			std::cout << ' ';

		auto next_idx(idx);
		while (true)
		{
			auto const &ns(links[next_idx]);
			next_idx = ns.index;
			if (tribble::next_string::max_index == next_idx)
				break;
			
			std::cout << (strings[next_idx].data() + ns.overlap_length);
			if (DEBUGGING_OUTPUT)
				std::cout << ' ';
		}
	}
	std::cout << std::endl;
		
	return 0;
}
