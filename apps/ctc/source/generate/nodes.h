/*******************************************************************************
 * Copyright (c) 2009-04-24 Joacim Jacobsson.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    Joacim Jacobsson - first implementation
 *******************************************************************************/

#ifndef NODES_H_INCLUDED
#define NODES_H_INCLUDED

struct Program;
struct Node;

int gen_con( Node* n, Program* p );
int gen_exe( Node* n, Program* p );
int gen_des( Node* n, Program* p );

void gen_setup_sequence( Node* n, Program* p );
void gen_teardown_sequence( Node* n, Program* p );
int gen_con_sequence( Node* n, Program* p );
int gen_exe_sequence( Node* n, Program* p );
int gen_des_sequence( Node* n, Program* p );

void gen_setup_selector( Node* n, Program* p );
void gen_teardown_selector( Node* n, Program* p );
int gen_con_selector( Node* n, Program* p );
int gen_exe_selector( Node* n, Program* p );
int gen_des_selector( Node* n, Program* p );

void gen_setup_parallel( Node* n, Program* p );
void gen_teardown_parallel( Node* n, Program* p );
int gen_con_parallel( Node* n, Program* p );
int gen_exe_parallel( Node* n, Program* p );
int gen_des_parallel( Node* n, Program* p );

void gen_setup_dynselector( Node* n, Program* p );
void gen_teardown_dynselector( Node* n, Program* p );
int gen_con_dynselector( Node* n, Program* p );
int gen_exe_dynselector( Node* n, Program* p );
int gen_des_dynselector( Node* n, Program* p );

void gen_setup_succeed( Node* n, Program* p );
void gen_teardown_succeed( Node* n, Program* p );
int gen_con_succeed( Node* n, Program* p );
int gen_exe_succeed( Node* n, Program* p );
int gen_des_succeed( Node* n, Program* p );

void gen_setup_fail( Node* n, Program* p );
void gen_teardown_fail( Node* n, Program* p );
int gen_con_fail( Node* n, Program* p );
int gen_exe_fail( Node* n, Program* p );
int gen_des_fail( Node* n, Program* p );

void gen_setup_work( Node* n, Program* p );
void gen_teardown_work( Node* n, Program* p );
int gen_con_work( Node* n, Program* p );
int gen_exe_work( Node* n, Program* p );
int gen_des_work( Node* n, Program* p );

void gen_setup_decorator( Node* n, Program* p );
void gen_teardown_decorator( Node* n, Program* p );
int gen_con_decorator( Node* n, Program* p );
int gen_exe_decorator( Node* n, Program* p );
int gen_des_decorator( Node* n, Program* p );


void gen_setup_action( Node* n, Program* p );
void gen_teardown_action( Node* n, Program* p );
int gen_con_action( Node* n, Program* p );
int gen_exe_action( Node* n, Program* p );
int gen_des_action( Node* n, Program* p );


#endif /* NODES_H_INCLUDED */
