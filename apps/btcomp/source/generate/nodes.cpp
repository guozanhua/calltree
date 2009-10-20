/*
 *
 * Copyright (C) 2009, Joacim Jacobsson ( j dot jacobsson at gmail dot com )
 * All rights reserved.
 *
 * See LICENSE file for details
 *
 */

#include "nodes.h"
#include "program.h"

#include <btree/btree_data.h>
#include <btree/btree_func.h>
#include <callback/callback.h>

using namespace callback;

int gen_con( Node* n, Program* p )
{
	if( !n->m_Declared )
		return -1;

	switch( n->m_Grist.m_Type )
	{
	case E_GRIST_SEQUENCE:
		return gen_con_sequence( n, p );
		break;
	case E_GRIST_SELECTOR:
		return gen_con_selector( n, p );
		break;
	case E_GRIST_PARALLEL:
		return gen_con_parallel( n, p );
		break;
	case E_GRIST_DYN_SELECTOR:
		return gen_con_dynselector( n, p );
		break;
	case E_GRIST_DECORATOR:
		return gen_con_decorator( n, p );
		break;
	case E_GRIST_ACTION:
		return gen_con_action( n, p );
		break;
	}
	return -1;
}

int gen_exe( Node* n, Program* p )
{
	if( !n->m_Declared )
		return -1;

	switch( n->m_Grist.m_Type )
	{
	case E_GRIST_SEQUENCE:
		return gen_exe_sequence( n, p );
		break;
	case E_GRIST_SELECTOR:
		return gen_exe_selector( n, p );
		break;
	case E_GRIST_PARALLEL:
		return gen_exe_parallel( n, p );
		break;
	case E_GRIST_DYN_SELECTOR:
		return gen_exe_dynselector( n, p );
		break;
	case E_GRIST_DECORATOR:
		return gen_exe_decorator( n, p );
		break;
	case E_GRIST_ACTION:
		return gen_exe_action( n, p );
		break;
	}
	return -1;
}

int gen_des( Node* n, Program* p )
{
	if( !n->m_Declared )
		return -1;

	switch( n->m_Grist.m_Type )
	{
	case E_GRIST_SEQUENCE:
		return gen_des_sequence( n, p );
		break;
	case E_GRIST_SELECTOR:
		return gen_des_selector( n, p );
		break;
	case E_GRIST_PARALLEL:
		return gen_des_parallel( n, p );
		break;
	case E_GRIST_DYN_SELECTOR:
		return gen_des_dynselector( n, p );
		break;
	case E_GRIST_DECORATOR:
		return gen_des_decorator( n, p );
		break;
	case E_GRIST_ACTION:
		return gen_des_action( n, p );
		break;
	}
	return -1;
}

/*
 *
 * Sequence
 *
 */

struct SequenceNodeData
{
	int m_bss_JumpBackTarget;
	int m_bss_ReEntry;
};

void gen_setup_sequence( Node* n, Program* p )
{
	//Alloc space needed for code generation
	SequenceNodeData* nd = new SequenceNodeData;
    //Store needed generation data in the node's UserData pointer
	n->m_UserData = nd;

	//Alloc storage area for function call stack.
    nd->m_bss_JumpBackTarget = p->m_B.Push( sizeof( int ), 4 );
    //Alloc storage area for re-entry instruction
    nd->m_bss_ReEntry = p->m_B.Push( sizeof( int ), 4 );
}

void gen_teardown_sequence( Node* n, Program* p )
{
	//Free the space used when generating code.
	delete ((SequenceNodeData*)n->m_UserData);
	n->m_UserData = 0x0;
}

int gen_con_sequence( Node* n, Program* p )
{
	SequenceNodeData* nd = (SequenceNodeData*)n->m_UserData;
    // Enter Debug scope
    p->m_I.PushDebugScope( p, n, ACT_CONSTRUCT );


    //Set jump-back pointer value to uninitialized
    p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_JumpBackTarget, 0xffffffff, 0 );

    //Set re-entry pointer value to uninitialized
    p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_ReEntry, 0xffffffff, 0 );

    // Exit Debug scope
    p->m_I.PopDebugScope( p, n, ACT_CONSTRUCT );

    return 0;
}

