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

#include <cstdlib>
#include <tribble/io.hh>

#ifdef __linux__
#include <pthread_workqueue.h>
#endif

#include "cmdline.h"
#include "verify_superstring.hh"


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	bool const multi_threaded(true);
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);

	std::ios_base::sync_with_stdio(false);	// Don't use C style IO after calling cmdline_parser.
	std::cin.tie(nullptr);					// We don't require any input from the user.
	
	// libdispatch on macOS does not need pthread_workqueue.
#ifdef __linux__
	pthread_workqueue_init_np();
#endif

	if (args_info.create_index_given)
	{
		tribble::file_ostream index_stream;
		tribble::open_file_for_writing(args_info.index_file_arg, index_stream);

		tribble::cst_type cst;
		std::cerr << "Constucting CST..." << std::endl;
		sdsl::construct(cst, args_info.superstring_file_arg, 1);
		std::cerr << "Serializing..." << std::endl;
		sdsl::serialize(cst, index_stream);
	}
	else if (args_info.verify_superstring_given)
	{
		tribble::verify_superstring(
			args_info.index_file_arg,
			args_info.source_file_arg,
			args_info.source_format_arg,
			multi_threaded
		);
	}
	else
	{
		std::cerr << "ERROR: No mode given." << std::endl;
		exit(EXIT_FAILURE);
	}

	// Not reached in case args_info.verify_superstring_given
	// since pthread_exit() is eventually called in verify_superstring().
	return EXIT_SUCCESS;
}
