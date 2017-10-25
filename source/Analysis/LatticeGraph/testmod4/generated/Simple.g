grammar Simple;

options
{
  ASTLabelType =pANTLR3_BASE_TREE;
  language = C;
  tokenVocab = SimpleLexer;
  
  
}

@parser::includes
{

  #include <string>
  #include <stdlib.h>
  #include <list>

#include <iostream>
#include "Analysis/LatticeGraph/ComplexCondition.h"
#include "Analysis/LatticeGraph/LatticeGraph.h"
#include "Analysis/LatticeGraph/LinearLib.h"
#include "Analysis/LatticeGraph/ExtendedQuast.h"
#include "Analysis/LatticeGraph/PIP.h"
#include "Analysis/LatticeGraph/Point.h"
using namespace LatticeGraph;
//using namespace std;
}

@parser::members
{
#define getTokenInteger(token)  (atoi((char*)token->getText(token)->chars))
//typedef list<ComplexCondition*> nodelist;
}

complex_cond
returns[ComplexCondition* c] 	
scope
{ComplexCondition* cc;
int flag;}
@init{ $complex_cond::flag=0;$complex_cond::cc=0;
}
 	:	expr[0]{$c=$complex_cond::cc;}
 	;	
 	
solution returns [ExtendedQuast* res]
scope
{
ExtendedQuast* sol;
}

   	:	quast_group[0]{$res=$solution::sol;}
   	/*| T_NIL {$res=0}*/
   	;


all_vectors
returns [std::list <std::list <int>* >* res]
@init {res=new std::list <std::list <int>* >;}
  :  (vector {res->push_back($vector.res);})+
  ;

digits_line
returns [std::list <int>* res]
@init {res=new std::list <int>;}
@after {}
  :(one_digit_string {int *rab=$one_digit_string.res; res->push_back(*rab); delete rab;})+
  ; 
vector
returns[std::list<int> * res]
  :T_VEC_beg digits_line T_VEC_end{res=$digits_line.res;}
  ;  
  
one_digit_string
returns [int* res]
@init {res=new int;}
  :  T_DIGIT_STRING {*res = getTokenInteger($T_DIGIT_STRING);}
  ;

end_of_lines
  	:	T_EOF

  	;
//quast

// $<extended quast group

quast_group[ExtendedQuast* cur]
@init   	
{ExtendedQuast* ncur;}      
        :
   	{
   	std::cout<<"quast group"<<std::endl;
   	if ($cur==0)
   	{
   	 //ncur= new ExtendedQuast(in_dim,out_dim);
   	 ncur= new ExtendedQuast(0,0);
   	 }
   	 else
   	 ncur=$cur;
   	 }
   	
   	quast[ncur]
   	|
   	ng=newparm_group
   	{
   	if ($cur==0)
   	{
   	 //ncur= new ExtendedQuast(in_dim,out_dim);
   	 ncur= new ExtendedQuast(0,0);
   	 $solution::sol=ncur;
   	 }
   	 else
   	 ncur=$cur;
        ncur->setNewParamVector($ng.npv);	 
   	 
   	 }
   	
   	 quast[ncur]
   	;


quast[ExtendedQuast* cur]
@init
{
ExtendedQuast* truebr;
ExtendedQuast* falsebr;
ExtendedQuast* ncur;
}  	
  	:
  	{ncur=$cur;}
  	form[ncur]
  	
  	|
  	T_OPEN_BR T_IF cc=complex_cond
  	{
  	ncur=$cur;
  	ncur->setCondition($cc.c);
  	ncur->setTypeOfNod(ExtendedQuast::TypeOfNod::INNER);
  	truebr= new ExtendedQuast(0,0);
  	falsebr= new ExtendedQuast(0,0);
  	ncur->setTrueBranch(truebr);
  	ncur->setFalseBranch(falsebr);
  	
  	}
  	 quast_group[truebr] quast_group[falsebr] T_CLOSE_BR
  	;


