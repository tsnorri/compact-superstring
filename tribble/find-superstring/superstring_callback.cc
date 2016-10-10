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
#include <cassert>
#include <algorithm>

Superstring_callback::Superstring_callback() 
: merges_done(0), n_strings(-1) {}

bool Superstring_callback::try_merge(std::size_t left_string, std::size_t right_string, std::size_t overlap_length){
	assert(left_string < leftend.size());
	assert(right_string < leftend.size());
	assert(right_string < rightavailable.size());
    assert(rightavailable[right_string]);

    //std::cout << "left = " << left_string << ", right = " << right_string << std::endl;

    //assert(rightavailable[right_string] == true);
    if(leftend[left_string] != right_string){
        merges.push_back(std::make_tuple(left_string, right_string, overlap_length));
        rightavailable[right_string] = false;
        leftend[rightend[right_string]] = leftend[left_string];
        rightend[leftend[left_string]] = rightend[right_string];
        merges_done++;
        //std::cout << "leftend[" << right_string << "] = leftend[" << left_string << "]" << std::endl;
        //std::cout << "leftend[" << right_string << "] = " << leftend[left_string] << std::endl;
        //for(int x : rightavailable) std::cout << x; std::cout << std::endl;
        //std::cout << "leftend:  "; for(int x : leftend) std::cout << x; std::cout << std::endl;
        //std::cout << "rightend: "; for(int x : rightend) std::cout << x; std::cout << std::endl;
        std::cout << "Merged " << left_string << " " << right_string << std::endl;
        return true;
    }
    return false;
}


void Superstring_callback::set_substring_count(std::size_t count){
    n_strings = count;
    leftend.resize(n_strings);
    rightend.resize(n_strings);
    next.resize(n_strings);
    rightavailable.resize(n_strings);
    for(std::size_t i = 0; i < n_strings; i++){
        leftend[i] = i;
        rightend[i] = i;
        next[i] = i+1;
        rightavailable[i] = true;
    }
}


bool Superstring_callback::callback(std::size_t read_lex_rank, std::size_t match_length, std::size_t match_sa_begin, std::size_t match_sa_end){
    assert(n_strings != -1);
    if(merges_done >= n_strings - 1) return true; // No more merges can be done
    
    std::cout << "CALLBACK " << read_lex_rank << " " << match_length << " " << match_sa_begin << " " << match_sa_end << std::endl;
    assert(read_lex_rank != 0);
    
    // Change to 0-based indexing
    read_lex_rank -= 2; 
    match_sa_begin -= 2;
    match_sa_end -= 2;
    
    std::size_t k = get_next_right_available(match_sa_begin-1);
    if(k > match_sa_end) {
        // Next one is outside of the suffix array interval, or not found at all
        return false;
    }
    
    if(try_merge(read_lex_rank, k, match_length)) return true;
    
    // Failed, try again a second time
    k = get_next_right_available(k);
    
    if(k > match_sa_end) {
        // Next one is outside of the suffix array interval, or not found at all
        return false;
    }
    
    if(try_merge(read_lex_rank, k, match_length)) return true;
    
    return false; // Should never come here, because the second try should always be succesful
    
}

std::size_t Superstring_callback::get_next_right_available(std::size_t index){
    for(std::size_t i = index+1; i < n_strings; i++){
		assert(i < rightavailable.size());
        if(rightavailable[i]) 
            return i;
    }
    return n_strings;
}

std::string Superstring_callback::build_final_superstring(std::vector<std::string> strings){
    if(strings.size() == 0) return "";
    if(strings.size() == 1) return strings[0];
    
    std::sort(merges.begin(), merges.end());
    std::size_t current_string_idx = n_strings;
    
    // Initilize current_string_idx to the first string in the final superstring
    for(std::size_t i = 0; i < n_strings; i++){
		assert(i < rightavailable.size());
        if(rightavailable[i]){
            current_string_idx = i;
            break;
        }
    }
    assert(current_string_idx != n_strings);
        
    // Concatenate all strings putting in the overlapping region of adjacent strings only once
    std::size_t current_overlap_length;
    std::string superstring;
    for(std::size_t i = 0; i < n_strings - 1; i++){
        std::size_t left_string_idx = std::get<0> (merges[current_string_idx]);
        std::size_t right_string_idx = std::get<1> (merges[current_string_idx]);
        std::size_t overlap = std::get<1> (merges[current_string_idx]);
        
        superstring += strings[left_string_idx].substr(strings[left_string_idx].size() - overlap);
        if(i == n_strings - 2) superstring += strings[right_string_idx]; // Last iteration
        left_string_idx = right_string_idx;
    }
    
    return superstring;
}

