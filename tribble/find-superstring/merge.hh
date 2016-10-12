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


#ifndef TRIBBLE_MERGE_HH
#define TRIBBLE_MERGE_HH

#include "boost_enable_if.hh"
#include <boost/iterator/iterator_facade.hpp>


namespace tribble { namespace detail {
	
	// Various classes to represent merges. These are needed in order to store
	// merges (tuples of unsigned integers) into SDSL integer vectors. The
	// merges also need to be sorted while they are stored into a merge_array,
	// hence the writable iterator implementation.
	
	class merge_array;

	
	struct merge
	{
		std::size_t left;
		std::size_t right;
		std::size_t length;
		
		merge() = default;
		
		merge(std::size_t left_val, std::size_t right_val, std::size_t length_val):
			left(left_val),
			right(right_val),
			length(length_val)
		{
		}
	};
	
	
	// std::sort requires writable iterators, so we provide ones by using this proxy.
	// To support the bare minimum, only assignment operators of the proxy (and not
	// those of the data members) are usable.
	// The representation isn't particularly efficient.
	class merge_proxy
	{
		template <typename> friend struct merge_array_iterator_trait;
		
	public:
		typedef std::size_t size_type;
		
	protected:
		merge m_merge;
		merge_array *m_array;
		size_type m_idx;
		
	public:
		std::size_t const &left{m_merge.left};
		std::size_t const &right{m_merge.right};
		std::size_t const &length{m_merge.length};
		
	protected:
		merge_proxy(merge_array &array, size_type idx):
			m_array(&array),
			m_idx(idx)
		{
			finish_init();
		}
		
		inline void finish_init();
		inline void set_value();
		
	public:
		merge_proxy(merge_proxy const &other) = default;
		merge_proxy(merge_proxy &&other) = default;

		// Proxies may be temporary, hence no “&” in the end of the declaration.
		inline merge_proxy &operator=(merge const &merge);
		inline merge_proxy &operator=(merge &&merge);
		inline merge_proxy &operator=(merge_proxy const &other);
		inline merge_proxy &operator=(merge_proxy &&other);
		
		inline bool operator<(merge const &merge) const;
		inline bool operator<(merge_proxy const &other) const;
		
		inline void swap(merge_proxy &other);
	};
	
	
	// Make dereference() return either a merge or a proxy.
	template <typename t_it_val>
	struct merge_array_iterator_trait {};
	
	template <>
	struct merge_array_iterator_trait <merge>
	{
		typedef merge_array const array_type;
		
		static inline merge dereference(array_type &merge_array, std::size_t const idx);
	};
	
	template <>
	struct merge_array_iterator_trait <merge_proxy>
	{
		typedef merge_array array_type;
		
		static inline merge_proxy dereference(array_type &merge_array, std::size_t const idx);
	};
	
	
	template <typename t_it_val>
	class merge_array_iterator_tpl : public boost::iterator_facade <
		merge_array_iterator_tpl <t_it_val>,
		t_it_val,
		boost::random_access_traversal_tag,
		t_it_val
	>
	{
		friend class boost::iterator_core_access;
		friend class merge_array;

	protected:
		typedef boost::iterator_facade <
			merge_array_iterator_tpl <t_it_val>,
			t_it_val,
			boost::random_access_traversal_tag,
			t_it_val
		> base_class;
		
		typedef merge_array_iterator_trait <t_it_val> trait_type;
		
	public:
		typedef std::size_t size_type;
		typedef typename base_class::difference_type difference_type;
		
	protected:
		typename trait_type::array_type *m_array{nullptr};
		size_type m_idx{0};
		
	private:
		// For the converting constructor below.
		struct enabler {};
		
	public:
		merge_array_iterator_tpl() = default;
		
		merge_array_iterator_tpl(typename trait_type::array_type &array, size_type idx):
			m_array(&array),
			m_idx(idx)
		{
		}

		// Remove the conversion from iterator to const_iterator as per
		// http://www.boost.org/doc/libs/1_58_0/libs/iterator/doc/iterator_facade.html#telling-the-truth
		template <typename t_other_val>
		merge_array_iterator_tpl(
			merge_array_iterator_tpl <t_other_val> const &other, typename boost::enable_if<
				boost::is_convertible <t_other_val *, t_it_val *>,
				enabler
			>::type = enabler()
		):
			m_array(other.m_array),
			m_idx(other.m_idx)
		{
		}
		
	private:
		void increment() { advance(1); }
		void decrement() { advance(-1); }
		
		void advance(difference_type n) { m_idx += n; }
		
		template <typename t_other_val>
		difference_type distance_to(merge_array_iterator_tpl <t_other_val> const &other) const
		{
			assert(m_array == other.m_array);
			return other.m_idx - m_idx;
		}
		
		template <typename t_other_val>
		bool equal(merge_array_iterator_tpl <t_other_val> const &other) const
		{
			return m_array == other.m_array && m_idx == other.m_idx;
		}
		
		t_it_val dereference() const
		{
			return merge_array_iterator_trait <t_it_val>::dereference(*m_array, m_idx);
		}
	};
	
	
	class merge_array
	{
	public:
		typedef sdsl::int_vector <>::size_type size_type;
		typedef merge_array_iterator_tpl <merge_proxy>	iterator;
		typedef merge_array_iterator_tpl <merge>		const_iterator;
		
