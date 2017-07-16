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


#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <tribble/timer.hh>
#include "find_superstring.hh"
#include "linked_list.hh"
#include "string_array.hh"

namespace ios = boost::iostreams;


namespace {
	
	// FIXME: not needed?
	//typedef ios::stream <ios::file_descriptor_source> source_stream_type;

	void find_suffixes_with_sorted(
		tribble::cst_type const &cst,
		char const sentinel,
		tribble::string_array &strings,
		tribble::find_superstring_match_callback &match_callback
	)
	{
		auto const root(cst.root());
		auto const string_count(strings.size());
		auto const max_length(strings.max_matching_suffix_length());
		
		if (DEBUGGING_OUTPUT)
		{
			for (auto const str : strings)
				std::cerr << str;
		}

		// Use O(m log m) bits for a linked list.
		std::size_t const bits_for_m(1 + sdsl::bits::hi(1 + string_count));
		tribble::linked_list index_list(1 + string_count, bits_for_m);

		std::size_t cl(0); // Current discarded prefix length w.r.t. the longest substring.
		while (index_list.reset() && cl < max_length)
		{
			auto const remaining_suffix_length(max_length - cl);
			
			// Iterate the range where the strings have equal lengths.
			while (index_list.can_advance())
			{
				// Follow one suffix link for each node on one iteration of the outer loop.
				
				auto const i(string_count - index_list.get_i());
				assert(i);
				
				// Get the next longest string.
				tribble::string_type string;
				strings.get(i - 1, string);
				
				// If the string is not unique, it need not be handled.
				if (!string.is_unique)
				{
					index_list.advance_and_mark_skipped();
					continue;
				}
				
				// If the remaining strings are shorter than the remaining suffix
				// length, they will not match anything.
				if (string.matching_suffix_length < remaining_suffix_length)
					break;
				
				// If the branching suffix is shorter than the number of characters available,
				// the string may be skipped.
				if (string.matching_suffix_length < remaining_suffix_length)
				{
					index_list.advance();
					continue;
				}
				
				// Get the corresponding suffix tree node.
				tribble::cst_type::node_type matching_node;
				string.get_matching_node(matching_node);
				
				// Try to follow the Weiner link.
				auto const prefix_node(cst.wl(matching_node, sentinel)); // O(t_rank_BWT) time.
				if (root != prefix_node)
				{
					// A match was found. Handle it.
					bool const should_remove(match_callback.callback(
						string.sa_idx,
						remaining_suffix_length,
						cst.lb(prefix_node),
						cst.rb(prefix_node)
					));
					
					if (should_remove)
					{
						index_list.advance_and_mark_skipped();
						continue;
					}
				}

				matching_node = cst.sl(matching_node); // O(rrenclose + log σ) time (uses csa.psi).
				index_list.advance();
				string.matching_node = matching_node;
				strings.set(i - 1, string);
			}
			
			++cl;
		}
		
		match_callback.finish_matching();
	}
}


namespace tribble {

	void find_suffixes(
		std::istream &index_stream,
		std::istream &strings_stream,
		find_superstring_match_callback &cb
	)
	{
		{
			index_type index;
			string_array strings_available;
			sdsl::bit_vector is_unique_sa_order;

			// Load the index.
			std::cerr << "Loading the index…" << std::flush;
			{
				auto const event(sdsl::memory_monitor::event("Load index"));
				timer timer;
				
				index.load(index_stream);
				
				if (TRIBBLE_ASSERTIONS_ENABLED && !index.index_contains_debugging_information)
				{
					throw std::runtime_error(
						"find-superstring was built with assertions enabled "
						"but the given index does not contain "
						" the necessary data structures."
					);
				}
				else if (!TRIBBLE_ASSERTIONS_ENABLED && index.index_contains_debugging_information)
				{
					std::cerr
					<< std::endl
					<< "WARNING: find-superstring was built with assertions disabled "
					<< "but the given index contains additional data structures for "
					<< "assertions. Memory usage information will not be accurate."
					<< std::endl;
				}
				
				timer.stop();
				std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
			}
			
			if (DEBUGGING_OUTPUT)
			{
				auto const &csa(index.cst.csa);
				auto const csa_size(csa.size());
				std::cerr << " i SA ISA PSI LF BWT   T[SA[i]..SA[i]-1]" << std::endl;
				sdsl::csXprintf(std::cerr, "%2I %2S %3s %3P %2p %3B   %:1T", csa);
				std::cerr << "First row: '";
				for (std::size_t i(0); i < csa_size; ++i)
					std::cerr << csa.F[i];
				std::cerr << "'" << std::endl;
				std::cerr << "Text: '" << sdsl::extract(csa, 0, csa_size - 1) << "'" << std::endl;
			}
			

			// Check uniqueness and find match starting positions.
			std::cerr << "Checking non-unique strings and finding match starting positions…" << std::flush;
			{
				auto const event(sdsl::memory_monitor::event("Check non-unique strings and find match starting positions"));
				timer timer;
				
				check_non_unique_strings(index.cst, index.string_lengths, index.sentinel, strings_available);
				assert(std::is_sorted(
					strings_available.cbegin(),
					strings_available.cend(),
					[](string_type const &lhs, string_type const &rhs) {
						return lhs.sa_idx < rhs.sa_idx;
					}
				));
				is_unique_sa_order = strings_available.is_unique_vector(); // Copy.
				timer.stop();
				std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
				
				if (DEBUGGING_OUTPUT)
				{
					for (size_type i(0), count(strings_available.size()); i < count; ++i)
					{
						string_type str;
						strings_available.get(i, str);
						std::cerr << str << std::endl;
					}
				}
			}
			
			// Sort.
			std::cerr << "Sorting by string length…" << std::flush;
			{
				auto const event(sdsl::memory_monitor::event("Sort strings"));
				timer timer;
				
				std::sort(strings_available.begin(), strings_available.end());
				
				timer.stop();
				std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
			}
			
			std::cerr << "Matching prefixes and suffixes…" << std::flush;
			{
				auto const event(sdsl::memory_monitor::event("Match strings"));
				timer timer;
				
				cb.set_substring_count(strings_available.size());
				cb.set_is_unique_vector(is_unique_sa_order);
				cb.set_alphabet(index.cst.csa.alphabet);
				cb.set_strings_stream(strings_stream);
				cb.set_sentinel_character(index.sentinel);

				find_suffixes_with_sorted(
					index.cst,
					index.sentinel,
					strings_available,
					cb
				);
				
				timer.stop();
				std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
			}
		}

		std::cerr << "Building the final superstring…" << std::flush;
		{
			auto const event(sdsl::memory_monitor::event("Build superstring"));
			timer timer;

			cb.build_final_superstring(std::cout);
			
			timer.stop();
			std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
		}
	}
}
