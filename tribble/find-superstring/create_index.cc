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
#include <unistd.h>
#include "fasta_reader.hh"
#include "find_superstring.hh"
#include "timer.hh"

namespace ios = boost::iostreams;


class create_index_cb
{
public:
	typedef tribble::vector_source::vector_type vector_type;
	
protected:
	std::ostream &m_index_stream;
	std::ostream &m_strings_stream;
	char const *m_strings_fname{};
	std::vector <vector_type> m_sequences;
	tribble::timer m_read_timer{};
	char m_sentinel{};

public:
	create_index_cb(std::ostream &index_stream, std::ostream &strings_stream, char const *strings_fname, char sentinel = '#'):
		m_index_stream(index_stream),
		m_strings_stream(strings_stream),
		m_strings_fname(strings_fname),
		m_sentinel(sentinel)
	{
		assert(m_strings_fname);
	}

	void handle_sequence(
		std::string const &identifier,
		std::unique_ptr <vector_type> &seq,
		std::size_t const seq_length,
		tribble::vector_source &vs
	)
	{
		// Copy the sequence to the collection.
		vector_type seq_copy;
		seq_copy.resize(seq_length);
		std::copy_n(seq->begin(), seq_length, seq_copy.begin());
		m_sequences.emplace_back(seq_copy);
		vs.put_vector(seq);
	}


	void finish()
	{
		{
			m_read_timer.stop();
			std::cerr << " finished in " << m_read_timer.ms_elapsed() << " ms." << std::endl;
		}
		
		std::cerr << "Sorting the sequences…" << std::flush;
		{
			tribble::timer timer;
			
			std::sort(m_sequences.begin(), m_sequences.end());
			
			timer.stop();
			std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
		}
		
		std::cerr << "Writing to the strings file…" << std::flush;
		if (m_sequences.size())
		{
			tribble::timer timer;

			// Output the first sequence.
			decltype(m_sequences)::value_type const &previous_seq(m_sequences[0]);
			m_strings_stream << m_sentinel;
			std::copy(previous_seq.begin(), previous_seq.end(), std::ostream_iterator <char>(m_strings_stream));

			// Output the remaining unique sequences.
			// Swapping performance for readability.
			for (auto const &seq : m_sequences)
			{
				if (seq != previous_seq)
				{
					// Write the sequence to the specified file.
					m_strings_stream << m_sentinel;
					std::copy(seq.begin(), seq.end(), std::ostream_iterator <char>(m_strings_stream));
				}
			}
			
			// Output the final sentinel.
			m_strings_stream << m_sentinel;
			m_strings_stream.flush();
			
			timer.stop();
			std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
		}

		// Read the sequence from the file.
		std::cerr << "Creating the CST…" << std::flush;
		cst_type cst;
		{
			tribble::timer timer;

			sdsl::construct(cst, m_strings_fname, 1);
			
			timer.stop();
			std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
		}
		
		// List the string lengths.
		std::cerr << "Creating other data structures…" << std::flush;
		sdsl::int_vector <> string_lengths;
		{
			tribble::timer timer;

			decltype(string_lengths)::size_type max_length(0);
			for (auto const &seq : m_sequences)
				max_length = std::max(max_length, seq.size());
			
			std::size_t const width(1 + sdsl::bits::hi(max_length));
			string_lengths.width(width);
			string_lengths.resize(m_sequences.size());
			
			std::size_t i(0);
			for (auto const &seq : m_sequences)
			{
				string_lengths[i] = seq.size();
				++i;
			}
			
			timer.stop();
			std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
		}

		// Serialize.
		std::cerr << "Serializing…" << std::flush;
		{
			tribble::timer timer;

			index_type index(cst, string_lengths);
			sdsl::serialize(index, m_index_stream);
			
			timer.stop();
			std::cerr << " finished in " << timer.ms_elapsed() << " ms." << std::endl;
		}
	}
};


void create_index(
	std::istream &source_stream,
	std::ostream &index_stream,
	std::ostream &strings_stream,
	char const *strings_fname,
	char const sentinel
)
{
	try
	{
		// Read the sequence from input and create the index in the callback.
		tribble::vector_source vs(1, false);
		tribble::fasta_reader <create_index_cb, 10 * 1024 * 1024> reader;

		std::cerr << "Reading the sequences…" << std::flush;
		create_index_cb cb(index_stream, strings_stream, strings_fname, sentinel);
		reader.read_from_stream(source_stream, vs, cb);
	}
	catch (std::exception const &exc)
	{
		handle_exception(exc);
	}
	catch (...)
	{
		handle_error();
	}
}
