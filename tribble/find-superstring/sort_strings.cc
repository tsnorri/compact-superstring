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

#include <sdsl/bits.hpp>
#include <sdsl/csa_wt.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/suffix_array_algorithm.hpp>
#include <sdsl/wt_algorithm.hpp>
#include "find_superstring.hh"
#include "linked_list.hh"


namespace tribble { namespace detail {
	
	typedef wt_type::size_type wt_size_type;
	typedef wt_type::value_type wt_value_type;
	
	struct bwt_range
	{
		size_type substring_range_left{0};
		size_type substring_range_right{0};
		size_type match_range_left{0};
		size_type match_range_right{0};
		
		bwt_range() = default;
		
		bwt_range(size_type sub_left, size_type sub_right, size_type match_left, size_type match_right):
			substring_range_left(sub_left),
			substring_range_right(sub_right),
			match_range_left(match_left),
			match_range_right(match_right)
		{
		}
		
		inline bool is_substring_range_singular() const { return substring_range_left == substring_range_right; }
		inline bool is_match_range_singular() const { return match_range_left == match_range_right; }
		inline bool has_equal_ranges() const { return (substring_range_left == match_range_left && substring_range_right == match_range_right); }
		inline size_type substring_count() const { return 1 + (substring_range_right - substring_range_left); }
		inline size_type match_count() const { return 1 + (match_range_right - match_range_left); }
		
		inline char next_substring_leftmost_character(csa_type const &csa)
		{
			return csa.bwt[substring_range_left];
		}
		
		inline void set_substring_range_singular(size_type val)
		{
			substring_range_left = val;
			substring_range_right = val;
		}
		
		inline void backtrack_substring_with_lf(csa_type const &csa)
		{
			auto const lf_val(csa.lf[substring_range_left]); // FIXME: check exact time.
			set_substring_range_singular(lf_val);
		}
		
		inline size_type backward_search_substring(csa_type const &csa, csa_type::char_type c)
		{
			return sdsl::backward_search(csa, substring_range_left, substring_range_right, c, substring_range_left, substring_range_right);
		}
		
		inline size_type backward_search_match(csa_type const &csa, csa_type::char_type c)
		{
			return sdsl::backward_search(csa, match_range_left, match_range_right, c, match_range_left, match_range_right);
		}
	};
	
	
	class bwt_range_array
	{
	protected:
#ifdef DEBUGGING_OUTPUT
		sdsl::bit_vector	m_used_indices;
#endif

		sdsl::int_vector <> m_substring_ranges;	// Ranges that have '#' on the right.
		sdsl::int_vector <> m_match_ranges;		// Ranges without the '#' on the right.
		
	public:
		bwt_range_array() = default;
		
		bwt_range_array(std::size_t const count, std::size_t const bits):
#ifdef DEBUGGING_OUTPUT
			m_used_indices(count, 0),
#endif
			m_substring_ranges(2 * count, 0, bits),
			m_match_ranges(2 * count, 0, bits)
		{
		}
		
		inline void get(std::size_t const k, bwt_range &range) const
		{
#ifdef DEBUGGING_OUTPUT
			assert(m_used_indices[k]);
#endif
			range.substring_range_left	= m_substring_ranges[2 * k];
			range.substring_range_right	= m_substring_ranges[2 * k + 1];
			range.match_range_left		= m_match_ranges[2 * k];
			range.match_range_right		= m_match_ranges[2 * k + 1];
		}
		
		inline void set(std::size_t const k, bwt_range const &range)
		{
			m_substring_ranges[2 * k]		= range.substring_range_left;
			m_substring_ranges[2 * k + 1]	= range.substring_range_right;
			m_match_ranges[2 * k]			= range.match_range_left;
			m_match_ranges[2 * k + 1]		= range.match_range_right;
			
#ifdef DEBUGGING_OUTPUT
			m_used_indices[k]				= 1;
#endif
		}
		
		inline void remove(std::size_t const k)
		{
			// Generally no-op.
#ifdef DEBUGGING_OUTPUT
			m_used_indices[k] = 0;
#endif
		}
		
		inline void print() const
		{
			// m_used_indices does not exist if DEBUGGING_OUTPUT has not been defined.
#ifdef DEBUGGING_OUTPUT
			std::cerr << "Substring ranges:";
			for (std::size_t i(0), count(m_used_indices.size()); i < count; ++i)
			{
				if (m_used_indices[i])
					std::cerr << " " << i << ": [" << m_substring_ranges[2 * i] << ", " << m_substring_ranges[2 * i + 1] << "]";
			}
			std::cerr << std::endl;
			
			std::cerr << "Match ranges:    ";
			for (std::size_t i(0), count(m_used_indices.size()); i < count; ++i)
			{
				if (m_used_indices[i])
					std::cerr << " " << i << ": [" << m_match_ranges[2 * i] << ", " << m_match_ranges[2 * i + 1] << "]";
			}
			std::cerr << std::endl;
#endif
		}
	};
	
	
	struct interval_symbols
	{
		std::vector <wt_value_type> cs;
		std::vector <wt_size_type> rank_c_i;
		std::vector <wt_size_type> rank_c_j;
		
