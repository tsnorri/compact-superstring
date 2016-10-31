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


// Make some CST operations faster when building with assertions.
#ifdef NDEBUG
#	define TRIBBLE_ASSERTIONS_ENABLED	(0)
#	define TRIBBLE_SA_SAMPLES			(1 << 20)
#	define TRIBBLE_ISA_SAMPLES			(1 << 20)
#else
#	define TRIBBLE_ASSERTIONS_ENABLED	(1)
#	define TRIBBLE_SA_SAMPLES			(4)
#	define TRIBBLE_ISA_SAMPLES			(4)
#endif

#ifndef DEBUGGING_OUTPUT
#	define DEBUGGING_OUTPUT 0
#endif

#ifdef EXPENSIVE_ASSERTIONS
#	define expensive_assert(x) assert((x))
#else
#	define expensive_assert(x) ((void)0)
#endif


namespace tribble {

	typedef sdsl::wt_hutu <>												wt_type;
	typedef sdsl::csa_wt <wt_type, TRIBBLE_SA_SAMPLES, TRIBBLE_ISA_SAMPLES>	csa_type;
	typedef sdsl::lcp_support_tree2 <256>									lcp_support_type;
	typedef sdsl::cst_sct3 <csa_type, lcp_support_type>						cst_type;
	typedef csa_type::size_type												size_type;
	typedef csa_type::alphabet_type											alphabet_type;

	class string_array;

	
	struct index_type
	{
		typedef std::size_t size_type;
		
		cst_type cst;
		sdsl::int_vector <> string_lengths;
		
		bool index_contains_debugging_information{TRIBBLE_ASSERTIONS_ENABLED};
		
		index_type() = default;
		
		index_type(cst_type &cst_p, sdsl::int_vector <> string_lengths_p):
			cst(std::move(cst_p)),
			string_lengths(std::move(string_lengths_p))
		{
		}
		
		size_type serialize(std::ostream &out, sdsl::structure_tree_node *v, std::string name) const
		{
			sdsl::structure_tree_node *child(sdsl::structure_tree::add_child(v, name, "tribble::index_type"));
			size_type written_bytes(0);
											 
			written_bytes += cst.serialize(out, child, "cst");
			written_bytes += string_lengths.serialize(out, child, "string_lengths");
			written_bytes += sdsl::write_member(
				index_contains_debugging_information,
				out,
				child,
				"index_contains_debugging_information"
			);

			sdsl::structure_tree::add_size(child, written_bytes);
			return written_bytes;
		}
		
		void load(std::istream &in)
		{
			cst.load(in);
			string_lengths.load(in);
			sdsl::read_member(index_contains_debugging_information, in);
		}
	};
	
	
	struct error_handler
	{
		virtual void handle_exception(std::exception const &exc) = 0;
		virtual void handle_unknown_exception() = 0;
	};


	struct find_superstring_match_callback
	{
		virtual ~find_superstring_match_callback() {}
		virtual void set_substring_count(std::size_t set_substring_count) = 0;
		virtual void set_alphabet(alphabet_type const &alphabet) = 0;
		virtual void set_strings_stream(std::istream &strings_stream) = 0;
		virtual void set_is_unique_vector(sdsl::bit_vector const &vec) = 0;
		virtual bool callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end) = 0;
		virtual void finish_matching() = 0;
		virtual void build_final_superstring(std::ostream &) = 0;
	};


	struct find_superstring_match_dummy_callback : public find_superstring_match_callback
	{
		void set_substring_count(std::size_t set_substring_count) override;
		void set_alphabet(alphabet_type const &alphabet) override;
		void set_strings_stream(std::istream &strings_stream) override;
		void set_is_unique_vector(sdsl::bit_vector const &vec) override;
		bool callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end) override;
		void finish_matching() override;
		void build_final_superstring(std::ostream &) override;
	};


	void create_index(
		std::istream &source_stream,
		std::ostream &index_stream,
		std::ostream &strings_stream,
		char const *strings_fname,
		char const sentinel,
		error_handler &error_handler
	);
	void check_non_unique_strings(
		cst_type const &cst,
		sdsl::int_vector <> const &string_lengths,
		char const sentinel,
		/* out */ tribble::string_array &strings_available
	);
	void find_suffixes(
		std::istream &index_stream,
		std::istream &strings_stream,
		char const sentinel,
		find_superstring_match_callback &cb
	);
	void visualize(std::istream &index_stream, std::ostream &memory_chart_stream);
}

#endif
