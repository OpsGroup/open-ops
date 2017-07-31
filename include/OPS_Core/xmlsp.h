// Copyright (c) 2007, Przemyslaw Grzywacz
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Przemyslaw Grzywacz nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY Przemyslaw Grzywacz ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL Przemyslaw Grzywacz BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef __xmlsp_H
#define __xmlsp_H

#include <map>
#include <string>
#include <stack>

namespace XMLSP {

	typedef std::map<std::string, std::string> StringMap;
	typedef std::stack<std::string> TagStack;
	
	class Parser
	{
	public:
		// error codes
		enum {
			E_OK		= 0,
			E_TAG_NOT_CLOSED,
			E_MANY_ROOT_TAGS,
			E_UNEXPECTED_CDATA,
			E_UNEXPECTED_CHAR,
			E_INVALID_ENTITY,
			E_UNKNOWN_ELEMENT,
			E_BLOCK_NOT_CLOSED,
			E_TAG_NOT_FINISHED,
			E_MULTIPL_ROOT,
			E_TAG_MISMATCH,
			E_INTERNAL
		};
	
		Parser() {
			// default XML 1.0 entities
			add_entity("lt", "<");
			add_entity("gt", ">");
			add_entity("amp", "&");
			add_entity("apos", "'");	
			add_entity("quot", "\"");
			skip_whitespaces = false;
		}
	
		virtual ~Parser();
	
		// Constructor and parse
		Parser(const std::string& xml) {
			add_entity("lt", "<");
			add_entity("gt", ">");
			add_entity("amp", "&");
			add_entity("apos", "'");	
			add_entity("quot", "\"");
			parse(xml);
		}
	
		// whole document parsing at once
		bool parse(const std::string& xml) {
			if (begin() == false) return false;
			if (parse_chunk(xml) == false) return false;
			return end();
		}
	
		// failure ?
		bool get_status() { return status_failure; }
	
		// stream parsing
		bool begin();
		bool parse_chunk(const std::string& xml);
		bool end();
	
		// adds a new entity
		void add_entity(const std::string& id, const std::string& value) { entities[id] = value; }
		void add_entity(const char* id, const char* value) { entities[id] = value; }
	
	
		// skip CDATA composed only of whitespaces
		void set_skip_whitespaces(bool v) { skip_whitespaces = v; }
		bool get_skip_whitespaces() { return skip_whitespaces; }
	
	
		// parse events
		virtual bool on_tag_open(const std::string& tag_name, StringMap& attributes);
		virtual bool on_cdata(const std::string& cdata);
		virtual bool on_tag_close(const std::string& tag_name);
		virtual bool on_comment(const std::string& comment);
		virtual bool on_processing(const std::string& value);
		virtual bool on_doctype(const std::string& value);
		virtual bool on_document_begin();
		virtual bool on_document_end();
		virtual void on_error(int errnr, int line, int col, const std::string& message);
	
		int get_line() const { return current_line; }
		int get_col() const { return current_col; }
	
	
	protected:
		// returns current tag for 
		std::string get_tag() { return (tag_stack.size() ? tag_stack.top() : ""); }
	
		void report(int errnr, const std::string& message) {
			on_error(errnr, current_line, current_col, message);
			status_failure = true;
		}
		
		void report(int errnr, const char* str) {
			on_error(errnr, current_line, current_col, std::string(str));
			status_failure = true;
		}
	
		// FIXME: is this function needed ??
		void report(int errnr, char* message) {
			std::string s = message;
			on_error(errnr, current_line, current_col, s);
			status_failure = true;
		}
	
		TagStack* get_tag_stack() { return &tag_stack; }
	
	private:
		bool check_whitespaces(const std::string& cdata);
	
		// entity list
		StringMap	entities;
	
		// state info
		// location of current byte
		int current_line, current_col;
		// status of converion of new lines to "\n" character
		int status_new_line;
		// status of failure
		bool status_failure;
		// did we have root tag ?
		bool status_had_root;
		// tag stack
		TagStack tag_stack;
		// state stack
		std::stack<int> state_stack;
		// temporary argument dictionary
		StringMap attributes_temp;
		// current parse status
		int status_parse, status_stack;
		// last two characters
		int last_char, before_last_char;
		// temp buffers
		std::string text_buffer1, text_buffer2, current_tag;
		std::string entity_buffer;
		// skip CDATA composed only of whitespaces
		bool skip_whitespaces;
	};
	
};

#endif // !__xmlsp_H
