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

#include <dispatch/dispatch.h>
#include <thread>
#include <tribble/dispatch_fn.hh>
#include <tribble/fasta_reader.hh>
#include <tribble/io.hh>
#include <tribble/line_reader.hh>
#include "verify_superstring.hh"


namespace {

	class verify_context
	{
	protected:
		tribble::cst_type											m_cst{};
		dispatch_queue_t											m_loading_queue{};
		dispatch_queue_t											m_verifying_queue{};
		tribble::vector_source										m_vs;
		std::mutex													m_cerr_mutex{};
		bool														m_did_succeed{true};

	protected:
		void cleanup() { delete this; }
		
		void handle_sequence_(
			std::unique_ptr <tribble::vector_source::vector_type> &seq,
			std::size_t const seq_length,
			tribble::vector_source &vs,
			std::function <void (std::ostream &stream)> err_msg_cb
		)
		{
			assert(&vs == &m_vs);
			
			// Take the pointer from the given unique_ptr and pass it to the callback.
			// When called, the callback packs it into an unique_ptr again and evetually
			// returns the buffer to vector_source.
			auto *seq_ptr(seq.release());
			
			auto verify_fn = [this, seq_ptr, seq_length, err_msg_cb](){
				std::unique_ptr <tribble::vector_source::vector_type> seq(seq_ptr);
				
				tribble::cst_type::node_type const root(m_cst.root());
				tribble::cst_type::node_type node(root);
				for (std::size_t i(0); i < seq_length; ++i)
				{
					auto const idx(seq_length - i - 1);
					auto const k((*seq)[idx]);
					node = m_cst.wl(node, k);
					if (root == node)
					{
						std::stringstream output;
						output << "Did not find path for string ";
						err_msg_cb(output);
						output << ".";
						
						std::lock_guard <std::mutex> guard(m_cerr_mutex);
						std::cerr << output.str() << std::endl;
						m_did_succeed = false;
						break;
					}
				}
					
				m_vs.put_vector(seq);
			};
			
			//std::cerr << "Dispatching verify block" << std::endl;
			tribble::dispatch_async_fn(m_verifying_queue, verify_fn);
		}

	public:
		verify_context(
			dispatch_queue_t loading_queue,
			dispatch_queue_t verifying_queue,
			bool const multi_threaded
		):
			m_loading_queue(loading_queue),
			m_verifying_queue(verifying_queue),
			m_vs(multi_threaded ? std::thread::hardware_concurrency() : 1, true)
		{
			dispatch_retain(m_loading_queue);
			dispatch_retain(m_verifying_queue);
		}
		
		
		~verify_context()
		{
			dispatch_release(m_loading_queue);
			dispatch_release(m_verifying_queue);
		}
		
		
		void load_and_verify(char const *cst_fname_, char const *source_fname_, enum_source_format const source_format)
		{
			assert(cst_fname_);
			assert(source_fname_);
			std::string cst_fname(cst_fname_);
			std::string source_fname(source_fname_);

			auto load_ds_fn = [this, cst_fname = std::move(cst_fname)](){
				tribble::file_istream ds_stream;
				tribble::open_file_for_reading(cst_fname.c_str(), ds_stream);
				
				// Load the CST.
				std::cerr << "Loading the CST…" << std::endl;
				m_cst.load(ds_stream);
				std::cerr << "Loading complete." << std::endl;
			};
			
			auto read_sequences_fn = [this, source_fname = std::move(source_fname), source_format](){
				tribble::file_istream source_stream;
				tribble::open_file_for_reading(source_fname.c_str(), source_stream);
				
				if (source_format_arg_FASTA == source_format)
				{
					tribble::fasta_reader <verify_context> reader;
					reader.read_from_stream(source_stream, m_vs, *this);
				}
				else if (source_format_arg_text == source_format)
				{
					tribble::line_reader <verify_context> reader;
					reader.read_from_stream(source_stream, m_vs, *this);
				}
				else
				{
					std::stringstream output;
					output << "Unexpected source file format '" << source_format << "'.";
					
					throw std::runtime_error(output.str());
				}
			};
			
			// Prevent aligning blocks from being executed before CST has been read.
			tribble::dispatch_barrier_async_fn(m_verifying_queue, std::move(load_ds_fn));

			// Load the data in the source file and process in callback.
			tribble::dispatch_async_fn(m_loading_queue, std::move(read_sequences_fn));
		}
		
		
		// Reader callbacks.
		void handle_sequence(
			std::string const &identifier,
			std::unique_ptr <tribble::vector_source::vector_type> &seq,
			std::size_t const seq_length,
			tribble::vector_source &vs
		)
		{
			std::function <void (std::ostream &stream)> cb([&](std::ostream &stream) {
				stream << "with identifier '" << identifier << "'";
			});
			handle_sequence_(seq, seq_length, vs, cb);
		}
		
		
		void handle_sequence(
			uint32_t const line_no,
			std::unique_ptr <tribble::vector_source::vector_type> &seq,
			std::size_t const seq_length,
			tribble::vector_source &vs
		)
		{
			std::function <void (std::ostream &stream)> cb([=](std::ostream &stream) {
				stream << "on line " << line_no;
			});
			handle_sequence_(seq, seq_length, vs, cb);
		}

		
		void finish()
		{
			//std::cerr << "Finish called" << std::endl;
			
			// All sequence handling blocks have been executed since m_loading_queue
			// is (supposed to be) serial. Make sure that all verifying blocks have
			// been executed before calling exit by using a barrier.
			auto exit_fn = [this](){
				
				bool const success(m_did_succeed);
				if (success)
					std::cerr << "All sequences were located." << std::endl;
				else
					std::cerr << "WARNING: not all sequences were located." << std::endl;
				
				// After calling cleanup *this is no longer valid.
				//std::cerr << "Calling cleanup" << std::endl;
				cleanup();
				
				//std::cerr << "Calling exit" << std::endl;
				exit(success ? EXIT_SUCCESS : EXIT_FAILURE);
			};
			
			//std::cerr << "Dispatching finish block" << std::endl;
			tribble::dispatch_barrier_async_fn(m_verifying_queue, std::move(exit_fn));
		}
	};
}


namespace tribble {
	
	void verify_superstring(
		char const *cst_fname,
		char const *source_fname,
		enum_source_format source_format,
		bool const multi_threaded
	)
	{
		dispatch_queue_t loading_queue(dispatch_get_main_queue());
		dispatch_queue_t verifying_queue(loading_queue);
		if (multi_threaded)
			verifying_queue = dispatch_queue_create("fi.iki.tsnorri.tribble_verifying_queue", DISPATCH_QUEUE_CONCURRENT);
		
		// verify_context needs to be allocated on the heap because later dispatch_main is called.
		// The class deallocates itself by calling cleanup() (in finish()).
		verify_context *ctx(new verify_context(
			loading_queue,
			verifying_queue,
			multi_threaded
		));
		
		if (multi_threaded)
			dispatch_release(verifying_queue);
		
		ctx->load_and_verify(cst_fname, source_fname, source_format);
		
		// Calls pthread_exit.
		dispatch_main();
	}
}