int gen_exe_sequence( Node* n, Program* p )
{
	SequenceNodeData* nd = (SequenceNodeData*)n->m_UserData;

    // Enter Debug scope
    p->m_I.PushDebugScope( p, n, ACT_EXECUTE );

    typedef std::vector<int> PatchList;
    PatchList exit_running;
    PatchList exit_fail;
    PatchList dest_jumps;

    //Jump to re-entry point if set
    p->m_I.Push( INST_JABB_C_DIFF_B, nd->m_bss_ReEntry, 0xffffffff, nd->m_bss_ReEntry );
    int err;
    Node* c = GetFirstChild( n );
    while( c )
    {
        //call child-node construction code
    	if( (err = gen_con( c, p )) != 0 )
    		return err;

        //store re-entry pointer
        p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_ReEntry, p->m_I.Count() + 1, 0 );

        //call child-node execution code
        if( (err = gen_exe( c, p )) != 0 )
        	return err;

        //set the destruction jump pointer (we re-use the bss for jump-back-target for this)
        dest_jumps.push_back( p->m_I.Count() );
        p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_JumpBackTarget, 0xffffffff, 0 );

        //"exit if running" jump.
        exit_running.push_back( p->m_I.Count() );
        p->m_I.Push( INST_JABC_R_EQUA_C, 0xffffffff, E_NODE_RUNNING, 0 );

        //Jump to destruction code and set the jump-back at the same time
        dest_jumps.push_back( p->m_I.Count() );
        p->m_I.Push( INST_JABC_S_C_IN_B, 0xffffffff, nd->m_bss_JumpBackTarget,  p->m_I.Count() + 1 );

        //exit this node if return value is non-success
        exit_fail.push_back( p->m_I.Count() );
        p->m_I.Push( INST_JABC_R_DIFF_C, 0xffffffff, E_NODE_SUCCESS, 0 );

        //Iterate
        c = c->m_Next;
    }

    //Success! Jump past all this destruction stuff
    int jump_to_exit_success = p->m_I.Count();
    p->m_I.Push( INST_JABC_CONSTANT, 0xffffffff, 0, 0 );

    //Here we generate the destruction code for all child-nodes.
    int i = 0;
    c = GetFirstChild( n );
    while( c )
    {
        //Patch store destruction code pointer instruction
        p->m_I.SetA2( dest_jumps[0+i*2], p->m_I.Count() );
        //Patch jump to destruction instruction
        p->m_I.SetA1( dest_jumps[1+i*2], p->m_I.Count() );
        //call child-node destruction code
        if( (err = gen_des( c, p )) != 0 )
        	return err;
        //Jump back to calling code and "de-initialize the jump-back-target at the same time
        p->m_I.Push( INST_JABB_S_C_IN_B, nd->m_bss_JumpBackTarget, nd->m_bss_JumpBackTarget, 0xffffffff );

        //Iterate
        c = c->m_Next;++i;
    }

    //Set return value to node fail
    int exit_fail_point = p->m_I.Count();
    p->m_I.Push( INST__STORE_C_IN_R, E_NODE_FAIL, 0, 0 );

    //Patch the "jump to exit success"
    p->m_I.SetA1( jump_to_exit_success, p->m_I.Count() );

    //clear re-entry pointer
    p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_ReEntry, 0xffffffff, 0 );

    //Patch jump instruction targets for fail.
    int s;
    s = exit_fail.size();
    for( int i = 0; i < s; ++i )
        p->m_I.SetA1( exit_fail[i], exit_fail_point );

    //Patch jump instruction targets for running.
    int exit_running_point = p->m_I.Count();
    s = exit_running.size();
    for( int i = 0; i < s; ++i )
        p->m_I.SetA1( exit_running[i], exit_running_point );

    // Exit Debug scope
    p->m_I.PopDebugScope( p, n, ACT_EXECUTE );
    return 0;
}

