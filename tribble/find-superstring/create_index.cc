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

#include <algorithm>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <fcntl.h>
#include <iostream>
#include <sdsl/io.hpp>
#include <tribble/fasta_reader.hh>
#include <tribble/line_reader.hh>
#include <unistd.h>
#include "find_superstring.hh"
#include "timer.hh"

namespace ios = boost::iostreams;


namespace tribble { namespace detail {

// FIXME: re-indent.

// Construct the CST.
// Based on SDSL's construct in construct.hpp.
	template <typename t_index>
	void construct(t_index &idx, std::string const &file, uint8_t const num_bytes)
	{
		auto event(sdsl::memory_monitor::event("Construct CST"));
		sdsl::cache_config config;
		const char* KEY_TEXT(sdsl::key_text_trait <t_index::alphabet_category::WIDTH>::KEY_TEXT);
		const char* KEY_BWT(sdsl::key_bwt_trait <t_index::alphabet_category::WIDTH>::KEY_BWT);
		sdsl::csa_tag csa_t;
		
		{
			// (1) Check if the compressed suffix array is cached.
			typename t_index::csa_type csa;
			if (!cache_file_exists(std::string(sdsl::conf::KEY_CSA) + "_" + sdsl::util::class_to_hash(csa), config))
			{
				sdsl::cache_config csa_config(false, config.dir, config.id, config.file_map);
				sdsl::construct(csa, file, csa_config, num_bytes, csa_t);
				auto event(sdsl::memory_monitor::event("Store CSA"));
				config.file_map = csa_config.file_map;
				sdsl::store_to_cache(csa, std::string(sdsl::conf::KEY_CSA) + "_" + sdsl::util::class_to_hash(csa), config);
			}
			sdsl::register_cache_file(std::string(sdsl::conf::KEY_CSA) + "_" + sdsl::util::class_to_hash(csa), config);
		}
		
		{
			// (2) Check if the longest common prefix array is cached.
			// cst_sct3 uses this for BP construction.
			auto event(sdsl::memory_monitor::event("LCP"));
			sdsl::register_cache_file(KEY_TEXT, config);
			sdsl::register_cache_file(KEY_BWT, config);
			sdsl::register_cache_file(sdsl::conf::KEY_SA, config);
			
			if (!sdsl::cache_file_exists(sdsl::conf::KEY_LCP, config))
			{
				if (t_index::alphabet_category::WIDTH==8)
					sdsl::construct_lcp_semi_extern_PHI(config);
				else
					sdsl::construct_lcp_PHI<t_index::alphabet_category::WIDTH>(config);
			}
			sdsl::register_cache_file(sdsl::conf::KEY_LCP, config);
		}
		
		{
			auto event(sdsl::memory_monitor::event("CST"));
			t_index tmp(config, true, false);
			tmp.swap(idx);
		}
		
		if (config.delete_files)
		{
			auto event(sdsl::memory_monitor::event("Delete temporary files"));
			sdsl::util::delete_all_files(config.file_map);
		}
	}

		
	class create_index_cb
	{
	public:
		typedef vector_source::vector_type vector_type;
		
	protected:
		std::ostream &m_index_stream;
		std::ostream &m_strings_stream;
		char const *m_strings_fname{};
		std::vector <vector_type> m_sequences;
		timer m_read_timer{};
		char m_sentinel{};
		uint32_t m_seqno{0};

	protected:
		void handle_sentinel_in_text()
		{
			throw std::runtime_error("The text contains the sentinel character.");
		}
		
		void copy_seq(
			std::unique_ptr <vector_type> &seq,
			std::size_t const seq_length,
			vector_source &vs
		)
		{
			// Copy the sequence to the collection.
			vector_type seq_copy;
			seq_copy.resize(seq_length);
			std::copy_n(seq->begin(), seq_length, seq_copy.begin());
			m_sequences.emplace_back(seq_copy);
			vs.put_vector(seq);
		}

		
	public:
		create_index_cb(std::ostream &index_stream, std::ostream &strings_stream, char const *strings_fname, char const sentinel):
			m_index_stream(index_stream),
			m_strings_stream(strings_stream),
			m_strings_fname(strings_fname),
			m_sentinel(sentinel)
		{
			assert(m_strings_fname);
		}
		
		// For FASTA.
		void handle_sequence(
			std::string const &identifier,
			std::unique_ptr <vector_type> &seq,
			std::size_t const seq_length,
			vector_source &vs
		)
		{
			copy_seq(seq, seq_length, vs);
			++m_seqno;

			if (0 == m_seqno % 10000)
				std::cerr << " " << m_seqno << std::flush;
		}
		
		// For line-oriented text.
		void handle_sequence(
			uint32_t const lineno,
			std::unique_ptr <vector_type> &seq,
			std::size_t const seq_length,
			vector_source &vs
		)
		{
			copy_seq(seq, seq_length, vs);

			if (0 == lineno % 10000)
				std::cerr << " " << lineno << std::flush;
		}

