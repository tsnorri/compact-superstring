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

#include <range/v3/all.hpp>
#include <sdsl/bits.hpp>
#include <sdsl/csa_wt.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/suffix_array_algorithm.hpp>
#include <sdsl/wt_algorithm.hpp>
#include "find_superstring.hh"
#include "linked_list.hh"
#include "string_array.hh"


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
		
		inline void substring_lf(csa_type const &csa)
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
	
	
	class branch_checker
	{
	protected:
		tribble::string_array	m_strings;
		bwt_range				m_initial_range;
		sdsl::int_vector <>		m_rank_c_i;
		sdsl::int_vector <>		m_rank_c_j;
		csa_type const			*m_csa{nullptr};
		wt_type const			*m_wt{nullptr};
		char const				m_sentinel{0};
		
	public:
		branch_checker(csa_type const &csa, char const sentinel, sdsl::int_vector <> const &string_lengths):
			m_csa(&csa),
			m_wt(&csa.wavelet_tree),
			m_sentinel(sentinel)
		{
			size_type const csa_size(m_csa->size());
			size_type left(0), right(csa_size - 1);
			auto const string_count(sdsl::backward_search(*m_csa, left, right, m_sentinel, left, right));

			if (2 <= string_count)
			{
				// Fill m_strings with index and length pairs.
				
				assert(string_lengths.size() == string_count - 1);
				
				auto const bits_for_n(1 + sdsl::bits::hi(csa_size));
				auto const bits_for_max_length(string_lengths.width());
				auto const bits_for_m(1 + sdsl::bits::hi(string_count));
				auto const bits_for_m_1(1 + sdsl::bits::hi(1 + string_count));
				
				// Initialize the result array.
				{
					{
						tribble::string_array tmp(string_count - 1, bits_for_m, bits_for_n, bits_for_max_length);
						m_strings = std::move(tmp);
					}
					
					{
						size_type i(0);
						for (auto const length : string_lengths)
						{
							// Remove “#$”.
							auto const sa_idx(1 + left + i);
							assert(sa_idx <= right);
							string_type string(sa_idx, length, false);
							m_strings.set(i, string);
							++i;
						}
					}
				}
				
				// Set the initial range for locating branch points and non-uniques.
				{
					// Include the range for “#$” for backward_search.
					bwt_range initial_range(left, right, 0, csa_size - 1);
					m_initial_range = std::move(initial_range);
				}
				
				// Prepare the reusable buffers for interval_symbols.
				{
					auto const sigma(m_wt->sigma);
					m_rank_c_i.width(bits_for_n);
					m_rank_c_j.width(bits_for_n);
					m_rank_c_i.resize(sigma);
					m_rank_c_j.resize(sigma);
				}
			}
		}
		

		// Get the result.
		inline void get_strings_available(tribble::string_array /* out */ &dst)
		{
			dst = std::move(m_strings);
		}
		
		
	protected:
		void add_match(size_type const branch_point, size_type const branching_suffix_length)
		{
			if (DEBUGGING_OUTPUT)
			{
				std::cerr << "*** Adding a match for branch_point " << branch_point << ", length " << branching_suffix_length << std::endl;
			}
			
			// Find the end of the substring that contains branch_point.
			size_type end_sa_idx(branch_point);
			for (std::size_t i(0); i < branching_suffix_length; ++i)
			{
				auto const psi_val(m_csa->psi[end_sa_idx]); // FIXME: check the exact time.
				end_sa_idx = psi_val;
			}
			
			// Convert to start position.
			size_type start_sa_idx(0);
			assert(end_sa_idx <= m_initial_range.substring_range_right);
			if (m_initial_range.substring_range_left == end_sa_idx)
				start_sa_idx = m_initial_range.substring_range_right;
			else
				start_sa_idx = end_sa_idx - 1;
			
			size_type i(start_sa_idx - 2);
			
			// Store the values.
			string_type string;
			m_strings.get(i, string);
			assert(string.sa_idx == start_sa_idx);
			string.is_unique = true;
			string.match_start_sa_idx = branch_point;
			string.branching_suffix_length = branching_suffix_length;
			m_strings.set(i, string);
		}
		

		template <size_type t_recursion_limit, bool t_is_first_range>
		void check_non_unique_strings(bwt_range const &initial_range, size_type const current_length)
		{
			// List the symbols that precede the ones in initial_range. Handle the
			// special cases where the character is either '$' (in case of t_is_first_range)
			// or '#'. After that, recurse in case the range length is less than half of that
			// of the initial range, or do tail recursion otherwise. If the substring range
			// has become singular, use LF repeatedly to find the preceding character and use
			// backward_search on the match range until it becomes singular, too.
			
			auto const initial_substring_count(initial_range.substring_count());
			
			// Smaller number of strings should be handled as a special case, which
			// has not been written yet.
			assert(1 < initial_substring_count);
			
			// Since the alphabet in our case is small, use an array
			// on the stack for storing interval_symbols's results.
			auto const sigma(m_wt->sigma);
			wt_value_type cs[sigma];
			wt_value_type *cs_ptr(cs);
			
			wt_size_type symbol_count{0}, si{0};
			sdsl::interval_symbols(
				*m_wt,
				initial_range.substring_range_left,
				1 + initial_range.substring_range_right,
				symbol_count,
				cs_ptr,
				m_rank_c_i,
				m_rank_c_j
			);
			
			// FIXME: for some reason cs is not lexicographically ordered even though the wavelet tree is balanced.
			if (true || !wt_type::lex_ordered)
			{
				auto cs_range(ranges::view::unbounded(cs_ptr));
				auto range(ranges::view::take_exactly(
					ranges::view::zip(
						cs_range,
						m_rank_c_i,
						m_rank_c_j
					),
					symbol_count
				));

				ranges::sort(range, [](auto const &lhs, auto const &rhs) -> bool {
					return std::get <0>(lhs) < std::get <0>(rhs);
				});
			}
			
			// Handle the first range (i.e. [0, csa.size() - 1]) separately;
			// other cases have the assertion below.
			if (t_is_first_range)
			{
				// Handle the '$' character by skipping it.
				if (0 == cs[si])
					++si;
			}
			else
			{
				// If this is not the first range, the first character should not be '$'.
				assert(0 != cs[si]);
			}
			
			// Check if the current substring range represents non-unique strings.
			if (cs[si] == m_sentinel)
			{
				// Represents unique string if match range is singular.
				assert(!initial_range.is_match_range_singular());
				++si;
			}
			
			// Proceed with the remaining characters.
			bwt_range remaining_range;
			bool should_handle_remaining_range(false);

			while (si < symbol_count)
			{
				bwt_range range(initial_range);
				
				// Check the next character.
				auto const next_character(cs[si++]);
				assert(next_character);
				
				auto const substring_count(range.backward_search_substring(*m_csa, next_character));
				auto match_count(range.backward_search_match(*m_csa, next_character));
				assert(substring_count);

				// Check if the current count exceeds recursion limit.
				if (t_recursion_limit <= initial_substring_count && initial_substring_count / 2 < substring_count)
				{
					// There should only be one remaining_range since its size is greater than
					// one half of the initial range.
					assert(!should_handle_remaining_range);
					
					should_handle_remaining_range = true;
					remaining_range = std::move(range);
					
					continue;
				}
				
				if (1 != substring_count)
				{
					// There are more than one possible substring, recurse.
					check_non_unique_strings <t_recursion_limit, false>(range, 1 + current_length);
				}
				else
				{
					auto string_length(current_length);
					while (1 != match_count)
					{
						// Check the next character.
						auto const next_character(range.next_substring_leftmost_character(*m_csa));
						
						// The substring range should not become singular before the suffix “$#”
						// has been handled. Hence it doesn't need to be handled here.
						assert(0 != next_character);
						
						// Check if the current substring range represents non-unique strings.
						if (next_character == m_sentinel)
						{
							assert(!range.is_match_range_singular());
							goto end_range;
						}
						
						// Look for the next character in the range that didn't begin with '#'.
						range.substring_lf(*m_csa);
						match_count = range.backward_search_match(*m_csa, next_character);
						
						// There should be at least one match b.c. the substring range has the same character.
						assert(match_count);
						
						++string_length;
					}
					
					// The match range should have become singular (not zero) by now.
					assert(range.is_match_range_singular());
					
					// The ranges should be equal by now since the match range has become singular.
					assert(range.has_equal_ranges());
					
					// Since the match range became singular, we have found the branching point.
					add_match(range.substring_range_left, 1 + string_length);
				}
				
			end_range:
				;
			}
			
			// If there was a long range, handle it.
			if (should_handle_remaining_range)
			{
				// Tail recursion.
				check_non_unique_strings <t_recursion_limit, false>(remaining_range, 1 + current_length);
			}
		}
		
		
	public:
		void check_non_unique_strings()
		{
			// Recursion limit chosen somewhat arbitrarily.
			check_non_unique_strings <32, true>(m_initial_range, 0);
		}
	};
}}


void check_non_unique_strings(
	csa_type const &csa,
	sdsl::int_vector <> const &string_lengths,
	char const sentinel,
	tribble::string_array /* out */ &strings_available
)
{
	tribble::detail::branch_checker checker(csa, sentinel, string_lengths);
	checker.check_non_unique_strings();
	checker.get_strings_available(strings_available);
}
