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


#include <cstdlib>
#include <sstream>
#include "OPS_Core/xmlsp.h"


//===================================================================

namespace XMLSP {
	
	//===================================================================
	// status codes
	enum {
		M_TOP = 0,
		M_TEXT,
		M_ENTITY,
		M_OPEN_TAG,
		M_CLOSE_TAG,
		M_START_TAG,
		M_ATTR_LVALUE,
		M_ATTR_EQUAL,
		M_ATTR_RVALUE,
		M_IN_TAG,
		M_SINGLE_TAG,
		M_COMMENT,
		M_COMMENT_DOCTYPE,
		M_COMMENT_START,
		M_DOCTYPE,
		M_PROCESSING,
		M_CDATA
	};
	
	//===================================================================
	
	bool Parser::begin()
	{
		if (!on_document_begin()) return false;
	
		attributes_temp.clear();
		while(tag_stack.size()) tag_stack.pop();
		last_char = 0;
		before_last_char = 0;
		text_buffer1 = "";
		text_buffer2 = "";
		current_tag = "";
		entity_buffer = "";
		status_parse = M_TOP;
		current_line = 1;
		current_col = 0;
		status_new_line = false;
		status_had_root = false;
	
		return true;
	}
	
	
	//===================================================================
	
	
	Parser::~Parser()
	{
	
	}
	
	
	//===================================================================
	
	#define STATE_PUSH	status_stack = status_parse;
	#define STATE_POP	status_parse = status_stack;
	
	#define ENDS(c1)	(last_char == c1)
	#define ENDS2(c1, c2)	(before_last_char == c1 && last_char == c2)
	