		void finish()
		{
			assert(m_sentinel);
			
			{
				m_read_timer.stop();
				std::cerr << " finished in " << m_read_timer.ms_elapsed() << " ms, read " << m_sequences.size() << " sequences." << std::endl;
			}
			
			std::cerr << "Sorting the sequences…" << std::flush;
			{
				timer timer;
				
				std::sort(m_sequences.begin(), m_sequences.end());
				
				timer.stop();
				std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
			}
			
			std::cerr << "Writing to the strings file…" << std::flush;
			auto string_count(m_sequences.size());
			if (string_count)
			{
				timer timer;

				// Output the first sequence.
				decltype(m_sequences)::value_type const *previous_seq(&m_sequences[0]);
				m_strings_stream << m_sentinel;
				
				{
					auto const begin(previous_seq->begin());
					auto const end(previous_seq->end());
					
					if (end != std::find(begin, end, m_sentinel))
						handle_sentinel_in_text();
					
					std::copy(begin, end, std::ostream_iterator <char>(m_strings_stream));
				}

				// Output the remaining unique sequences.
				std::size_t unique_count(1);
				for (auto it(1 + m_sequences.begin()), end(m_sequences.end()); it != end; ++it)
				{
					auto &seq(*it); // Returns a reference.
					
					// Check uniqueness.
					if (seq == *previous_seq)
					{
						--string_count;
						
						// Set a placeholder.
						vector_type empty;
						seq = std::move(empty);
						assert(0 == seq.size());
					}
					else
					{
						auto const begin(seq.begin());
						auto const end(seq.end());
						
						// Check for the sentinel.
						if (end != std::find(begin, end, m_sentinel))
							handle_sentinel_in_text();
						
						// Write the sequence to the specified file.
						m_strings_stream << m_sentinel;
						std::copy(begin, end, std::ostream_iterator <char>(m_strings_stream));
						previous_seq = &seq;
						
						++unique_count;
					}
				}
				
				// Output the final sentinel.
				m_strings_stream << m_sentinel;
				m_strings_stream.flush();
				
				timer.stop();
				std::cerr << " finished in " << timer.ms_elapsed() << " ms, found " << unique_count << " unique strings." << std::endl;
			}

			// Read the sequence from the file.
			std::cerr << "Creating the CST…" << std::flush;
			cst_type cst;
			{
				timer timer;

				// Construct with LCP if assertions have been enabled.
				if (TRIBBLE_ASSERTIONS_ENABLED)
					::sdsl::construct(cst, m_strings_fname, 1);
				else
				{
					::tribble::detail::construct(cst, m_strings_fname, 1);
					if (!cst.lcp.empty())
						throw std::runtime_error("Expected LCP to be empty.");
				}
				
				// Check the sentinel.
				auto const comp_val(cst.csa.char2comp[m_sentinel]);
				if (1 != comp_val)
				{
					std::stringstream output;
					output
					<< "The text contains the character '"
					<< std::hex << cst.csa.comp2char[1]
					<< "' the lexicographic value is less than that of the sentinel, '"
					<< std::hex << static_cast <int>(m_sentinel)
					<< "'.";
					
					throw std::runtime_error(output.str());
				}
				
				timer.stop();
				std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
			}
			
			// List the string lengths.
			std::cerr << "Creating other data structures…" << std::flush;
			sdsl::int_vector <> string_lengths;
			{
				timer timer;

				decltype(string_lengths)::size_type max_length(0);
				for (auto const &seq : m_sequences)
					max_length = std::max(max_length, seq.size());
				
				std::size_t const width(1 + sdsl::bits::hi(max_length));
				string_lengths.width(width);
				string_lengths.resize(string_count);
				
				std::size_t i(0);
				for (auto const &seq : m_sequences)
				{
					auto const size(seq.size());
					if (size)
					{
						string_lengths[i] = size;
						++i;
					}
				}
				
				timer.stop();
				std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
			}

			// Serialize.
			std::cerr << "Serializing…" << std::flush;
			{
				timer timer;

				index_type index(cst, string_lengths, m_sentinel);
				sdsl::serialize(index, m_index_stream);
				
				timer.stop();
				std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
			}
		}
	};
}}


namespace tribble {

	void create_index(
		std::istream &source_stream,
		std::ostream &index_stream,
		std::ostream &strings_stream,
		char const *strings_fname,
		enum_source_format const source_format,
		char const sentinel,
		error_handler &error_handler
	)
	{
		try
		{
			// Read the sequence from input and create the index in the callback.
			std::cerr << "Reading the sequences…" << std::flush;
			vector_source vs(1, false);
			detail::create_index_cb cb(index_stream, strings_stream, strings_fname, sentinel);

			if (source_format_arg_FASTA == source_format)
			{
				fasta_reader <detail::create_index_cb> reader;
				reader.read_from_stream(source_stream, vs, cb);
			}
			else if (source_format_arg_text == source_format)
			{
				line_reader <detail::create_index_cb> reader;
				reader.read_from_stream(source_stream, vs, cb);
			}
			else
			{
				std::stringstream output;
				output << "Unexpected source file format '" << source_format << "'.";

				throw std::runtime_error(output.str());
			}
		}
		catch (std::exception const &exc)
		{
			error_handler.handle_exception(exc);
		}
		catch (...)
		{
			error_handler.handle_unknown_exception();
		}
	}
}
