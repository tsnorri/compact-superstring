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

#ifndef TRIBBLE_VECTOR_SOURCE_HH
#define TRIBBLE_VECTOR_SOURCE_HH

#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <sdsl/int_vector.hpp>


namespace tribble {
	
	class vector_source
	{
	public:
		typedef sdsl::int_vector <8>	vector_type;
		
	protected:
		std::vector <std::unique_ptr <vector_type>>	m_store;
		std::mutex									m_mutex;
		std::size_t									m_in_use{0};
		bool										m_allow_resize{true};
		
	protected:
		void resize(std::size_t const size);
		
	public:
		vector_source(std::size_t size = 1, bool allow_resize = true):
			m_store(size)
		{
			assert(0 < size);
			resize(size);
			m_allow_resize = allow_resize;
		}
		
		void get_vector(std::unique_ptr <vector_type> &target_ptr);
		void put_vector(std::unique_ptr <vector_type> &source_ptr);
	};
}

#endif
