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


#include "vector_source.hh"


namespace tribble {

	void vector_source::resize(std::size_t const size)
	{
		if (!m_allow_resize)
			throw std::runtime_error("Trying to allocate more vectors than allowed");
		
		// Fill from the beginning so that m_in_use slots in the end remain empty.
		m_store.resize(size);
		for (decltype(m_in_use) i(0); i < size - m_in_use; ++i)
			m_store[i].reset(new vector_type);
	}


	void vector_source::get_vector(std::unique_ptr <vector_type> &target_ptr)
	{
		assert(nullptr == target_ptr.get());
		
		std::lock_guard <std::mutex> lock_guard(m_mutex);
		auto total(m_store.size());
		assert(total);
		
		// Check if there are any vectors left.
		if (m_in_use == total)
		{
			auto const new_size(2 * total);
			resize(new_size);
			assert(m_store[m_in_use - 1].get());
			total = new_size;
		}
		
		auto &ptr(m_store[total - m_in_use - 1]);
		target_ptr.swap(ptr);
		++m_in_use;
	}


	void vector_source::put_vector(std::unique_ptr <vector_type> &source_ptr)
	{
		assert(source_ptr.get());
		
		std::lock_guard <std::mutex> lock_guard(m_mutex);
		auto const total(m_store.size());
		assert(total);
		assert(m_in_use);
		
		auto &ptr(m_store[total - m_in_use]);
		assert(nullptr == ptr.get());
		source_ptr.swap(ptr);
		--m_in_use;
	}
}
