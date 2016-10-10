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
#include "cmdline.h"
#include "find_superstring.hh"
#include "superstring_callback.hh"


namespace chrono = std::chrono;
namespace ios = boost::iostreams;


// Timing with wallclock.
static chrono::milliseconds s_start_timestamp{};


// Get the current time in milliseconds.
chrono::milliseconds timestamp_ms_now()
{
	return chrono::duration_cast <chrono::milliseconds>(chrono::system_clock::now().time_since_epoch());
}


void handle_error()
{
	char const *errmsg(strerror(errno));
	std::cerr << "Got an error: " << errmsg << std::endl;
	exit(EXIT_FAILURE);
}


// Report the elapsed time at exit.
void handle_atexit()
{
	chrono::milliseconds end_timestamp(timestamp_ms_now());
	std::cerr << "Milliseconds elapsed: " << (end_timestamp - s_start_timestamp).count() << std::endl;
}


// If the source file was given, open it. Otherwise read from stdin.
void open_source_file_and_execute(gengetopt_args_info const &args_info, std::function <void(std::istream &)> cb)
{
	if (args_info.source_file_given)
	{
		int fd(open(args_info.source_file_arg, O_RDONLY));
		if (-1 == fd)
			handle_error();
		ios::stream <ios::file_descriptor_source> source_stream(fd, ios::close_handle);
		cb(source_stream);
	}
	else
	{
		cb(std::cin);
	}
}


int main(int argc, char **argv)
{
	s_start_timestamp = timestamp_ms_now();
	std::atexit(handle_atexit);

	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);

	// Check the mode and execute.
	if (args_info.create_index_given)
	{
		open_source_file_and_execute(args_info, [](std::istream &istream){ create_index(istream, '#'); });
	}
	else if (args_info.find_superstring_given)
	{
		/*auto cb = [](std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end) -> bool {
			std::cerr
				<< "Found match for substring " << read_lex_rank << " of length " << match_length
				<< ": [" << match_sa_begin << ", " << match_sa_end << "]" << std::endl;
			return true;
		};*/
		
		namespace p = std::placeholders;
		
		Superstring_callback cb(5); // TODO: 5 should be the number of reads
		auto fn(std::bind(&Superstring_callback::callback, &cb, p::_1, p::_2, p::_3, p::_4));
		find_suffixes(args_info.source_file_given ? args_info.source_file_arg : nullptr, '#', fn);
	}
	else
	{
		std::cerr << "Error: No mode given." << std::endl;
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}
