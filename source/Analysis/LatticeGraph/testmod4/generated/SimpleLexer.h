/** \file
 *  This C header file was generated by $ANTLR version 3.2 Sep 23, 2009 12:02:23
 *
 *     -  From the grammar source file : C:\\Documents and Settings\\Sergey\\Рабочий стол\\SimpleLexer.g
 *     -                            On : 2010-10-11 13:13:32
 *     -                 for the lexer : SimpleLexerLexer *
 * Editing it, at least manually, is not wise. 
 *
 * C language generator and runtime by Jim Idle, jimi|hereisanat|idle|dotgoeshere|ws.
 *
 *
 * The lexer SimpleLexer has the callable functions (rules) shown below,
 * which will invoke the code for the associated rule in the source grammar
 * assuming that the input stream is pointing to a token/text stream that could begin
 * this rule.
 * 
 * For instance if you call the first (topmost) rule in a parser grammar, you will
 * get the results of a full parse, but calling a rule half way through the grammar will
 * allow you to pass part of a full token stream to the parser, such as for syntax checking
 * in editors and so on.
 *
 * The parser entry points are called indirectly (by function pointer to function) via
 * a parser context typedef pSimpleLexer, which is returned from a call to SimpleLexerNew().
 *
 * As this is a generated lexer, it is unlikely you will call it 'manually'. However
 * the methods are provided anyway.
 * * The methods in pSimpleLexer are  as follows:
 *
 *  -  void      pSimpleLexer->T_NEWLINE(pSimpleLexer)
 *  -  void      pSimpleLexer->WS(pSimpleLexer)
 *  -  void      pSimpleLexer->T_DIGIT_STRING(pSimpleLexer)
 *  -  void      pSimpleLexer->T_VEC_beg(pSimpleLexer)
 *  -  void      pSimpleLexer->T_VEC_end(pSimpleLexer)
 *  -  void      pSimpleLexer->T_OPEN_BR(pSimpleLexer)
 *  -  void      pSimpleLexer->T_CLOSE_BR(pSimpleLexer)
 *  -  void      pSimpleLexer->T_NEWPARAM(pSimpleLexer)
 *  -  void      pSimpleLexer->T_DIV(pSimpleLexer)
 *  -  void      pSimpleLexer->T_NIL(pSimpleLexer)
 *  -  void      pSimpleLexer->T_IF(pSimpleLexer)
 *  -  void      pSimpleLexer->T_NOT(pSimpleLexer)
 *  -  void      pSimpleLexer->T_AND(pSimpleLexer)
 *  -  void      pSimpleLexer->T_OR(pSimpleLexer)
 *  -  void      pSimpleLexer->T_LIST(pSimpleLexer)
 *  -  void      pSimpleLexer->T_EOF(pSimpleLexer)
 *  -  void      pSimpleLexer->Digit(pSimpleLexer)
 *  -  void      pSimpleLexer->Tokens(pSimpleLexer)
 *
 * The return type for any particular rule is of course determined by the source
 * grammar file.
 */
// [The "BSD licence"]
// Copyright (c) 2005-2009 Jim Idle, Temporal Wave LLC
// http://www.temporal-wave.com
// http://www.linkedin.com/in/jimidle
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef	_SimpleLexer_H
#define _SimpleLexer_H
/* =============================================================================
 * Standard antlr3 C runtime definitions
 */
#include    <antlr3.h>

/* End of standard antlr 3 runtime definitions
 * =============================================================================
 */
 
#ifdef __cplusplus
extern "C" {
#endif

// Forward declare the context typedef so that we can use it before it is
// properly defined. Delegators and delegates (from import statements) are
// interdependent and their context structures contain pointers to each other
// C only allows such things to be declared if you pre-declare the typedef.
//
typedef struct SimpleLexer_Ctx_struct SimpleLexer, * pSimpleLexer;



#ifdef	ANTLR3_WINDOWS
// Disable: Unreferenced parameter,							- Rules with parameters that are not used
//          constant conditional,							- ANTLR realizes that a prediction is always true (synpred usually)
//          initialized but unused variable					- tree rewrite variables declared but not needed
//          Unreferenced local variable						- lexer rule declares but does not always use _type
//          potentially unitialized variable used			- retval always returned from a rule 
//			unreferenced local function has been removed	- susually getTokenNames or freeScope, they can go without warnigns
//
// These are only really displayed at warning level /W4 but that is the code ideal I am aiming at
// and the codegen must generate some of these warnings by necessity, apart from 4100, which is
// usually generated when a parser rule is given a parameter that it does not use. Mostly though
// this is a matter of orthogonality hence I disable that one.
//
#pragma warning( disable : 4100 )
#pragma warning( disable : 4101 )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4189 )
#pragma warning( disable : 4505 )
#pragma warning( disable : 4701 )
#endif

/** Context tracking structure for SimpleLexer
 */
struct SimpleLexer_Ctx_struct
{
    /** Built in ANTLR3 context tracker contains all the generic elements
     *  required for context tracking.
     */
    pANTLR3_LEXER    pLexer;


     void (*mT_NEWLINE)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mWS)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_DIGIT_STRING)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_VEC_beg)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_VEC_end)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_OPEN_BR)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_CLOSE_BR)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_NEWPARAM)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_DIV)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_NIL)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_IF)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_NOT)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_AND)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_OR)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_LIST)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mT_EOF)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mDigit)	(struct SimpleLexer_Ctx_struct * ctx);
     void (*mTokens)	(struct SimpleLexer_Ctx_struct * ctx);    const char * (*getGrammarFileName)();
    void	    (*free)   (struct SimpleLexer_Ctx_struct * ctx);
        
};

// Function protoypes for the constructor functions that external translation units
// such as delegators and delegates may wish to call.
//
ANTLR3_API pSimpleLexer SimpleLexerNew         (pANTLR3_INPUT_STREAM instream);
ANTLR3_API pSimpleLexer SimpleLexerNewSSD      (pANTLR3_INPUT_STREAM instream, pANTLR3_RECOGNIZER_SHARED_STATE state);

/** Symbolic definitions of all the tokens that the lexer will work with.
 * \{
 *
 * Antlr will define EOF, but we can't use that as it it is too common in
 * in C header files and that would be confusing. There is no way to filter this out at the moment
 * so we just undef it here for now. That isn't the value we get back from C recognizers
 * anyway. We are looking for ANTLR3_TOKEN_EOF.
 */
#ifdef	EOF
#undef	EOF
#endif
#ifdef	Tokens
#undef	Tokens
#endif 
#define T_NIL      14
#define T_IF      15
#define T_VEC_end      9
#define T_NEWPARAM      12
#define T_EOF      20
#define T_OR      18
#define Digit      6
#define EOF      -1
#define T_AND      17
#define T_CLOSE_BR      11
#define T_OPEN_BR      10
#define T_DIV      13
#define T_LIST      19
#define WS      5
#define T_DIGIT_STRING      7
#define T_VEC_beg      8
#define T_NEWLINE      4
#define T_NOT      16
#ifdef	EOF
#undef	EOF
#define	EOF	ANTLR3_TOKEN_EOF
#endif

#ifndef TOKENSOURCE
#define TOKENSOURCE(lxr) lxr->pLexer->rec->state->tokSource
#endif

/* End of token definitions for SimpleLexer
 * =============================================================================
 */
/** \} */

#ifdef __cplusplus
}
#endif

#endif

/* END - Note:Keep extra line feed to satisfy UNIX systems */
