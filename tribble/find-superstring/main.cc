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


// Timing with wallclock.
static tribble::timer s_timer;


void handle_error()
{
	char const *errmsg(strerror(errno));
	std::cerr << "Got an error: " << errmsg << std::endl;
	exit(EXIT_FAILURE);
}


// Report the elapsed time at exit.
void handle_atexit()
{
	s_timer.stop();
	std::cerr << "Total time elapsed: " << s_timer.ms_elapsed() << " ms." << std::endl;
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
	std::atexit(handle_atexit);

	gengetopt_args_info args_info;
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);
	
	// See if memory monitor should be used.
	bool const log_memory_usage(args_info.output_memory_usage_given);
	ios::stream <ios::file_descriptor_sink> mem_usage_stream;
	if (log_memory_usage)
	{
		int fd(open(args_info.output_memory_usage_arg, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR));
		if (-1 == fd)
			handle_error();
		
		ios::file_descriptor_sink sink(fd, ios::close_handle);
		mem_usage_stream.open(sink);

		sdsl::memory_monitor::start();
	}

	// Check the mode and execute.
	if (args_info.create_index_given)
	{
		open_source_file_and_execute(args_info, [](std::istream &istream){ create_index(istream, '#'); });
	}
	else if (args_info.index_visualization_given)
	{
		open_source_file_and_execute(args_info, [](std::istream &istream){ visualize(istream); });
	}
	else if (args_info.find_superstring_given)
	{
		tribble::Superstring_callback cb;
		find_suffixes(args_info.source_file_given ? args_info.source_file_arg : nullptr, '#', cb);
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
