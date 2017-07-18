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

#ifndef TRIBBLE_LINKED_LIST_HH
#define TRIBBLE_LINKED_LIST_HH

#include <sdsl/int_vector.hpp>


namespace tribble {
	
	// Maintain a pointerless linked list.
	class linked_list
	{
	public:
		typedef std::size_t size_type;
		
	protected:
		sdsl::int_vector <>	m_jump;
		size_type			m_prev{0};
		size_type			m_curr{0};
		size_type			m_limit{0};
		size_type			m_size{0};
		
	public:
		linked_list() = default;
		
		linked_list(std::size_t const count, std::size_t const bits):
			m_jump(1 + count, 0, bits),
			m_limit(count),
			m_size(count)
		{
			for (size_type i(0); i < 1 + count; ++i)
				m_jump[i] = i;
		}
		
		void resize(size_type const count, size_type const width)
		{
			size_type count_(1 + count);
			sdsl::int_vector <> temp(count_, 0, width);
			for (size_type i(0); i < count_; ++i)
				temp[i] = i;
			
			m_jump = std::move(temp);
			m_limit = count;
			m_size = count;
		}
		
		inline size_type current() const { return m_curr - 1; }
		inline size_type size() const { return m_size; }
		
		inline bool reset()
		{
			m_prev = m_jump[0];
			m_curr = 1 + m_prev;
			return 0 < m_size;
		}
		
		inline bool at_end() const
		{
			return 0 == m_size || (! (m_curr <= m_limit));
		}
		
		inline void advance()
		{
			auto const next(1 + m_jump[m_curr]);
			m_prev = m_curr;
			m_curr = next;
		}
		
		inline void advance_and_mark_skipped()
		{
			auto cv(m_jump[m_curr]);
			m_jump[m_prev] = cv;
			m_curr = 1 + cv;
			--m_size;
		}
		
		inline void print() const
		{
			std::cerr << "curr: " << m_curr << " prev: " << m_prev << " limit: " << m_limit << " size: " << m_size << std::endl;
			std::cerr << "jump:";
			for (auto const k : m_jump)
				std::cerr << " " << k;
			std::cerr << std::endl;
		}
	};
}

#endif
