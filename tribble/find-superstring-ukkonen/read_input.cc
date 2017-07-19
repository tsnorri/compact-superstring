/*
 Copyright (c) 2017 Tuukka Norri
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


#include <tribble/fasta_reader.hh>
#include <tribble/line_reader.hh>
#include <tribble/timer.hh>
#include "read_input.hh"


namespace tribble { namespace detail {
	
	class map_alphabet_cb {
		
	protected:
		tribble::timer		m_timer;
		char_map_type		*m_char2comp;
		char_map_type		*m_comp2char;
		std::size_t			m_seqno{0};
		std::size_t			m_charno{0};
		
	public:
		map_alphabet_cb(
			char_map_type &char2comp,
			char_map_type &comp2char
		):
			m_char2comp(&char2comp),
			m_comp2char(&comp2char)
		{
		}
		
		std::size_t const sigma() { return m_charno; }
		
		void process_sequence(vector_source::vector_type &seq)
		{
			for (auto const c : seq)
			{
				auto &ref_1((*m_char2comp)[c]);
				
				// If a character number has already been assigned, skip.
				if (m_charno <= ref_1)
				{
					ref_1 = m_charno;
					auto &ref_2((*m_comp2char)[m_charno]);
					ref_2 = c;
					
					++m_charno;
					
					// If this was the last available character number, stop.
					if (256 == m_charno)
						break;
				}
			}
		}
		
		// For fasta_reader.
		void handle_sequence(
			std::string const &identifier,
			std::unique_ptr <vector_source::vector_type> &seq,
			std::size_t const &seq_length,
			vector_source &vector_source
		)
		{
			process_sequence(*seq);
			vector_source.put_vector(seq);

			++m_seqno;
			if (0 == m_seqno % 10000)
				std::cerr << " " << m_seqno << std::flush;
		}
		
		// For line_reader.
		void handle_sequence(
			uint32_t lineno,
			std::unique_ptr <vector_source::vector_type> &seq,
			std::size_t const &seq_length,
			vector_source &vector_source
		)
		{
			process_sequence(*seq);
			vector_source.put_vector(seq);

			if (0 == lineno % 10000)
				std::cerr << " " << lineno << std::flush;
		}
		
		void start()
		{
			m_timer.start();
			std::cerr << "  Compressing the alphabet…" << std::flush;
		}
		
		void finish()
		{
			m_timer.stop();
			std::cerr << " finished in " << m_timer.ms_elapsed() << " ms." << std::endl;
			
			transition_map_base::set_sigma(m_charno);
		}
	};
	
	
	class create_index_cb {
		
	protected:
		tribble::timer		m_timer;
		string_vector_type	*m_strings{nullptr};
		trie_type			*m_trie{nullptr};
		string_map_type		*m_strings_by_state{nullptr};
		state_map_type		*m_states_by_string{nullptr};
		char_map_type const	*m_char2comp{nullptr};
		std::size_t			m_idx{0};
		std::size_t			m_seqno{0};
		
	public:
		create_index_cb(
			string_vector_type &strings,
			trie_type &trie,
			string_map_type &strings_by_state,
			state_map_type &states_by_string,
			char_map_type const &char2comp
		):
			m_strings(&strings),
			m_trie(&trie),
			m_strings_by_state(&strings_by_state),
			m_states_by_string(&states_by_string),
			m_char2comp(&char2comp)
		{
		}
		
		void insert_and_check(std::string &str)
		{
			// Compress the string.
			for (auto &c : str)
				c = (*m_char2comp)[c];
				
			if (insert(*m_trie, *m_strings_by_state, str, m_idx))
			{
				m_strings->emplace_back(std::move(str));
				++m_idx;
			}
		}
		
		// For fasta_reader.
		void handle_sequence(
			std::string const &identifier,
			std::unique_ptr <vector_source::vector_type> &seq,
			std::size_t const &seq_length,
			vector_source &vector_source
		)
		{
			std::string str(reinterpret_cast <char const *>(seq->data()), seq_length);
			insert_and_check(str);
			vector_source.put_vector(seq);

			++m_seqno;
			if (0 == m_seqno % 10000)
				std::cerr << " " << m_seqno << std::flush;
		}
		
		// For line_reader.
		void handle_sequence(
			uint32_t lineno,
			std::unique_ptr <vector_source::vector_type> &seq,
			std::size_t const &seq_length,
			vector_source &vector_source
		)
		{
			std::string str(reinterpret_cast <char const *>(seq->data()), seq_length);
			insert_and_check(str);
			vector_source.put_vector(seq);

			if (0 == lineno % 10000)
				std::cerr << " " << lineno << std::flush;
		}
		
		void start()
		{
			m_timer.start();
			std::cerr << "  Inserting the sequences…" << std::flush;
			m_trie->reset_root();
		}
		
		void finish()
		{
			m_states_by_string->resize(m_strings_by_state->size());
			for (auto const &kv : *m_strings_by_state)
			{
				assert(nullptr == (*m_states_by_string)[kv.second]);
				(*m_states_by_string)[kv.second] = kv.first;
			}
			
			m_timer.stop();
			std::cerr << " finished in " << m_timer.ms_elapsed() << " ms." << std::endl;
		}
	};

}}


namespace tribble {
	
	struct reader_template
	{
		template <typename t_cb>
		using fasta_reader = tribble::fasta_reader <t_cb>;

		template <typename t_cb>
		using line_reader = tribble::line_reader <t_cb>;
	};
	
	
	template <template <typename> class t_reader, typename t_cb>
	void read_from_stream(tribble::file_istream &source_stream, vector_source &vs, t_cb &cb)
	{
		source_stream.seekg(0);
		t_reader <t_cb> reader;
		reader.read_from_stream(source_stream, vs, cb);
	}
	
	
	void read_input(
		tribble::file_istream &source_stream,
		enum_source_format const source_format,
		tribble::string_vector_type &strings,
		char_map_type &comp2char,
		char_map_type &char2comp,
		tribble::trie_type &trie,
		tribble::string_map_type &strings_by_state,
		tribble::state_map_type &states_by_string
	)
	{
		{
			char_map_type comp2char_temp(256, 255);
			char_map_type char2comp_temp(256, 255);
			
			using std::swap;
			swap(comp2char, comp2char_temp);
			swap(char2comp, char2comp_temp);
		}
		
		// Read the sequence from input and create the index in the callback.
		vector_source vs(1, false);
		detail::map_alphabet_cb map_cb(char2comp, comp2char);
		detail::create_index_cb index_cb(strings, trie, strings_by_state, states_by_string, char2comp);

		std::cerr << std::endl;
		if (source_format_arg_FASTA == source_format)
		{
			read_from_stream <reader_template::fasta_reader>(source_stream, vs, map_cb);
			read_from_stream <reader_template::fasta_reader>(source_stream, vs, index_cb);
		}
		else if (source_format_arg_text == source_format)
		{
			read_from_stream <reader_template::line_reader>(source_stream, vs, map_cb);
			read_from_stream <reader_template::line_reader>(source_stream, vs, index_cb);
		}
		else
		{
			std::stringstream output;
			output << "Unexpected source file format '" << source_format << "'.";

			throw std::runtime_error(output.str());
		}
	}
}
