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

namespace ios = boost::iostreams;


typedef ios::stream <ios::file_descriptor_source> source_stream_type;


int open_file(char const *fname)
{
	int fd(open(fname, O_RDONLY));
	if (-1 == fd)
		handle_error();
	return fd;
}


void find_suffixes_with_sorted(
	cst_type const &cst,
	char const sentinel,
	sdsl::int_vector <> const &sorted_substrings,
	sdsl::int_vector <> const &sorted_substring_start_indices,
	sdsl::int_vector <> const &string_lengths,
	find_superstring_match_callback &match_callback
)
{
	assert(sorted_substrings.size() == sorted_substring_start_indices.size());
	assert(sorted_substrings.size() == string_lengths.size());
	
	auto const root(cst.root());
	auto const string_count(sorted_substrings.size());
	auto const max_length(string_lengths[string_count - 1]);
	
	// Use O(m log n) bits for the nodes.
	tribble::node_array <cst_type::size_type> sorted_nodes(string_count, cst.size());
	
	{
		std::size_t i(0);
		for (auto const idx : sorted_substrings)
		{
			auto const node(cst.select_leaf(1 + idx));
			
			if (DEBUGGING_OUTPUT)
			{
				std::cerr << "Start: " << sdsl::extract(cst, cst.select_leaf(1 + sorted_substring_start_indices[i])) << std::endl;
				std::cerr << "Node:  " << sdsl::extract(cst, node) << std::endl;
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
			if (string_lengths[i - 1] < remaining_suffix_length)
				break;
			
			auto const substring_length(string_lengths[i - 1]);
			auto node(sorted_nodes.get(i - 1));
			node = cst.sl(node);							// O(rrenclose) time.
			auto parent(cst.parent(node));					// O(1) time in cst_sct3.
			auto parent_string_depth(cst.depth(parent));	// O(1) time in cst_sct3 since non-leaf.
			
			// Ascend if possible.
			// FIXME: time complexity? Do we get the worst case of implicit suffix links?
			while (remaining_suffix_length < parent_string_depth)
			{
				node = parent;
				parent = cst.parent(node);
				parent_string_depth = cst.depth(parent);
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
						sorted_substring_start_indices[i - 1],
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


void find_suffixes(char const *source_fname, char const sentinel, find_superstring_match_callback &cb)
{
	cst_type cst;

	// Load the CST.
	std::cerr << "Loading the CST…" << std::endl;
	{
		auto const event(sdsl::memory_monitor::event("Load CST"));
		
		if (source_fname)
		{
			int fd(open_file(source_fname));
			source_stream_type ds_stream(fd, ios::close_handle);
			cst.load(ds_stream);
		}
		else
		{
			cst.load(std::cin);
		}
	}

	// Find the superstring.
	// Sort the strings by length.
	std::cerr << "Sorting the sequences by unique suffix length…" << std::endl;

	sdsl::int_vector <> sorted_substrings;
	sdsl::int_vector <> sorted_substring_start_indices;
	sdsl::int_vector <> substring_lengths;
	{
		auto const event(sdsl::memory_monitor::event("Sort sequences"));
		sort_strings_by_length(cst.csa, sentinel, sorted_substrings, sorted_substring_start_indices, substring_lengths);
		cb.set_substring_count(sorted_substrings.size());
	}

	std::cerr << "Matching prefixes and suffixes…" << std::endl;
	{
		auto const event(sdsl::memory_monitor::event("Match strings"));
		
		find_suffixes_with_sorted(
			cst,
			sentinel,
			sorted_substrings,
			sorted_substring_start_indices,
			substring_lengths,
			cb
		);
	}
}
