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
#include "string_array.hh"


namespace tribble { namespace detail {
	
	typedef wt_type::size_type wt_size_type;
	typedef wt_type::value_type wt_value_type;
	
	
	struct bwt_range
	{
		size_type left{0};
		size_type right{0};
		
		bwt_range() = default;
		
		bwt_range(size_type left_, size_type right_):
			left(left_),
			right(right_)
		{
		}
		
		bwt_range(cst_type const &cst, cst_type::node_type const &node):
			bwt_range(cst.lb(node), cst.rb(node))
		{
		}
		
		inline bool is_singular() const { return left == right; }
		inline bool operator==(bwt_range const &other) const { return (left == other.left && right == other.right); }
		inline size_type count() const { return 1 + (right - left); }
		
		inline char next_leftmost_character(csa_type const &csa) const
		{
			return csa.bwt[left];
		}
		
		inline void set_range_singular(size_type const val)
		{
			left = val;
			right = val;
		}
		
		inline void lf(csa_type const &csa)
		{
			auto const lf_val(csa.lf[left]); // FIXME: check exact time.
			set_range_singular(lf_val);
		}
		
		inline size_type backward_search(csa_type const &csa, csa_type::char_type c)
		{
			return sdsl::backward_search(csa, left, right, c, left, right);
		}
	};

	
	struct range_pair
	{
		bwt_range substring_range;
		cst_type::node_type match_range;
		
		range_pair() = default;
		
		range_pair(bwt_range const &substring_range_, cst_type::node_type const &match_range_):
			substring_range(substring_range_),
			match_range(match_range_)
		{
		}
		
		inline size_type substring_count() const { return substring_range.count(); }
		inline size_type match_count(cst_type const &cst) const { return cst.size(match_range); }
		inline size_type substring_lb(cst_type const &cst) const { return substring_range.left; }
		inline size_type substring_rb(cst_type const &cst) const { return substring_range.right; }
		inline void get_substring_range(bwt_range &target) const { target = substring_range; /* Copy */ }
		inline void get_match_range(cst_type::node_type &target) const { target = match_range; /* Copy */ }
		
		inline char next_substring_leftmost_character(cst_type const &cst) const
		{
			return substring_range.next_leftmost_character(cst.csa);
		}
		
		inline bool has_equal_ranges(cst_type const &cst) const
		{
			bwt_range temp(cst, match_range);
			return temp == substring_range;
		}
		
		inline void substring_lf(cst_type const &cst)
		{
			substring_range.lf(cst.csa);
		}
		
		inline size_type backward_search_substring(cst_type const &cst, char const character)
		{
			return substring_range.backward_search(cst.csa, character);
		}
		
		inline size_type backward_search_match(cst_type const &cst, char const character)
		{
			match_range = cst.wl(match_range, character);
			return cst.size(match_range);
		}
		
		inline bool check_match_suffix_link(cst_type const &cst, cst_type::node_type const &expected)
		{
			cst_type::node_type actual_target = cst.sl(match_range); // Takes O(rr_enclose + log σ) time.
			return (actual_target == expected);
		}
	};
	
	
	class branch_checker
	{
	protected:
		string_array			m_strings;
		range_pair				m_initial_range_pair;
		sdsl::int_vector <>		m_rank_c_i;
		sdsl::int_vector <>		m_rank_c_j;
		cst_type const			*m_cst{nullptr};
		wt_type const			*m_wt{nullptr};
		char const				m_sentinel{0};
		
