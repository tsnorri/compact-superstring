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
#include "timer.hh"


namespace ios = boost::iostreams;


typedef ios::stream <ios::file_descriptor_source> file_istream;
typedef ios::stream <ios::file_descriptor_sink> file_ostream;


// Timing with wallclock.
static tribble::timer s_timer;


void handle_error()
{
	char const *errmsg(strerror(errno));
	std::cerr << "Got an error: " << errmsg << std::endl;
	exit(EXIT_FAILURE);
}

void handle_exception(std::exception const &exc)
{
	std::cerr << "Got an exception: " << exc.what() << std::endl;
	exit(EXIT_FAILURE);
}

void handle_file_error(char const *fname)
{
	char const *errmsg(strerror(errno));
	std::cerr << "Got an error while trying to open '" << fname << "': " << errmsg << std::endl;
	exit(EXIT_FAILURE);
}


// Report the elapsed time at exit.
void handle_atexit()
{
	s_timer.stop();
	std::cerr << "Total time elapsed: " << s_timer.ms_elapsed() << " ms." << std::endl;
}


void open_file_for_reading(char const *fname, file_istream &stream)
{
	int fd(open(fname, O_RDONLY));
	if (-1 == fd)
		handle_file_error(fname);
	
	ios::file_descriptor_source source(fd, ios::close_handle);
	stream.open(source);
}


void open_file_for_writing(char const *fname, file_ostream &stream)
{
	int fd(open(fname, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR));
	if (-1 == fd)
		handle_file_error(fname);
	
	ios::file_descriptor_sink sink(fd, ios::close_handle);
	stream.open(sink);
}


int main(int argc, char **argv)
{
	std::atexit(handle_atexit);

	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);
	
	// Check the remaining options.
	if (args_info.create_index_given || args_info.find_superstring_given)
	{
		if (!args_info.sorted_strings_file_given)
		{
			std::cerr << "The sorted strings file needs to be specified." << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	
	// See if memory monitor should be used.
	bool const log_memory_usage(args_info.output_memory_usage_given);
	file_ostream mem_usage_stream;
	if (log_memory_usage)
	{
		open_file_for_writing(args_info.output_memory_usage_arg, mem_usage_stream);
		sdsl::memory_monitor::start();
	}

	// Check the mode and execute.
	if (args_info.create_index_given)
	{
		file_istream fasta_stream;
		file_ostream index_stream;
		file_ostream strings_stream;
		
		open_file_for_reading(args_info.source_file_arg, fasta_stream);
		open_file_for_writing(args_info.index_file_arg, index_stream);
		open_file_for_writing(args_info.sorted_strings_file_arg, strings_stream);
		
		create_index(fasta_stream, index_stream, strings_stream, args_info.sorted_strings_file_arg, '#');
	}
	else if (args_info.find_superstring_given)
	{
		file_istream index_stream;
		file_istream strings_stream;
		
		open_file_for_reading(args_info.index_file_arg, index_stream);
		open_file_for_reading(args_info.sorted_strings_file_arg, strings_stream);
		
		tribble::Superstring_callback cb;
		//find_superstring_match_dummy_callback cb;
		find_suffixes(index_stream, strings_stream, '#', cb);
	}
	else if (args_info.index_visualization_given)
	{
		file_istream index_stream;
		open_file_for_reading(args_info.index_file_arg, index_stream);
		visualize(index_stream);
	}
	else
	{
		std::cerr << "Error: No mode given." << std::endl;
		exit(EXIT_FAILURE);
	}
	
	if (log_memory_usage)
	{
		sdsl::memory_monitor::stop();
		sdsl::memory_monitor::write_memory_log <sdsl::HTML_FORMAT>(mem_usage_stream);
	}

	return EXIT_SUCCESS;
}