int gen_des_sequence( Node* n, Program* p )
{
	SequenceNodeData* nd = (SequenceNodeData*)n->m_UserData;

    // Enter Debug scope
    p->m_I.PushDebugScope( p, n, ACT_DESTRUCT );

    //Jump past destruction code if m_bss_JumpBackTarget is uninitialized
    p->m_I.Push( INST_JABC_C_EQUA_B, p->m_I.Count() + 2, 0xffffffff, nd->m_bss_JumpBackTarget );
    //Jump to destruction code and set the jump-back at the same time
    p->m_I.Push( INST_JABB_S_C_IN_B, nd->m_bss_JumpBackTarget, nd->m_bss_JumpBackTarget, p->m_I.Count() + 1 );

    // Exit Debug scope
    p->m_I.PopDebugScope( p, n, ACT_DESTRUCT );

    return 0;
}

/*
 *
 * Selector
 *
 */

struct SelectorNodeData
{
	int m_bss_JumpBackTarget;
	int m_bss_ReEntry;
};

void gen_setup_selector( Node* n, Program* p )
{
	//Alloc space needed for code generation
	SelectorNodeData* nd = new SelectorNodeData;

	//Store needed generation data in the node's UserData pointer
	n->m_UserData = nd;

	//Alloc storage area for function call stack.
    nd->m_bss_JumpBackTarget = p->m_B.Push( sizeof( int ), 4 );

    //Alloc storage area for re-entry instruction
    nd->m_bss_ReEntry = p->m_B.Push( sizeof( int ), 4 );
}

void gen_teardown_selector( Node* n, Program* p )
{
	//Free the space used when generating code.
	delete ((SelectorNodeData*)n->m_UserData);
	n->m_UserData = 0x0;
}

int gen_con_selector( Node* n, Program* p )
{
	SelectorNodeData* nd = (SelectorNodeData*)n->m_UserData;

    // Enter Debug scope
    p->m_I.PushDebugScope( p, n, ACT_CONSTRUCT );

    //Set re-entry pointer value to uninitialized
    p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_ReEntry, 0xffffffff, 0 );

    //Set re-entry pointer value to uninitialized
    p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_JumpBackTarget, 0xffffffff, 0 );

    // Exit Debug scope
    p->m_I.PopDebugScope( p, n, ACT_CONSTRUCT );

	return 0;
}

