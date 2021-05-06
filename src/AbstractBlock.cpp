/*--------------------------------------------------------------------------*/
/*------------------------ File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the AbstractBlock class.
 *
 * \version 0.20
 *
 * \date 19 - 11 - 2020
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

#include <boost/algorithm/string.hpp> // Used in read()

#include "AbstractBlock.h"

#include "ColVariable.h"

#include "LinearFunction.h"
#include "FRowConstraint.h"
#include "OneVarConstraint.h"

#include "FRealObjective.h"

#include "ColVariableSolution.h"
#include "RowConstraintSolution.h"
#include "ColRowSolution.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register AbstractBlock to the Block factory

SMSpp_insert_in_factory_cpp_1( AbstractBlock );

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS of AbstractBlock --------------------------*/
/*--------------------------------------------------------------------------*/

AbstractBlock::~AbstractBlock()
{
 // first, clear() all Constraint
 auto & sc = get_static_constraints();
 for( Index i = get_first_static_Constraint(); i < sc.size(); ++i ) {
  if( un_any_const_static( sc[ i ],
                           []( FRowConstraint & cnst ) { cnst.clear(); },
                           un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ],
                           []( BoxConstraint & cnst ) { cnst.clear(); },
                           un_any_type< BoxConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ],
                           []( LB0Constraint & cnst ) { cnst.clear(); },
                           un_any_type< LB0Constraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ],
                           []( UB0Constraint & cnst ) { cnst.clear(); },
                           un_any_type< UB0Constraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ],
                           []( LBConstraint & cnst ) { cnst.clear(); },
                           un_any_type< LBConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ],
                           []( UBConstraint & cnst ) { cnst.clear(); },
                           un_any_type< UBConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ],
                           []( NNConstraint & cnst ) { cnst.clear(); },
                           un_any_type< NNConstraint >() ) )
   continue;
  if( un_any_const_static( sc[ i ],
                           []( NPConstraint & cnst ) { cnst.clear(); },
                           un_any_type< NPConstraint >() ) )
   continue;
  un_any_const_static( sc[ i ], []( ZOConstraint & cnst ) { cnst.clear(); },
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
                            []( LB0Constraint & cnst ) { cnst.clear(); } ,
                            un_any_type< LB0Constraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ] ,
                            []( UB0Constraint & cnst ) { cnst.clear(); } ,
                            un_any_type< UB0Constraint >() ) )
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
  un_any_const_dynamic( dc[ i ] ,
			[]( ZOConstraint & cnst ) { cnst.clear(); } ,
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
  if( un_any_thing( FRowConstraint , sc[ i ] , { delete &var; } ) )
   continue;
  if( un_any_thing( BoxConstraint , sc[ i ] , { delete &var; } ) )
   continue;
  if( un_any_thing( LB0Constraint , sc[ i ] , { delete &var; } ) )
   continue;
  if( un_any_thing( UB0Constraint , sc[ i ] , { delete &var; } ) )
   continue;
  if( un_any_thing( LBConstraint , sc[ i ] , { delete &var; } ) )
   continue;
  if( un_any_thing( UBConstraint , sc[ i ] , { delete &var; } ) )
   continue;
  if( un_any_thing( NNConstraint , sc[ i ] , { delete &var; } ) )
   continue;
  if( un_any_thing( NPConstraint , sc[ i ] , { delete &var; } ) )
   continue;
  un_any_thing( ZOConstraint , sc[ i ] , { delete &var; } );
 }

 for( Index i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
  if( un_any_thing( std::list< FRowConstraint > , dc[ i ] ,
		    { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< BoxConstraint > , dc[ i ] ,
		    { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< LB0Constraint > , dc[ i ] ,
		    { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< UB0Constraint > , dc[ i ] ,
		    { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< LBConstraint > , dc[ i ] ,
		    { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< UBConstraint > , dc[ i ] ,
		    { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< NNConstraint > , dc[ i ] ,
		    { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< NPConstraint > , dc[ i ] ,
		    { delete &var; } ) )
   continue;
  un_any_thing( std::list< ZOConstraint > , dc[ i ] , { delete &var; } );
  }

 // now delete all the Variable
 auto & sv = get_static_variables();
 for( Index i = get_first_static_Variable() ; i < sv.size() ; ++i )
  un_any_thing( ColVariable , sv[ i ] , { delete &var; } );

 auto & dv = get_dynamic_variables();
 for( Index i = get_first_dynamic_Variable() ; i < dv.size() ; ++i )
  un_any_thing( std::list< ColVariable > , dv[ i ] , { delete &var; } );

 // now delete the Objective
 if( ( ! is_Objective_reserved() ) && get_objective() )
  delete get_objective();

 }  // end( ~AbstractBlock )

/*--------------------------------------------------------------------------*/

bool AbstractBlock::is_feasible( bool useabstract, Configuration * fsbc )
{
 if( ! useabstract )
  return( false );

 // compute the accuracy parameter- - - - - - - - - - - - - - - - - - - - - -
 double eps = 0;
 auto tfsbc = dynamic_cast<SimpleConfiguration< double > *>( fsbc );

 if( ( ! tfsbc ) && f_BlockConfig &&
     f_BlockConfig->f_is_feasible_Configuration )
  tfsbc = dynamic_cast<SimpleConfiguration< double > *>(
   f_BlockConfig->f_is_feasible_Configuration );
 if( tfsbc )
  eps = tfsbc->f_value;

 bool feas = true;

 // the static Constraints of the Block - - - - - - - - - - - - - - - - - - -
 auto & sc = get_static_constraints();
 for( Index i = get_first_static_Constraint() ; i < sc.size() ; ++i ) {
  if( un_any_const_static( sc[ i ] ,
			   [ & feas , eps ]( FRowConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                            } ,
                           un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] ,
			   [ & feas , eps ]( BoxConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                            } ,
                           un_any_type< BoxConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] ,
			   [ & feas , eps ]( LB0Constraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                            } ,
                           un_any_type< LB0Constraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] ,
			   [ & feas , eps ]( UB0Constraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                            } ,
                           un_any_type< UB0Constraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] ,
			   [ & feas , eps ]( LBConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                            } ,
                           un_any_type< LBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] ,
			   [ & feas , eps ]( UBConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                            } ,
                           un_any_type< UBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ] ,
			   [ & feas , eps ]( NNConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                            } ,
                           un_any_type< NNConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sc[ i ], [ & feas, eps ]( NPConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                           },
                           un_any_type< NPConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_static( sc[ i ] ,
			   [ & feas , eps ]( ZOConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                            } ,
                           un_any_type< ZOConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( std::logic_error(
       "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
  }

 // the static Variables of the Block - - - - - - - - - - - - - - - - - - - -
 auto & sv = get_static_variables();
 for( Index i = get_first_static_Variable() ; i < sv.size() ; ++i ) {
  if( un_any_const_static( sv[ i ] ,
			   [ & feas , eps ]( ColVariable & var ) {
                            feas = var.is_feasible( eps );
                            } ,
                           un_any_type< ColVariable >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( std::logic_error( "some static Variable not ColVariable" ) );
  }

 // the dynamic Constraints of the Block-  - - - - - - - - - - - - - - - - - -
 auto & dc = get_dynamic_constraints();
 for( Index i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
  if( un_any_const_dynamic( dc[ i ] ,
                            [ & feas, eps ]( FRowConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                             } ,
                            un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] ,
			    [ & feas , eps ]( BoxConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                             } ,
                            un_any_type< BoxConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] ,
			    [ & feas , eps ]( LB0Constraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                             } ,
                            un_any_type< LB0Constraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] ,
			    [ & feas , eps ]( UB0Constraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                             } ,
                            un_any_type< UB0Constraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] ,
			    [ & feas , eps ]( LBConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                             } ,
                            un_any_type< LBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] ,
			    [ & feas , eps ]( UBConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                             } ,
                            un_any_type< UBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] ,
			    [ & feas , eps ]( NNConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                             },
                            un_any_type< NNConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] ,
			    [ & feas , eps ]( NPConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                             } ,
                            un_any_type< NPConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dc[ i ] ,
			    [ & feas, eps ]( ZOConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                             } ,
                            un_any_type< ZOConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( std::logic_error(
   "some dynamic Constraint not FRowConstraint or :OneVarConstraint" ) );
 }

 // the dynamic Variables of the Block- - - - - - - - - - - - - - - - - - - -
 auto & dv = get_dynamic_variables();
 for( Index i = get_first_dynamic_Variable() ; i < dv.size() ; ++i ) {
  if( un_any_const_dynamic( dv[ i ] ,
			    [ & feas , eps ]( ColVariable & var ) {
                             feas = var.is_feasible( eps );
                             } ,
                            un_any_type< ColVariable >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( std::logic_error( "some dynamic Variable not ColVariable" ) );
  }

 // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( Index i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
  if( ! v_Block[ i ]->is_feasible( true ) )
   return( false );

 return( true );

 }  // end( AbstractBlock::is_feasible )

/*--------------------------------------------------------------------------*/

void AbstractBlock::check_Variable( Variable * var )
{
 if( var->get_Block() != this )
  std::cout << std::endl << "Variable " << var
            << " not of the right Block";

 for( Index as = 0 ; as < var->get_num_active() ; ++as ) {
  auto tvdi = var->get_active( as );
  if( tvdi->is_active( var ) >= tvdi->get_num_active_var() )
   std::cout << std::endl << "Variable " << var
             << " not active in its " << as << "-th active stuff";
  }
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::check_Constraint( Constraint * cnst )
{
 if( cnst->get_Block() != this )
  std::cout << std::endl << "Constraint " << cnst
	    << " not of the right Block";

 for( Index av = 0 ; av < cnst->get_num_active_var() ; ++av ) {
  auto var = cnst->get_active_var( av );
  if( var->is_active( cnst ) >= var->get_num_active() )
   std::cout << std::endl << "Constraint " << cnst
             << " not active in its " << av << "-th active Variable";
  }
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::check_Objective( Objective * obj )
{
 if( obj->get_Block() != this )
  std::cout << std::endl << "Objective " << obj << " not of the right Block";

 for( Index av = 0 ; av < obj->get_num_active_var() ; ++av ) {
  auto var = obj->get_active_var( av );
  if( var->is_active( obj ) >= var->get_num_active() )
   std::cout << std::endl << "Objective " << obj
             << " not active in its " << av << "-th active Variable";
  }
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::is_correct( void )
{
 // the static Variables of the Block - - - - - - - - - - - - - - - - - - - -
 auto & sv = get_static_variables();
 for( Index i = 0 ; i < sv.size() ; ++i ) {
  if( un_any_const_static( sv[ i ] ,
                           [ this ]( ColVariable & var ) {
                            check_Variable( &var );
                            } , un_any_type< ColVariable >() ) ) {
   continue;
   }
  throw( std::logic_error( "some static Variable not ColVariable" ) );
  }

 // the dynamic Variables of the Block- - - - - - - - - - - - - - - - - - - -
 auto & dv = get_dynamic_variables();
 for( Index i = 0 ; i < dv.size() ; ++i ) {
  if( un_any_const_dynamic( dv[ i ] ,
                            [ this ]( ColVariable & var ) {
                             check_Variable( &var );
                             } , un_any_type< ColVariable >() ) ) {

   continue;
   }
  throw( std::logic_error( "some dynamic Variable not ColVariable" ) );
  }

 // the static Constraints of the Block - - - - - - - - - - - - - - - - - - -
 auto & sc = get_static_constraints();
 for( Index i = 0 ; i < sc.size() ; ++i ) {
  if( un_any_const_static( sc[ i ] ,
                           [ this ]( FRowConstraint & cnst ) {
                            check_Constraint( &cnst );
                            } , un_any_type< FRowConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ] ,
                           [ this ]( BoxConstraint & cnst ) {
                            check_Constraint( &cnst );
                            } , un_any_type< BoxConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ] ,
                           [ this ]( LB0Constraint & cnst ) {
                            check_Constraint( &cnst );
                            } , un_any_type< LB0Constraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ] ,
                           [ this ]( UB0Constraint & cnst ) {
                            check_Constraint( &cnst );
                            } , un_any_type< UB0Constraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ] ,
                           [ this ]( LBConstraint & cnst ) {
                            check_Constraint( &cnst );
                            } , un_any_type< LBConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ] ,
                           [ this ]( UBConstraint & cnst ) {
                            check_Constraint( &cnst );
                            } , un_any_type< UBConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ] ,
                           [ this ]( NNConstraint & cnst ) {
                            check_Constraint( &cnst );
                            } , un_any_type< NNConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ] ,
                           [ this ]( NPConstraint & cnst ) {
                            check_Constraint( &cnst );
                            } , un_any_type< NPConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ] ,
                           [ this ]( ZOConstraint & cnst ) {
                            check_Constraint( &cnst );
                            } , un_any_type< ZOConstraint >() ) )
   continue;

  throw( std::logic_error(
   "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
 }

 // the dynamic Constraints of the Block- - - - - - - - - - - - - - - - - - -
 auto & dc = get_dynamic_constraints();
 for( Index i = 0 ; i < dc.size() ; ++i ) {
  if( un_any_const_dynamic( dc[ i ] ,
                            [ this ]( FRowConstraint & cnst ) {
                             check_Constraint( &cnst );
                             } , un_any_type< FRowConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ] ,
                            [ this ]( BoxConstraint & cnst ) {
                             check_Constraint( &cnst );
                             } , un_any_type< BoxConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ] ,
                            [ this ]( LB0Constraint & cnst ) {
                             check_Constraint( &cnst );
                             } , un_any_type< LB0Constraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ] ,
                            [ this ]( UB0Constraint & cnst ) {
                             check_Constraint( &cnst );
                             } , un_any_type< UB0Constraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ] ,
                            [ this ]( LBConstraint & cnst ) {
                             check_Constraint( &cnst );
                             } , un_any_type< LBConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ] ,
                            [ this ]( UBConstraint & cnst ) {
                             check_Constraint( &cnst );
                             } , un_any_type< UBConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ] ,
                            [ this ]( NNConstraint & cnst ) {
                             check_Constraint( &cnst );
                             } , un_any_type< NNConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ] ,
                            [ this ]( NPConstraint & cnst ) {
                             check_Constraint( &cnst );
                             } , un_any_type< NPConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ] ,
                            [ this ]( ZOConstraint & cnst ) {
                             check_Constraint( &cnst );
                             } , un_any_type< ZOConstraint >() ) )
   continue;

  throw( std::logic_error(
   "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
  }

 // the Objective of the Block- - - - - - - - - - - - - - - - - - - - - - - -
 if( auto obj = get_objective() )
  check_Objective( obj );

 // check every sub-Block of AbstractBlock - - - - - - - - - - - - - - - - -

 for( Index i = 0 ; i < get_number_nested_Blocks() ; ++i ) {
  auto sb = get_nested_Block( i );
  if( sb->get_f_Block() != this )
   std::cout << std::endl << "sub-Block " << i << " has wrong father";
  if( auto asb = dynamic_cast< AbstractBlock * >( sb ) )
   asb->is_correct();
  }
 }  // end( AbstractBlock::is_correct )

