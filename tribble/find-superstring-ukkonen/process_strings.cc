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


#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <sdsl/int_vector.hpp>
#include "process_strings.hh"


namespace tribble {

	// Create L(s) for each state.
	void fill_l_lists(
		state_ptr_queue_type const &final_states,
		string_map_type const &strings_by_state,
		index_list_map_type &dst
	)
	{
		for (auto state_ptr : final_states)
		{
			auto const it(strings_by_state.find(state_ptr));
			assert(strings_by_state.cend() != it);
			auto const string_idx(it->second);
	
			while (state_ptr)
			{
				auto const state_idx(state_ptr->index());
				auto &l_list(dst[state_idx]);
				l_list.push_back(string_idx);
				state_ptr = state_ptr->parent();
			}
		}
	}
	
	
	// Helper for get_states_in_reverse_bfs_order.
	void handle_state(
		trie_type::state_ptr_type state,
		state_ptr_queue_type &dst,
		state_ptr_queue_type &q,
		sdsl::bit_vector &processed_states
	)
	{
		assert(0 == dst.size() || dst.back()->greater_than_bfs_order(*state));
		
		if (DEBUGGING_OUTPUT)
			std::cerr << "Adding state " << state->index() << std::endl;
		
		dst.push_back(state);
		auto parent_ptr(state->parent());

		if (DEBUGGING_OUTPUT)
			std::cerr << "  Parent: " << parent_ptr << std::endl;
		
		if (parent_ptr)
		{
			auto const parent_idx(parent_ptr->index());
			if (0 == processed_states[parent_idx])
			{
				processed_states[parent_idx] = 1;
				q.push_back(parent_ptr);
			}
			else
			{
				if (DEBUGGING_OUTPUT)
					std::cerr << "  Already processed." << std::endl;
			}
		}
	}
	
	
	// Fill a queue of states in reverse BFS order.
	void get_states_in_reverse_bfs_order(
		trie_type const &trie,
		state_ptr_queue_type const &final_states,
		state_ptr_queue_type &dst
	)
	{
		sdsl::bit_vector processed_states(trie.num_states(), 0);
	
		state_ptr_queue_type q;
		for (auto state_ptr : boost::adaptors::reverse(final_states))
		{
			while (q.size())
			{
				auto &other(q.front());
				if (state_ptr->greater_than_bfs_order(*other))
					break;
	
				// Other is before state_ptr in BFS order.
				handle_state(other, dst, q, processed_states);
	
				q.pop_front();
			}
	
			handle_state(state_ptr, dst, q, processed_states);
		}
		
		while (q.size())
		{
			handle_state(q.front(), dst, q, processed_states);
			q.pop_front();
		}
	}
	
	
	// Find the Hamiltonian path as discussed in Ukkonen's paper.
	void process_strings(
		trie_type &trie,
		string_map_type const &strings_by_state,
		state_map_type const &states_by_string,
		next_string_map_type &dst,
		index_list_type &start_positions
	)
	{
		auto const string_count(trie.num_keywords());
		
		sdsl::bit_vector left_available(string_count, 1);
		sdsl::bit_vector right_available(string_count, 1);
	
		state_ptr_queue_type final_states;
		trie.get_final_states_in_bfs_order(final_states);
	
		// Available after calling get_final_states_in_bfs_order.
		auto const state_count(trie.num_states());
		
		state_ptr_queue_type all_states_rev_bfs;
		get_states_in_reverse_bfs_order(trie, final_states, all_states_rev_bfs);
	
		index_list_map_type l_map(state_count);
		fill_l_lists(final_states, strings_by_state, l_map);
		
		if (DEBUGGING_OUTPUT)
		{
			std::cerr << "Supporters: " << std::endl;

			std::size_t i(0);
			for (auto const &kv : l_map)
			{
				std::cerr << "  state idx: " << i << std::endl;
				for (auto const v : kv)
					std::cerr << "    string idx: " << v << std::endl;
				++i;
			}
		}
	
		// Initial values for P(s), FIRST and LAST (1)
		index_queue_map_type p_map(state_count);
		index_list_type first, last;
		first.resize(string_count);
		last.resize(string_count);
		for (auto const state_ptr : final_states)
		{
			auto const it(strings_by_state.find(state_ptr));
			assert(strings_by_state.cend() != it);
			auto const string_idx(it->second);
			auto const failure_state_ptr(state_ptr->failure());
			auto const failure_state_idx(failure_state_ptr->index());
			p_map[failure_state_idx].push_back(string_idx);
	
			first[string_idx] = string_idx;
			last[string_idx] = string_idx;
		}
	
		sdsl::bit_vector processed_strings(trie.num_keywords(), 0);
	
		// Main loop of the algorithm (3)
		for (auto const state_ptr : all_states_rev_bfs)
		{
			auto state_idx(state_ptr->index());
			auto &p_queue(p_map[state_idx]);
			
			// (4)
			if (p_queue.size())
			{
				// (5) Get string indices by state.
				auto const &l_list(l_map[state_idx]);
				for (auto const string_idx : l_list)
				{
					if (DEBUGGING_OUTPUT)
					{
						std::cerr << "First: ";
						std::cerr << boost::algorithm::join(first | boost::adaptors::transformed(static_cast <std::string(*)(size_t)>(std::to_string)), ", ") << std::endl;
					
						std::cerr << "Last: ";
						std::cerr << boost::algorithm::join(last | boost::adaptors::transformed(static_cast <std::string(*)(size_t)>(std::to_string)), ", ") << std::endl;
					}

					if (0 == p_queue.size())
						break;

					if (processed_strings[string_idx])
						continue;
					
					// (6), (7), (8)
					auto ii(p_queue.front());
					if (first[ii] == string_idx)
					{
						if (1 == p_queue.size())
							continue;
	
						p_queue.pop_front();
						p_queue.push_back(ii);
						ii = p_queue.front();
					}
					assert(p_queue.size());
					p_queue.pop_front(); // (11)
					
					//if (left_available[ii] && right_available[string_idx])
					{
						//left_available[ii] = false;
						right_available[string_idx] = false;
	
						if (DEBUGGING_OUTPUT)
							std::cerr << "Overlap: " << ii << ", " << string_idx << " length: " << state_ptr->get_depth() << std::endl;
						
						// Add to H.
						next_string ns(string_idx, state_ptr->get_depth());
						dst[ii] = ns;
					}
	
					// (10)
					processed_strings[string_idx] = true;
	
					// (12), (13)
					first[last[string_idx]] = first[ii];
					last[first[ii]] = last[string_idx];
				}
	
				// (14)
				auto failure_state_ptr(state_ptr->failure() ?: trie.get_root());
				auto const failure_state_idx(failure_state_ptr->index());
				auto &p_failure_queue(p_map[failure_state_idx]);
				for (auto const string_idx : p_queue)
					p_failure_queue.push_back(string_idx);
			}
		}
		
		std::size_t i(0);
		for (auto const val : right_available)
		{
			if (val && states_by_string[i]->get_emits().size())
				start_positions.push_back(i);
			++i;
		}
	}
}