		interval_symbols(std::size_t const sigma):
			cs(sigma),
			rank_c_i(sigma),
			rank_c_j(sigma)
		{
		}
	};
	
	
	class string_sorter
	{
	protected:
		csa_type const		*m_csa{nullptr};
		bwt_range_array		m_ranges;
		linked_list			m_index_list;
		interval_symbols	m_is_buffer;
		sdsl::int_vector <>	m_sorted_bwt_indices;
		sdsl::int_vector <>	m_sorted_bwt_start_indices;
		sdsl::int_vector <>	m_string_lengths;
		size_type			m_length{0};
		size_type			m_sorted_bwt_ptr{0};
		char const			m_sentinel{0};
		
	public:
		string_sorter(csa_type const &csa, char const sentinel):
			m_csa(&csa),
			m_is_buffer(csa.wavelet_tree.sigma),
			m_sentinel(sentinel)
		{
			auto const csa_size(m_csa->size());
			
			// Store the lexicographic range of the sentinel character.
			size_type left(0), right(csa_size - 1);
			auto const string_count(sdsl::backward_search(*m_csa, left, right, sentinel, left, right));
			
			std::size_t const bits_for_m(1 + sdsl::bits::hi(1 + string_count));
			std::size_t const bits_for_n(1 + sdsl::bits::hi(csa_size));
			
			{
				bwt_range_array ranges(string_count, bits_for_n);
				bwt_range initial_range(left, right, 0, csa_size - 1);
				ranges.set(0, initial_range);
				
				m_ranges = std::move(ranges);
			}
			
			{
				linked_list index_list(1 + string_count, bits_for_m);
				m_index_list = std::move(index_list);
			}
			
			{
				sdsl::int_vector <> sorted_bwt_indices(string_count - 1, 0, bits_for_n);
				sdsl::int_vector <> sorted_bwt_start_indices(string_count - 1, 0, bits_for_n);
				sdsl::int_vector <> string_lengths(string_count - 1, 0, bits_for_n);
				
				m_sorted_bwt_indices		= std::move(sorted_bwt_indices);
				m_sorted_bwt_start_indices	= std::move(sorted_bwt_start_indices);
				m_string_lengths			= std::move(string_lengths);
			}
		}
		
	protected:
		inline void add_match(bwt_range range)
		{
			assert(range.has_equal_ranges());

			if (DEBUGGING_OUTPUT)
			{
				std::cerr << "*** Adding a match for range ["
					<< range.substring_range_left << ", " << range.substring_range_right
					<< "]" << std::endl;
			}

			auto const range_start(range.match_range_left);
			m_sorted_bwt_indices[m_sorted_bwt_ptr] = range_start;
			
			// Find the start of the substring.
			while (range.next_substring_leftmost_character(*m_csa) != m_sentinel)
				range.backtrack_substring_with_lf(*m_csa);
			range.backtrack_substring_with_lf(*m_csa);
			
			m_sorted_bwt_start_indices[m_sorted_bwt_ptr] = range.substring_range_left;
			m_string_lengths[m_sorted_bwt_ptr] = m_length;
			
			++m_sorted_bwt_ptr;
		}
		
		// Return true if the position may be removed from the linked list.
		inline void handle_singular_range(bwt_range range)
		{
			// Suppose the substring range (one that begins with '#') is singular,
			// i.e. the left bound is equal to the right bound. Check the next
			// character and look for it in the range of the whole concatenated
			// string (“match range”). If that range becomes singular, a branching
			// point for the suffix of the substring in question has been found and
			// may be stored by calling add_match().
			
			// Check the next character.
			auto const next_character(range.next_substring_leftmost_character(*m_csa));
			if (0 == next_character)
			{
				m_index_list.advance_and_mark_skipped();
				return;
			}
			
			// Look for the next character in the range that didn't begin with '#'.
			range.backtrack_substring_with_lf(*m_csa);
			auto const match_count(range.backward_search_match(*m_csa, next_character));
			assert(match_count);
			if (range.is_match_range_singular())
			{
				// Since the match range became singular, we have found the branching point.
				add_match(range);
				m_ranges.remove(m_index_list.get_i());
				m_index_list.advance_and_mark_skipped();
			}
			else
			{
				// Otherwise update the current range in the list and move to the next one.
				auto const i(m_index_list.get_i());
				m_ranges.set(i, range);
				m_index_list.advance();
			}
		}
		
