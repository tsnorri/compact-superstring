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


#ifndef TRIBBLE_TRANSITION_MAP_HH
#define TRIBBLE_TRANSITION_MAP_HH

#include <boost/iterator/iterator_adaptor.hpp>


namespace tribble {
	
	class transition_map_base
	{
	protected:
		static std::size_t			s_sigma;
		
	public:
		static void set_sigma(std::size_t const count) { s_sigma = count; }
	};
	
	
	template <typename t_char, typename t_state_ptr>
	class transition_map : public transition_map_base
	{
	public:
		typedef typename t_state_ptr::pointer			ptr_type;
		typedef std::vector <t_state_ptr>				vector_type;
		typedef typename vector_type::size_type			size_type;
		typedef typename vector_type::reference			reference;
		typedef typename vector_type::const_reference	const_reference;
		
	protected:
		vector_type					m_states{};
		std::vector <uint8_t>		m_used_states{};
		
	public:
		transition_map():
			m_states(transition_map_base::s_sigma)
		{
		}
		
		void set_transition(t_char const character, ptr_type const next)
		{
			m_states[character].reset(next);
			m_used_states.push_back(character);
		}
		
		size_type size() const { return m_used_states.size(); }
		
		void freeze() {}
		
		bool find(t_char const character, ptr_type &result) const
		{
			auto ptr(m_states[character].get());
			if (ptr)
			{
				result = ptr;
				return true;
			}
			return false;
		}
		
		template <typename T>
		void get_states(T &result) const
		{
			for (auto const idx : m_used_states)
				result.push_back(m_states[idx].get());
		}
		
		template <typename T>
		void get_transitions(T &result) const
		{
			for (auto const idx : m_used_states)
				result.push_back(idx);
		}
	};
}

#endif
