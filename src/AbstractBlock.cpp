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

#include "AbstractBlock.h"

#include "ColVariable.h"

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

AbstractBlock::~AbstractBlock() {
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
 for( Index i = get_first_dynamic_Constraint(); i < dc.size(); ++i ) {
  if( un_any_const_dynamic( dc[ i ],
                            []( FRowConstraint & cnst ) { cnst.clear(); },
                            un_any_type< FRowConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ],
                            []( BoxConstraint & cnst ) { cnst.clear(); },
                            un_any_type< BoxConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ],
                            []( LBConstraint & cnst ) { cnst.clear(); },
                            un_any_type< LBConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ],
                            []( UBConstraint & cnst ) { cnst.clear(); },
                            un_any_type< UBConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ],
                            []( NNConstraint & cnst ) { cnst.clear(); },
                            un_any_type< NNConstraint >() ) )
   continue;
  if( un_any_const_dynamic( dc[ i ],
                            []( NPConstraint & cnst ) { cnst.clear(); },
                            un_any_type< NPConstraint >() ) )
   continue;
  un_any_const_dynamic( dc[ i ], []( ZOConstraint & cnst ) { cnst.clear(); },
                        un_any_type< ZOConstraint >() );
 }

 // then clear the Objective
 if( ( !is_Objective_reserved() ) && get_objective() )
  get_objective()->clear();

 // now delete all the inner Block
 for( Index i = get_first_inner_Block(); i < v_Block.size(); ++i )
  delete v_Block[ i ];

 v_Block.clear();

 // now delete all the Constraint
 for( Index i = get_first_static_Constraint(); i < sc.size(); ++i ) {
  if( un_any_thing( FRowConstraint, sc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( BoxConstraint, sc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( LBConstraint, sc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( UBConstraint, sc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( NNConstraint, sc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( NPConstraint, sc[ i ], { delete &var; } ) )
   continue;
  un_any_thing( ZOConstraint, sc[ i ], { delete &var; } );
 }

 for( Index i = get_first_dynamic_Constraint(); i < dc.size(); ++i ) {
  if( un_any_thing( std::list< FRowConstraint >, dc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< BoxConstraint >, dc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< LBConstraint >, dc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< UBConstraint >, dc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< NNConstraint >, dc[ i ], { delete &var; } ) )
   continue;
  if( un_any_thing( std::list< NPConstraint >, dc[ i ], { delete &var; } ) )
   continue;
  un_any_thing( std::list< ZOConstraint >, dc[ i ], { delete &var; } );
 }

 // now delete all the Variable
 auto & sv = get_static_variables();
 for( Index i = get_first_static_Variable(); i < sv.size(); ++i )
  un_any_thing( ColVariable, sv[ i ], { delete &var; } );

 auto & dv = get_dynamic_variables();
 for( Index i = get_first_dynamic_Variable(); i < dv.size(); ++i )
  un_any_thing( std::list< ColVariable >, dv[ i ], { delete &var; } );

 // now delete the Objective
 if( ( !is_Objective_reserved() ) && get_objective() )
  delete get_objective();

}  // end( ~AbstractBlock )

/*--------------------------------------------------------------------------*/

bool AbstractBlock::is_feasible( bool useabstract, Configuration * fsbc ) {
 if( !useabstract )
  return ( false );

 // compute the accuracy parameter- - - - - - - - - - - - - - - - - - - - - -
 double eps = 0;
 auto tfsbc = dynamic_cast<SimpleConfiguration< double > *>( fsbc );

 if( ( !tfsbc ) && f_BlockConfig &&
     f_BlockConfig->f_is_feasible_Configuration )
  tfsbc = dynamic_cast<SimpleConfiguration< double > *>(
   f_BlockConfig->f_is_feasible_Configuration );
 if( tfsbc )
  eps = tfsbc->f_value;

 bool feas = true;

 // the static Constraints of the Block - - - - - - - - - - - - - - - - - - -
 auto & sc = get_static_constraints();
 for( Index i = get_first_static_Constraint(); i < sc.size(); ++i ) {
  if( un_any_const_static( sc[ i ], [ & feas, eps ]( FRowConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                           },
                           un_any_type< FRowConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_static( sc[ i ], [ & feas, eps ]( BoxConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                           },
                           un_any_type< BoxConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_static( sc[ i ], [ & feas, eps ]( LBConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                           },
                           un_any_type< LBConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_static( sc[ i ], [ & feas, eps ]( UBConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                           },
                           un_any_type< UBConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_static( sc[ i ], [ & feas, eps ]( NNConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                           },
                           un_any_type< NNConstraint >() ) ) {
   if( !feas )
    return ( false );
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
  if( un_any_const_static( sc[ i ], [ & feas, eps ]( ZOConstraint & cnst ) {
                            feas = ( cnst.rel_viol() <= eps );
                           },
                           un_any_type< ZOConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  throw ( std::logic_error(
   "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
 }

 // the static Variables of the Block - - - - - - - - - - - - - - - - - - - -
 auto & sv = get_static_variables();
 for( Index i = get_first_static_Variable(); i < sv.size(); ++i ) {
  if( un_any_const_static( sv[ i ], [ & feas, eps ]( ColVariable & var ) {
                            feas = var.is_feasible( eps );
                           },
                           un_any_type< ColVariable >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  throw ( std::logic_error( "some static Variable not ColVariable" ) );
 }

 // the dynamic Constraints of the Block-  - - - - - - - - - - - - - - - - - -
 auto & dc = get_dynamic_constraints();
 for( Index i = get_first_dynamic_Constraint(); i < dc.size(); ++i ) {
  if( un_any_const_dynamic( dc[ i ],
                            [ & feas, eps ]( FRowConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                            },
                            un_any_type< FRowConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_dynamic( dc[ i ], [ & feas, eps ]( BoxConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                            },
                            un_any_type< BoxConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_dynamic( dc[ i ], [ & feas, eps ]( LBConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                            },
                            un_any_type< LBConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_dynamic( dc[ i ], [ & feas, eps ]( UBConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                            },
                            un_any_type< UBConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_dynamic( dc[ i ], [ & feas, eps ]( NNConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                            },
                            un_any_type< NNConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_dynamic( dc[ i ], [ & feas, eps ]( NPConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                            },
                            un_any_type< NPConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  if( un_any_const_dynamic( dc[ i ], [ & feas, eps ]( ZOConstraint & cnst ) {
                             feas = ( cnst.rel_viol() <= eps );
                            },
                            un_any_type< ZOConstraint >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  throw ( std::logic_error(
   "some dynamic Constraint not FRowConstraint or :OneVarConstraint" ) );
 }

 // the dynamic Variables of the Block- - - - - - - - - - - - - - - - - - - -
 auto & dv = get_dynamic_variables();
 for( Index i = get_first_dynamic_Variable(); i < dv.size(); ++i ) {
  if( un_any_const_dynamic( dv[ i ], [ & feas, eps ]( ColVariable & var ) {
                             feas = var.is_feasible( eps );
                            },
                            un_any_type< ColVariable >() ) ) {
   if( !feas )
    return ( false );
   continue;
  }
  throw ( std::logic_error( "some dynamic Variable not ColVariable" ) );
 }

 // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( Index i = get_first_inner_Block(); i < v_Block.size(); ++i )
  if( !v_Block[ i ]->is_feasible( true ) )
   return ( false );

 return ( true );

}  // end( AbstractBlock::is_feasible )

/*--------------------------------------------------------------------------*/

void AbstractBlock::check_Variable( Variable * var ) {
 if( var->get_Block() != this )
  std::cout << std::endl << "Variable " << var
            << " not of the right Block";

 for( Index as = 0; as < var->get_num_active(); ++as ) {
  auto tvdi = var->get_active( as );
  if( tvdi->is_active( var ) >= tvdi->get_num_active_var() )
   std::cout << std::endl << "Variable " << var
             << " not active in its " << as << "-th active stuff";
 }
}

/*--------------------------------------------------------------------------*/

void AbstractBlock::check_Constraint( Constraint * cnst ) {
 if( cnst->get_Block() != this )
  std::cout << std::endl << "Constraint " << cnst << " not of the right Block";

 for( Index av = 0; av < cnst->get_num_active_var(); ++av ) {
  auto var = cnst->get_active_var( av );
  if( var->is_active( cnst ) >= var->get_num_active() )
   std::cout << std::endl << "Constraint " << cnst
             << " not active in its " << av << "-th active Variable";
 }
}

/*--------------------------------------------------------------------------*/

void AbstractBlock::check_Objective( Objective * obj ) {
 if( obj->get_Block() != this )
  std::cout << std::endl << "Objective " << obj << " not of the right Block";

 for( Index av = 0; av < obj->get_num_active_var(); ++av ) {
  auto var = obj->get_active_var( av );
  if( var->is_active( obj ) >= var->get_num_active() )
   std::cout << std::endl << "Objective " << obj
             << " not active in its " << av << "-th active Variable";
 }
}

/*--------------------------------------------------------------------------*/

void AbstractBlock::is_correct() {
 // the static Variables of the Block - - - - - - - - - - - - - - - - - - - -
 auto & sv = get_static_variables();
 for( Index i = 0; i < sv.size(); ++i ) {
  if( un_any_const_static( sv[ i ],
                           [ this ]( ColVariable & var ) {
                            check_Variable( &var );
                           }, un_any_type< ColVariable >() ) ) {
   continue;
  }
  throw ( std::logic_error( "some static Variable not ColVariable" ) );
 }

 // the dynamic Variables of the Block- - - - - - - - - - - - - - - - - - - -
 auto & dv = get_dynamic_variables();
 for( Index i = 0; i < dv.size(); ++i ) {
  if( un_any_const_dynamic( dv[ i ],
                            [ this ]( ColVariable & var ) {
                             check_Variable( &var );
                            }, un_any_type< ColVariable >() ) ) {

   continue;
  }
  throw ( std::logic_error( "some dynamic Variable not ColVariable" ) );
 }

 // the static Constraints of the Block - - - - - - - - - - - - - - - - - - -
 auto & sc = get_static_constraints();
 for( Index i = 0; i < sc.size(); ++i ) {
  if( un_any_const_static( sc[ i ],
                           [ this ]( FRowConstraint & cnst ) {
                            check_Constraint( &cnst );
                           }, un_any_type< FRowConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ],
                           [ this ]( BoxConstraint & cnst ) {
                            check_Constraint( &cnst );
                           }, un_any_type< BoxConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ],
                           [ this ]( LBConstraint & cnst ) {
                            check_Constraint( &cnst );
                           }, un_any_type< LBConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ],
                           [ this ]( UBConstraint & cnst ) {
                            check_Constraint( &cnst );
                           }, un_any_type< UBConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ],
                           [ this ]( NNConstraint & cnst ) {
                            check_Constraint( &cnst );
                           }, un_any_type< NNConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ],
                           [ this ]( NPConstraint & cnst ) {
                            check_Constraint( &cnst );
                           }, un_any_type< NPConstraint >() ) )
   continue;

  if( un_any_const_static( sc[ i ],
                           [ this ]( ZOConstraint & cnst ) {
                            check_Constraint( &cnst );
                           }, un_any_type< ZOConstraint >() ) )
   continue;

  throw ( std::logic_error(
   "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
 }

 // the dynamic Constraints of the Block- - - - - - - - - - - - - - - - - - -
 auto & dc = get_dynamic_constraints();
 for( Index i = 0; i < dc.size(); ++i ) {
  if( un_any_const_dynamic( dc[ i ],
                            [ this ]( FRowConstraint & cnst ) {
                             check_Constraint( &cnst );
                            }, un_any_type< FRowConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ],
                            [ this ]( BoxConstraint & cnst ) {
                             check_Constraint( &cnst );
                            }, un_any_type< BoxConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ],
                            [ this ]( LBConstraint & cnst ) {
                             check_Constraint( &cnst );
                            }, un_any_type< LBConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ],
                            [ this ]( UBConstraint & cnst ) {
                             check_Constraint( &cnst );
                            }, un_any_type< UBConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ],
                            [ this ]( NNConstraint & cnst ) {
                             check_Constraint( &cnst );
                            }, un_any_type< NNConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ],
                            [ this ]( NPConstraint & cnst ) {
                             check_Constraint( &cnst );
                            }, un_any_type< NPConstraint >() ) )
   continue;

  if( un_any_const_dynamic( dc[ i ],
                            [ this ]( ZOConstraint & cnst ) {
                             check_Constraint( &cnst );
                            }, un_any_type< ZOConstraint >() ) )
   continue;

  throw ( std::logic_error(
   "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
 }

 // the Objective of the Block- - - - - - - - - - - - - - - - - - - - - - - -
 if( auto obj = get_objective() )
  check_Objective( obj );

 // check every sub-Block of AbstractBlock - - - - - - - - - - - - - - - - -

 for( Index i = 0; i < get_number_nested_Blocks(); ++i ) {
  auto sb = get_nested_Block( i );
  if( sb->get_f_Block() != this )
   std::cout << std::endl << "sub-Block " << i << " has wrong father";
  if( auto asb = dynamic_cast< AbstractBlock * >( sb ) )
   asb->is_correct();
 }
}  // end( AbstractBlock::is_correct )

/*--------------------------------------------------------------------------*/

Solution * AbstractBlock::get_Solution( Configuration * csolc, bool emptys ) {

 auto config = dynamic_cast<SimpleConfiguration< int > *>( csolc );

 if( ( !config ) && f_BlockConfig )
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

void AbstractBlock::serialize( netCDF::NcGroup & group ) const {
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
     ( !is_Objective_reserved() ) )
  throw ( std::logic_error(
   "AbstractBlock::serialize not fully implemented yet" ) );

 group.putAtt( "type", name() );

 if( v_Block.size() > get_first_inner_Block() ) {
  group.addDim( "NumberInnerBlock", v_Block.size() );

  for( Index i = get_first_inner_Block(); i < v_Block.size(); ++i ) {
   auto gi = group.addGroup( "Block_" + std::to_string( i ) );
   v_Block[ i ]->serialize( gi );
  }
 }
}  // end( AbstractBlock::serialize )

/*--------------------------------------------------------------------------*/

void AbstractBlock::print( std::ostream & output ) const {
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
  for( unsigned int i = get_first_static_Constraint(); i < sc.size(); ++i ) {
   output << i;
   if( ( !get_s_const_name().empty() ) &&
       ( !get_s_const_name()[ i ].empty() ) )
    output << " (" << get_s_const_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_static( sc[ i ], [ & output ]( FRowConstraint & cnst ) {
                             output << cnst << std::endl;
                            },
                            un_any_type< FRowConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ], [ & output ]( BoxConstraint & cnst ) {
                             output << cnst << std::endl;
                            },
                            un_any_type< BoxConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ], [ & output ]( LBConstraint & cnst ) {
                             output << cnst << std::endl;
                            },
                            un_any_type< LBConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ], [ & output ]( UBConstraint & cnst ) {
                             output << cnst << std::endl;
                            },
                            un_any_type< UBConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ], [ & output ]( NNConstraint & cnst ) {
                             output << cnst << std::endl;
                            },
                            un_any_type< NNConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ], [ & output ]( NPConstraint & cnst ) {
                             output << cnst << std::endl;
                            },
                            un_any_type< NPConstraint >() ) )
    continue;
   if( un_any_const_static( sc[ i ], [ & output ]( ZOConstraint & cnst ) {
                             output << cnst << std::endl;
                            },
                            un_any_type< ZOConstraint >() ) )
    continue;
   throw ( std::logic_error(
    "some static Constraint not FRowConstraint or :OneVarConstraint" ) );
  }

  // the static Variables of the Block- - - - - - - - - - - - - - - - - - - -
  output << "Static Variables:" << std::endl;
  auto & sv = get_static_variables();
  for( unsigned int i = get_first_static_Variable(); i < sv.size(); ++i ) {
   output << i;
   if( ( !get_s_var_name().empty() ) &&
       ( !get_s_var_name()[ i ].empty() ) )
    output << " (" << get_s_var_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_static( sv[ i ], [ & output ]( ColVariable & var ) {
                             output << var << std::endl;
                            },
                            un_any_type< ColVariable >() ) )
    continue;
   throw ( std::logic_error( "some static Variable not ColVariable" ) );
  }

  // the dynamic Constraints of the Block- - - - - - - - - - - - - - - - - -
  output << "Dynamic Constraints:" << std::endl;
  auto & dc = get_dynamic_constraints();
  for( unsigned int i = get_first_dynamic_Constraint(); i < dc.size(); ++i ) {
   output << i;
   if( ( !get_d_const_name().empty() ) &&
       ( !get_d_const_name()[ i ].empty() ) )
    output << " (" << get_d_const_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_dynamic( dc[ i ],
                             [ & output ]( FRowConstraint & cnst ) {
                              output << cnst << std::endl;
                             },
                             un_any_type< FRowConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ],
                             [ & output ]( BoxConstraint & cnst ) {
                              output << cnst << std::endl;
                             },
                             un_any_type< BoxConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ],
                             [ & output ]( LBConstraint & cnst ) {
                              output << cnst << std::endl;
                             },
                             un_any_type< LBConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ],
                             [ & output ]( UBConstraint & cnst ) {
                              output << cnst << std::endl;
                             },
                             un_any_type< UBConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ],
                             [ & output ]( NNConstraint & cnst ) {
                              output << cnst << std::endl;
                             },
                             un_any_type< NNConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ],
                             [ & output ]( NPConstraint & cnst ) {
                              output << cnst << std::endl;
                             },
                             un_any_type< NPConstraint >() ) )
    continue;
   if( un_any_const_dynamic( dc[ i ],
                             [ & output ]( ZOConstraint & cnst ) {
                              output << cnst << std::endl;
                             },
                             un_any_type< ZOConstraint >() ) )
    continue;
   throw ( std::logic_error(
    "some dynamic Constraint not FRowConstraint or :OneVarConstraint" ) );
  }

  // the dynamic Variables of the Block - - - - - - - - - - - - - - - - - - -
  output << "Dynamic Variables:" << std::endl;
  auto & dv = get_dynamic_variables();
  for( unsigned int i = get_first_dynamic_Variable(); i < dv.size(); ++i ) {
   output << i;
   if( ( !get_d_var_name().empty() ) &&
       ( !get_d_var_name()[ i ].empty() ) )
    output << " (" << get_d_var_name()[ i ] << "): ";
   else
    output << ": ";

   if( un_any_const_dynamic( dv[ i ], [ & output ]( ColVariable & var ) {
                              output << var << std::endl;
                             },
                             un_any_type< ColVariable >() ) )
    continue;
   throw ( std::logic_error( "some dynamic Variable not ColVariable" ) );
  }

  // the Objective of the Block - - - - - - - - - - - - - - - - - - - - - - -
  if( !is_Objective_reserved() )
   output << "Objective:" << *get_objective() << std::endl;

  // the inner Blocks - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  output << std::endl << "Nested Blocks:" << std::endl;
  for( Index i = get_first_inner_Block(); i < v_Block.size(); ++i )
   output << *v_Block[ i ];
 }
}  // end( AbstractBlock::print )

/*--------------------------------------------------------------------------*/

void AbstractBlock::guts_of_deserialize( netCDF::NcGroup & group ) {
 // deserialize the "abstract only inner Block"
 netCDF::NcDim nib = group.getDim( "NumberInnerBlock" );
 if( nib.isNull() )
  return;

 auto nibs = nib.getSize();

 if( v_Block.size() < nibs )
  v_Block.resize( nibs, nullptr );

 for( Index i = get_first_inner_Block(); i < nibs; ++i ) {
  auto bi = group.getGroup( "Block_" + std::to_string( i ) );
  if( bi.isNull() )
   throw ( std::invalid_argument( "inner Block not found" ) );
  v_Block[ i ] = new_Block( bi );
 }
}  // end( AbstractBlock::guts_of_deserialize )

/*--------------------------------------------------------------------------*/
/*-------------------- End File AbstractBlock.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