	public:
		branch_checker(cst_type const &cst, char const sentinel, sdsl::int_vector <> const &string_lengths):
			m_cst(&cst),
			m_wt(&cst.csa.wavelet_tree),
			m_sentinel(sentinel)
		{
			// Sentinel may not be '\0'.
			assert(sentinel);
			
			// Find the lexicographic range of the sentinels. In the suffix tree,
			// this corresponds to a child of the root.
			size_type const csa_size(cst.csa.size());
			bwt_range sentinel_range(0, csa_size - 1);
			auto const string_count(sentinel_range.backward_search(cst.csa, sentinel));
			
			if (2 <= string_count)
			{
				// Fill m_strings with index and length pairs.
				assert(string_lengths.size() == string_count - 1);
				
				auto const bits_for_n(1 + sdsl::bits::hi(csa_size));
				auto const bits_for_2n(1 + sdsl::bits::hi(2 * csa_size));
				auto const bits_for_max_length(string_lengths.width());
				auto const bits_for_m(1 + sdsl::bits::hi(string_count));
				auto const bits_for_m_1(1 + sdsl::bits::hi(1 + string_count));
				
				// Initialize the result array.
				{
					{
						string_array tmp(string_count - 1, bits_for_m, bits_for_n, bits_for_2n, bits_for_max_length);
						m_strings = std::move(tmp);
					}
					
					{
						size_type i(0);
						for (auto const length : string_lengths)
						{
							// Remove “#$”.
							auto const sa_idx(1 + sentinel_range.left + i);
							assert(sa_idx <= sentinel_range.right);
							string_type string(sa_idx, length, false);
							m_strings.set(i, string);
							++i;
						}
					}
				}
				
				// Set the initial range for locating branch points and non-uniques.
				{
					// Include the range for “#$” for backward_search.
					cst_type::node_type const root(cst.root());
					range_pair temp(sentinel_range, root);
					m_initial_range_pair = std::move(temp);
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
		inline void get_strings_available(string_array /* out */ &dst)
		{
			dst = std::move(m_strings);
		}
		
		
	protected:
		void list_next_substring_characters(
			bwt_range const &range,
			wt_value_type *cs_ptr,
			wt_size_type /* out */ &symbol_count
		)
		{
			sdsl::interval_symbols(
				*m_wt,
				range.left,
				1 + range.right,
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
		}

		
		void add_match(
			cst_type::node_type const &matching_node,
			size_type const suffix_branch_point,
			size_type const matching_suffix_length,
			size_type const branching_suffix_length
		)
		{
			auto const &csa(m_cst->csa);
			
			if (DEBUGGING_OUTPUT)
			{
				std::cerr << "*** Adding a match for range ["
					<< m_cst->lb(matching_node) << ", "
					<< m_cst->rb(matching_node) << "],"
					<< " suffix_branch_point: " << suffix_branch_point
					<< " matching_suffix_length: " << matching_suffix_length
					<< " branching_suffix_length: " << branching_suffix_length
					<< std::endl;
			}
			
			// Find the end of the substring that contains branch_point.
			size_type end_sa_idx(suffix_branch_point);
			for (std::size_t i(0); i < branching_suffix_length; ++i)
			{
				auto const psi_val(csa.psi[end_sa_idx]); // FIXME: check the exact time.
				end_sa_idx = psi_val;
			}
			
			// Check that the lengths match.
#ifndef NDEBUG
			{
				assert(m_sentinel == csa.bwt[csa.psi[end_sa_idx]]);

				cst_type::node_type node(matching_node);
				for (std::size_t i(0); i < matching_suffix_length; ++i)
					node = m_cst->sl(node);
				
				assert(m_cst->root() == node);
			}
#endif
			
			// Convert to start position.
			size_type start_sa_idx(0);
			assert(end_sa_idx <= m_initial_range_pair.substring_rb(*m_cst));
			if (m_initial_range_pair.substring_lb(*m_cst) == end_sa_idx)
				start_sa_idx = m_initial_range_pair.substring_rb(*m_cst);
			else
				start_sa_idx = end_sa_idx - 1;
			
			size_type const idx(start_sa_idx - 2);
			
			// Store the values.
			string_type string;
			m_strings.get(idx, string);
			assert(string.sa_idx == start_sa_idx);
			string.is_unique = true;
			string.matching_node = matching_node;
			string.matching_suffix_length = matching_suffix_length;
			m_strings.set(idx, string);
		}
		
		
		template <size_type t_recursion_limit>
		void add_match_for_unique_substrings(
			cst_type::node_type const &match_range,
			bwt_range const &initial_substring_range,
			size_type const matching_suffix_length,
			size_type const branching_suffix_length
		)
		{
			// Find the unique strings that may be extended left from initial_substring_range.
			// When done, report them with match_range.
			
			auto const initial_substring_count(initial_substring_range.count());

			// List the next characters.
			auto const sigma(m_wt->sigma);
			wt_value_type cs[sigma];
			wt_size_type symbol_count{0}, si{0};
			list_next_substring_characters(initial_substring_range, cs, symbol_count);
			
			// The first character should not be '$' since the range is not [0, csa.size() - 1].
			assert(0 != cs[si]);
			
			// Check if the current substring range represents non-unique strings.
			if (cs[si] == m_sentinel)
				++si;
			
			bwt_range remaining_range;
			bool should_handle_remaining_range(false);
			
			while (si < symbol_count)
			{
				bwt_range substring_range(initial_substring_range);

				// Check the next character.
				auto const next_character(cs[si++]);
				assert(next_character);
				
				// Search with the character.
				auto const substring_count(substring_range.backward_search(m_cst->csa, next_character));
				assert(substring_count);
				
				// Check if the current count exceeds recursion limit.
				if (t_recursion_limit <= initial_substring_count && initial_substring_count / 2 < substring_count)
				{
					// There should only be one remaining_range since its size is greater than
					// one half of the initial range.
					assert(!should_handle_remaining_range);
					
					should_handle_remaining_range = true;
					remaining_range = std::move(substring_range);
					
					continue;
				}

				// Recurse if there are more than one possible substring.
				if (1 != substring_count)
				{
					add_match_for_unique_substrings <t_recursion_limit>(
						match_range,
						substring_range,
						matching_suffix_length,
						1 + branching_suffix_length
					);
					continue;
				}
				
				assert(1 == substring_count);
				assert(substring_range.is_singular());
				
				// If there is one matching substring, report the match.
				add_match(match_range, substring_range.left, matching_suffix_length, 1 + branching_suffix_length);
			}
			
			// If there was a long range, handle it.
			if (should_handle_remaining_range)
			{
				// Tail recursion.
				add_match_for_unique_substrings <t_recursion_limit>(
					match_range,
					remaining_range,
					matching_suffix_length,
					1 + branching_suffix_length
				);
			}
		}
		
		
		template <size_type t_recursion_limit, bool t_is_first_range>
		void check_non_unique_strings(range_pair const &initial_range_pair, size_type const current_length)
		{
			// List the symbols that precede the ones in initial_range. Handle the
			// special cases where the character is either '$' (in case of t_is_first_range)
			// or '#'. After that, recurse in case the range length is less than half of that
			// of the initial range, or do tail recursion otherwise. If the substring range
			// has become singular, use LF repeatedly to find the preceding character and use
			// backward_search on the match range until it becomes singular, too. If the
			// Weiner link used to extend the match range to the left points to an implicit
			// node, stop extending.
			
			auto const initial_substring_count(initial_range_pair.substring_count());
			
			// Smaller number of strings should be handled as a special case, which
			// has not been written yet.
			assert(1 < initial_substring_count);
			assert(1 < initial_range_pair.match_count(*m_cst));
			
			// Since the alphabet in our case is small, use an array
			// on the stack for storing interval_symbols's results.
			auto const sigma(m_wt->sigma);
			wt_value_type cs[sigma];
			wt_size_type symbol_count{0}, si{0};
			list_next_substring_characters(initial_range_pair.substring_range, cs, symbol_count);
			
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
				++si;
			
			// Proceed with the remaining characters.
			range_pair remaining_range_pair;
			bool should_handle_remaining_range(false);

			while (si < symbol_count)
			{
				range_pair range_pair(initial_range_pair);
				
				// Store the previous match range before calling backward_search_match().
				cst_type::node_type previous_match_range;
				range_pair.get_match_range(previous_match_range);

				// Check the next character.
				auto const next_character(cs[si++]);
				assert(next_character);
				
				// Search with the character.
				auto const substring_count(range_pair.backward_search_substring(*m_cst, next_character));
				auto match_count(range_pair.backward_search_match(*m_cst, next_character));
				assert(substring_count);
				
				// Check if the suffix link from the new match node leads to the previous node. If not,
				// the Weiner link lead to an implicit node and the previous node was the last suitable one
				// (w.r.t. extending left).
				if (!range_pair.check_match_suffix_link(*m_cst, previous_match_range))
				{
					if (1 == range_pair.substring_count())
					{
						add_match(
							previous_match_range,
							range_pair.substring_lb(*m_cst),
							current_length,
							1 + current_length
						);
					}
					else
					{
						add_match_for_unique_substrings <t_recursion_limit>(
							previous_match_range,
							range_pair.substring_range,
							current_length,
							1 + current_length
						);
					}
					
					continue;
				}

				// Check if the current count exceeds recursion limit.
				if (t_recursion_limit <= initial_substring_count && initial_substring_count / 2 < substring_count)
				{
					// There should only be one remaining_range since its size is greater than
					// one half of the initial range.
					assert(!should_handle_remaining_range);
					
					should_handle_remaining_range = true;
					remaining_range_pair = std::move(range_pair);
					
					continue;
				}
				
				// Recurse if there are more than one possible substring.
				if (1 != substring_count)
				{
					check_non_unique_strings <t_recursion_limit, false>(range_pair, 1 + current_length);
					continue;
				}
				
				// There is only one possible substring.
				auto matching_suffix_length(current_length);
				auto branching_suffix_length(current_length);
				while (1 != match_count)
				{
					// Check the next character.
					auto const next_character(range_pair.next_substring_leftmost_character(*m_cst));
					
					// The substring range should not become singular before the suffix “$#”
					// has been handled. Hence it doesn't need to be handled here.
					assert(0 != next_character);
					
					// Check if the current substring range represents non-unique strings.
					// If not, skip the current range.
					if (next_character == m_sentinel)
					{
						assert(1 < range_pair.match_count(*m_cst));
						goto end_range;
					}
					
					// Store the previous match range.
					range_pair.get_match_range(previous_match_range);

					// Look for the next character in the range that didn't begin with '#'.
					range_pair.substring_lf(*m_cst);
					match_count = range_pair.backward_search_match(*m_cst, next_character);
					
					// There should be at least one match b.c. the substring range has the same character.
					assert(match_count);
					
					++branching_suffix_length;

					// Again, check if the suffix link from the new match node leads to the previous node.
					// In this case, the range has not become singular so the assertions may be skipped.
					if (!range_pair.check_match_suffix_link(*m_cst, previous_match_range))
						goto add_match;
					
					++matching_suffix_length;
				}
				
				// The match range should have become singular (not zero) by now.
				assert(1 == range_pair.match_count(*m_cst));
				
				// The ranges should be equal by now since the match range has become singular.
				assert(range_pair.has_equal_ranges(*m_cst));
				
				// Since the match range became singular, we have found the branching point.
			add_match:
				add_match(
					previous_match_range,
					range_pair.substring_lb(*m_cst),
					1 + matching_suffix_length,
					1 + branching_suffix_length
				);
				
			end_range:
				;
			}
			
			// If there was a long range, handle it.
			if (should_handle_remaining_range)
			{
				// Tail recursion.
				check_non_unique_strings <t_recursion_limit, false>(remaining_range_pair, 1 + current_length);
			}
		}
		
		
	public:
		void check_non_unique_strings()
		{
			// Recursion minimum limit chosen somewhat arbitrarily.
			check_non_unique_strings <32, true>(m_initial_range_pair, 0);
		}
	};
}}


namespace tribble {

	void check_non_unique_strings(
		cst_type const &cst,
		sdsl::int_vector <> const &string_lengths,
		char const sentinel,
		string_array /* out */ &strings_available
	)
	{
		detail::branch_checker checker(cst, sentinel, string_lengths);
		checker.check_non_unique_strings();
		checker.get_strings_available(strings_available);
	}
}