form[ExtendedQuast* cur]
@init
{
Point* reval;
SimpleLinearExpression* sle;
int k=0;
} 	
 	:
 	
 	T_OPEN_BR T_LIST
 	{
 	reval= new Point(10);
 	
 	}
 	
 	 (v=vector
 	 {
 	int vsize=$v.res->size();
 	int * a=new int[vsize];
 	std::list<int>::iterator it=$v.res->begin();
 	int i=0;
 	do
 	{
 	
 	
 	a[i]=*it;
 	it++;
 	i++;
 	}
 	while(i<vsize);
 	
 	 sle =new SimpleLinearExpression(a,vsize);
 	 (*reval)[k]=sle;
 	 k++;
 	 }
 	 
 	 )+ T_CLOSE_BR
 	 
 	 {$cur->setLinExpr(reval);
 	 $cur->setTypeOfNod(ExtendedQuast::TypeOfNod::LEAF);
 	 }
 	
 	|
 	T_OPEN_BR T_CLOSE_BR
 	;
 	
 	
newparm_group returns [NewParamVector* npv]
@init
{
NewParamVector* npvec;
} 	
 	:
 	{
 	npvec= new NewParamVector();
 	}
 	(
 	n=newparmvector
 	{
 	npvec->PushBack($n.npe);
 	}
 	)+
 	{$npv=npvec;}
 	;


newparmvector returns [NewParamEquation* npe] 
@init
{
NewParamEquation* npeq;
}  	
  	
  	:
  	T_OPEN_BR T_NEWPARAM one_digit_string? T_OPEN_BR T_DIV
  	
  	 v=vector ch=one_digit_string
  	 {
  	int vsize=$v.res->size();
 	int * a=new int[vsize];
 	std::list<int>::iterator it=$v.res->begin();
 	int i=0;
 	do
 	{
 	
 	
 	a[i]=*it;
 	it++;
 	i++;
 	}
 	while(i<vsize);
 	
 	npeq= new NewParamEquation();
 	npeq->m_dim=vsize;
 	npeq->m_denom=*($ch.res);
 	npeq->m_coefs=a;
 	
 	$npe=npeq;
  	  
  	 }
  	 T_CLOSE_BR T_CLOSE_BR
  	;	 	
 
 
 
// $>
// $<complex_condition group

//complex condition

expr[ComplexCondition* cur] returns [ComplexCondition* truecur]
options {backtrack=true;}
scope
{

std::list<ComplexCondition*>* leaflist;
std::list<ComplexCondition*>::iterator* it2;
std::list<ComplexCondition*>::iterator* it1;
int k;
ComplexCondition* left;
ComplexCondition* right;
ComplexCondition* root;
bool flag;
ComplexCondition* ecur;
} 
@init
{
$expr::leaflist=new std::list<ComplexCondition*>(0);
$expr::ecur=cur;
$expr::it1=new std::list<ComplexCondition*>::iterator();
$expr::it2=new std::list<ComplexCondition*>::iterator();
$expr::k=1;
$expr::flag=false;
$expr::right=0;
}
@after
{
if ($expr::leaflist!=0)
  delete $expr::leaflist;
if ($expr::it1!=0)
  delete $expr::it1;
if ($expr::it2!=0)
  delete $expr::it2;
}    
     
 	:
 	
 	
 	
 	{
 	$expr::left= new ComplexCondition();
 	
 	}
 	childlf=term[$expr::left] 
 	{$expr::left=$childlf.truecur;}
 	(
 	{
 	
 	std::cout<<"expr enter"<<std::endl;
 	$expr::leaflist->push_front(new ComplexCondition());
 	*$expr::it1=$expr::leaflist->begin();
 	$expr::right=**($expr::it1); 
 	}
 	
 	 T_OR childrt=term[$expr::right]
 	
 	{
 	$expr::leaflist->push_front($childrt.truecur);
 	*$expr::it1=$expr::leaflist->begin();
 	std::cout<<"expr enter"<<std::endl;
 	if (($expr::ecur==0)&&($expr::flag==false))
 	{
 	   std::cout<<'1'<<std::endl;
 	   $expr::root =new ComplexCondition(ComplexCondition::Operation::OR,$expr::left,**$expr::it1);
 	   /* if ($complex_cond::flag=false)
 	   $complex_cond::cc=$expr::root;
 	   */
 	   $expr::flag=true;
 	}
 	else
 	  if ($expr::flag==true)
 	  {
 	     	std::cout<<'2'<<std::endl;
 	    
 	    $expr::leaflist->push_front(new ComplexCondition(ComplexCondition::Operation::OR,$expr::left,**$expr::it1));
            $expr::root=*($expr::leaflist->begin());
          
           
            
          
 	   
 	 
 	   }
 	   else 
 	   if (($expr::ecur!=0)&&($expr::flag==false))
 	   {
 	   std::cout<<'3'<<std::endl;
 	   $expr::root=$expr::ecur;
 	   $expr::root->setOperation(ComplexCondition::Operation::OR);
 	   $expr::root->setLeft($expr::left);
 	   $expr::root->setRight(**$expr::it1);
 	   $expr::flag=true;
 	   }
 	 if ($complex_cond::flag==0)
 	   $complex_cond::cc=$expr::root;
 	   
 	std::cout<<'4'<<std::endl;
 	$expr::left->setFather($expr::root);
 	(**$expr::it1)->setFather($expr::root);
 	$expr::root->setOperation(ComplexCondition::Operation::OR);
 	$expr::left=$expr::root;
 	/*$expr::right=**($expr::it1);*/ 	
       	$truecur=$expr::root;
       	}
       	
 	)+
 	|
 	child=term[$expr::ecur]{$truecur=$child.truecur;}
 	;

