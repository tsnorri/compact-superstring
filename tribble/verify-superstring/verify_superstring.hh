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

#ifndef TRIBBLE_VERIFY_SUPERSTRING_HH
#define TRIBBLE_VERIFY_SUPERSTRING_HH

#include <istream>
#include <sdsl/cst_sct3.hpp>

namespace tribble {
	
	typedef sdsl::wt_hutu <>							wt_type;
	typedef sdsl::csa_wt <wt_type, 1 << 20, 1 << 20>	csa_type;
	typedef sdsl::lcp_support_tree2 <256>				lcp_support_type;
	typedef sdsl::cst_sct3 <csa_type, lcp_support_type>	cst_type;
	typedef csa_type::size_type							size_type;
	
	void verify_superstring(
		char const *cst_fname,
		char const *fasta_fname,
		bool const multi_threaded
	);
}

#endif
