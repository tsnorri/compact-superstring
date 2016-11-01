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
#include "cmdline.h"
#include "verify_superstring.hh"


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	bool const multi_threaded(true);
	if (0 != cmdline_parser(argc, argv, &args_info))
		exit(EXIT_FAILURE);

	if (args_info.create_index_given)
	{
		tribble::cst_type cst;
		sdsl::construct(cst, args_info.superstring_file_arg, 1);
		sdsl::serialize(cst, std::cout);
	}
	else if (args_info.verify_superstring_given)
	{
		tribble::verify_superstring(
			args_info.index_file_arg,
			args_info.source_file_arg,
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