	#define IS_SPACE(c)	(c == ' ' || c == '\n' || c == '\t')
	#define IS_LABEL(c)	(isalnum(c) || c == '-' || c == '_' || c == '.' || c == ':')
	bool Parser::parse_chunk(const std::string& xml)
	{
		int c, i, l;
		l = xml.size();
		for(i = 0; i < l; i++) {
			c = xml.at(i);
	
			// Normalize all possible newlines to "\n"
			if(c == '\n' && status_new_line) {
				status_new_line = false;
				continue;
			} else if(status_new_line) {
				status_new_line = false;
			} else if(c == '\n') {
				current_line++;
				current_col = 0;
	 		} else if(c == '\r') {
				status_new_line = true;
				c = '\n';
				current_line++;
				current_col = 0;
			} else {
				current_col++;
			}
	
	
			if (status_parse == M_TOP) {
				if (c == '<') {
					status_parse = M_START_TAG;
				} else if (!IS_SPACE(c)) {
					report(E_UNEXPECTED_CDATA, "Unexpected character outside tag");	
					return false;
				}
			} else if (status_parse == M_START_TAG) {
				if (c == '?' && tag_stack.size() == 0) {
					// allowed only at top level
					status_parse = M_PROCESSING;
				} else if (c == '!') {
					// comment or doctype
					status_parse = M_COMMENT_DOCTYPE;
				} else if (c == '/') {
					status_parse = M_CLOSE_TAG;
				} else if (IS_LABEL(c)) {
					if (tag_stack.size() == 0 && status_had_root) {
						// only one root element is allowed
						report(E_MULTIPL_ROOT, "A second root tag found");
						return false;
					}
					status_parse = M_OPEN_TAG;
					text_buffer1 = "";
					text_buffer1.append(1, c);
				} else {
					report(E_UNEXPECTED_CHAR, "Unexpected character at the begining of a tag");
					return false;
				}
			} else if (status_parse == M_OPEN_TAG) {
				// in a open tag 
				if (IS_LABEL(c)) {
					text_buffer1.append(1, c);
				} else if (IS_SPACE(c)) {
					// end of tag name
					current_tag = text_buffer1;
					text_buffer1 = "";
					status_parse = M_IN_TAG;
				} else if (c == '/') {
					current_tag = text_buffer1;
					status_parse = M_SINGLE_TAG;
				} else if (c == '>') {
					// to handle tags without attributes
					if (attributes_temp.size() == 0)
						current_tag = text_buffer1;
					tag_stack.push(current_tag);
					on_tag_open(current_tag, attributes_temp);
					text_buffer1 = "";
					status_parse = M_TEXT;
					attributes_temp.clear();
				} else if (c == '/') {
					status_parse = M_SINGLE_TAG;
				} else {
					report(E_UNEXPECTED_CHAR, "Unexpected character in tag");
					return false;
				}
			} else if (status_parse == M_SINGLE_TAG) {
				// expecting >
				if (c == '>') {
					tag_stack.push(current_tag);
					on_tag_open(current_tag, attributes_temp);
					on_tag_close(current_tag);
					tag_stack.pop();
					attributes_temp.clear();
					text_buffer1 = "";
					if (tag_stack.size()) {
						status_parse = M_TEXT;
						current_tag = tag_stack.top();
					} else {
						// we just closed the root tag
						status_parse = M_TOP;
						status_had_root = true;
					}
				} else {
					// had "<tag_name [attr="foo" ...] /" but ">" not found
					report(E_UNEXPECTED_CHAR, "Unexpected character in tag level after slash");
					return false;
				}
			} else if (status_parse == M_PROCESSING) {
				if (c == '>' && ENDS('?')) {
					text_buffer1 = text_buffer1.substr(0, text_buffer1.size() - 1);
					on_processing(text_buffer1);
					status_parse = M_TOP;
					text_buffer1 = "";
				} else {
					text_buffer1.append(1, c);
				}
			} else if (status_parse == M_COMMENT_DOCTYPE) {
				if (c == '-') {
					status_parse = M_COMMENT_START;
				} else if (c == '[') {
					// this isn't realy a doctype but a CDATA block
					// TODO: Fix this!!!!
					status_parse = M_DOCTYPE;
					text_buffer1 = "[";
				} else if (tag_stack.size() == 0 && status_had_root == false) {
					status_parse = M_DOCTYPE;
					text_buffer1 = "";
					text_buffer1.append(1, c);
				} else {
					report(E_UNEXPECTED_CHAR, "Unexpected character, expected comment or DOCTYPE");
					return false;
				}
			} else if (status_parse == M_DOCTYPE) {
				if (c == '>') {
					if (text_buffer1.size() < 8) {
						report(E_UNKNOWN_ELEMENT, "Unknonw element found");
						return false;
					}
					status_parse = M_TOP;
					on_doctype(text_buffer1);
					text_buffer1 = "";
				} else {
					text_buffer1.append(1, c);
					if (text_buffer1.size() == 7) {
						if (text_buffer1 == "[CDATA[") {
							text_buffer1 = "";
							status_parse = M_CDATA;
						} else if (text_buffer1 != "DOCTYPE") {
							report(E_UNKNOWN_ELEMENT, "Unknonw element found");
							return false;
						}
					}		
				}
			} else if (status_parse == M_COMMENT_START) {
				if (c == '-') status_parse = M_COMMENT;
				else {
					report(E_UNEXPECTED_CHAR, "Unexpected character, expecting '-' for comment");
					return false;
				}
			} else if (status_parse == M_COMMENT) {
				if (c == '>' && ENDS2('-', '-')) {
					text_buffer1 = text_buffer1.substr(0, text_buffer1.size() - 2);
					on_comment(text_buffer1);
					text_buffer1 = "";
					if (tag_stack.size()) status_parse = M_TEXT;
					else status_parse = M_TOP;
				} else text_buffer1.append(1, c);
			} else if (status_parse == M_IN_TAG) {
				if (IS_LABEL(c)) {
					status_parse = M_ATTR_LVALUE;
					text_buffer1 = "";
					text_buffer1.append(1, c);
				} else if (IS_SPACE(c)) {
					// nothing
				} else if (c == '>') {
					tag_stack.push(current_tag);
					on_tag_open(current_tag, attributes_temp);
					text_buffer1 = "";
					status_parse = M_TEXT;
					attributes_temp.clear();
				} else if (c == '/') {
					status_parse = M_SINGLE_TAG;
				} else {
					report(E_UNEXPECTED_CHAR, "Unexpected character in tag, expected attribute or close");
					return false;
				}
			} else if (status_parse == M_ATTR_LVALUE) {
				if (IS_LABEL(c)) {
					text_buffer1.append(1, c);
				} else if (c == '=') {
					text_buffer2 = text_buffer1;
					text_buffer1 = "";
					status_parse = M_ATTR_EQUAL;
				} else {
					report(E_UNEXPECTED_CHAR, "Unexpected character in attribute name");
					return false;
				}	
			} else if (status_parse == M_ATTR_EQUAL) {
				if (c == '"') status_parse = M_ATTR_RVALUE;
				else {
					report(E_UNEXPECTED_CHAR, "Unexpected character, expected double quote");
					return false;
				}
			} else if (status_parse == M_ATTR_RVALUE) {
				if (c == '"') {
					// closing value;
					attributes_temp[text_buffer2] = text_buffer1;
					text_buffer1 = "";
					status_parse = M_IN_TAG;
				} else if (c == '&') {
					// entity
					entity_buffer = "";
					STATE_PUSH;
					status_parse = M_ENTITY;
				} else {
					text_buffer1.append(1, c);
				}
			} else if (status_parse == M_ENTITY) {
				// handle entities
				if (c == ';') {
					if (entity_buffer.size() == 0) {
						report(E_INVALID_ENTITY, "Empty entity identifier found");
						return false;
					} else if (entity_buffer.at(0) == '#') {
						// hex value
						entity_buffer = entity_buffer.substr(1);
						if (entity_buffer.empty()) {
							report(E_INVALID_ENTITY, "Empty decimal entity identifier found");
							return false;
						} else if (entity_buffer.at(0) == 'x') {
							// hex entity
							if (entity_buffer.size() != 2) {
								report(E_INVALID_ENTITY, "Hex entity should have 2 character value");
								return false;
							} else text_buffer1.append(1, (char)strtol(entity_buffer.c_str(), NULL, 16));
						} else text_buffer1.append(1, atoi(entity_buffer.c_str()));
					} else {
						std::string tmp2 = entities[entity_buffer];
						if (tmp2.empty()) {
							report(E_INVALID_ENTITY, "Unknown entity");
							return false;
						} else text_buffer1.append(tmp2);
					}
					entity_buffer = "";
					STATE_POP;
				} else entity_buffer.append(1, c);	
			} else if (status_parse == M_CDATA) {
				if (c == '>' && ENDS2(']', ']')) {
					text_buffer1 = text_buffer1.substr(0, text_buffer1.size() - 2);
					// don't check whitespaces because this CDATA must preserve
					// everything (including whitespaces).
					on_cdata(text_buffer1);
					status_parse = M_TEXT;
					text_buffer1 = "";
				} else text_buffer1.append(1, c);
			} else if (status_parse == M_CLOSE_TAG) {
				// </foo> - must match with tag from top stack (or current_tag)
				if (IS_LABEL(c)) {
					text_buffer1.append(1, c);
				} else if (c == '>') {
					if (text_buffer1 != current_tag) {
						std::stringstream s;
						s<<"Closing tag '"<<text_buffer1<<"' at level of '"<<current_tag<<"'";
						report(E_TAG_MISMATCH, s.str());
						return false;
					} else {
						on_tag_close(current_tag);
						tag_stack.pop();
						
						if (tag_stack.empty()) status_parse = M_TOP;
						else {
							status_parse = M_TEXT;
							current_tag = tag_stack.top();
						}
						text_buffer1 = "";
					}
				} else {
					report(E_UNEXPECTED_CHAR, "Unexpected character in closing tag");
					return false;
	
				}
			} else if (status_parse == M_TEXT) {
				if (c == '&') {
					STATE_PUSH;
					status_parse = M_ENTITY;
				} else if (c == '<') {
					status_parse = M_START_TAG;
					if (text_buffer1.size()) {
						if (check_whitespaces(text_buffer1))
							on_cdata(text_buffer1);
						text_buffer1 = "";
					}
				} else if (c == '>') {
					report(E_UNEXPECTED_CHAR, "Unexpected character '>' in CDATA");
					return false;
				} else text_buffer1.append(1, c);
			} else {
				std::stringstream s;
				s<<"Unknown internal status: "<<status_parse;
				report(E_INTERNAL, s.str());
			}
		
			before_last_char = last_char;
			last_char = c;
		}
	
		return true;
	}
	
	
	//===================================================================
	
	
	bool Parser::end()
	{
		if (status_parse != M_TOP) {
			// findout why it's not finished
			switch (status_parse) {
			case M_CDATA: 
			case M_COMMENT:
			case M_COMMENT_DOCTYPE:
			case M_COMMENT_START:
				report(E_BLOCK_NOT_CLOSED, "Comment, DOCTYPE, CDATA or processing instruction not closed");
				break;
			case M_OPEN_TAG:
			case M_CLOSE_TAG:
			case M_SINGLE_TAG:
			case M_ATTR_LVALUE:
			case M_ATTR_EQUAL:
			case M_ATTR_RVALUE:
			case M_START_TAG:
			case M_IN_TAG:
				report(E_TAG_NOT_FINISHED, "Tag not finished or is invalid");
				break;
			case M_TEXT:
				report(E_TAG_NOT_CLOSED, "Open tag missing close tag");
				break;
			case M_ENTITY:
				report(E_INVALID_ENTITY, "Entity not finished, missing ';'");
				break;
			default:
				report(E_INTERNAL, "XML document not finished");
				break;
			}
	
			return false;
		}
	
		return on_document_end();
	}
	
	
	//===================================================================
	
	
	bool Parser::check_whitespaces(const std::string& cdata)
	{
		int c;
		unsigned int i;
		if (skip_whitespaces == false) return true;
		else {
			for(i = 0; i < cdata.size(); i++) {
				c = cdata[i];
				if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
					// CDATA has printable characters
					return true;
			}
			// CDATA composed of whitespaces
			return false;
		}
	}
	
	
	//===================================================================
	
	
	bool Parser::on_tag_open(const std::string& tag_name, StringMap& attributes)
	{
		return true;
	}
	
	
	//===================================================================
	
	
	bool Parser::on_cdata(const std::string& cdata)
	{
		return true;
	}
	
	
	//===================================================================
	
	
	bool Parser::on_tag_close(const std::string& tag_name)
	{
		return true;
	}
	
	
	//===================================================================
	
	
	bool Parser::on_comment(const std::string& comment)
	{
		return true;
	}
	
	
	//===================================================================
	
	
	bool Parser::on_processing(const std::string& value)
	{
		return true;
	}
	
	
	//===================================================================
	
	
	bool Parser::on_doctype(const std::string& value)
	{
		return true;
	}
	
	
	//===================================================================
	
	
	bool Parser::on_document_begin()
	{
		return true;
	}
	
	
	//===================================================================
	
	
	bool Parser::on_document_end()
	{
		return true;
	}
	
	
	//===================================================================
	
	
	void Parser::on_error(int errnr, int line, int col, const std::string& message)
	{
		return;
	}
	
	
	//===================================================================

}; // namespace XMLSP;

