/*
 *
 * Copyright (C) 2009, Joacim Jacobsson ( j dot jacobsson at gmail dot com )
 * All rights reserved.
 *
 * See LICENSE file for details
 *
 */

%pure-parser
%parse-param { SParserContext* ctx }
%parse-param { void* scanner }
%lex-param   { yyscan_t* scanner }
%error-verbose 
%verbose 

%start expressions

%{

#include "common.h"
#include <stdio.h>

#define YYMALLOC ctx->m_Alloc
#define YYFREE ctx->m_Free

bool DeclareAction( SParserContext* ctx, const Identifier& id, Variable* vars, Variable* args );
bool DeclareDecorator( SParserContext* ctx, const Identifier& id, Variable* vars, Variable* args );
Node* AllocateNode( BehaviorTreeContext ctx, NodeGristType type, Node* child );
Variable* AllocateVariable( BehaviorTreeContext ctx, VariableType type, const Identifier& id );
Decorator* LookUpDecorator( BehaviorTreeContext ctx, const Identifier& id );
Action* LookUpAction( BehaviorTreeContext ctx, const Identifier& id );

%}

%token            T_LPARE        /* '(' */
%token            T_RPARE        /* ')' */
%token            T_QUOTE        /* ''' */
%token            T_DEFTREE      /* literal string "deftree" */
%token            T_DEFACT       /* literal string "defact" */
%token            T_DEFDEC       /* literal string "defdec" */
%token            T_TREE         /* literal string "tree" */
%token            T_INCLUDE      /* literal string "include" */
%token            T_SEQUENCE     /* literal string "sequence" */
%token            T_SELECTOR     /* literal string "selector" */
%token            T_DSELECTOR    /* literal string "dyn_selector" */
%token            T_PARALLEL     /* literal string "parallel" */
%token            T_ACTION       /* literal string "action" */
%token            T_DECORATOR    /* literal string "decorator" */
%token            T_INT32        /* literal string "int32" */
%token            T_BOOL         /* literal string "bool" */
%token            T_FLOAT        /* literal string "float" */
%token            T_STRING       /* literal string "string" */

%token<m_Integer> T_INT32_VALUE  /* a integer value */
%token<m_Bool>    T_BOOL_VALUE   /* a boolean value (i.e. "true" or "false) */
%token<m_Float>   T_FLOAT_VALUE  /* a float value */
%token<m_String>  T_STRING_VALUE /* a string value */
%token<m_Id>      T_ID           /* a legal identifier string */

%type<m_Node> node nmembers sequence selector parallel dselector decorator action nlist
%type<m_Variable> vlist vmember variable vtypes vdlist vdmember vardec vdtypes

%union {
    NodeGrist      m_NodeGrist;
    Identifier     m_Id;
    Node*          m_Node;
    Action*        m_Action;
    Decorator*     m_Decorator;
    Variable*      m_Variable;
    StringData     m_String;
    int            m_Integer;
    float          m_Float;
    bool           m_Bool;
}

%%

expressions: list
           | list expressions
           ;

list: T_LPARE atom T_RPARE
    | T_LPARE T_RPARE
    ;

atom: deftree
    | include
    | defact
    | defdec
    ;

deftree: T_DEFTREE T_ID node
       {
		printf( "deftree %s\n", $2.m_Text );
       }
       ;

include: T_INCLUDE T_STRING_VALUE
       {
       	printf( "include %s\n", $2.m_Parsed );
       }
       ;

defact: T_DEFACT T_ID vlist vdlist
	  {
	  	printf( "defact %s\n", $2.m_Text );
	  }
      ;
      
defdec: T_DEFDEC T_ID vlist vdlist
      {
      	printf( "defdec %s\n", $2.m_Text );
      }
      ;

node: T_LPARE sequence T_RPARE  { $$ = $2; }
    | T_LPARE selector T_RPARE  { $$ = $2; }
    | T_LPARE parallel T_RPARE  { $$ = $2; }
    | T_LPARE dselector T_RPARE { $$ = $2; }
    | T_LPARE decorator T_RPARE { $$ = $2; }
    | T_LPARE action T_RPARE    { $$ = $2; }
    ;
    
nlist: T_LPARE nmembers T_RPARE         { $$ = $2; }
     | T_QUOTE T_LPARE nmembers T_RPARE { $$ = $3; }
     | T_QUOTE T_LPARE T_RPARE          { $$ = 0x0;}
     ;

nmembers: node	{ $$ = $1; }
        | node nmembers { $$ = $1; AppendToEndOfList( $1, $2 ); }
        ;

sequence: T_SEQUENCE nlist 
        {
        	Node* n = AllocateNode( ctx->m_Tree, E_GRIST_SEQUENCE, $2 );
        	$$ = n;
        }
        ;
        
selector: T_SELECTOR nlist 
        {
        	Node* n = AllocateNode( ctx->m_Tree, E_GRIST_SELECTOR, $2 );
        	$$ = n;
        }
        ;
        
parallel: T_PARALLEL nlist 
        {
        	Node* n = AllocateNode( ctx->m_Tree, E_GRIST_PARALLEL, $2 );
        	$$ = n;
        }
        ;
        
dselector: T_DSELECTOR nlist
         {
        	Node* n = AllocateNode( ctx->m_Tree, E_GRIST_DYN_SELECTOR, $2 );
        	$$ = n;
         }
         ;
         
