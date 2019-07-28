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
/*---------------------- METHODS of AbstractBlock --------------------------*/
/*--------------------------------------------------------------------------*/

AbstractBlock::~AbstractBlock( )
{
 // first, clear() all Constraint
 for( auto & ci : get_static_constraints() ) {
  if( un_any_const_static( ci ,
			   []( FRowConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( BoxConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< BoxConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( LBConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< LBConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( UBConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< UBConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( NNConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< NNConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( NPConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< NPConstraint >() ) )
   continue;
  un_any_const_static( ci , []( ZOConstraint & cnst ) { cnst.clear(); } ,
		       un_any_type< ZOConstraint >() );
  }

 for( auto & ci : get_dynamic_constraints() ) {
  if( un_any_const_static( ci,
			   []( FRowConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( BoxConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< BoxConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( LBConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< LBConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( UBConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< UBConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( NNConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< NNConstraint >() ) )
   continue;
  if( un_any_const_static( ci ,
			   []( NPConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< NPConstraint >() ) )
   continue;
  un_any_const_static( ci , []( ZOConstraint & cnst ) { cnst.clear(); } ,
		       un_any_type< ZOConstraint >() );
  }

 // then clear the Objective
 if( get_objective() )
  get_objective()->clear();

 // now delete all the inner Block
 for( auto bi : v_Block )
  delete bi;

 v_Block.clear();

 // now delete all the Constraint
 for( auto & ci : get_static_constraints() ) {
  if( un_any_thing( FRowConstraint , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( BoxConstraint , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( LBConstraint , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( UBConstraint , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( NNConstraint , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( NPConstraint , ci , { delete & var; } ) )
   continue;
  un_any_thing( ZOConstraint , ci , { delete & var; } );
  }

 for( auto & ci : get_dynamic_constraints() ) {
  if( un_any_thing( std::list<FRowConstraint> , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<BoxConstraint> , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<LBConstraint> , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<UBConstraint> , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<NNConstraint> , ci , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<NPConstraint> , ci , { delete & var; } ) )
   continue;
  un_any_thing( std::list<ZOConstraint> , ci , { delete & var; } );
  }

 // now delete all the Variable
 for( auto & vi : get_static_variables() )
  un_any_thing( ColVariable , vi , { delete & var; } );

 for( auto & vi : get_dynamic_variables() )
  un_any_thing( std::list<ColVariable> , vi , { delete & var; } );

 }  // end( ~AbstractBlock )

/*--------------------------------------------------------------------------*/

bool AbstractBlock::is_feasible( bool useabstract , Configuration *fsbc )
{
 if( ! useabstract )
  return( false );

 // compute the accuracy parameter- - - - - - - - - - - - - - - - - - - - - -
 double eps = 0;
 auto tfsbc = dynamic_cast<SimpleConfiguration<double> *>( fsbc );

 if( ( ! tfsbc ) && f_BlockConfig &&
     f_BlockConfig->f_is_feasible_Configuration )
  tfsbc = dynamic_cast<SimpleConfiguration<double> *>(
			         f_BlockConfig->f_is_feasible_Configuration );
 if( tfsbc )
  eps = tfsbc->f_value;

 bool feas = true;

 // the static Constraints of the Block - - - - - - - - - - - - - - - - - - -
 for( auto & ci : get_static_constraints() ) {
  if( un_any_const_static( ci , [ & feas , eps ]( FRowConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( BoxConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< BoxConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( LBConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< LBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( UBConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< UBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( NNConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< NNConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( NPConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< NPConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( ZOConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< ZOConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( logic_error(
	"some static Constraint not FRowConstraint or :OneVarConstraint" ) );
  }

 // the static Variables of the Block - - - - - - - - - - - - - - - - - - - -
 for( auto & vi : get_static_variables() ) {
  if( un_any_const_static( vi , [ & feas , eps ]( ColVariable & var )
		                          { feas = var.is_feasible( eps ); } ,
		     un_any_type< ColVariable >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( logic_error( "some static Variable not ColVariable" ) );
  }

 // the dynamic Constraints of the Block-  - - - - - - - - - - - - - - - - - -
 for( auto & ci : get_dynamic_constraints() ) {
  if( un_any_const_dynamic( ci , [ & feas , eps ]( FRowConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( BoxConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< BoxConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( LBConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< LBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( UBConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< UBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( NNConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< NNConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( NPConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< NPConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( ci , [ & feas , eps ]( ZOConstraint & cnst )
		                    { feas = ( cnst.rel_viol() <= eps ); } ,
		     un_any_type< ZOConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( logic_error(
       "some dynamic Constraint not FRowConstraint or :OneVarConstraint" ) );
  }

 // the dynamic Variables of the Block- - - - - - - - - - - - - - - - - - - -
 for( auto & vi : get_dynamic_variables() ) {
  if( un_any_const_dynamic( vi , [ & feas, eps ]( ColVariable & var )
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
 output << endl << get_static_variables().size()
	        << " types of static Variables, "
                << get_dynamic_variables().size()
	        << " types of dynamic Variables, "
        << endl << get_static_constraints().size()
	        << " types of static Constraints, "
	        << get_dynamic_constraints().size()
	        << " types of dynamic Constraints, "
        << endl << v_Block.size() << " inner Blocks" << endl;

 if( verbosity_lvl == Block::medium || verbosity_lvl == Block::high ) {
  // the static Constraints of the Block- - - - - - - - - - - - - - - - - - -
  output << "Static Constraints:" << endl;
  auto & sc = get_static_constraints();
  for( unsigned int i = 0 ; i < sc.size() ; ++i ) {
   output << i;
   if( ( ! get_s_const_name().empty() ) &&
       ( ! get_s_const_name()[ i ].empty() ) )
    output << " (" << get_s_const_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_static( sc[ i ] , [ &output ]( FRowConstraint & cnst )
		                                 { output << cnst << endl; } ,
		      un_any_type< FRowConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ &output ]( BoxConstraint & cnst )
		                                 { output << cnst << endl; } ,
		     un_any_type< BoxConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ &output ]( LBConstraint & cnst )
		                                 { output << cnst << endl; } ,
		     un_any_type< LBConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ &output ]( UBConstraint & cnst )
		                                 { output << cnst << endl; } ,
		     un_any_type< UBConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ &output ]( NNConstraint & cnst )
		                                 { output << cnst << endl; } ,
		     un_any_type< NNConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ &output ]( NPConstraint & cnst )
		                                 { output << cnst << endl; } ,
		     un_any_type< NPConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ &output ]( ZOConstraint & cnst )
		                                 { output << cnst << endl; } ,
		     un_any_type< ZOConstraint >() ) )
    continue;
   throw( logic_error(
        "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
   }

  // the static Variables of the Block- - - - - - - - - - - - - - - - - - - -
  output << "Static Variables:" << endl;
  auto & sv = get_static_variables();
  for( unsigned int i = 0 ; i < sv.size() ; ++i ) {
   output << i;
   if( ( ! get_s_var_name().empty() ) &&
       ( ! get_s_var_name()[ i ].empty() ) )
    output << " (" << get_s_var_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_static( sv[ i ] , [ &output ]( ColVariable & var )
		                                 { output << var << endl; } ,
		      un_any_type< ColVariable >() ) )
    continue;
   throw( logic_error( "some static Variable not ColVariable" ) );
   }

  // the dynamic Constraints of the Block- - - - - - - - - - - - - - - - - -
  output << "Dynamic Constraints:" << endl;
  auto & dc = get_dynamic_constraints();
  for( unsigned int i = 0 ; i < dc.size() ; ++i ) {
   output << i;
   if( ( ! get_d_const_name().empty() ) &&
       ( ! get_d_const_name()[ i ].empty() ) )
    output << " (" << get_d_const_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_dynamic( dc[ i ] ,
			     [ &output ]( FRowConstraint & cnst )
		                        { output << cnst << endl; } ,
			     un_any_type< FRowConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
			     [ &output ]( BoxConstraint & cnst )
		                        { output << cnst << endl; } ,
			     un_any_type< BoxConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
			     [ &output ]( LBConstraint & cnst )
		                        { output << cnst << endl; } ,
			     un_any_type< LBConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
			     [ &output ]( UBConstraint & cnst )
			                { output << cnst << endl; } ,
			     un_any_type< UBConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
			     [ &output ]( NNConstraint & cnst )
			                { output << cnst << endl; } ,
			     un_any_type< NNConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
			     [ &output ]( NPConstraint & cnst )
			                { output << cnst << endl; } ,
			     un_any_type< NPConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
			     [ &output ]( ZOConstraint & cnst )
			                { output << cnst << endl; } ,
			     un_any_type< ZOConstraint >() ) )
    continue;
   throw( logic_error(
       "some dynamic Constraint not FRowConstraint or :OneVarConstraint" ) );
   }

  // the dynamic Variables of the Block - - - - - - - - - - - - - - - - - - -
  output << "Dynamic Variables:" << endl;
  auto & dv = get_dynamic_variables();
  for( unsigned int i = 0 ; i < dv.size() ; ++i ) {
   output << i;
   if( ( ! get_d_var_name().empty() ) &&
       ( ! get_d_var_name()[ i ].empty() ) )
    output << " (" << get_d_var_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_dynamic( dv[ i ] , [ &output ]( ColVariable & var )
		                                  { output << var << endl; } ,
		       un_any_type< ColVariable >() ) )
    continue;
   throw( logic_error( "some dynamic Variable not ColVariable" ) );
   }

  // the Objective of the Block - - - - - - - - - - - - - - - - - - - - - - -
  output << "Objective:" << *get_objective() << endl;

  // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  output  << endl << "Nested Blocks:" << endl;
  for( p_Block blk : v_Block )
   output << *blk;
  }
 }  // end( Block::print )

/*--------------------------------------------------------------------------*/
/*-------------------- End File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