factor[ComplexCondition* cur] returns [ComplexCondition* truecur]
@init
{
ComplexCondition* left=0;
ComplexCondition* root=0;
}
 	: 
 	{
 	left= new ComplexCondition();
 	std::cout<<"not1"<<std::endl;
 	}
 	T_NOT childlt=nest_factor[left]
 	{
 	std::cout<<"not2"<<std::endl;
 	left=$childlt.truecur;
 	if (cur==0) 
 	{
 	   
 	   root =new ComplexCondition(ComplexCondition::Operation::NOT,left);
 	   if ($complex_cond::flag==0)
 	   $complex_cond::cc=root;
 	}
 	else
 	  {root=cur;
 	   root->setOperation(ComplexCondition::Operation::NOT);
 	   root->setLeft(left);
 	   }
 	std::cout<<"factor"<<std::endl;
 	 	left->setFather(root);  
 	
 	$truecur=root;
 	   
 	
 	}
 	
 	
 	| childrt=nest_factor[cur] {$truecur=$childrt.truecur;}
 	;

nest_factor[ComplexCondition* cur] returns [ComplexCondition* truecur]
@init
{
ComplexCondition* root;

}       
        :
      	{$complex_cond::flag++;} T_OPEN_BR child=expr[cur] T_CLOSE_BR {$complex_cond::flag--;$truecur=$child.truecur;}	
      	| 
      	 vector
 	{
 	
 	int * a=new int[$vector.res->size()];
 	std::list<int>::iterator it=$vector.res->begin();
 	for (int i=0;i<$vector.res->size();i++)
 	{
 	
 	std::advance(it,i); 
 	a[i]=*it;
 	std::cout<<*it<<std::endl;
 	it=$vector.res->begin();
 	
 	}
 	LatticeGraph::Inequality* in= new LatticeGraph::Inequality(a,$vector.res->size());
 	
 	if (cur==0) 
 	{
 	   root =new LatticeGraph::ComplexCondition(in);
 	    if ($complex_cond::flag==0)
 	   $complex_cond::cc=root;
 	   
 	}
 	else
 	{  
 	  /*
 	  root=cur;
 	  ComplexCondition* nod=new ComplexCondition(in) ;
 	  ComplexCondition* buf=root->getFather();
 	  std::cout<<"in"<<std::endl;
 	  if (buf->getLeft()==root){
 	      std::cout<<"out1"<<std::endl;
 	      delete buf->getLeft();
 	      buf->setLeft(nod);
 	      buf->getLeft()->setFather(buf);
 	  }
 	  else
 	  {
 	   std::cout<<"out2"<<std::endl;
 	  delete buf->getRight();
 	      buf->setRight(nod);
 	      buf->getRight()->setFather(buf);
 	  }
 	  */
 	root=cur;
 	root->setInequality(in);
 	root->setOperation(ComplexCondition::Operation::COMPARE);
 	}
 	$truecur=cur;
 	} 
 	
 	
 	
 	
 	
 	;	
 term[ComplexCondition* cur] returns [ComplexCondition* truecur]