/*--------------------------------------------------------------------------*/

Solution * AbstractBlock::get_Solution( Configuration * csolc, bool emptys )
{

 auto config = dynamic_cast<SimpleConfiguration< int > *>( csolc );

 if( ( ! config ) && f_BlockConfig )
  config = dynamic_cast<SimpleConfiguration< int > *>(
   f_BlockConfig->f_solution_Configuration );

 auto solution_type = config ? config->f_value : 0;

 Solution * sol = nullptr;
 switch( solution_type ) {
  case 1:
   sol = new RowConstraintSolution;
   break;
  case 2:
   sol = new ColRowSolution;
   break;
  default:
   sol = new ColVariableSolution;
  }

 if( ! emptys )
  sol->read( this );
 
 return( sol );
 }

/*--------------------------------------------------------------------------*/

void AbstractBlock::serialize( netCDF::NcGroup & group ) const
{
 // call the method of Block- - - - - - - - - - - - - - - - - - - - - - - - -

 Block::serialize( group );

 // now the AbstractBlock data- - - - - - - - - - - - - - - - - - - - - - - -

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

 group.putAtt( "type", name() );

 if( v_Block.size() > get_first_inner_Block() ) {
  group.addDim( "NumberInnerBlock", v_Block.size() );

  for( auto i = get_first_inner_Block() ; i < v_Block.size() ; ++i ) {
   auto gi = group.addGroup( "Block_" + std::to_string( i ) );
   v_Block[ i ]->serialize( gi );
  }
 }
}  // end( AbstractBlock::serialize )

