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
#include "find_superstring.hh"
#include "linked_list.hh"
#include "node_array.hh"
#include "string_array.hh"
#include "timer.hh"

namespace ios = boost::iostreams;


typedef ios::stream <ios::file_descriptor_source> source_stream_type;


void find_suffixes_with_sorted(
	cst_type const &cst,
	char const sentinel,
	tribble::string_array const &strings,
	find_superstring_match_callback &match_callback
)
{
	auto const root(cst.root());
	auto const string_count(strings.size());
	auto const max_length(strings.max_string_length());
	
	// Use O(m log n) bits for the nodes.
	tribble::node_array <cst_type::size_type> sorted_nodes(string_count, cst.size());
	
	{
		std::size_t i(0);
		for (auto const str : strings)
		{
			auto const match_start_sa_idx(str.match_start_sa_idx);
			// Simplify the algorithm by backtracking one letter.
			auto const next_sa_idx(cst.csa.lf[match_start_sa_idx]);
			auto const node(cst.select_leaf(1 + next_sa_idx));
			
			if (DEBUGGING_OUTPUT)
			{
				auto const sa_idx(str.sa_idx);
				std::cerr << std::endl;
				std::cerr << "Is unique: " << str.is_unique << " matching suffix length: " << str.branching_suffix_length << std::endl;
				std::cerr << "Start ("<< sa_idx << "): " << sdsl::extract(cst, cst.select_leaf(1 + sa_idx)) << std::endl;
				std::cerr << "Node: (" << match_start_sa_idx << "): " << sdsl::extract(cst, node) << std::endl;
				std::cerr << "----------" << std::endl;
			}
			
			sorted_nodes.set(i, node);
			++i;
		}
	}
	
	// Use O(m log m) bits for a linked list.
	std::size_t const bits_for_m(1 + sdsl::bits::hi(1 + string_count));
	tribble::linked_list index_list(1 + string_count, bits_for_m);
	
	// Follow the suffix links until the string depth of the parent node
	// is exactly the same as the number of characters left. In this case,
	// the first character of the label of the edge towards the leaf should
	// be the sentinel, in which case there must be at least one suffix
	// the next character of which is not the sentinel. This is a potential
	// match. In order to find the highest node, try to ascend in the tree
	// as long as the string depth of the current node is still greater than
	// the length of the string in question.
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
			if (string.length < remaining_suffix_length)
				break;
			
			// If the branching suffix is shorter than the number of characters available,
			// the string may be skipped.
			if (string.branching_suffix_length < remaining_suffix_length)
			{
				index_list.advance();
				continue;
			}
			
			auto node(sorted_nodes.get(i - 1));
			node = cst.sl(node);							// O(rrenclose) time.
			auto parent(cst.parent(node));					// O(1) time in cst_sct3.
			auto parent_string_depth(cst.depth(parent));	// O(1) time in cst_sct3 since non-leaf.
			
			if (false && DEBUGGING_OUTPUT)
			{
				std::cerr << "Node:   '" << sdsl::extract(cst, node) << "'" << std::endl;
				std::cerr << "Parent: '" << sdsl::extract(cst, parent) << "'" << std::endl;
			}

			// Ascend if possible.
			// FIXME: time complexity? Do we get the worst case of implicit suffix links
			// since the BWT index is allowed to contain substrings (but not duplicates)?
			while (remaining_suffix_length < parent_string_depth)
			{
				node = parent;
				parent = cst.parent(node);
				parent_string_depth = cst.depth(parent);
			}

			if (false && DEBUGGING_OUTPUT)
			{
				std::cerr << "Node:   '" << sdsl::extract(cst, node) << "'" << std::endl;
				std::cerr << "----------" << std::endl;
			}

			if (remaining_suffix_length == parent_string_depth)
			{
				// Potential match, try to follow the Weiner link.
				auto const match_node(cst.wl(parent, sentinel));	// O(t_rank_BWT) time.
				if (root != match_node)
				{
					expensive_assert(cst.edge(node, 1 + parent_string_depth) == sentinel);	// edge takes potentially more time so just verify.
					
					// A match was found. Handle it.
					bool const should_remove(match_callback.callback(
						string.sa_idx,
						remaining_suffix_length,
						cst.lb(match_node),
						cst.rb(match_node)
					));
					
					if (should_remove)
					{
						index_list.advance_and_mark_skipped();
						continue;
					}
				}
			}
			
			index_list.advance();
			sorted_nodes.set(i - 1, node);
		}
		
		++cl;
	}
}


void find_suffixes(
	std::istream &index_stream,
	std::istream &strings_stream,
	char const sentinel,
	find_superstring_match_callback &cb
)
{
	index_type index;

	// Load the index.
	std::cerr << "Loading the index…" << std::flush;
	{
		auto const event(sdsl::memory_monitor::event("Load index"));
		tribble::timer timer;
		
		index.load(index_stream);
		
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
	

	// Check uniqueness.
	std::cerr << "Checking non-unique strings…" << std::flush;
	tribble::string_array strings_available;
	sdsl::bit_vector is_unique_sa_order;
	{
		auto const event(sdsl::memory_monitor::event("Check non-unique strings"));
		tribble::timer timer;
		
		check_non_unique_strings(index.cst.csa, index.string_lengths, sentinel, strings_available);
		assert(std::is_sorted(
			strings_available.cbegin(),
			strings_available.cend(),
			[](tribble::string_type const &lhs, tribble::string_type const &rhs) {
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
				tribble::string_type str;
				strings_available.get(i, str);
				std::cerr << str << std::endl;
			}
		}
	}
	
	// Sort.
	std::cerr << "Sorting by string length…" << std::flush;
	{
		auto const event(sdsl::memory_monitor::event("Sort strings"));
		tribble::timer timer;
		
		std::sort(strings_available.begin(), strings_available.end());
		
		timer.stop();
		std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
	}
	
	cb.set_substring_count(strings_available.size());
	cb.set_is_unique_vector(is_unique_sa_order);
	cb.set_alphabet(index.cst.csa.alphabet);
	cb.set_strings_stream(strings_stream);

	std::cerr << "Matching prefixes and suffixes…" << std::flush;
	{
		auto const event(sdsl::memory_monitor::event("Match strings"));
		tribble::timer timer;
		
		find_suffixes_with_sorted(
			index.cst,
			sentinel,
			strings_available,
			cb
		);
		
		timer.stop();
		std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
	}
	
	std::cerr << "Building the final superstring…" << std::flush;
	{
		auto const event(sdsl::memory_monitor::event("Build superstring"));
		tribble::timer timer;

		cb.build_final_superstring(std::cout);
		
		timer.stop();
		std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
	}
}
