/*
 Copyright (c) 2016 Jarno Alanko
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

#include "superstring_callback.hh"
#include <iostream>

bool Superstring_callback::callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end){
    std::cout << read_lex_rank << " " << match_length << " " << match_sa_begin << " " << match_sa_end << std::endl;
    
    return true;
}