/*--------------------------------------------------------------------------*/

void AbstractBlock::print( std::ostream & output ) const
{
 output << std::endl << "AbstractBlock with: ";
 output << std::endl << get_static_variables().size()
        << " types of static Variables, "
        << get_dynamic_variables().size()
        << " types of dynamic Variables, "
        << std::endl << get_static_constraints().size()
        << " types of static Constraints, "
        << get_dynamic_constraints().size()
        << " types of dynamic Constraints, "
        << std::endl << v_Block.size() << " inner Blocks" << std::endl;

 if( verbosity_lvl == Block::medium || verbosity_lvl == Block::high ) {
  // the static Constraints of the Block- - - - - - - - - - - - - - - - - - -
  output << "Static Constraints:" << std::endl;
  auto & sc = get_static_constraints();
  for( auto i = get_first_static_Constraint() ; i < sc.size() ; ++i ) {
   output << i;
   if( ( ! get_s_const_name().empty() ) &&
       ( ! get_s_const_name()[ i ].empty() ) )
    output << " (" << get_s_const_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_static( sc[ i ] , [ & output ]( FRowConstraint & cnst ) {
                             output << cnst << std::endl;
                             } , un_any_type< FRowConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ & output ]( BoxConstraint & cnst ) {
                             output << cnst << std::endl;
                             } , un_any_type< BoxConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ & output ]( LB0Constraint & cnst ) {
                             output << cnst << std::endl;
                             } , un_any_type< LB0Constraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ & output ]( UB0Constraint & cnst ) {
                             output << cnst << std::endl;
                             } , un_any_type< UB0Constraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ & output ]( LBConstraint & cnst ) {
                             output << cnst << std::endl;
                             } , un_any_type< LBConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ & output ]( UBConstraint & cnst ) {
                             output << cnst << std::endl;
                             } , un_any_type< UBConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ & output ]( NNConstraint & cnst ) {
                             output << cnst << std::endl;
                             } , un_any_type< NNConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ & output ]( NPConstraint & cnst ) {
                             output << cnst << std::endl;
                             } , un_any_type< NPConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ] , [ & output ]( ZOConstraint & cnst ) {
                             output << cnst << std::endl;
                             } , un_any_type< ZOConstraint >() ) )
    continue;
   throw( std::logic_error(
    "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
   }

  // the static Variables of the Block- - - - - - - - - - - - - - - - - - - -
  output << "Static Variables:" << std::endl;
  auto & sv = get_static_variables();
  for( auto i = get_first_static_Variable() ; i < sv.size() ; ++i ) {
   output << i;
   if( ( ! get_s_var_name().empty() ) &&
       ( ! get_s_var_name()[ i ].empty() ) )
    output << " (" << get_s_var_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_static( sv[ i ] , [ & output ]( ColVariable & var ) {
                             output << var << std::endl;
                             } , un_any_type< ColVariable >() ) )
    continue;
   throw( std::logic_error( "some static Variable not ColVariable" ) );
   }

  // the dynamic Constraints of the Block- - - - - - - - - - - - - - - - - -
  output << "Dynamic Constraints:" << std::endl;
  auto & dc = get_dynamic_constraints();
  for( auto i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
   output << i;
   if( ( ! get_d_const_name().empty() ) &&
       ( ! get_d_const_name()[ i ].empty() ) )
    output << " (" << get_d_const_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_dynamic( dc[ i ] ,
                             [ & output ]( FRowConstraint & cnst ) {
                              output << cnst << std::endl;
                              } , un_any_type< FRowConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
                             [ & output ]( BoxConstraint & cnst ) {
                              output << cnst << std::endl;
                              } , un_any_type< BoxConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
                             [ & output ]( LB0Constraint & cnst ) {
                              output << cnst << std::endl;
                              } , un_any_type< LB0Constraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
                             [ & output ]( UB0Constraint & cnst ) {
                              output << cnst << std::endl;
                              } , un_any_type< UB0Constraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
                             [ & output ]( LBConstraint & cnst ) {
                              output << cnst << std::endl;
                              } , un_any_type< LBConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
                             [ & output ]( UBConstraint & cnst ) {
                              output << cnst << std::endl;
                              } , un_any_type< UBConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
                             [ & output ]( NNConstraint & cnst ) {
                              output << cnst << std::endl;
                              } , un_any_type< NNConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
                             [ & output ]( NPConstraint & cnst ) {
                              output << cnst << std::endl;
                              } , un_any_type< NPConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ] ,
                             [ & output ]( ZOConstraint & cnst ) {
                              output << cnst << std::endl;
                              } , un_any_type< ZOConstraint >() ) )
    continue;
   throw( std::logic_error(
    "some dynamic Constraint not FRowConstraint or :OneVarConstraint" ) );
   }

  // the dynamic Variables of the Block - - - - - - - - - - - - - - - - - - -
  output << "Dynamic Variables:" << std::endl;
  auto & dv = get_dynamic_variables();
  for( auto i = get_first_dynamic_Variable() ; i < dv.size() ; ++i ) {
   output << i;
   if( ( ! get_d_var_name().empty() ) &&
       ( ! get_d_var_name()[ i ].empty() ) )
    output << " (" << get_d_var_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_dynamic( dv[ i ] , [ & output ]( ColVariable & var ) {
                              output << var << std::endl;
                              } , un_any_type< ColVariable >() ) )
    continue;
   throw( std::logic_error( "some dynamic Variable not ColVariable" ) );
   }

  // the Objective of the Block - - - - - - - - - - - - - - - - - - - - - - -
  if( !is_Objective_reserved() )
   output << "Objective:" << *get_objective() << std::endl;

  // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  output << std::endl << "Nested Blocks:" << std::endl;
  for( auto i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
   output << *v_Block[ i ];
  }
 }  // end( AbstractBlock::print )

