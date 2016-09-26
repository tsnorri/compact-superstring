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

namespace ios = boost::iostreams;


typedef ios::stream <ios::file_descriptor_source> source_stream_type;


int open_file(char const *fname)
{
	int fd(open(fname, O_RDONLY));
	if (-1 == fd)
		handle_error();
	return fd;
}


void sort_strings_by_length(
	cst_type::csa_type const &csa,
	char const sentinel,
	/* out */ sdsl::int_vector <> &sorted_bwt_indices,
	/* out */ sdsl::int_vector <> &string_lengths
)
{
	// Store the lexicographic range of the sentinel character.
	cst_type::csa_type::size_type left(0), right(csa.size() - 1);
	auto string_count(sdsl::backward_search(csa, left, right, sentinel, left, right));
	assert(string_count);

	// Store a jump table for looping the lexicographic range
	// using O(m log m) bits, one-based.
	std::size_t bits_for_m(1 + sdsl::bits::hi(1 + string_count));
	sdsl::int_vector <> jump(1 + string_count, 0, bits_for_m);

	// Store the lexicographic indices of the $ characters ordered by
	// substring lengths using O(m log n) bits.
	std::size_t bits_for_n(1 + sdsl::bits::hi(csa.size()));
	sdsl::int_vector <> sorted_bwt_indices_t(string_count, 0, bits_for_n);

	// Store the lexicographic indices of the $ characters in
	// BWT order using O(m log n) bits.
	sdsl::int_vector <> bwt_indices(string_count, 0, bits_for_n);

	// Also store the string lengths using O(m log m) bits.
	sdsl::int_vector <> string_lengths_t(string_count, 0, bits_for_m);

	// FIXME: consider using additional O(m log n) bits for storing the
	// lexicographic ranks where the interval first has length one in
	// order to reduce the number of sl() operations needed.

	{
		std::size_t i(0);
		while (left + i <= right)
		{
			assert(left + i < ~(~0ULL << bits_for_n));
			assert(i < ~(~0ULL << bits_for_m));
			bwt_indices[i] = left + i;
			jump[i + 1] = i + 1; // jump is one-based.

			++i;
		}
	}
	
	// Repeatedly iterate the BWT indices to count the string lengths.
	std::size_t sorted_bwt_ptr(0);
	std::size_t length(0);
	while (jump[0] < string_count)
	{
		std::size_t j_idx(0);								// Jump table index.
		std::size_t j_val(jump[j_idx]);						// Jump table value.
		while (j_val < string_count)
		{
			std::size_t const next_j_idx(1 + jump[j_idx]);	// Next jump table index.
			std::size_t const next_j_val(jump[next_j_idx]);	// Next jump table value.
			
			auto const bwt_val(bwt_indices[j_val]);
			auto const lf(csa.lf[bwt_val]);
			auto const c(csa.bwt[bwt_val]);
			
			if (0 == c)
			{
				// Found the first separator. Don't follow it.
				jump[j_idx] = next_j_val;
			}
			else if (sentinel == c)
			{
				// End of the substring was reached, push to the stack.
				jump[j_idx] = next_j_val;
				sorted_bwt_indices_t[sorted_bwt_ptr] = lf;
				string_lengths_t[sorted_bwt_ptr] = length;
				++sorted_bwt_ptr;
			}
			else
			{
				// Advance to the previous character.
				bwt_indices[j_val] = lf;
			}
			
			// Advance the pointers.
			j_idx = next_j_idx;
			j_val = jump[j_idx];
		}

		++length;
	}

	// Move the results.
	sorted_bwt_indices = std::move(sorted_bwt_indices_t);
	string_lengths = std::move(string_lengths_t);
}


void find_superstring_with_sorted(
	cst_type const &cst,
	char const sentinel,
	sdsl::int_vector <> const &sorted_substrings,
	sdsl::int_vector <> const &string_lengths
)
{
	auto const root(cst.root());
	auto const string_count(sorted_substrings.size());
	auto i(string_count);
	while (0 < i)
	{
		// Take the longest string.
		auto const bwt_idx(sorted_substrings[i - 1]);
		auto const substring_length(string_lengths[i - 1]);

		// Find the corresponding leaf in the CST and remove the sentinel.
		auto const leaf(cst.select_leaf(bwt_idx));
		auto node(cst.sl(leaf));

		// Follow the suffix links until the string depth of the parent node
		// is exactly the same as the number of characters left. In this case,
		// the first character of the label of the edge to the leaf should
		// be the sentinel, in which case there must be at least one suffix
		// the next character of which is not the sentinel. This is a potential
		// match.
		std::size_t length(0);
		while (node != root)
		{
			// sl() may be used initially twice since the strings are not
			// substrings of each other.
			node = cst.sl(leaf);						// O(rrenclose) time.
			auto parent(cst.parent(node));				// O(1) time in cst_sct3.
			auto const string_depth(cst.depth(parent));	// O(1) time in cst_sct3 since non-leaf.

			if (string_depth == substring_length - length)
			{
				// Potential match, try to follow the Weiner link.
				auto const match_node(cst.wl(parent, sentinel));	// O(t_rank_BWT) time.
				if (root != match_node)
				{
					assert(cst.edge(node, 1 + string_depth) == sentinel);	// edge takes potentially more time so just verify.
					// A match was found.
					// FIXME: report.
				}
			}
			++length;
		}
		--i;
	}
}


void find_superstring(char const *source_fname, char const sentinel)
{
	cst_type cst;

	// Load the CST.
	std::cerr << "Loading the CST…" << std::endl;

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

	// Find the superstring.
	// Sort the strings by length.
	sdsl::int_vector <> sorted_substrings;
	sdsl::int_vector <> substring_lengths;

	std::cerr << "Sorting strings…" << std::endl;
	sort_strings_by_length(cst.csa, sentinel, sorted_substrings, substring_lengths);

	std::cerr << "Finding matches…" << std::endl;
	find_superstring_with_sorted(cst, sentinel, sorted_substrings, substring_lengths);
}