int gen_exe_selector( Node* n, Program* p )
{
	SelectorNodeData* nd = (SelectorNodeData*)n->m_UserData;

    // Enter Debug scope
    p->m_I.PushDebugScope( p, n, ACT_EXECUTE );

    typedef std::vector<int> PatchList;
    PatchList exit_running;
    PatchList exit_success;
    PatchList dest_jumps;

    //Jump to re-entry point if set
    p->m_I.Push( INST_JABB_C_DIFF_B, nd->m_bss_ReEntry, 0xffffffff, nd->m_bss_ReEntry );

    int err;
    Node* c = GetFirstChild( n );
    while( c )
    {
        //call child-node construction code
        if( (err = gen_con( c, p )) != 0 )
        	return err;

        //store re-entry pointer
        p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_ReEntry, p->m_I.Count() + 1, 0 );

        //call child-node execution code
        if( (err = gen_exe( c, p )) != 0 )
        	return err;

        //set the destruction jump pointer (we re-use the bss for jump-back-target for this)
        dest_jumps.push_back( p->m_I.Count() );
        p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_JumpBackTarget, 0xffffffff, 0 );

        //"exit if running" jump.
        exit_running.push_back( p->m_I.Count() );
        p->m_I.Push( INST_JABC_R_EQUA_C, 0xffffffff, E_NODE_RUNNING, 0 );

        //Jump to destruction code and set the jump-back at the same time
        dest_jumps.push_back( p->m_I.Count() );
        p->m_I.Push( INST_JABC_S_C_IN_B, 0xffffffff, nd->m_bss_JumpBackTarget,  p->m_I.Count() + 1 );

        //exit this node if return value success
        exit_success.push_back( p->m_I.Count() );
        p->m_I.Push( INST_JABC_R_EQUA_C, 0xffffffff, E_NODE_SUCCESS, 0 );

        //Iterate
        c = c->m_Next;
    }

    //Fail! Jump past all this destruction stuff
    int jump_to_exit_fail = p->m_I.Count();
    p->m_I.Push( INST_JABC_CONSTANT, 0xffffffff, 0, 0 );

    //Here we generate the destruction code for all child-nodes.
    int i = 0;
    c = GetFirstChild( n );
    while( c )
    {
        //Patch store destruction code pointer instruction
        p->m_I.SetA2( dest_jumps[0+i*2], p->m_I.Count() );
        //Patch jump to destruction instruction
        p->m_I.SetA1( dest_jumps[1+i*2], p->m_I.Count() );
        //call child-node destruction code
        if( (err = gen_des( c, p )) != 0 )
        	return err;
        //Jump back to calling code and reset the jump-back-target at the same time
        p->m_I.Push( INST_JABB_S_C_IN_B, nd->m_bss_JumpBackTarget, nd->m_bss_JumpBackTarget, 0xffffffff );
        //Iterate
        c = c->m_Next; ++i;
    }

    //Set return value to node success
    int exit_success_point = p->m_I.Count();
    p->m_I.Push( INST__STORE_C_IN_R, E_NODE_SUCCESS, 0, 0 );

    //Patch the "jump to exit fail"
    p->m_I.SetA1( jump_to_exit_fail, p->m_I.Count() );

    //clear re-entry pointer
    p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_ReEntry, 0xffffffff, 0 );

    //Patch jump instruction targets for success.
    int s;
    s = exit_success.size();
    for( int i = 0; i < s; ++i )
        p->m_I.SetA1( exit_success[i], exit_success_point );

    //Patch jump instruction targets for running.
    int exit_running_point = p->m_I.Count();
    s = exit_running.size();
    for( int i = 0; i < s; ++i )
        p->m_I.SetA1( exit_running[i], exit_running_point );

    // Exit Debug scope
    p->m_I.PopDebugScope( p, n, ACT_EXECUTE );

    return 0;
}

int gen_des_selector( Node* n, Program* p )
{
	SelectorNodeData* nd = (SelectorNodeData*)n->m_UserData;
    // Enter Debug scope
    p->m_I.PushDebugScope( p, n, ACT_DESTRUCT );

    //Jump past destruction code if nd->m_bss_JumpBackTarget is uninitialized
    p->m_I.Push( INST_JABC_C_EQUA_B, p->m_I.Count() + 2, 0xffffffff, nd->m_bss_JumpBackTarget );
    //Jump to destruction code and set the jump-back at the same time
    p->m_I.Push( INST_JABB_S_C_IN_B, nd->m_bss_JumpBackTarget, nd->m_bss_JumpBackTarget, p->m_I.Count() + 1 );

    // Exit Debug scope
    p->m_I.PopDebugScope( p, n, ACT_DESTRUCT );

    return 0;
}

/*
 *
 * Parallel
 *
 */

struct ParallelNodeData
{
	int m_bss_SuccessCounter;
};

void gen_setup_parallel( Node* n, Program* p )
{
	//Alloc space needed for code generation
	ParallelNodeData* nd = new ParallelNodeData;

	//Store needed generation data in the node's UserData pointer
	n->m_UserData = nd;

    //Alloc storage space for the success counter
    nd->m_bss_SuccessCounter = p->m_B.Push( sizeof( int ), 4 );
}

void gen_teardown_parallel( Node* n, Program* p )
{
	//Free the space used when generating code.
	delete ((ParallelNodeData*)n->m_UserData);
	n->m_UserData = 0x0;
}

int gen_con_parallel( Node* n, Program* p )
{
	ParallelNodeData* nd = (ParallelNodeData*)n->m_UserData;
    // Enter Debug scope
    p->m_I.PushDebugScope( p, n, ACT_CONSTRUCT );

    int err;
    Node* c = GetFirstChild( n );
    while( c )
    {
        //call child-node construction code
        if( (err = gen_con( c, p )) != 0 )
        	return err;
        //Iterate
        c = c->m_Next;
    }

    // Exit Debug scope
    p->m_I.PopDebugScope( p, n, ACT_CONSTRUCT );
    return 0;
}

