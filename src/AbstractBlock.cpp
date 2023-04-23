/*--------------------------------------------------------------------------*/
/*------------------------ File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the AbstractBlock class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
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

void AbstractBlock::load( std::istream & input , char frmt )
{
 if( ( ! frmt ) || ( frmt == 'M' ) ) {
  read_mps( input );
  return;
  }

 if( frmt == 'L' ) {
  read_lp( input );
  return;
  }

 throw( std::invalid_argument( "AbstractBlock::read: unsupported file format"
                               ) );
 }

/*--------------------------------------------------------------------------*/

bool AbstractBlock::is_feasible( bool useabstract , Configuration * fsbc )
{
 // compute the accuracy parameter- - - - - - - - - - - - - - - - - - - - - -
 double eps = 0;
 bool rel_viol = true;

 // Try to extract, from "c", the parameters that determine feasibility.
 // If it succeeds, it sets the values of the parameters and returns
 // true. Otherwise, it returns false.
 auto extract_parameters = [ & eps , & rel_viol ]( Configuration * c )
  -> bool {
  if( auto tc = dynamic_cast< SimpleConfiguration< double > * >( c ) ) {
   eps = tc->f_value;
   return( true );
   }
  if( auto tc = dynamic_cast< SimpleConfiguration<
      std::pair< double , int > > * >( c ) ) {
   eps = tc->f_value.first;
   rel_viol = tc->f_value.second;
   return( true );
   }
  return( false );
  };

 if( ( ! extract_parameters( fsbc ) ) && f_BlockConfig )
  // if the given Configuration is not valid, try the one from the BlockConfig
  extract_parameters( f_BlockConfig->f_is_feasible_Configuration );

 bool feas = true;

 // check if a RowConstraint is satisfied, but without computing it
 auto check_feasibility = [ & feas , eps , rel_viol ]( auto & cnst ) {
                           if( ( ! feas ) || cnst.is_relaxed() )
                            return;
                           feas = ( ( rel_viol ? cnst.rel_viol() :
                                      cnst.abs_viol() ) <= eps ); };

 // the static Constraints of the Block - - - - - - - - - - - - - - - - - - -
 // note: AbstractBlock::is_feasible() is now checking *all* the abstract
 //       representation, both the "reserved" part and all the rest. another
 //       approach would be to leave the "reserved" part to derived classes
 //       and only test here the "non reserved" part. This might be
 //       fractionally more efficient but it would require every derived
 //       class to implement is_feasible(); so far we prefer the general
 //       even if possibly slower solution
 // auto & sc = get_static_constraints();
 //!! for( Index i = get_first_static_Constraint() ; i < sc.size() ; ++i ) {
 for( auto & sci : get_static_constraints() ) {
  if( un_any_const_static( sci ,
                           [ & feas , eps , rel_viol ]
                           ( FRowConstraint & cnst ) {
                            if( ( ! feas ) || cnst.is_relaxed() )
                             return;
                            if( auto ret = cnst.compute() ;
                                ( ret <= FRowConstraint::kUnEval ) ||
                                ( ret > FRowConstraint::kOK ) )
                             feas = false;
                            else
                             feas = ( ( rel_viol ? cnst.rel_viol() :
                                        cnst.abs_viol() ) <= eps );
                            } ,
                           un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sci , check_feasibility ,
                           un_any_type< BoxConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sci , check_feasibility ,
                           un_any_type< LB0Constraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sci , check_feasibility ,
                           un_any_type< UB0Constraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sci , check_feasibility ,
                           un_any_type< LBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sci , check_feasibility ,
                           un_any_type< UBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sci , check_feasibility ,
                           un_any_type< NNConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sci , check_feasibility ,
                           un_any_type< NPConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_static( sci , check_feasibility ,
                           un_any_type< ZOConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( std::logic_error(
       "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
  }

 // the static Variables of the Block - - - - - - - - - - - - - - - - - - - -
 // auto & sv = get_static_variables();
 //!! for( Index i = get_first_static_Variable() ; i < sv.size() ; ++i ) {
 // see above for comments
 for( auto & svi : get_static_variables() ) {
  if( un_any_const_static( svi ,
                           [ & feas , eps ]( ColVariable & var ) {
                            feas = feas && var.is_feasible( eps );
                            } ,
                           un_any_type< ColVariable >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( std::logic_error( "some static Variable not ColVariable" ) );
  }

 // the dynamic Constraints of the Block-  - - - - - - - - - - - - - - - - - -
 // auto & dc = get_dynamic_constraints();
 //!! for( Index i = get_first_dynamic_Constraint() ; i < dc.size() ; ++i ) {
 // see above for comments
 for( auto & dci : get_dynamic_constraints() ) {
  if( un_any_const_dynamic( dci ,
                            [ & feas , eps , rel_viol ]
                            ( FRowConstraint & cnst ) {
                             if( ( ! feas ) || cnst.is_relaxed() )
                              return;
                             if( auto ret = cnst.compute() ;
                                 ( ret <= FRowConstraint::kUnEval ) ||
                                 ( ret > FRowConstraint::kOK ) )
                              feas = false;
                             else
                              feas = ( ( rel_viol ? cnst.rel_viol() :
                                         cnst.abs_viol() ) <= eps );
                             } ,
                            un_any_type< FRowConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dci , check_feasibility ,
                            un_any_type< BoxConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dci , check_feasibility ,
                            un_any_type< LB0Constraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dci , check_feasibility ,
                            un_any_type< UB0Constraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dci , check_feasibility ,
                            un_any_type< LBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dci , check_feasibility ,
                            un_any_type< UBConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dci , check_feasibility ,
                            un_any_type< NNConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dci , check_feasibility ,
                            un_any_type< NPConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  if( un_any_const_dynamic( dci , check_feasibility ,
                            un_any_type< ZOConstraint >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( std::logic_error(
   "some dynamic Constraint not FRowConstraint or :OneVarConstraint" ) );
  }

 // the dynamic Variables of the Block- - - - - - - - - - - - - - - - - - - -
 // auto & dv = get_dynamic_variables();
 //!! for( Index i = get_first_dynamic_Variable() ; i < dv.size() ; ++i ) {
 // see above for comments
 for( auto & dvi : get_dynamic_variables() ) {
  if( un_any_const_dynamic( dvi ,
                            [ & feas , eps ]( ColVariable & var ) {
                             feas = feas && var.is_feasible( eps );
                             } ,
                            un_any_type< ColVariable >() ) ) {
   if( ! feas )
    return( false );
   continue;
   }
  throw( std::logic_error( "some dynamic Variable not ColVariable" ) );
  }

 // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 //!! for( Index i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
 // see above for comments
 for( auto bi : v_Block )
  if( ! bi->is_feasible( useabstract ) )
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

 auto config = dynamic_cast< SimpleConfiguration< int > * >( csolc );

 if( ( ! config ) && f_BlockConfig )
  config = dynamic_cast< SimpleConfiguration< int > * >(
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

void AbstractBlock::print( std::ostream & output , char vlvl ) const
{
 if( vlvl == 'M' )
  throw( std::invalid_argument(
        "AbstractBlock::print: output in MPS format not implemented yet" ) );

 if( vlvl == 'L' )
  throw( std::invalid_argument(
         "AbstractBlock::print: output in LP format not implemented yet" ) );
 
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

 if( vlvl ) {
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
  if( ! is_Objective_reserved() )
   output << "Objective:" << *get_objective() << std::endl;

  // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  output << std::endl << "Nested Blocks:" << std::endl;
  for( auto i = get_first_inner_Block() ; i < v_Block.size() ; ++i )
   output << *v_Block[ i ];
  }
 }  // end( AbstractBlock::print )

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

 if( v_Block.size() > get_first_inner_Block() ) {
  group.addDim( "NumberInnerBlock", v_Block.size() );

  for( auto i = get_first_inner_Block() ; i < v_Block.size() ; ++i ) {
   auto gi = group.addGroup( "Block_" + std::to_string( i ) );
   v_Block[ i ]->serialize( gi );
   }
  }
 }  // end( AbstractBlock::serialize )

/*--------------------------------------------------------------------------*/

void AbstractBlock::read_mps( std::istream & file )
{
 auto dbl_val = []( std::string & s ) {
  assert( ! s.empty() );
  if( s[ 0 ] == '.' )
   s.insert( 0 , "0" );
  else
   if( ( s[ 0 ] == '-' ) && ( s[ 1 ] == '.' ) )
    s.insert( 1 , "0" );

  if( s.back() == '.' )
   s.pop_back();

  return( std::stod( s ) );
  };

 std::string problem_name;
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

 std::string word;
 auto max = std::numeric_limits< std::streamsize >::max();

 // Eat initial comments
 while( file.peek() == file.widen( '*' ) ) {
  file.ignore( max, '\n' );
 }

 // Read NAME
 file >> word;
 if( word != "NAME" ) {
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
 }
 file >> word;
 if( word != "OBJSENSE" ) {
  problem_name = word;
  file.ignore( max, '\n' );
  file >> word;
 }

 // Read OBJSENSE (optional)
 if( word == "OBJSENSE" ) {
  file >> word;
  if( word == "MAX" || word == "MAXIMIZE" ) {
   of->set_sense( Objective::eMax, eNoMod );
  } else if( word == "MIN" || word == "MINIMIZE" ) {
   of->set_sense( Objective::eMin, eNoMod );
  } else {
   throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
  }
  file >> word;
 } else {
  // Minimize by default
  of->set_sense( Objective::eMin, eNoMod );
 }

 // Read OBJNAME (optional)
 if( word == "OBJNAME" ) {
  file >> of_name;
  file >> word;
 }

 /*
  * First pass: get rows and columns number
  */

 // Read ROWS
 if( word != "ROWS" ) {
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
 }
 file.ignore( max, '\n' );
 auto pos = file.tellg(); // Save it for later
 while( file.peek() == file.widen( ' ' ) ) {
  file >> word;
  if( word == "E" || word == "L" || word == "G" ) {
   ++num_rows;
   file.ignore( max, '\n' ); // Skip row name for now
  } else if( word == "N" ) {
   file.ignore( max, '\n' ); // Skip row name for now
  } else {
   throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
  }
 }

 // Read COLUMNS
 file >> word;
 if( word != "COLUMNS" ) {
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
 }
 file.ignore( max, '\n' );
 std::string tmp;
 while( file.peek() == file.widen( ' ' ) ) {
  std::string row;

  file >> word;
  file >> row;

  if( row == "\'MARKER\'" ) {
   file.ignore( max, '\n' );
   continue; // Skip integrality markers for now
  }

  if( word != tmp ) {
   tmp = word;
   ++num_cols;
  }
  file.ignore( max, '\n' );
 }

 /*
  * Initialize stuff
  */

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

 rhs.resize( num_rows, 0 );
 rng.resize( num_rows, Inf< double >() );

 /*
  * Second pass: fill data
  */

 // Read ROWS
 file.seekg( pos, file.beg ); // Go back to line after "ROWS"
 int i = 0;
 while( file.peek() == file.widen( ' ' ) ) {

  file >> word;
  if( word == "E" || word == "L" || word == "G" ) {
   row_type[ i ] = word[ 0 ];
   file >> row_names[ i ];
   ++i;
   file.ignore( max, '\n' );
  } else if( word == "N" ) {
   if( of_name.empty() ) {
    file >> of_name;
   }
   // FIXME: Other N rows are ignored
   file.ignore( max, '\n' );
  } else {
   throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
  }
 }

 // Read COLUMNS
 file.ignore( max, '\n' ); // Ignore "COLUMNS" line
 i = 0;
 bool marker = false;
 tmp.clear();
 ColVariable * v;
 LinearFunction * f;
 while( file.peek() == file.widen( ' ' ) ) {
  std::string row;
  std::string value;

  file >> word;
  file >> row;
  file >> value;

  // Check integrality marker
  if( row == "\'MARKER\'" ) {
   if( value == "\'INTORG\'" ) {
    marker = true;
   } else if( value == "\'INTEND\'" ) {
    marker = false;
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }
   file.ignore( max, '\n' );
   continue;
  }

  // Column name
  if( word != tmp ) {
   tmp = word;
   col_names[ i ] = tmp;
   v = &( *cols )[ i ];
   if( marker ) {
    v->set_type( ColVariable::kInteger, eNoMod );
   }
   ++i;
  }

  // First name/value pair
  if( row == of_name ) {
   f = static_cast< LinearFunction * >(of->get_function());
  } else {
   auto it = std::find( row_names.begin(), row_names.end(), row );
   if( it != row_names.end() ) {
    auto j = std::distance( row_names.begin(), it );
    f = static_cast< LinearFunction * >(( *rows )[ j ].get_function());
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }
  }
  f->add_variable( v, dbl_val( value ) );

  // Optional second name/value pair
  if( file.peek() != file.widen( '\n' ) ) {
   file >> row;
   file >> value;
   if( row == of_name ) {
    // Will add to OF
    f = static_cast< LinearFunction * >(of->get_function());
   } else {
    // Will add to a constraint
    auto it = std::find( row_names.begin(), row_names.end(), row );
    if( it != row_names.end() ) {
     auto j = std::distance( row_names.begin(), it );
     f = static_cast< LinearFunction * >(( *rows )[ j ].get_function());
    } else {
     throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
    }
   }
   f->add_variable( v, dbl_val( value ) );
  }
  file.ignore( max, '\n' );
 }

 /*
  * Continue with RHS and RANGES
  */

 // Read RHS
 file >> word;
 if( word != "RHS" ) {
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
 }
 file.ignore( max, '\n' );
 while( file.peek() == file.widen( ' ' ) ) {

  // RHS name
  file >> word;
  if( rhs_name.empty() ) {
   rhs_name = word;
  } else if( word != rhs_name ) {
   throw( std::invalid_argument( "Only one RHS vector is supported" ) );
  }

  std::string row;
  std::string value;

  // First name/value pair
  file >> row;
  file >> value;
  auto it = std::find( row_names.begin(), row_names.end(), row );
  if( it != row_names.end() ) {
   auto j = std::distance( row_names.begin(), it );
   rhs[ j ] = dbl_val( value );
  } else {
   throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
  }

  // Optional second name/value pair
  if( file.peek() != file.widen( '\n' ) ) {
   file >> row;
   file >> value;
   it = std::find( row_names.begin(), row_names.end(), row );
   if( it != row_names.end() ) {
    auto j = std::distance( row_names.begin(), it );
    rhs[ j ] = dbl_val( value );
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }
  }
  file.ignore( max, '\n' );
 }

 // Read RANGES (optional)
 file >> word;
 if( word == "RANGES" ) {
  file.ignore( max, '\n' );
  while( file.peek() == file.widen( ' ' ) ) {

   // RANGES name
   file >> word;
   if( rng_name.empty() ) {
    rng_name = word;
   } else if( word != rng_name ) {
    throw( std::invalid_argument( "Only one RANGE vector is supported" ) );
   }

   std::string row;
   std::string value;

   // First name/value pair
   file >> row;
   file >> value;
   auto it = std::find( row_names.begin(), row_names.end(), row );
   if( it != row_names.end() ) {
    auto j = std::distance( row_names.begin(), it );
    rng[ j ] = dbl_val( value );
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }

   // Optional second name/value pair
   if( file.peek() != file.widen( '\n' ) ) {
    file >> row;
    file >> value;
    it = std::find( row_names.begin(), row_names.end(), row );
    if( it != row_names.end() ) {
     auto j = std::distance( row_names.begin(), it );
     rng[ j ] = dbl_val( value );
    } else {
     throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
    }
   }
   file.ignore( max, '\n' );
  }
  file >> word;
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
 if( word == "BOUNDS" ) {
  file.ignore( max, '\n' );
  while( file.peek() == file.widen( ' ' ) ) {

   std::string type;
   file >> type;

   // BOUNDS name
   file >> word;
   if( bnd_name.empty() ) {
    bnd_name = word;
   } else if( word != bnd_name ) {
    throw( std::invalid_argument( "Only one BOUNDS vector is supported" ) );
   }

   std::string col;
   std::string value;
   file >> col;

   auto it = std::find( col_names.begin(), col_names.end(), col );
   if( it != col_names.end() ) {
    auto j = std::distance( col_names.begin(), it );
    auto & b = ( *bounds )[ j ];
    auto & c = ( *cols )[ j ];
    if( type == "LO" ) {        // Lower bound
     file >> value;
     b.set_lhs( dbl_val( value ), eNoMod );
    } else if( type == "UP" ) { // Upper bound
     file >> value;
     b.set_rhs( dbl_val( value ), eNoMod );
    } else if( type == "FX" ) { // Fixed variable
     file >> value;
     b.set_both( dbl_val( value ), eNoMod );
     c.set_value( dbl_val( value ) );
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
     file >> value;
     c.set_type( ColVariable::kInteger, eNoMod );
     b.set_lhs( dbl_val( value ), eNoMod );
    } else if( type == "UI" ) { // Integer variable
     file >> value;
     c.set_type( ColVariable::kInteger, eNoMod );
     b.set_rhs( dbl_val( value ), eNoMod );
    } else {
     throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
    }
   } else {
    throw( std::invalid_argument( "Invalid syntax in MPS file" ) );
   }
   file.ignore( max, '\n' );
  }
  file >> word;
 }

 if( word != "ENDATA" )
  throw( std::invalid_argument( "Invalid syntax in MPS file" ) );

 // Reset and set abstract representation
 reset_static_constraints();
 reset_static_variables();
 reset_objective();

 set_objective( of, eNoMod );
 add_static_variable( *cols );
 add_static_constraint( *rows );
 add_static_constraint( *bounds );

 // Issue the NBModification
 if( anyone_there() )
  add_Modification( std::make_shared< NBModification >( this ) );

 }  // end( AbstractBlock::read_mps )

/*--------------------------------------------------------------------------*/

void AbstractBlock::read_lp( std::istream & file )
{
 throw( std::logic_error( "AbstractBlock::read_lp() not implemented yet" ) );
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
