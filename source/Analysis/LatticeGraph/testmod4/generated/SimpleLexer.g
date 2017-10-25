lexer  grammar SimpleLexer;

options
{
  language = C;
}

T_NEWLINE
  :  '\r'? '\n' {$channel = HIDDEN;}
  ;

WS  :  (' ' | '\t') {$channel = HIDDEN;}
    ;

T_DIGIT_STRING
  : Digit+
  ; 

T_VEC_beg
  :'*['
  ;
T_VEC_end
  :']'
  ;
  
T_OPEN_BR
  :'('
  ;
T_CLOSE_BR
  :')'
  ;
T_NEWPARAM
  	:'newparm'
  	;
  	
T_DIV
 	:'div'
 	;
T_NIL
  	:'nil'
  	;
T_IF
        :'if'
	;
T_NOT
   	:'not'
   	;
T_AND
  	:'and'
  	;
T_OR
  	:'or'
  	;	
 T_LIST
  	:'list'
  	;	
  	 	
	


T_EOF : '__T_EOF__';      

/* fragments */

fragment
Digit : '0'..'9'  ;

