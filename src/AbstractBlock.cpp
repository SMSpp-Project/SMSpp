/*--------------------------------------------------------------------------*/
/*------------------------ File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the AbstractBlock class.
 *
 * \version 0.10
 *
 * \date 25 - 07 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractBlock.h"
#include "ColVariable.h"
#include "FRowConstraint.h"
#include "OneVarConstraint.h"
#include "FRealObjective.h"


/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;
using namespace std;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register AbstractBlock to the Block factory

SMSpp_insert_in_factory_cpp_1( AbstractBlock );

/*--------------------------------------------------------------------------*/
/*-------------------------- METHODS of Block ------------------------------*/
/*--------------------------------------------------------------------------*/

~AbstractBlock::AbstractBlock( ) {
 // first, clear() all Constraint
 for( const auto & ci : q_Block->get_static_constraints() ) {
  if( un_any_const_static( ci, []( FRowConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< FRowConstraint >() ) )
   break;
  if( un_any_const_static( ci, []( OneVarConstraint & cnst )
			       { cnst.clear(); } ,
			   un_any_type< OneVarConstraint >() ) )
   break;
  throw( logic_error(
	  "some static Constraint not FRowConstraint or OneVarConstraint" ) );
  }

 for( const auto & ci : q_Block->get_dynamic_constraints() ) {
  if( un_any_const_static( ci, []( FRowConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< FRowConstraint >() ) )
   break;
  if( un_any_const_static( ci, []( OneVarConstraint & cnst )
			         { cnst.clear(); } ,
			   un_any_type< OneVarConstraint >() ) )
   break;
  throw( logic_error(
	"some dynamic Constraint not FRowConstraint or OneVarConstraint" ) );
  }

 // then clear the Objective
 if( f_Objective )
  f_Objective->clear();

 // now delete all the inner Block
 for( auto bi : v_Block )
  delete bi;

 v_Block.clear();

 // now delete all the Constraint
 for( const auto & ci : q_Block->get_static_constraints() ) {
  if( un_any_thing( ci, FRowConstraint , { delete & var; } ) )
   break;
  if( un_any_thing( ci, OneVarConstraint , { delete & var; } ) )
   break;
  throw( logic_error(
	  "some static Constraint not FRowConstraint or OneVarConstraint" ) );
  }

 for( const auto & ci : q_Block->get_dynamic_constraints() ) {
  if( un_any_thing( ci, FRowConstraint , { delete & var; } ) )
   break;
  if( un_any_thing( ci, OneVarConstraint , { delete & var; } ) )
   break;
  throw( logic_error(
	"some dynamic Constraint not FRowConstraint or OneVarConstraint" ) );
   }

 // now delete all the Variable
 for( const auto & vi : q_Block->get_static_variables() ) {
  if( un_any_thing( vi, ColVariable , { delete & var; } ) )
   break;
  throw( logic_error( "some static Variable not ColVariable" ) );
  }

 for( const auto & vi : q_Block->get_dynamic_constraints() ) {
  if( un_any_thing( vi, ColVariable , { delete & var; } ) )
   break;
  throw( logic_error( "some dynamic Variable not ColVariable" ) );
  }
 }  // end( ~AbstractBlock )

/*--------------------------------------------------------------------------*/
/*-------------------- End File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