		inline void handle_non_singular_range(bwt_range range)
		{
			// Suppose the substring range (one that begins with '#') is not singular,
			// i.e. the left bound is not equal to the right bound. Use the wavelet tree
			// to list all the distinct characters in that range. First check for the '$'
			// character and skip it. Then iterate over the remaining characters and
			// use backward_search to find the matching substrings. Also use
			// backward_search on the current substring in order to reduce the number of
			// possible symbols.
			
			auto const original_substring_count(range.substring_count());
			std::size_t count_diff(original_substring_count);
			assert(1 < original_substring_count);
			
			wt_size_type symbol_count{0}, si{0};
			sdsl::interval_symbols(
				m_csa->wavelet_tree,
				range.substring_range_left,
				1 + range.substring_range_right,
				symbol_count,
				m_is_buffer.cs,
				m_is_buffer.rank_c_i,
				m_is_buffer.rank_c_j
			);

			assert(symbol_count);

			// For some reason cs is not lexicographically ordered even though the wavelet tree is balanced.
			if (true || !wt_type::lex_ordered)
				std::sort(m_is_buffer.cs.begin(), symbol_count + m_is_buffer.cs.begin());
			
			// Handle the '$' character.
			if (0 == m_is_buffer.cs[0])
			{
				++si;
				--count_diff;
				m_ranges.remove(m_index_list.get_i());
				m_index_list.advance_and_mark_skipped();
			}
			
			while (count_diff)
			{
				bwt_range new_range(range);

				// Check the next character.
				assert(si < symbol_count);
				auto const next_character(m_is_buffer.cs[si++]);
				assert(next_character);
				auto const substring_count(new_range.backward_search_substring(*m_csa, next_character));
				auto const match_count(new_range.backward_search_match(*m_csa, next_character));
				assert(match_count);
				
				if (new_range.is_match_range_singular())
				{
					assert(1 == substring_count);
					assert(1 == match_count);
					
					add_match(new_range);
					m_ranges.remove(m_index_list.get_i());
					m_index_list.advance_and_mark_skipped();
				}
				else
				{
					auto const i(m_index_list.get_i());
					m_ranges.set(i, new_range);
					m_index_list.advance(substring_count);
				}
				
				count_diff -= substring_count;
			}
		}
		
	public:
		inline void get_sorted_bwt_indices(sdsl::int_vector <> &target)
		{
			target = std::move(m_sorted_bwt_indices);
		}
		
		inline void get_sorted_bwt_start_indices(sdsl::int_vector <> &target)
		{
			target = std::move(m_sorted_bwt_start_indices);
		}
		
		inline void get_string_lengths(sdsl::int_vector <> &target)
		{
			target = std::move(m_string_lengths);
		}
		
		inline void sort_strings_by_length()
		{
			auto const csa_size(m_csa->size());
			
			if (DEBUGGING_OUTPUT)
			{
				std::cerr << " i SA ISA PSI LF BWT   T[SA[i]..SA[i]-1]" << std::endl;
				sdsl::csXprintf(std::cerr, "%2I %2S %3s %3P %2p %3B   %:1T", *m_csa);
				std::cerr << "First row: '";
				for (std::size_t i(0), count(m_csa->size()); i < count; ++i)
					std::cerr << m_csa->F[i];
				std::cerr << "'" << std::endl;
				std::cerr << "Text: '" << sdsl::extract(*m_csa, 0, csa_size - 1) << "'" << std::endl;
			}
			
			while (m_index_list.reset())
			{
				while (m_index_list.can_advance())
				{
					if (DEBUGGING_OUTPUT)
					{
						m_index_list.print();
						m_ranges.print();
					}
					
					auto const i(m_index_list.get_i());
					bwt_range range;
					m_ranges.get(i, range);
				
					if (range.is_substring_range_singular())
						handle_singular_range(range);
					else
						handle_non_singular_range(range);
				}
				
				++m_length;
			}
		}
	};
}}


void sort_strings_by_length(
	csa_type const &csa,
	char const sentinel,
	/* out */ sdsl::int_vector <> &sorted_bwt_indices,
	/* out */ sdsl::int_vector <> &sorted_bwt_start_indices,
	/* out */ sdsl::int_vector <> &string_lengths
)
{
	tribble::detail::string_sorter sorter(csa, sentinel);
	sorter.sort_strings_by_length();
	
	sorter.get_sorted_bwt_indices(sorted_bwt_indices);
	sorter.get_sorted_bwt_start_indices(sorted_bwt_start_indices);
	sorter.get_string_lengths(string_lengths);
}
