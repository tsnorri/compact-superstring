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


#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <fcntl.h>
#include <iostream>
#include <sdsl/io.hpp>
#include <unistd.h>
#include "fasta_reader.hh"
#include "find_superstring.hh"

namespace ios = boost::iostreams;


class create_index_cb
{
protected:
	char const *m_source_fname{};
	std::ostream *m_output_stream{};
	char m_sentinel{};

public:
	create_index_cb(char const *source_fname, std::ostream &output_stream, char sentinel = '#'):
		m_source_fname(source_fname),
		m_output_stream(&output_stream),
		m_sentinel(sentinel)
	{
		assert(m_source_fname);
	}

	void handle_sequence(
		std::string const &identifier,
		std::unique_ptr <std::vector <char>> &seq,
		tribble::vector_source &vs
	)
	{
		// Output the sentinel.
		*m_output_stream << m_sentinel;

		// Write the sequence to the specified file.
		std::copy(seq->begin(), seq->end(), std::ostream_iterator <char>(*m_output_stream));
		m_output_stream->flush();
		vs.put_vector(seq);
	}


	void finish()
	{
		// Output the final sentinel.
		*m_output_stream << m_sentinel;
		m_output_stream->flush();

		// Read the sequence from the file.
		std::cerr << "Creating the CST…" << std::endl;
		cst_type cst;
		sdsl::construct(cst, m_source_fname, 1);

		// Serialize.
		std::cerr << "Serializing…" << std::endl;
		sdsl::serialize(cst, std::cout);
	}
};


void create_index(std::istream &source_stream)
{
	// SDSL reads the whole string from a file so copy the contents without the newlines
	// into a temporary file, then create the index.
	char temp_fname[]{"/tmp/tribble_find_superstring_XXXXXX"};
	int const temp_fd(mkstemp(temp_fname));
	if (-1 == temp_fd)
		handle_error();

	try
	{
		// Read the sequence from input and create the index in the callback.
		tribble::vector_source vs(1, false);
		tribble::fasta_reader <create_index_cb, 10 * 1024 * 1024> reader;
		ios::stream <ios::file_descriptor_sink> output_stream(temp_fd, ios::close_handle);
		create_index_cb cb(temp_fname, output_stream);

		reader.read_from_stream(source_stream, vs, cb);
	}
	catch (...)
	{
		if (-1 == unlink(temp_fname))
			handle_error();
	}
}