int gen_exe_parallel( Node* n, Program* p )
{
	ParallelNodeData* nd = (ParallelNodeData*)n->m_UserData;
    // Enter Debug scope
    p->m_I.PushDebugScope( p, n, ACT_EXECUTE );

    //Set the success counter to 0
    p->m_I.Push( INST__STORE_C_IN_B, nd->m_bss_SuccessCounter, 0, 0 );

    typedef std::vector<int> PatchList;
    PatchList exit_fail;

    int i = 0, err;
    Node* c = GetFirstChild( n );
    while( c )
    {
        //call child-node execution code
    	if( (err = gen_exe(c, p)) != 0 )
    		return err;

    	//jump out if failed
        exit_fail.push_back( p->m_I.Count() );
        p->m_I.Push( INST_JABC_R_EQUA_C, 0xffffffff, E_NODE_FAIL, 0 );

        //Skip count up if running
        p->m_I.Push( INST_JABC_R_DIFF_C, p->m_I.Count() + 2, E_NODE_SUCCESS, 0 );

        //Count up number of successful child nodes.
        p->m_I.Push( INST__INC_BSSVALUE, nd->m_bss_SuccessCounter, 1, 0 );

        //Iterate
        c = c->m_Next; ++i;
    }

    //Set return value to running
    p->m_I.Push( INST__STORE_C_IN_R, E_NODE_RUNNING, 0, 0 );
    //Jump out if number any child was non-successfull.
    p->m_I.Push( INST_JABC_C_DIFF_B, p->m_I.Count() + 2, i, nd->m_bss_SuccessCounter );
    //Set return value to success (skiped if nd->m_bss_SuccessCounter != m_Children->Size())
    p->m_I.Push( INST__STORE_C_IN_R, E_NODE_SUCCESS, 0, 0 );

    //Patch exit-fail jumps
    int exit_point = p->m_I.Count();
    int s = exit_fail.size();
    for( int i = 0; i < s; ++i )
        p->m_I.SetA1( exit_fail[i], exit_point );

    // Exit Debug scope
    p->m_I.PopDebugScope( p, n, ACT_EXECUTE );
	return 0;
}

int gen_des_parallel( Node* n, Program* p )
{
	ParallelNodeData* nd = (ParallelNodeData*)n->m_UserData;
    // Enter Debug scope
    p->m_I.PushDebugScope( p, n, ACT_DESTRUCT );

    int err;
    Node* c = GetFirstChild( n );
    while( c )
    {
        //call child-node destruction code
        if( (err = gen_des( c, p )) != 0 )
        	return err;
        //Iterate
        c = c->m_Next;
    }

    // Exit Debug scope
    p->m_I.PopDebugScope( p, n, ACT_DESTRUCT );
	return 0;
}

/*
 *
 * Dynamic Selector
 *
 */

void gen_setup_dynselector( Node* n, Program* p )
{

}

void gen_teardown_dynselector( Node* n, Program* p )
{

}

int gen_con_dynselector( Node* n, Program* p )
{
	return -1;
}

int gen_exe_dynselector( Node* n, Program* p )
{
	return -1;
}

int gen_des_dynselector( Node* n, Program* p )
{
	return -1;
}

/*
 *
 * Decorator
 *
 */

void gen_setup_decorator( Node* n, Program* p )
{

}

void gen_teardown_decorator( Node* n, Program* p )
{

}

int gen_con_decorator( Node* n, Program* p )
{
	return -1;
}

int gen_exe_decorator( Node* n, Program* p )
{
	return -1;
}

int gen_des_decorator( Node* n, Program* p )
{
	return -1;
}

/*
 *
 * Action
 *
 */

void gen_setup_action( Node* n, Program* p )
{

}

void gen_teardown_action( Node* n, Program* p )
{

}

int gen_con_action( Node* n, Program* p )
{
	return -1;
}

int gen_exe_action( Node* n, Program* p )
{
	return -1;
}

int gen_des_action( Node* n, Program* p )
{
	return -1;
}