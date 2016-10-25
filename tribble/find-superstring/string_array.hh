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

#ifndef TRIBBLE_STRING_ARRAY_HH
#define TRIBBLE_STRING_ARRAY_HH

#include <boost/iterator/iterator_facade.hpp>
#include "find_superstring.hh"
#include "boost_enable_if.hh"


namespace tribble {
	
	class string_array;
	
	struct string_type
	{
		typedef sdsl::int_vector <0>::size_type size_type;
		
		size_type	sa_idx;
		size_type	match_start_sa_idx;
		size_type	length;
		size_type	branching_suffix_length;
		bool		is_unique;
		
		string_type():
			string_type(0, 0, false)
		{
		}
		
		string_type(size_type const idx, size_type const len, bool const is_unique):
			sa_idx(idx),
			match_start_sa_idx(0),
			length(len),
			branching_suffix_length(0),
			is_unique(is_unique)
		{
		}
		
		inline bool operator<(string_type const &string) const
		{
			// Only compare lengths for now.
			return length < string.length;
		}
	};
	
	
	inline std::ostream &operator<<(std::ostream &os, string_type &s)
	{
		os
		<< "sa_idx: " << s.sa_idx
		<< " match_start_sa_idx: " << s.match_start_sa_idx
		<< " length: " << s.length
		<< " branching_suffix_length: " << s.branching_suffix_length
		<< " is_unique: " << s.is_unique;
		return os;
	}
}


namespace tribble { namespace detail {
	
	// std::sort requires writable iterators, so we provide ones by using this proxy.
	// To provide minimum support, only assignment operators of the proxy (and not
	// those of the data members) are usable.
	// The representation isn't particularly efficient.
	class string_proxy
	{
		template <typename> friend struct string_array_iterator_trait;
		
	public:
		typedef std::size_t size_type;
		
	protected:
		string_type m_string{};
		string_array *m_array;
		size_type m_idx;
		
	public:
		// FIXME: these do not work.
#if 0
		size_type const	&sa_idx{m_string.sa_idx};
		size_type const	&match_start_sa_idx{m_string.match_start_sa_idx};
		size_type const	&length{m_string.length};
		size_type const	&branching_suffix_length{m_string.branching_suffix_length};
		bool const		&is_unique{m_string.is_unique};
#endif
		
	protected:
		string_proxy(string_array &array, size_type idx):
			m_array(&array),
			m_idx(idx)
		{
			finish_init();
		}
		
		inline void finish_init();
		inline void set_value();
		
	public:
		string_proxy(string_proxy const &other) = default;
		string_proxy(string_proxy &&other) = default;

		// Proxies may be temporary, hence no “&” in the end of the declaration.
		inline string_proxy &operator=(string_type const &string);
		inline string_proxy &operator=(string_type &&string_type);
		inline string_proxy &operator=(string_proxy const &other);
		inline string_proxy &operator=(string_proxy &&other);
		
		inline bool operator<(string_type const &string) const;
		inline bool operator<(string_proxy const &other) const;
		
		inline void swap(string_proxy &other);
	};
	
	
	// Make dereference() return either a string_type or a proxy.
	template <typename t_it_val>
	struct string_array_iterator_trait {};
	
	template <>
	struct string_array_iterator_trait <string_type>
	{
		typedef string_array const array_type;
		
		static inline string_type dereference(array_type &string_array, std::size_t const idx);
	};
	
	template <>
	struct string_array_iterator_trait <string_proxy>
	{
		typedef string_array array_type;
		
		static inline string_proxy dereference(array_type &string_array, std::size_t const idx);
	};
	
	
	template <typename t_it_val>
	class string_array_iterator_tpl : public boost::iterator_facade <
		string_array_iterator_tpl <t_it_val>,
		t_it_val,
		boost::random_access_traversal_tag,
		t_it_val
	>
	{
		friend class boost::iterator_core_access;
		friend class string_array;

	protected:
		typedef boost::iterator_facade <
			string_array_iterator_tpl <t_it_val>,
			t_it_val,
			boost::random_access_traversal_tag,
			t_it_val
		> base_class;
		
		typedef string_array_iterator_trait <t_it_val> trait_type;
		
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
		string_array_iterator_tpl() = default;
		
		string_array_iterator_tpl(typename trait_type::array_type &array, size_type idx):
			m_array(&array),
			m_idx(idx)
		{
		}

		// Remove the conversion from iterator to const_iterator as per
		// http://www.boost.org/doc/libs/1_58_0/libs/iterator/doc/iterator_facade.html#telling-the-truth
		template <typename t_other_val>
		string_array_iterator_tpl(
			string_array_iterator_tpl <t_other_val> const &other, typename boost::enable_if<
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
		difference_type distance_to(string_array_iterator_tpl <t_other_val> const &other) const
		{
			assert(m_array == other.m_array);
			return other.m_idx - m_idx;
		}
		
		template <typename t_other_val>
		bool equal(string_array_iterator_tpl <t_other_val> const &other) const
		{
			return m_array == other.m_array && m_idx == other.m_idx;
		}
		
		t_it_val dereference() const
		{
			return string_array_iterator_trait <t_it_val>::dereference(*m_array, m_idx);
		}
	};
}}
	

namespace tribble {
	
