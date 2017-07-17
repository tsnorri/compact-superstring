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

#include <sdsl/int_vector.hpp>


namespace tribble {
	
	// Maintain a pointerless linked list.
	class linked_list
	{
	public:
		typedef std::size_t size_type;
		
	protected:
		sdsl::int_vector <>	m_jump;
		std::size_t			m_j_idx{0};
		size_type			m_i{0};
		size_type			m_limit{0};
		
	public:
		linked_list() = default;
		
		linked_list(std::size_t const count, std::size_t const bits):
			m_jump(count, 0, bits),
			m_limit(count - 1)
		{
			for (std::size_t i(0); i < count; ++i)
				m_jump[i] = i;
		}
		
		inline size_type get_i() const { return m_i; }
		
		inline bool reset()
		{
			m_j_idx = 0;
			m_i = m_jump[m_j_idx];
			
			if (m_i < m_limit)
				return true;
			
			return false;
		}
		
		inline bool can_advance() const
		{
			return m_i < m_limit;
		}
	
		inline void advance()
		{
			advance(1);
		}
		
		inline void advance_and_mark_skipped()
		{
			advance_and_mark_skipped(1);
		}
		
		inline void advance(size_type const count)
		{
			std::size_t const next_j_idx(m_i + count);
			std::size_t const next_i(m_jump[next_j_idx]);
			m_j_idx = next_j_idx;
			m_i = next_i;
		}
		
		inline void advance_and_mark_skipped(size_type const count)
		{
			std::size_t const next_j_idx(m_i + count);
			std::size_t const next_i(m_jump[next_j_idx]);
			m_jump[m_j_idx] = next_i;
			m_i = next_i;
		}
		
		inline void print() const
		{
			std::cerr << "j_idx: " << m_j_idx << " i: " << m_i << " limit: " << m_limit << std::endl;
			std::cerr << "jump:";
			for (auto const k : m_jump)
				std::cerr << " " << k;
			std::cerr << std::endl;
		}
	};
}
