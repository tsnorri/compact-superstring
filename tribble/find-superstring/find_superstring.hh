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


#ifndef TRIBBLE_FIND_SUPERSTRING_HH
#define TRIBBLE_FIND_SUPERSTRING_HH

#include <istream>
#include <sdsl/cst_sct3.hpp>

#ifndef DEBUGGING_OUTPUT
#define DEBUGGING_OUTPUT 0
#endif

typedef sdsl::wt_huff <>										wt_type;
typedef sdsl::csa_wt <wt_type, 16, 1 << 20>						csa_type;
typedef sdsl::cst_sct3 <csa_type, sdsl::lcp_support_sada <>>	cst_type;
typedef csa_type::size_type										size_type;


struct find_superstring_match_callback
{
	virtual ~find_superstring_match_callback() {}
	virtual void set_substring_count(std::size_t set_substring_count) = 0;
	virtual bool callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end) = 0;
};

extern "C" void create_index(std::istream &source_stream, char const sentinel);
extern "C" void sort_strings_by_length(
	csa_type const &csa,
	char const sentinel,
	/* out */ sdsl::int_vector <> &sorted_bwt_indices,
	/* out */ sdsl::int_vector <> &sorted_bwt_start_indices,
	/* out */ sdsl::int_vector <> &string_lengths
);
extern "C" void find_suffixes(char const *source_fname, char const sentinel, find_superstring_match_callback &cb);
extern "C" void visualize(std::istream &stream);
extern "C" void handle_error();

#endif