decorator: T_DECORATOR T_QUOTE T_ID vlist node 
         {
        	Node* n = AllocateNode( ctx->m_Tree, E_GRIST_DECORATOR, $5 );
        	$$ = n;
        	n->m_Grist.m_Decorator.m_Arguments = $4;
        	n->m_Grist.m_Decorator.m_Decorator = LookUpDecorator( ctx->m_Tree, $3 );
         }
         ;
         
action: T_ACTION T_QUOTE T_ID vlist 
      {
        	Node* n = AllocateNode( ctx->m_Tree, E_GRIST_ACTION, 0x0 );
        	$$ = n;
        	n->m_Grist.m_Action.m_Arguments = $4;
        	n->m_Grist.m_Action.m_Action = LookUpAction( ctx->m_Tree, $3 );
      }
      ;

vlist: T_QUOTE T_LPARE vmember T_RPARE { $$ = $3; }
     | T_QUOTE T_LPARE T_RPARE         { $$ = 0x0; }
     ;

vmember: variable         { $$ = $1; }
       | variable vmember { $$ = $1; AppendToEndOfList( $$, $2 ); }
       ;
       
variable: T_LPARE vtypes T_RPARE { $$ = $2; }
        ;

vtypes: T_ID T_INT32_VALUE  { $$ = AllocateVariable( ctx->m_Tree, E_VART_INTEGER, $1 ); $$->m_Data.m_Integer = $2; }
      | T_ID T_STRING_VALUE { $$ = AllocateVariable( ctx->m_Tree, E_VART_STRING, $1 );  $$->m_Data.m_String = $2; }
      | T_ID T_BOOL_VALUE   { $$ = AllocateVariable( ctx->m_Tree, E_VART_BOOL, $1 );    $$->m_Data.m_Bool = $2; }
      | T_ID T_FLOAT_VALUE  { $$ = AllocateVariable( ctx->m_Tree, E_VART_FLOAT, $1 );   $$->m_Data.m_Float = $2; }
      ;

vdlist: T_LPARE vdmember T_RPARE { $$ = $2; }
      | T_LPARE T_RPARE          { $$ = 0x0; }

vdmember: vardec          { $$ = $1; }
        | vardec vdmember { $$ = $1; AppendToEndOfList( $$, $2 ); }
        ;

vardec: T_QUOTE T_LPARE vdtypes T_RPARE { $$ = $3; }
      ;

vdtypes: T_INT32 T_ID  { $$ = AllocateVariable( ctx->m_Tree, E_VART_INTEGER, $2 ); }
       | T_STRING T_ID { $$ = AllocateVariable( ctx->m_Tree, E_VART_STRING, $2 ); }
       | T_BOOL T_ID   { $$ = AllocateVariable( ctx->m_Tree, E_VART_BOOL, $2 ); }
       | T_FLOAT T_ID  { $$ = AllocateVariable( ctx->m_Tree, E_VART_FLOAT, $2 ); }
       ;

%%

Decorator* LookUpDecorator( BehaviorTreeContext ctx, const Identifier& id )
{
/*
    Decorator* d = ctx->m_Tree->LookupDecorator( id );
	if( !d )
	{
    	d = new Decorator;
    	InitDecorator( d );
    	d->m_Id = id;
		d->m_Args = args;
		d->m_Vars = vars;	
	    d->m_Declared = true;
	    ctx->m_Tree->RegisterDecorator( d );
    }
	else if( d && !d->m_Declared )
    {
    	d->m_Id = id;
    	d->m_Args = args;
    	d->m_Vars = vars;
    	d->m_Declared = true;
	}
    else if( d && d->m_Declared )
    {
    	DeleteVariableList( vars );
    	DeleteVariableList( args );
    	
        char tmp[2048];
        sprintf( tmp, "Decorator \"%s\" was previously declared on line %d.\n", id.m_Text, d->m_Id.m_Line );
        yyerror( ctx, tmp );
        return false;
    }
    return true;
    */
    return 0x0;
}

Action* LookUpAction( BehaviorTreeContext ctx, const Identifier& id )
{
/*
    Action* a = ctx->m_Tree->LookupAction( id );
	if( !a )
	{
    	a = new Action;
    	InitAction( a );
    	a->m_Id = id;
		a->m_Args = args;
		a->m_Vars = vars;	
	    a->m_Declared = true;
	    ctx->m_Tree->RegisterAction( a );
    }
	else if( a && !a->m_Declared )
    {
    	a->m_Id = id;
    	a->m_Args = args;
    	a->m_Vars = vars;
    	a->m_Declared = true;
	}
    else if( a && a->m_Declared )
    {
    	DeleteVariableList( vars );
    	DeleteVariableList( args );
    	
        char tmp[2048];
        sprintf( tmp, "action \"%s\" was previously declared on line %d.\n", id.m_Text, a->m_Id.m_Line );
        yyerror( ctx, tmp );
        return false;
    }
    return true;
    */
    return 0x0;
}

Node* AllocateNode( BehaviorTreeContext ctx, NodeGristType type, Node* child )
{
	Node* n = (Node*)AllocateObject( ctx );
	if( n )
	{
		InitNode( n );
		n->m_Grist.m_Type = type;
		SetFirstChild( n, child );
		SetParentOnChildren( n );
	}
	return n;
}

Variable* AllocateVariable( BehaviorTreeContext ctx, VariableType type, const Identifier& id )
{
	Variable* v = (Variable*)AllocateObject( ctx );
	if( v )
	{
		InitVariable( v );
		v->m_Type = type;
		v->m_Id = id;
	}
	return v;
} 
