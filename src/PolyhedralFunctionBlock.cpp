/*--------------------------------------------------------------------------*/
/*------------------- File PolyhedralFunctionBlock.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the PolyhedralFunctionBlock class.
 *
 * \version 0.10
 *
 * \date 13 - 10 - 2019
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

#include "PolyhedralFunctionBlock.h"

#include "ColVariable.h"

#include "FRowConstraint.h"

#include "FRealObjective.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register PolyhedralFunctionBlock to the Block factory

SMSpp_insert_in_factory_cpp_1( PolyhedralFunctionBlock );

/*--------------------------------------------------------------------------*/
/*----------------- METHODS of PolyhedralFunctionBlock ---------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::generate_abstract_variables(
						        Configuration * stvv )
{
 int wsol = 0;
 auto tstvv = dynamic_cast<SimpleConfiguration<int> *>( stvv );

 if( ( ! tstvv ) && f_BlockConfig && f_BlockConfig->f_solution_Configuration )
  tstvv = dynamic_cast<SimpleConfiguration<int> *>(
			            f_BlockConfig->f_solution_Configuration );
 if( tstvv )
  wsol = tstvv->f_value;

 f_rep = ( f_value != 0 );
 
 }  // end( PolyhedralFunctionBlock::generate_abstract_variables )

/*--------------------------------------------------------------------------*/

