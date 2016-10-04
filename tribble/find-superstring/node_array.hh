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


#include <sdsl/bits.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/cst_sct3.hpp>


// Container for sdsl::cst_sct3 nodes. Since each node consists of
// five integers, the values are stored column-wise in order to
// achieve O(m log n) space complexity where n is the suffix array
// size and m is the number of stored items.
namespace tribble {

	template <typename t_int>
	class node_array
	{
	public:
		typedef sdsl::bp_interval <t_int> node_type;
		
	protected:
		sdsl::int_vector <> m_i;		// LCP interval
		sdsl::int_vector <> m_j;		// LCP interval
		sdsl::int_vector <> m_ipos;		// BPS
		sdsl::int_vector <> m_cipos;	// BPS
		sdsl::int_vector <> m_jp1pos;	// BPS
		
	protected:
		static std::size_t bits_for_n(std::size_t const n)
		{
			return 1 + sdsl::bits::hi(n);
		}
		
		static std::size_t bits_for_2n(std::size_t const n)
		{
			return 1 + sdsl::bits::hi(2 * n);
		}
		
	public:
		node_array(std::size_t const count, std::size_t const n):
			m_i(count, 0, bits_for_n(n)),
			m_j(count, 0, bits_for_n(n)),
			m_ipos(count, 0, bits_for_2n(n)),
			m_cipos(count, 0, bits_for_2n(n)),
			m_jp1pos(count, 0, bits_for_2n(n))
		{
		}
		
		node_type get(std::size_t const i) const
		{
			node_type retval(m_i[i], m_j[i], m_ipos[i], m_cipos[i], m_jp1pos[i]);
			return retval;
		}
		
		void set(std::size_t const i, node_type const &node)
		{
			m_i[i] = node.i;
			m_j[i] = node.j;
			m_ipos[i] = node.ipos;
			m_cipos[i] = node.cipos;
			m_jp1pos[i] = node.jp1pos;
		}
	};
}