options {backtrack=true;}
scope
{

std::list<ComplexCondition*>* leaflist;
std::list<ComplexCondition*>::iterator* it2;
std::list<ComplexCondition*>::iterator* it1;
int k;
ComplexCondition* left;
ComplexCondition* right;
ComplexCondition* root;
bool flag;
bool flag2;
ComplexCondition* ecur;
} 
@init
{
$term::leaflist=new std::list<ComplexCondition*>(0);
$term::it1=new std::list<ComplexCondition*>::iterator();
$term::it2=new std::list<ComplexCondition*>::iterator();
$term::ecur=cur;
$term::k=1;
$term::flag=false;
$term::right=0;
$term::flag2=false;
} 
@after
{
if ($term::leaflist!=0)
  delete $term::leaflist;
if ($term::it1!=0)
  delete $term::it1;
if ($term::it2!=0)
  delete $term::it2;
}    
 	:
 	
 	
 	
 	{
 	$term::left= new ComplexCondition();
 	
 	}
 	childlf=factor[$term::left] 
 	{$term::left=$childlf.truecur;}
 	
 	(
 	{
 	bool fl=false;
 	std::cout<<"term enter"<<std::endl;
 	$term::leaflist->push_front(new ComplexCondition());
 	*$term::it1=$term::leaflist->begin();
 	$term::right=*(*$term::it1); 
 	
 	
 	}
 	
 	T_AND childrt=factor[$term::right]
 	
 	{
 	$term::leaflist->push_front($childrt.truecur);
 	*$term::it1=$term::leaflist->begin();
 	std::cout<<"term enter"<<std::endl;
 	if (($term::ecur==0)&&($term::flag==false)) 
 	{
 	    std::cout<<"t 1"<<std::endl;
 	   $term::root =new ComplexCondition(ComplexCondition::Operation::AND,$term::left,**$term::it1);
 	    if ($complex_cond::flag==0)
 	    {
 	   $complex_cond::cc=$term::root;
 	  $term::flag2=true;
 	   }
 	   $term::flag=true;
 	}
 	else
 	  if ($term::flag==true)
 	  {
 	     	 std::cout<<"t 2"<<std::endl;
 	     	
 	    
 	    
 	    $term::leaflist->push_front(new ComplexCondition(ComplexCondition::Operation::AND,$term::left,**$term::it1));
            $term::root=*($term::leaflist->begin());
           
           
            /*
            $term::leaflist->push_front(new ComplexCondition( ComplexCondition::Operation::AND,**$term::it1,$term::left));  	
 	     $term::root=*($term::leaflist->begin());
 	    $term::k=1;
 	 */
 	 
 	   }
 	   else 
 	   if (($term::ecur!=0)&&($term::flag==false))
 	   {
 	    std::cout<<"t 3"<<std::endl;
 	   $term::root=$term::ecur;
 	   $term::root->setOperation(ComplexCondition::Operation::AND);
 	   $term::root->setLeft($term::left);
 	   $term::root->setRight(**$term::it1);
 	   $term::flag=true;
 	   }
 	  if (($complex_cond::flag==0)&&($term::flag2==true))
 	    	   $complex_cond::cc=$term::root;
 	   
 	 std::cout<<"t 4"<<std::endl;
 	$term::left->setFather($term::root);
 	(**$term::it1)->setFather($term::root);
 	/*$term::root->setOperation(ComplexCondition::Operation::AND);*/
 	$term::left=$term::root;
 	std::cout<<"t 4"<<std::endl;
 	/*$term::right=*(*$term::it1); */
 	std::cout<<"t 4"<<std::endl; 	
       	$truecur=$term::root;
       	}
       	
 	 )+
 	|
 	 child=factor[$term::ecur]{$truecur=$child.truecur;}  
 	;

// $>
 	


 	