PolyhedralFunctionBlock::~PolyhedralFunctionBlock( )
{
 // first, clear() all Constraint
 auto & sc = get_static_constraints();
 for( Index i = get_first_static_Constraint() ; i < sc.size() ; ++i ) {
  if( un_any_const_static( sc[ i ] ,
			   []( FRowConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( BoxConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< BoxConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( LBConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< LBConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( UBConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< UBConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( NNConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< NNConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( NPConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< NPConstraint >() ) )
   continue;
  un_any_const_static( sc[ i ] , []( ZOConstraint & cnst ) { cnst.clear(); } ,
		       un_any_type< ZOConstraint >() );
  }

 auto & dc = get_dynamic_constraints();
 for( Index i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
  if( un_any_const_dynamic( dc[ i ] ,
			    []( FRowConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( BoxConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< BoxConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( LBConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< LBConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( UBConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< UBConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( NNConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< NNConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( NPConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< NPConstraint >() ) )
   continue;
  un_any_const_dynamic( dc[ i ] , []( ZOConstraint & cnst ) { cnst.clear(); } ,
			un_any_type< ZOConstraint >() );
  }

 // then clear the Objective
 if( ( ! is_Objective_reserved() ) && get_objective() )
  get_objective()->clear();

 // now delete all the inner Block
 for( Index i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
  delete v_Block[ i ];

 v_Block.clear();

 // now delete all the Constraint
 for( Index i = get_first_static_Constraint() ; i < sc.size() ; ++i ) {
  if( un_any_thing( FRowConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( BoxConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( LBConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( UBConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( NNConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( NPConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  un_any_thing( ZOConstraint , sc[ i ] , { delete & var; } );
  }

 for( Index i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
  if( un_any_thing( std::list<FRowConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<BoxConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<LBConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<UBConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<NNConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<NPConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  un_any_thing( std::list<ZOConstraint> , dc[ i ] , { delete & var; } );
  }

 // now delete all the Variable
 auto & sv = get_static_variables();
 for( Index i = get_first_static_Variable() ; i < sv.size() ; ++i )
  un_any_thing( ColVariable , sv[ i ] , { delete & var; } );

 auto & dv = get_dynamic_variables();
 for( Index i = get_first_dynamic_Variable() ; i < dv.size() ; ++i )
  un_any_thing( std::list<ColVariable> , dv[ i ] , { delete & var; } );

 // now delete the Objective
 if( ! is_Objective_reserved() )
  delete get_objective();

 }  // end( ~PolyhedralFunctionBlock )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::is_feasible( bool useabstract , Configuration *fsbc )
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
 auto & sc = get_static_constraints();
 for( Index i = get_first_static_Constraint() ; i < sc.size() ; ++i ) {
  if( un_any_const_static( sc[ i ] , [ & feas , eps ]( FRowConstraint & cnst )
			             { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] , [ & feas , eps ]( BoxConstraint & cnst )
			             { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< BoxConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] , [ & feas , eps ]( LBConstraint & cnst )
			             { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< LBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] , [ & feas , eps ]( UBConstraint & cnst )
			             { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< UBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] , [ & feas , eps ]( NNConstraint & cnst )
			             { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< NNConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] , [ & feas , eps ]( NPConstraint & cnst )
			             { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< NPConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] , [ & feas , eps ]( ZOConstraint & cnst )
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
 auto & sv = get_static_variables();
 for( Index i = get_first_static_Variable() ; i < sv.size() ; ++i )
 {
  if( un_any_const_static( sv[ i ] , [ & feas , eps ]( ColVariable & var )
			             { feas = var.is_feasible( eps ); } ,
			   un_any_type< ColVariable >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( logic_error( "some static Variable not ColVariable" ) );
  }

 // the dynamic Constraints of the Block-  - - - - - - - - - - - - - - - - - -
 auto & dc = get_dynamic_constraints();
 for( Index i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
  if( un_any_const_dynamic( dc[ i ] , [ & feas , eps ]( FRowConstraint & cnst )
			              { feas = ( cnst.rel_viol() <= eps ); } ,
			    un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] , [ & feas , eps ]( BoxConstraint & cnst )
			              { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< BoxConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] , [ & feas , eps ]( LBConstraint & cnst )
			              { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< LBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ], [ & feas , eps ]( UBConstraint & cnst )
			             { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< UBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] , [ & feas , eps ]( NNConstraint & cnst )
			              { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< NNConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] , [ & feas , eps ]( NPConstraint & cnst )
			              { feas = ( cnst.rel_viol() <= eps ); } ,
			   un_any_type< NPConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] , [ & feas , eps ]( ZOConstraint & cnst )
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
 auto & dv = get_dynamic_variables();
 for( Index i = get_first_dynamic_Variable() ; i < dv.size() ; ++i )
 {
  if( un_any_const_dynamic( dv[ i ] , [ & feas, eps ]( ColVariable & var )
			              { feas = var.is_feasible( eps ); } ,
			    un_any_type< ColVariable >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( logic_error( "some dynamic Variable not ColVariable" ) );
  }

 // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( Index i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
  if( ! v_Block[ i ]->is_feasible( true ) )
   return( false );

 return( true );

 }  // end( PolyhedralFunctionBlock::is_feasible )
>>>>>>> dual

/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::serialize( netCDF::NcGroup & group ) const
{
 auto & sc = get_static_constraints();
 auto & sv = get_static_variables();
 auto & dc = get_dynamic_constraints();
 auto & dv = get_dynamic_variables();

 if( ( sc.size() > get_first_static_Constraint() ) ||
     ( dc.size() > get_first_dynamic_Constraint() ) ||
     ( sv.size() > get_first_static_Variable() ) ||
     ( dv.size() > get_first_dynamic_Variable() ) ||
     ( ! is_Objective_reserved() ) )
  throw( std::logic_error(
		   "AbstractBlock::serialize not fully implemented yet" ) );

 group.putAtt( "type" , name() );

 if( v_Block.size() > get_first_inner_Block() ) {
  group.addDim( "NumberInnerBlock" , v_Block.size() );

  for( Index i = get_first_inner_Block() ; i < v_Block.size() ; ++i ) {
   auto gi = group.addGroup( "Block_" + std::to_string( i ) );
   v_Block[ i ]->serialize( gi );
   }
  }
 }  // end( PolyhedralFunctionBlock::serialize )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::print( ostream &output ) const
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
  for( unsigned int i = get_first_static_Constraint() ; i < sc.size() ; ++i ) {
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
  for( unsigned int i = get_first_static_Variable() ; i < sv.size() ; ++i ) {
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
  for( unsigned int i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
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
  for( unsigned int i = get_first_dynamic_Variable() ; i < dv.size() ; ++i ) {
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
  if( ! is_Objective_reserved() )
   output << "Objective:" << *get_objective() << endl;

  // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  output  << endl << "Nested Blocks:" << endl;
  for( Index i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
   output << *v_Block[ i ];
  }
 }  // end( PolyhedralFunctionBlock::print )


/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE METHODS ------------------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::guts_of_destructor( void )
{
 // first, clear() all Constraint
 auto & sc = get_static_constraints();
 for( Index i = get_first_static_Constraint() ; i < sc.size() ; ++i ) {
  if( un_any_const_static( sc[ i ] ,
			   []( FRowConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( BoxConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< BoxConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( LBConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< LBConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( UBConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< UBConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( NNConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< NNConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ] ,
			   []( NPConstraint & cnst ) { cnst.clear(); } ,
			   un_any_type< NPConstraint >() ) )
   continue;
  un_any_const_static( sc[ i ] , []( ZOConstraint & cnst ) { cnst.clear(); } ,
		       un_any_type< ZOConstraint >() );
  }

 auto & dc = get_dynamic_constraints();
 for( Index i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
  if( un_any_const_dynamic( dc[ i ] ,
			    []( FRowConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( BoxConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< BoxConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( LBConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< LBConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( UBConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< UBConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( NNConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< NNConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
			    []( NPConstraint & cnst ) { cnst.clear(); } ,
			    un_any_type< NPConstraint >() ) )
   continue;
  un_any_const_dynamic( dc[ i ] , []( ZOConstraint & cnst ) { cnst.clear(); } ,
			un_any_type< ZOConstraint >() );
  }

 // then clear the Objective
 if( ( ! is_Objective_reserved() ) && get_objective() )
  get_objective()->clear();

 // now delete all the inner Block
 for( Index i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
  delete v_Block[ i ];

 v_Block.clear();

 // now delete all the Constraint
 for( Index i = get_first_static_Constraint() ; i < sc.size() ; ++i ) {
  if( un_any_thing( FRowConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( BoxConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( LBConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( UBConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( NNConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( NPConstraint , sc[ i ] , { delete & var; } ) )
   continue;
  un_any_thing( ZOConstraint , sc[ i ] , { delete & var; } );
  }

 for( Index i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
  if( un_any_thing( std::list<FRowConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<BoxConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<LBConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<UBConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<NNConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  if( un_any_thing( std::list<NPConstraint> , dc[ i ] , { delete & var; } ) )
   continue;
  un_any_thing( std::list<ZOConstraint> , dc[ i ] , { delete & var; } );
  }

 // now delete all the Variable
 auto & sv = get_static_variables();
 for( Index i = get_first_static_Variable() ; i < sv.size() ; ++i )
  un_any_thing( ColVariable , sv[ i ] , { delete & var; } );

 auto & dv = get_dynamic_variables();
 for( Index i = get_first_dynamic_Variable() ; i < dv.size() ; ++i )
  un_any_thing( std::list<ColVariable> , dv[ i ] , { delete & var; } );

 // now delete the Objective
 if( ! is_Objective_reserved() )
  delete get_objective();

 }  // end( PolyhedralFunctionBlock::guts_of_destructor )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::guts_of_add_Modification( sp_Mod mod )
{

 }  // end( PolyhedralFunctionBlock::guts_of_add_Modification )

/*--------------------------------------------------------------------------*/
/*--------------- End File PolyhedralFunctionBlock.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