	class string_array
	{
	public:
		typedef sdsl::int_vector <0>::size_type								size_type;
		typedef detail::string_array_iterator_tpl <detail::string_proxy>	iterator;
		typedef detail::string_array_iterator_tpl <string_type>				const_iterator;

	protected:
		sdsl::int_vector <0> m_sa_idxs;
		sdsl::int_vector <0> m_match_start_sa_idxs;
		sdsl::int_vector <0> m_lengths;
		sdsl::int_vector <0> m_branching_suffix_lengths;
		sdsl::bit_vector m_is_unique;
		
	public:
		string_array() = default;
		
		string_array(size_type const count, size_type const count_bits, size_type const n_bits, size_type const max_length_bits):
			m_sa_idxs(count, 0, count_bits),
			m_match_start_sa_idxs(count, 0, n_bits),
			m_lengths(count, 0, max_length_bits),
			m_branching_suffix_lengths(count, 0, max_length_bits),
			m_is_unique(count, 1)
		{
		}
		
		inline size_type size() const { return m_sa_idxs.size(); }
		inline size_type max_string_length() const { return m_lengths[size() - 1]; }
		inline sdsl::bit_vector const &is_unique_vector() const { return m_is_unique; }
		
		inline void get(size_type const k, string_type &string) const
		{
			string.sa_idx					= m_sa_idxs[k];
			string.match_start_sa_idx		= m_match_start_sa_idxs[k];
			string.length					= m_lengths[k];
			string.branching_suffix_length	= m_branching_suffix_lengths[k];
			string.is_unique				= m_is_unique[k];
		}
		
		inline void set(size_type const k, string_type &string)
		{
			m_sa_idxs[k]					= string.sa_idx;
			m_match_start_sa_idxs[k]		= string.match_start_sa_idx;
			m_lengths[k]					= string.length;
			m_branching_suffix_lengths[k]	= string.branching_suffix_length;
			m_is_unique[k]					= string.is_unique;
		}
		
		inline iterator begin()					{ return iterator(*this, 0); }
		inline const_iterator begin() const		{ return const_iterator(*this, 0); }
		inline const_iterator cbegin() const	{ return const_iterator(*this, 0); }
		inline iterator end()					{ return iterator(*this, size()); }
		inline const_iterator end() const		{ return const_iterator(*this, size()); }
		inline const_iterator cend() const		{ return const_iterator(*this, size()); }
	};
}
	
namespace tribble { namespace detail {
	
	inline void swap(string_proxy &lhs, string_proxy &rhs)
	{
		lhs.swap(rhs);
	}

	// Proxies may be temporary but need to be swapped in order for
	// set() to be called.
	inline void swap(string_proxy &&lhs, string_proxy &&rhs)
	{
		lhs.swap(rhs);
	}

	
	inline void string_proxy::swap(string_proxy &other)
	{
		using std::swap;
		swap(m_string, other.m_string);
		
		set_value();
		other.set_value();
	}
	
	inline void string_proxy::set_value()
	{
		m_array->set(m_idx, m_string);
	}

	inline void string_proxy::finish_init()
	{
		m_array->get(m_idx, m_string);
	}
	
	inline string_proxy &string_proxy::operator=(string_type const &string)
	{
		m_string = string;
		set_value();
		return *this;
	}
	
	inline string_proxy &string_proxy::operator=(string_type &&string)
	{
		m_string = std::move(string);
		set_value();
		return *this;
	}
	
	inline string_proxy &string_proxy::operator=(string_proxy const &other)
	{
		m_string = other.m_string;
		set_value();
		return *this;
	}
	
	inline string_proxy &string_proxy::operator=(string_proxy &&other)
	{
		m_string = std::move(other.m_string);
		set_value();
		return *this;
	}
	
	inline bool string_proxy::operator<(string_type const &string) const
	{
		return m_string < string;
	}
	
	inline bool string_proxy::operator<(string_proxy const &other) const
	{
		return m_string < other.m_string;
	}
	
	
	inline string_type string_array_iterator_trait <string_type>::dereference(array_type &string_array, std::size_t const idx)
	{
		string_type string;
		string_array.get(idx, string);
		return string;
	}
	
	inline string_proxy string_array_iterator_trait <string_proxy>::dereference(array_type &string_array, std::size_t const idx)
	{
		string_proxy sp(string_array, idx);
		return sp;
	}
}}

#endif
