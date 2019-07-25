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

#include "ColVariableSolution.h"

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

~AbstractBlock::AbstractBlock( )
{
 // first, clear() all Constraint
 for( auto & ci : v_s_Constraint ) {
  if( un_any_static( ci, []( FRowConstraint & cnst ) { cnst.clear(); } ,
		     un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_static( ci, []( OneVarConstraint & cnst ) { cnst.clear(); } ,
		     un_any_type< OneVarConstraint >() ) )
   continue;
  throw( logic_error(
	  "some static Constraint not FRowConstraint or OneVarConstraint" ) );
  }

 for( auto & ci : v_d_Constraint ) {
  if( un_any_static( ci, []( FRowConstraint & cnst ) { cnst.clear(); } ,
		     un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_static( ci, []( OneVarConstraint & cnst ) { cnst.clear(); } ,
		     un_any_type< OneVarConstraint >() ) )
   continue;
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
 for( auto & ci : v_s_Constraint ) {
  if( un_any_thing( ci, FRowConstraint , { delete & var; } ) )
   continue;
  if( un_any_thing( ci, OneVarConstraint , { delete & var; } ) )
   continue;
  throw( logic_error(
	  "some static Constraint not FRowConstraint or OneVarConstraint" ) );
  }

 for( auto & ci : v_d_Constraint ) {
  if( un_any_thing( ci, FRowConstraint , { delete & var; } ) )
   continue;
  if( un_any_thing( ci, OneVarConstraint , { delete & var; } ) )
   continue;
  throw( logic_error(
	"some dynamic Constraint not FRowConstraint or OneVarConstraint" ) );
   }

 // now delete all the Variable
 for( auto & vi : v_s_Variable ) {
  if( un_any_thing( vi , ColVariable , { delete & var; } ) )
   continue;
  throw( logic_error( "some static Variable not ColVariable" ) );
  }

 for( auto & vi : v_d_Variable ) {
  if( un_any_thing( vi , ColVariable , { delete & var; } ) )
   continue;
  throw( logic_error( "some dynamic Variable not ColVariable" ) );
  }
 }  // end( ~AbstractBlock )

/*--------------------------------------------------------------------------*/

bool AbstractBlock::is_feasible( bool useabstract , Configuration *fsbc )
{
 if( ! useabstract )
  return( false );

 // compute the accuracy parameter- - - - - - - - - - - - - - - - - - - - - -
 FNumber eps = 0;
 auto tfsbc = dynamic_cast<SimpleConfiguration<FNumber> *>( fsbc );

 if( ( ! tfsbc ) && f_BlockConfig &&
     f_BlockConfig->f_is_feasible_Configuration )
  tfsbc = dynamic_cast<SimpleConfiguration<FNumber> *>(
			         f_BlockConfig->f_is_feasible_Configuration );
 if( tfsbc )
  eps = tfsbc->f_value;

 bool feas = true;

 // the static Constraints of the Block - - - - - - - - - - - - - - - - - - -
 for( auto & ci : v_s_Constraint ) {
  if( un_any_static( ci , [ & feas , eps ]( FRowConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_static( ci , [ & feas , eps ]( OneVarConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< OneVarConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( logic_error(
	  "some static Constraint not FRowConstraint or OneVarConstraint" ) );
  }

 // the static Variables of the Block - - - - - - - - - - - - - - - - - - - -
 for( auto & vi : v_s_Variable ) {
  if( un_any_static( vi , [ & feas , eps ]( ColVariable & car )
		                          { feas = var.is_feasible( eps ); } ,
		     un_any_type< ColVariable >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( logic_error( "some static Variable not ColVariable" ) );
  }

 // the dynamic Constraints of the Block-  - - - - - - - - - - - - - - - - - -
 for( auto & ci : v_d_Constraint ) {
  if( un_any_dynamic( ci , [ & feas , eps ]( FRowConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_dynamic( ci , [ & feas , eps ]( OneVarConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< OneVarConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( logic_error(
	"some dynamic Constraint not FRowConstraint or OneVarConstraint" ) );
  }

 // the dynamic Variables of the Block- - - - - - - - - - - - - - - - - - - -
 for( auto & vi : v_d_Variable ) {
  if( un_any_dynamic( vi , [ & feas, eps ]( ColVariable & car )
		                          { feas = var.is_feasible( eps ); } ,
		     un_any_type< ColVariable >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( logic_error( "some dynamic Variable not ColVariable" ) );
  }

 // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( auto blck : v_Block )
  if( ! blck->is_feasible( true ) )
   return( false );

 return( true );

 }  // end( AbstractBlock::is_feasible )

/*--------------------------------------------------------------------------*/

Solution * AbstractBlock::get_Solution( Configuration *solc , bool emptys )
{
 return( new ColVariableSolution() );
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::print( ostream &output ) const
{
 output << endl << "Block with: ";
 output << endl << v_s_Variable.size() << " types of static Variables, "
                << v_d_Variable.size() << " types of dynamic Variables, "
        << endl << v_s_Constraint.size() << " types of static Constraints, "
                << v_d_Constraint.size() << " types of dynamic Constraints, "
        << endl << v_Block.size() << " inner Blocks"
        << endl;

 if( verbosity_lvl == Block::medium || verbosity_lvl == Block::high ) {
  // the static Constraints of the Block- - - - - - - - - - - - - - - - - - -
  output << "Static Constraints:" << endl;
  for( unsigned int i = 0 ; i < v_s_Constraint.size() ; ++i ) {
   output << i;
   if( ! v_s_Constraint_names[ i ].empty() )
    output << " (" << v_s_Constraint_names[ i ] << "): ";
   else
    output << ": ";

   if( un_any_static( ci, []( FRowConstraint & cnst )
		            { output << cnst << endl; } ,
		      un_any_type< FRowConstraint >() ) )
    continue;
   if( un_any_static( ci, []( OneVarConstraint & cnst )
		            { output << cnst << endl; } ,
		     un_any_type< OneVarConstraint >() ) )
    continue;
   }
  throw( logic_error(
	  "some static Constraint not FRowConstraint or OneVarConstraint" ) );
  }

  // the static Variables of the Block- - - - - - - - - - - - - - - - - - - -
  output << "Static Variables:" << endl;
  for( unsigned int i = 0 ; i < v_s_Variable.size() ; ++i ) {
   output << i;
   if( ! v_s_Variable_names[ i ].empty() )
    output << " (" << v_s_Variable_names[ i ] << "): ";
   else
    output << ": ";

   if( un_any_static( ci, []( ColVariable & var )
		            { output << var << endl; } ,
		      un_any_type< ColVariable >() ) )
    continue;
   throw( logic_error( "some static Variable not ColVariable" ) );
   }

  // the dynamic Constraints of the Block- - - - - - - - - - - - - - - - - -
  output << "Dynamic Constraints:" << endl;
  for( unsigned int i = 0 ; i < v_d_Constraint.size() ; ++i ) {
   output << i;
   if( ! v_d_Constraint_names[ i ].empty() )
    output << " (" << v_d_Constraint_names[ i ] << "): ";
   else
    output << ": ";

   if( un_any_dynamic( ci, []( FRowConstraint & cnst )
		             { output << cnst << endl; } ,
		      un_any_type< FRowConstraint >() ) )
    continue;
   if( un_any_dynamic( ci, []( OneVarConstraint & cnst )
		             { output << cnst << endl; } ,
		       un_any_type< OneVarConstraint >() ) )
    continue;
   throw( logic_error(
	"some dynamic Constraint not FRowConstraint or OneVarConstraint" ) );
   }

  // the dynamic Variables of the Block - - - - - - - - - - - - - - - - - - -
  output << "Dynamic Variables:" << endl;
  for( unsigned int i = 0 ; i < v_d_Variable.size() ; ++i ) {
   output << i;
   if( ! v_d_Variable_names[ i ].empty() )
    output << " (" << v_d_Variable_names[ i ] << "): ";
   else
    output << ": ";

   if( un_any_dynamic( ci, []( ColVariable & var )
		             { output << var << endl; } ,
		       un_any_type< ColVariable >() ) )
    continue;
   throw( logic_error( "some dynamic Variable not ColVariable" ) );
   }

  // the Objective of the Block - - - - - - - - - - - - - - - - - - - - - - -
  output << "Objective:" << *f_Objective << <endl;

  // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  output  << endl << "Nested Blocks:" << endl;
  for( p_Block blk : v_Block )
   output << *blk;
  }
 }  // end( Block::print )

/*--------------------------------------------------------------------------*/
/*-------------------- End File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