	protected:
		sdsl::int_vector <> m_left;
		sdsl::int_vector <> m_right;
		sdsl::int_vector <> m_length;
		
	public:
		merge_array() = default;
		
		merge_array(std::size_t const count, std::size_t const bits):
			m_left(count, 0, bits),
			m_right(count, 0, bits),
			m_length(count, 0, bits)
		{
		}
		
		inline void get(size_type const k, merge &merge) const
		{
			merge.left		= m_left[k];
			merge.right		= m_right[k];
			merge.length	= m_length[k];
		}
		
		inline void set(size_type const k, merge const &merge)
		{
			m_left[k]		= merge.left;
			m_right[k]		= merge.right;
			m_length[k]		= merge.length;
		}
		
		inline iterator begin()					{ return iterator(*this, 0); }
		inline const_iterator begin() const		{ return const_iterator(*this, 0); }
		inline const_iterator cbegin() const	{ return const_iterator(*this, 0); }
		inline iterator end()					{ return iterator(*this, m_left.size()); }
		inline const_iterator end() const		{ return const_iterator(*this, m_left.size()); }
		inline const_iterator cend() const		{ return const_iterator(*this, m_left.size()); }
	};
	
	
	inline void swap(merge_proxy &lhs, merge_proxy &rhs)
	{
		lhs.swap(rhs);
	}

	// Proxies may be temporary but need to be swapped in order for
	// set() to be called.
	inline void swap(merge_proxy &&lhs, merge_proxy &&rhs)
	{
		lhs.swap(rhs);
	}

	
	inline void merge_proxy::swap(merge_proxy &other)
	{
		using std::swap;
		swap(m_merge, other.m_merge);
		
		set_value();
		other.set_value();
	}
	
	inline void merge_proxy::set_value()
	{
		m_array->set(m_idx, m_merge);
	}

	inline void merge_proxy::finish_init()
	{
		m_array->get(m_idx, m_merge);
	}
	
	inline merge_proxy &merge_proxy::operator=(merge const &merge)
	{
		m_merge = merge;
		set_value();
		return *this;
	}
	
	inline merge_proxy &merge_proxy::operator=(merge &&merge)
	{
		m_merge = std::move(merge);
		set_value();
		return *this;
	}
	
	inline merge_proxy &merge_proxy::operator=(merge_proxy const &other)
	{
		m_merge = other.m_merge;
		set_value();
		return *this;
	}
	
	inline merge_proxy &merge_proxy::operator=(merge_proxy &&other)
	{
		m_merge = std::move(other.m_merge);
		set_value();
		return *this;
	}
	
	inline bool merge_proxy::operator<(merge const &merge) const
	{
		return m_merge.left < merge.left;
	}
	
	inline bool merge_proxy::operator<(merge_proxy const &other) const
	{
		return m_merge.left < other.m_merge.left;
	}
	
	
	inline merge merge_array_iterator_trait <merge>::dereference(array_type &merge_array, std::size_t const idx)
	{
		merge merge;
		merge_array.get(idx, merge);
		return merge;
	}
	
	inline merge_proxy merge_array_iterator_trait <merge_proxy>::dereference(array_type &merge_array, std::size_t const idx)
	{
		merge_proxy mp(merge_array, idx);
		return mp;
	}
}}

#endif