/*--------------------------------------------------------------------------*/

void AbstractBlock::read( const std::string & filename,
                          const std::string & ext ) {

 // Check for explicit LP
 if( boost::iequals( ext, "lp" ) ) {
  read_lp( filename );
  return;
 }

 // Check for explicit MPS
 if( boost::iequals( ext, "mps" ) ) {
  read_mps( filename );
  return;
 }

 // Infer from file extension
 if( ext.empty() ) {
  std::size_t pos = filename.find_last_of( '.' );
  if( pos != std::string::npos ) {
   auto e = filename.substr( pos + 1 );
   if( boost::iequals( e, "lp" ) ) {
    read_lp( filename );
    return;
   }
   if( boost::iequals( e, "mps" ) ) {
    read_mps( filename );
    return;
   }
  }
  throw std::invalid_argument( "Cannot infer file type from extension" );
 }

 throw std::invalid_argument( "Specify a valid file type" );
}

/*--------------------------------------------------------------------------*/

void AbstractBlock::read_mps( const std::string & filename ) {
 std::ifstream file( filename );
 if( !file.is_open() ) {
  throw std::invalid_argument( "Cannot open file" );
 }

 int num_rows = 0;
 int num_cols = 0;

 auto * of = new FRealObjective();
 std::string of_name;

 std::vector< FRowConstraint > * rows;
 std::vector< std::string > row_names;
 std::vector< char > row_type;

 std::vector< ColVariable > * cols;
 std::vector< std::string > col_names;
 std::vector< BoxConstraint > * bounds;

 std::string rhs_name; // Only one RHS vector is supported
 std::string rng_name; // Only one RANGES vector is supported
 std::string bnd_name; // Only one BOUNDS vector is supported
 std::vector< double > rhs;
 std::vector< double > rng;

 // Read NAME
 std::string line;
 getline( file, line );
 if( line.substr( 0, 4 ) == "NAME" ) {
  // TODO
 } else {
  throw std::invalid_argument( "Invalid syntax in MPS file" );
 }
 getline( file, line );

 /*
  * First pass: get rows and columns number
  */

 // Read ROWS
 if( line.substr( 0 ) == "ROWS" ) {
  while( getline( file, line ) && line.at( 0 ) == ' ' ) {
   auto type = line.at( 1 );
   switch( type ) {
    case 'E':
    case 'L':
    case 'G':
     ++num_rows;
     break;
    case 'N':
     break;
    default:
     throw std::invalid_argument( "Invalid syntax in MPS file" );
   }
  }
 } else {
  throw std::invalid_argument( "Invalid syntax in MPS file" );
 }

 // Read COLUMNS
 if( line.substr( 0 ) == "COLUMNS" ) {
  std::string name;
  while( getline( file, line ) && line.substr( 0, 4 ) == "    " ) {
   auto tmp = boost::algorithm::trim_right_copy( line.substr( 4, 10 ) );
   if( tmp != name ) {
    name = tmp;
    ++num_cols;
   }
  }
 } else {
  throw std::invalid_argument( "Invalid syntax in MPS file" );
 }

 row_names.resize( num_rows );
 row_type.resize( num_rows );
 rows = new std::vector< FRowConstraint >( num_rows );

 for( auto & r: *rows ) {
  r.set_function( new LinearFunction(), eNoMod );
  r.set_Block( this );
 }
 of->set_function( new LinearFunction(), eNoMod );

 cols = new std::vector< ColVariable >( num_cols );
 col_names.resize( num_cols );
 bounds = new std::vector< BoxConstraint >( num_cols );

 for( int i = 0; i < num_cols; ++i ) {
  ( *bounds )[ i ].set_variable( &( *cols )[ i ], eNoMod );
  ( *bounds )[ i ].set_Block( this );
  ( *cols )[ i ].set_Block( this );
 }

 rhs.resize( num_rows, Inf< double >() );
 rng.resize( num_rows, Inf< double >() );

 /*
 * Second pass: fill data
 */

 file.seekg( 0, file.beg );
 getline( file, line ); // NAME
 getline( file, line ); // ROWS

 // Read ROWS
 int i = 0;
 while( getline( file, line ) && line.at( 0 ) == ' ' ) {
  auto type = line.at( 1 );
  std::string row_name = boost::algorithm::trim_right_copy( line.substr( 4 ) );
  switch( type ) {
   case 'E':
   case 'L':
   case 'G':
    row_names[ i ] = row_name;
    row_type[ i ] = type;
    ++i;
    break;
   case 'N':
    if( of_name.empty() ) {
     of_name = row_name;
    }
    // FIXME: Other N rows are ignored
    break;
   default:;
  }
 }

 // Read COLUMNS
 i = 0;
 std::string x_name;
 ColVariable * v;
 LinearFunction * f;
 while( getline( file, line ) && line.substr( 0, 4 ) == "    " ) {

  // Column name
  auto tmp = boost::algorithm::trim_right_copy( line.substr( 4, 10 ) );
  if( tmp != x_name ) {
   x_name = tmp;
   col_names[ i ] = x_name;
   v = &( *cols )[ i ];
   ++i;
  }

  std::string row;
  double value;

  // First name/value pair
  row = boost::algorithm::trim_right_copy( line.substr( 14, 8 ) );
  value = std::stod( boost::algorithm::trim_copy( line.substr( 24, 12 ) ) );

  if( row == of_name ) {
   f = static_cast<LinearFunction *>(of->get_function());
  } else {
   auto it = std::find( row_names.begin(), row_names.end(), row );
   if( it != row_names.end() ) {
    auto j = std::distance( row_names.begin(), it );
    f = static_cast<LinearFunction *>(( *rows )[ j ].get_function());
   } else {
    throw std::invalid_argument( "Invalid syntax in MPS file" );
   }
  }
  f->add_variable( v, value, eNoMod );

  // Optional second name/value pair
  if( line.size() > 36 ) {
   row = boost::algorithm::trim_right_copy( line.substr( 39, 8 ) );
   value = std::stod( boost::algorithm::trim_copy( line.substr( 49, 12 ) ) );

   if( row == of_name ) {
    // Will add to OF
    f = static_cast<LinearFunction *>(of->get_function());
   } else {
    // Will add to a constraint
    auto it = std::find( row_names.begin(), row_names.end(), row );
    if( it != row_names.end() ) {
     auto j = std::distance( row_names.begin(), it );
     f = static_cast<LinearFunction *>(( *rows )[ j ].get_function());
    } else {
     throw std::invalid_argument( "Invalid syntax in MPS file" );
    }
   }
   f->add_variable( v, value, eNoMod );
  }
 }

 /*
  * Continue with RHS and RANGES
  */

 // Read RHS
 if( line.substr( 0 ) == "RHS" ) {
  while( getline( file, line ) && line.substr( 0, 4 ) == "    " ) {

   // RHS name
   auto tmp = boost::algorithm::trim_right_copy( line.substr( 4, 10 ) );
   if( rhs_name.empty() ) {
    rhs_name = tmp;
   } else if( tmp != rhs_name ) {
    throw std::invalid_argument( "Only one RHS vector is supported" );
   }

   std::string row;
   double value;

   // First name/value pair
   row = boost::algorithm::trim_right_copy( line.substr( 14, 8 ) );
   value = std::stod( boost::algorithm::trim_copy( line.substr( 24, 12 ) ) );

   auto it = std::find( row_names.begin(), row_names.end(), row );
   if( it != row_names.end() ) {
    auto j = std::distance( row_names.begin(), it );
    rhs[ j ] = value;
   } else {
    throw std::invalid_argument( "Invalid syntax in MPS file" );
   }

   // Optional second name/value pair
   if( line.size() > 36 ) {
    row = boost::algorithm::trim_right_copy( line.substr( 39, 8 ) );
    value = std::stod( boost::algorithm::trim_copy( line.substr( 49, 12 ) ) );

    it = std::find( row_names.begin(), row_names.end(), row );
    if( it != row_names.end() ) {
     auto j = std::distance( row_names.begin(), it );
     rhs[ j ] = value;
    } else {
     throw std::invalid_argument( "Invalid syntax in MPS file" );
    }
   }
  }
 } else {
  throw std::invalid_argument( "Invalid syntax in MPS file" );
 }

 // Read RANGES
 if( line.substr( 0 ) == "RANGES" ) {
  while( getline( file, line ) && line.substr( 0, 4 ) == "    " ) {

   // RANGES name
   auto tmp = boost::algorithm::trim_right_copy( line.substr( 4, 10 ) );
   if( rng_name.empty() ) {
    rng_name = tmp;
   } else if( tmp != rng_name ) {
    throw std::invalid_argument( "Only one RANGES vector is supported" );
   }

   std::string row;
   double value;

   // First name/value pair
   row = boost::algorithm::trim_right_copy( line.substr( 14, 8 ) );
   value = std::stod( boost::algorithm::trim_copy( line.substr( 24, 12 ) ) );

   auto it = std::find( row_names.begin(), row_names.end(), row );
   if( it != row_names.end() ) {
    auto j = std::distance( row_names.begin(), it );
    rng[ j ] = value;
   } else {
    throw std::invalid_argument( "Invalid syntax in MPS file" );
   }

   // Optional second name/value pair
   if( line.size() > 36 ) {
    row = boost::algorithm::trim_right_copy( line.substr( 39, 8 ) );
    value = std::stod( boost::algorithm::trim_copy( line.substr( 49, 12 ) ) );

    it = std::find( row_names.begin(), row_names.end(), row );
    if( it != row_names.end() ) {
     auto j = std::distance( row_names.begin(), it );
     rng[ j ] = value;
    } else {
     throw std::invalid_argument( "Invalid syntax in MPS file" );
    }
   }
  }
 }

 // Process RHS and ranges
 for( int r = 0; r < num_rows; ++r ) {
  auto & row = ( *rows )[ r ];

  switch( row_type[ r ] ) {
   case 'G' :
    // G: rhs =< f() =< rhs + |rng|
    row.set_lhs( rhs[ r ], eNoMod );
    if( rng[ r ] == Inf< double >() ) {
     row.set_rhs( Inf< double >(), eNoMod );
    } else {
     row.set_rhs( rhs[ r ] + std::abs( rng[ r ] ), eNoMod );
    }
    break;

   case 'L' :
    // L: rhs - |rng| =< f() =< rhs
    row.set_rhs( rhs[ r ], eNoMod );
    if( rng[ r ] == Inf< double >() ) {
     row.set_lhs( -Inf< double >(), eNoMod );
    } else {
     row.set_lhs( rhs[ r ] - std::abs( rng[ r ] ), eNoMod );
    }
    break;

   case 'E' :
    if( rng[ r ] == Inf< double >() || rng[ r ] == 0 ) {
     // E (no range): rhs =< f() =< rhs
     row.set_both( rhs[ r ], eNoMod );
    } else if( rng[ r ] > 0 ) {
     // E+: rhs + rng =< f() =< rhs
     row.set_lhs( rhs[ r ] + rng[ r ], eNoMod );
     row.set_rhs( rhs[ r ], eNoMod );
    } else {
     // E-: rhs =< f() =< rhs + rng
     row.set_lhs( rhs[ r ], eNoMod );
     row.set_rhs( rhs[ r ] + rng[ r ], eNoMod );
    }
    break;
   default:;
  }
 }

 /*
  * Continue with BOUNDS
  */
 if( line.substr( 0 ) == "BOUNDS" ) {

  while( getline( file, line ) && line.at( 0 ) == ' ' ) {

   // BOUNDS name
   auto tmp = boost::algorithm::trim_right_copy( line.substr( 4, 10 ) );
   if( bnd_name.empty() ) {
    bnd_name = tmp;
   } else if( tmp != bnd_name ) {
    throw std::invalid_argument( "Only one BOUNDS vector is supported" );
   }

   auto type = line.substr( 1, 2 );
   auto col = boost::algorithm::trim_right_copy( line.substr( 14, 8 ) );
   auto value =
    std::stod( boost::algorithm::trim_copy( line.substr( 24, 12 ) ) );

   auto it = std::find( col_names.begin(), col_names.end(), col );
   if( it != col_names.end() ) {
    auto j = std::distance( col_names.begin(), it );
    auto & b = ( *bounds )[ j ];
    auto & c = ( *cols )[ j ];
    if( type == "LO" ) {        // Lower bound
     b.set_lhs( value, eNoMod );
    } else if( type == "UP" ) { // Upper bound
     b.set_rhs( value, eNoMod );
    } else if( type == "FX" ) { // Fixed variable
     b.set_both( value, eNoMod );
     c.is_fixed( true, eNoMod );
    } else if( type == "FR" ) { // Free variable
     b.set_lhs( -Inf< double >(), eNoMod );
     b.set_rhs( Inf< double >(), eNoMod );
    } else if( type == "MI" ) { // Lower bound -inf
     b.set_lhs( -Inf< double >(), eNoMod );
     b.set_rhs( 0, eNoMod );
    } else if( type == "PL" ) { // Upper bound +inf
     b.set_lhs( 0, eNoMod );
     b.set_rhs( Inf< double >(), eNoMod );
    } else if( type == "BV" ) { // Binary variable
     c.set_type( ColVariable::kBinary, eNoMod );
    } else if( type == "LI" ) { // Integer variable
     c.set_type( ColVariable::kInteger, eNoMod );
     b.set_lhs( value, eNoMod );
    } else if( type == "UI" ) { // Integer variable
     c.set_type( ColVariable::kInteger, eNoMod );
     b.set_rhs( value, eNoMod );
    } else {
     throw std::invalid_argument( "Invalid syntax in MPS file" );
    }
   } else {
    throw std::invalid_argument( "Invalid syntax in MPS file" );
   }
  }
 }

 // Reset and set abstract representation
 reset_static_constraints();
 reset_static_variables();
 reset_objective();

 set_objective( of, eNoMod );
 add_static_variable( *cols );
 add_static_constraint( *rows );
 add_static_constraint( *bounds );

 // Issue the NBModification
 if( anyone_there() ) {
  add_Modification( std::make_shared< NBModification >( this ) );
 }

 file.close();
}

/*--------------------------------------------------------------------------*/

void AbstractBlock::read_lp( const std::string & filename ) {
 throw std::logic_error( "AbstractBlock::read_lp() not implemented yet" );
}

/*--------------------------------------------------------------------------*/

void AbstractBlock::guts_of_deserialize( const netCDF::NcGroup & group )
{
 // deserialize the "abstract only inner Block"
 netCDF::NcDim nib = group.getDim( "NumberInnerBlock" );
 if( nib.isNull() )
  return;

 auto nibs = nib.getSize();

 if( v_Block.size() < nibs )
  v_Block.resize( nibs, nullptr );

 for( auto i = get_first_inner_Block() ; i < nibs ; ++i ) {
  auto bi = group.getGroup( "Block_" + std::to_string( i ) );
  if( bi.isNull() )
   throw( std::invalid_argument( "inner Block not found" ) );
  v_Block[ i ] = new_Block( bi );
  }
 }  // end( AbstractBlock::guts_of_deserialize )

/*--------------------------------------------------------------------------*/
/*-------------------- End File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
