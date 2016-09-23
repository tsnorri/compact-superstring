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

#ifndef TRIBBLE_FASTA_READER_HH
#define TRIBBLE_FASTA_READER_HH

#include <array>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <iostream>
#include <iterator>
#include <vector>
#include "vector_source.hh"


namespace tribble { namespace detail {
	struct fasta_reader_cb {
		void handle_sequence(
			std::string const &identifier,
			std::unique_ptr <std::vector <char>> &seq,
			vector_source &vector_source
		)
		{
			vector_source.put_vector(seq);
		}
		
		void finish() {}
	};
}}


namespace tribble {
	
	template <typename t_callback = detail::fasta_reader_cb, size_t t_initial_size = 0>
	class fasta_reader
	{
	protected:
		typedef std::vector <char> vector_type;
		
	public:
		void read_from_stream(std::istream &stream, vector_source &vector_source, t_callback &cb) const
		{
			std::size_t const size(1024 * 1024);
			std::size_t seq_length(0);
			std::vector <char> buffer;
			buffer.resize(size);
			buffer[size - 1] = '\0';
			std::unique_ptr <vector_type> seq;
			std::string current_identifier;
			
			try
			{
				uint32_t i(0);

				vector_source.get_vector(seq);
				if (t_initial_size)
					seq->reserve(t_initial_size);

				while (stream.getline(buffer.data(), size - 1, '\n'))
				{
					++i;
					
					// Discard comments.
					auto const first(buffer[0]);
					if (';' == first)
						continue;
					
					// Delimiter is counted in gcount.
					std::streamsize const count(stream.gcount() - 1);
					if ('>' == first)
					{
						// Reset the sequence length and get a new vector.
						if (seq_length)
						{
							seq_length = 0;
							
							assert(seq.get());
							cb.handle_sequence(current_identifier, seq, vector_source);
							assert(nullptr == seq.get());
							
							vector_source.get_vector(seq);
							if (t_initial_size && seq->capacity() < t_initial_size)
								seq->reserve(t_initial_size);
							seq->clear(); // Doesn't change the capacity.
						}
						
						current_identifier = std::string(1 + buffer.data());
						continue;
					}
					
					seq_length += count;
					
					auto const capacity(seq->capacity());
					if (capacity < seq_length)
					{
						if (2 * capacity < capacity)
							throw std::runtime_error("Can't reserve more space.");
						seq->reserve(2 * capacity); // May reserve more than 2 * capacity.
					}

					auto const it(buffer.cbegin());
					std::copy(it, it + count, std::back_inserter(*seq.get()));
				}
				
				if (seq_length)
				{
					assert(seq.get());
					cb.handle_sequence(current_identifier, seq, vector_source);
				}
			}
			catch (std::ios_base::failure &exc)
			{
				if (std::cin.eof())
				{
					if (seq_length)
					{
						assert(seq.get());
						cb.handle_sequence(current_identifier, seq, vector_source);
					}
				}
				else
				{
					throw (exc);
				}
			}
			
			cb.finish();
		}
	};
}

#endif
