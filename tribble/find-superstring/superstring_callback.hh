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


#ifndef TRIBBLE_SUPERSTRING_CALLBACK_HH
#define TRIBBLE_SUPERSTRING_CALLBACK_HH

#include <cstddef> // std::size_t

class Superstring_callback{
  
public:
    
    bool callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end);
    
private:
    
};

//bool superstring_callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end);

#endif