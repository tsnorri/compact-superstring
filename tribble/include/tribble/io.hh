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

#ifndef TRIBBLE_IO_HH
#define TRIBBLE_IO_HH

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>


namespace tribble {
	
	typedef boost::iostreams::stream <
		boost::iostreams::file_descriptor_source
	> file_istream;
	typedef boost::iostreams::stream <
		boost::iostreams::file_descriptor_sink
	> file_ostream;

	void open_file_for_reading(char const *fname, file_istream &stream);
	void open_file_for_writing(char const *fname, file_ostream &stream);
}

#endif