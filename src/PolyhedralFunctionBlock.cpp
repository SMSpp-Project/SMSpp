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
 if( f_rep & 2 )  // done already
  return;         // nothing else to do

 int wsol = 0;
 auto tstvv = dynamic_cast<SimpleConfiguration<int> *>( stvv );

 if( ( ! tstvv ) && f_BlockConfig &&
     f_BlockConfig->f_solution_Configuration )
  tstvv = dynamic_cast<SimpleConfiguration<int> *>(
                         f_BlockConfig->f_solution_Configuration );
 if( tstvv )
  wsol = tstvv->f_value;

 if( wsol )
  f_rep |= 1;

 if( f_rep & 1 ) {  // use linearized representation
  // note: the static ColVariable "v" is added "in front"
  f_1st_stat_var = 1;
  add_static_variable( f_v , "" , true );
  }

 f_rep |= 2;

 }  // end( PolyhedralFunctionBlock::generate_abstract_variables )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::generate_abstract_constraints(
						        Configuration * stcc )
{
 if( f_rep & 4 )  // done already
  return;         // nothing else to do

 if( ! ( f_rep & 2 ) )  // variables not constructed
  throw( std::logic_error( "Variable must be generated before Constraint" ) );

 if( f_rep & 1 ) {  // use linearized representation
  // add the bounds on v
  f_bcv.set_variable( &f_v );
  f_bcv.set_rhs( f_polyf.get_global_upper_bound() , eNoMod );
  f_bcv.set_lhs( f_polyf.get_global_lower_bound() , eNoMod );
  

  // note: the bounds on v are added "in front"
  f_1st_stat_cnst = 1;
  add_static_constraint( f_bcv , "" , true );

  // add the linear constraints
  f_const.resize( f_polyf.get_A().size() );
  auto cit = f_const.begin();
  for( Index i = 0 ; i < f_polyf.get_A().size() ; )
   ConstructLPConstraint( i++ , *(cit++) );

  // note: the linear constraints are added "in front"
  f_1st_dym_cnst = 1;
  add_dynamic_constraint( f_const , "" , true );
  }

 f_rep |= 4;

 }  // end( PolyhedralFunctionBlock::generate_abstract_constraints )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::generate_objective( Configuration * objc )
{
 if( f_rep & 8 )  // done already
  return;         // nothing else to do

 if( ! ( f_rep & 2 ) )  // variables not constructed
  throw( std::logic_error( "Variable must be generated before Objective" ) );

 f_res_obj = true;  // in either representation the objective is "reserved"

 auto obj = new FRealObjective();
 obj->set_sense( f_polyf.is_convex() ? FRealObjective::eMin :
		                       FRealObjective::eMax , eNoMod );

 if( f_rep & 1 )  // use linearized representation
  obj->set_function( new LinearFunction( { std::make_pair( & f_v , 1 ) } ) );
 else             // use natural representation
  obj->set_function( & f_polyf );

 set_objective( obj );

 f_rep |= 8;

 }  // end( PolyhedralFunctionBlock::generate_objective )

/*--------------------------------------------------------------------------*/
/*------- Methods for reading the data of the PolyhedralFunctionBlock ------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--- METHODS FOR LOADING, PRINTING & SAVING THE PolyhedralFunctionBlock ---*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::print( std::ostream & output ) const
{
 output << std::endl << "PolyhedralFunctionBlock[";
 if( f_rep & 1 )
  output << "l/";
 else
  output << "n/";
 if( f_polyf.is_convex() )
  output << "cvx";
 else
  output << "cnc";
 output << "] with PolyhedralFunction( " << f_polyf.get_num_active_var()
	<< ", " << f_polyf.get_A().size() << " )" << std::endl;

 if( verbosity_lvl == Block::medium || verbosity_lvl == Block::high ) {
  for( Index i = 0 ; i < f_polyf.get_A().size()  ; ++i ) {
   output << "A[ " << i << " ] = [ ";
   for( Index j = 0 ; j < f_polyf.get_num_active_var() ; ++j )
    output << f_polyf.get_A()[ i ][ j ] << " ";
   output << "], b[ " << i << " ] = " << f_polyf.get_b()[ i ] << std::endl;
   }

  /*!! can't do as get_global_*_bound() are not const
  if( f_polyf.is_bound_set() ) {
   if( f_polyf.is_convex() )
    output << "LB = " << f_polyf.get_global_lower_bound();
   else
    output << "UB = " << f_polyf.get_global_upper_bound();

   output << std::endl;
   }
   !!*/
  }

 AbstractBlock::print( output );

 }  // end( PolyhedralFunctionBlock::print )

/*--------------------------------------------------------------------------*/
/*--------------------------- PRIVATE METHODS ------------------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::guts_of_destructor( void )
{
 // clear the Objective (if any)
 auto obj = static_cast< FRealObjective * >( get_objective() );
 if( obj )
  obj->clear();

 if( f_rep & 1 ) {  // use linearized representation
  // first clear() all the constraints
  for( auto & ci : f_const )
   ci.clear();

  f_bcv.clear();

  // then nothing, they will be deleted when f_const/f_bcv are

  // ensure that the LinearFunction inside the Objective is deleted
  if( obj )
   obj->set_function( nullptr , eNoMod , true );
  }
 else {             // use natural representation
  // ensure that the PolyhedrakFunction inside the Objective is NOT deleted
  if( obj )
   obj->set_function( nullptr , eNoMod , false );
  }

 // finally delete the Objective
 delete obj;

 }  // end( PolyhedralFunctionBlock::guts_of_destructor )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::guts_of_add_Modification_PF(
			  std::shared_ptr< FunctionMod > mod , ChnlName chnl )
{
 // process a FunctionMod produced by the PolyhedralFunction- - - - - - - - -
 /* This requires to patiently sift through the possible Modification types
  * (but only those derived from FunctionMod) to find what this Modification
  * exactly is, and appropriately mirror the changes to the PolyhedralFunction
  * (which in this case counts as the "physical representation") into the
  * "abstract" one, i.e., performing the corresponding changes on the LP. */

 // C05FunctionModVarsAddd- - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<C05FunctionModVarsAddd>( mod );
  if( tmod ) {  // this is "add Variables"
   c_Index frst = tmod->first();
   c_Index nav = f_polyf.get_num_active_var();

   // open a new GroupModification, not concerning PolyhedralFunctionBlock
   bool newchnl = f_const.size() > 1;
   auto ichnl = open_or_nest( newchnl , chnl );
   auto par = make_par( eNoBlck , ichnl );

   Index i = 0;
   for( auto & ci : f_const ) {
    LinearFunction::v_coeff_pair vars( nav - frst );
    auto vit = vars.begin();
    auto Aiit = f_polyf.get_A()[ i++ ].begin(); 
    for( Index j = frst ; j < nav ; ++j )
     *(vit++) = std::make_pair( static_cast< ColVariable * >(
					     f_polyf.get_active_var( j ) ) ,
				- *(Aiit++) );
    static_cast< LinearFunction * >( ci.get_function() )->
     add_variables( std::move( vars ) , par );
    }

   if( newchnl ) {
    if( chnl )
     un_nest_channel( ichnl );
    else
     close_channel( ichnl );
    }
   return;
   }
  }

 // C05FunctionModVarsRngd- - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<C05FunctionModVarsRngd>( mod );
  if( tmod ) {  // this is "remove Variables, ranged"
   auto rng = tmod->range();
   rng.first++;   // variables names in the constraints are +1 w.r.t. those
   rng.second++;  // of the PolyhedralFunction

   // open a new GroupModification, not concerning PolyhedralFunctionBlock
   bool newchnl = f_const.size() > 1;
   auto ichnl = open_or_nest( newchnl , chnl );
   auto par = make_par( eNoBlck , ichnl );

   for( auto & ci : f_const )
    static_cast< LinearFunction * >( ci.get_function() )->
     remove_variables( rng , par );

   if( newchnl ) {
    if( chnl )
     un_nest_channel( ichnl );
    else
     close_channel( ichnl );
    }
   return;
   }
  }

 // C05FunctionModVarsSbst- - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<C05FunctionModVarsSbst>( mod );
  if( tmod ) {  // this is "remove Variables, subset"
   Subset sbst( tmod->subset() );
   for( auto & si : sbst )  // variables names in the constraints are +1
    si++;                   // w.r.t. those of the PolyhedralFunction

   if( ! tmod->ordered() )
    std::sort( sbst.begin() , sbst.end() );

   // open a new GroupModification, not concerning PolyhedralFunctionBlock
   bool newchnl = f_const.size() > 1;
   auto ichnl = open_or_nest( newchnl , chnl );
   auto par = make_par( eNoBlck , ichnl );

   for( auto & ci : f_const )
    static_cast< LinearFunction * >( ci.get_function() )->
     remove_variables( std::move( Subset( sbst ) ) , true , par );

   if( newchnl ) {
    if( chnl )
     un_nest_channel( ichnl );
    else
     close_channel( ichnl );
    }
   return;
   }
  }

 // PolyhedralFunctionModRngd - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<PolyhedralFunctionModRngd>( mod
									  );
  if( tmod ) {  // this is "modify/delete a range of rows"
   Index strt = tmod->range().first;
   Index stop = tmod->range().second;

   if( strt == stop ) {  // special case: the lower/upper bound
    if( f_polyf.is_convex() )  // convex ==> lower bound
     f_bcv.set_lhs( f_polyf.get_lower_estimate() ,
		    make_par( eNoBlck , chnl ) );
    else                       // concave ==> upper bound
     f_bcv.set_rhs( f_polyf.get_upper_estimate() ,
		    make_par( eNoBlck , chnl ) );
    return;
    }

   // open a new GroupModification, not concerning PolyhedralFunctionBlock
   // unless it's deleting or only one row and *not* also its constant
   bool newchnl = ( tmod->PFtype() != PolyhedralFunctionMod::DeleteRows ) &&
                    ( ( stop > strt + 1 ) ||
		      ( tmod->PFtype() == PolyhedralFunctionMod::ModifyCnst )
		      );
   auto ichnl = open_or_nest( newchnl , chnl );
   auto par = make_par( eNoBlck , ichnl );

   auto cit = std::next( f_const.begin() , strt );
   if( tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
    // delete rows
    std::vector< std::list< FRowConstraint >::iterator > rmvd( stop - strt );
    for( auto & ri : rmvd )
     ri = cit++;
    remove_dynamic_constraints( f_const , rmvd , par );
    }
   else
    if( tmod->PFtype() == PolyhedralFunctionMod::ModifyRows ) {
     // modify rows & constants
     Range rng = Range( 1 , f_polyf.get_num_active_var() + 1 );
     for( Index i = strt ; i < stop ; ) {
      static_cast< LinearFunction * >( cit->get_function() )->
       modify_coefficients( std::move( RealVector( f_polyf.get_A()[ i ] ) ) ,
			    rng , par );
      if( f_polyf.is_convex() )
       (cit++)->set_lhs( f_polyf.get_b()[ i++ ] , par );
      else
       (cit++)->set_rhs( f_polyf.get_b()[ i++ ] , par );
      }
     }
    else  // modify constants only
     if( f_polyf.is_convex() )
      for( Index i = strt ; i < stop ; )
       cit->set_lhs( f_polyf.get_b()[ i++ ] , par );
     else
      for( Index i = strt ; i < stop ; )
       cit->set_rhs( f_polyf.get_b()[ i++ ] , par );

   if( newchnl ) {
    if( chnl )
     un_nest_channel( ichnl );
    else
     close_channel( ichnl );
    }
   return;
   }
  }

 // PolyhedralFunctionModSbst - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<PolyhedralFunctionModSbst>( mod
									  );
  if( tmod ) {  // this is "modify/delete a subset of rows"

   // open a new GroupModification, not concerning PolyhedralFunctionBlock
   // unless it's deleting or only one row and *not* also its constant
   bool newchnl = ( tmod->PFtype() != PolyhedralFunctionMod::DeleteRows ) &&
                    ( ( tmod->rows().size() > 1 ) ||
		      ( tmod->PFtype() == PolyhedralFunctionMod::ModifyCnst )
		      );
   auto ichnl = open_or_nest( newchnl , chnl );
   auto par = make_par( eNoBlck , ichnl );

   Index prev = 0;
   auto cit = f_const.begin();
   auto rit = tmod->rows().begin();
   if( tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
    // delete rows
    std::vector< std::list< FRowConstraint >::iterator > rmvd(
						      tmod->rows().size() );
    auto rmvdit = rmvd.begin();
    for( ; rit != tmod->rows().end() ; ) {
     cit = std::next( cit , *rit - prev );
     *(rmvdit++) = cit;
     prev = *(rit++);
     }
    remove_dynamic_constraints( f_const , rmvd , par );
    }
   else
    if( tmod->PFtype() == PolyhedralFunctionMod::ModifyRows ) {
     // modify rows & constants
     Range rng = Range( 1 , f_polyf.get_num_active_var() + 1 );
     for( ; rit != tmod->rows().end() ; ) {
      cit = std::next( cit , *rit - prev );
      static_cast< LinearFunction * >( cit->get_function() )->
       modify_coefficients(
	    std::move( RealVector( f_polyf.get_A()[ *rit ] ) ) , rng , par );
      if( f_polyf.is_convex() )
       cit->set_lhs( f_polyf.get_b()[ *rit ] , par );
      else
       cit->set_rhs( f_polyf.get_b()[ *rit ] , par );
      prev = *(rit++);
      }
     }
    else  // modify constants only
     for( ; rit != tmod->rows().end() ; ) {
      cit = std::next( cit , *rit - prev );
      if( f_polyf.is_convex() )
       cit->set_lhs( f_polyf.get_b()[ *rit ] , par );
      else
       cit->set_rhs( f_polyf.get_b()[ *rit ] , par );
      prev = *(rit++);
      }
 
   if( newchnl ) {
    if( chnl )
     un_nest_channel( ichnl );
    else
     close_channel( ichnl );
    }
   return;
   }
  }

 // PolyhedralFunctionModAddd - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<PolyhedralFunctionModAddd>( mod
									  );
  if( tmod ) {  // this is "add new rows"
   Index nr = f_polyf.get_A().size();
   std::list< FRowConstraint > newc( tmod->addedrows() );
   auto cit = newc.begin();
   for( Index i = nr - tmod->addedrows() ; i < nr ; )
    ConstructLPConstraint( i++ , *(cit++) );

   add_dynamic_constraints( f_const , newc , make_par( eNoBlck , chnl ) );
   return;
   }
  }

 // C05FunctionMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<C05FunctionMod>( mod );
  if( tmod ) {  // this is a change of the "verse" of the PolyhedralFunction
   if( tmod->type() != C05FunctionMod::NothingChanged )
    throw( std::logic_error( "wrong C05FunctionMod in PolyhedralFunction" ) );

   // open a new GroupModification, not concerning PolyhedralFunctionBlock
   auto ichnl = open_or_nest( true , chnl );
   auto par = make_par( eNoBlck , ichnl );
   Index i = 0;

   if( f_polyf.is_convex() ) {
    // change the "verse" of the objective accordingly
    get_objective()->set_sense( Objective::eMin , par );

    // set upper/lower bound on v
    f_bcv.set_lhs( f_polyf.get_lower_estimate() , par );
    f_bcv.set_rhs( Inf< Function::FunctionValue >() , par );

    // properly set the lhs/rhs of the constraints
    for( auto & ci : f_const ) {
     ci.set_lhs( f_polyf.get_b()[ i++ ] , par );
     ci.set_rhs( Inf< Function::FunctionValue >() , par );
     }
    }
   else {
    // change the "verse" of the objective accordingly
    get_objective()->set_sense( Objective::eMax , par );

    // properly set upper/lower bound on v
    f_bcv.set_lhs( - Inf< Function::FunctionValue >() , par );
    f_bcv.set_rhs( f_polyf.get_upper_estimate() , par );

    // properly set the lhs/rhs of the constraints
    for( auto & ci : f_const ) {
     ci.set_lhs( - Inf< Function::FunctionValue >() , par );
     ci.set_rhs( f_polyf.get_b()[ i++ ] , par );
     }
    }

   if( chnl )
    un_nest_channel( ichnl );
   else
    close_channel( ichnl );
   return;
   }
  }

 // FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // if all else fails, this must be a "simple" FunctionMod, whose
 // meaning is "everything is changed", hence change everything

 assert( std::isnan( mod->shift() ) );

 // set upper/lower bound on v
 f_bcv.set_rhs( f_polyf.get_upper_estimate() , eNoMod );
 f_bcv.set_lhs( f_polyf.get_lower_estimate() , eNoMod );

 // clear out the linear constraints
 for( auto & ci : f_const )
  ci.clear();
 f_const.clear();

 // now add the linear constraints back again
 f_const.resize( f_polyf.get_A().size() );
 auto cit = f_const.begin();
 for( Index i = 0 ; i < f_polyf.get_A().size() ; )
  ConstructLPConstraint( i++ , *(cit++) );
 
 // finally issue a NBModification
 AbstractBlock::add_Modification( std::make_shared<NBModification>( this ) );

 }  // end( PolyhedralFunctionBlock::guts_of_add_Modification_PF )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::guts_of_add_Modification_LR( sp_Mod mod ,
							   ChnlName chnl )
{
 // process a Modification produced by the "linearized" representation - - - -
 /* This requires to patiently sift through the possible Modification types
  * find what this Modification exactly, is and appropriately mirror the
  * changes of the "abstract" representation into the PolyhedralFunction
  * (which in this case counts as the "physical" one). Note, however, that
  *
  *     SOME Modification OF THE LP ARE NOT SUPPORTED SINCE THEY WOULD
  *     LEAVE THE PolyhedralFunction IN AN INCONSISTENT STATE
  */

 // BlockModAdd< FRowConstraint > - - - - - - - - - - - - - - - - - - - - - -
 // adding a dynamic constraint
 {
  const auto tmod =
            std::dynamic_pointer_cast< BlockModAdd< FRowConstraint > >( mod );
  if( tmod ) {
   if( & tmod->whc() == & f_const ) {
    throw( std::logic_error( "adding FRowConstraint is not handled yet" ) );
    }

   return;  // if it's not about f_const, none of my business
   }
  }

 // BlockModRmv< FRowConstraint > - - - - - - - - - - - - - - - - - - - - - -
 // removing a dynamic constraint
 {
  const auto tmod =
            std::dynamic_pointer_cast< BlockModRmv< FRowConstraint > >( mod );
  if( tmod ) {
   if( & tmod->whc() == & f_const ) {
    throw( std::logic_error( "removing FRowConstraint is not handled yet" ) );
    }

   return;  // if it's not about f_const, none of my business
   }
  }

 // ObjectiveMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<ObjectiveMod>( mod );
  if( tmod )
   throw( std::logic_error(
		   "ObjectiveMod not allowed in PolyhedralFunctionBlock" ) );
  }

 // RowConstraintMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<RowConstraintMod>( mod );
  if( tmod ) {
   // first check if it's about the box constraint on v
   if( & f_bcv == tmod->constraint() ) {
    if( ( tmod->type() == RowConstraintMod::eChgBTS ) ||
	( ( tmod->type() == RowConstraintMod::eChgRHS ) &&
	  f_polyf.is_convex() ) ||
	( ( tmod->type() == RowConstraintMod::eChgLHS ) &&
	  ( ! f_polyf.is_convex() ) ) )
     throw( std::logic_error(
		    "wrong RowConstraintMod in PolyhedralFunctionBlock" ) );

    f_polyf.modify_bound( f_polyf.is_convex() ? f_bcv.get_lhs()
			                      : f_bcv.get_rhs() ,
			  make_par( eNoBlck , chnl ) );
    return;
    }

   // now check if it's about one linear constraint
   Index i = 0;
   auto ci = f_const.begin();
   for( ; ci != f_const.end() ; ++ci , ++i )
    if( & (*ci) == tmod->constraint() )
     break;

   if( ci == f_const.end() )  // that's not in the linearized representation
    return;                   // none of my business

   if( ( tmod->type() == RowConstraintMod::eChgBTS ) ||
       ( ( tmod->type() == RowConstraintMod::eChgRHS ) &&
	 f_polyf.is_convex() ) ||
       ( ( tmod->type() == RowConstraintMod::eChgLHS ) &&
	 ( ! f_polyf.is_convex() ) ) )
    throw( std::logic_error(
		    "wrong RowConstraintMod in PolyhedralFunctionBlock" ) );

   f_polyf.modify_constant( i , f_polyf.is_convex() ? ci->get_lhs()
			                            : ci->get_rhs() ,
			    make_par( eNoBlck , chnl ) );
   return;
   }
  }

 // VariableMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<VariableMod>( mod );
  if( tmod ) {
   if( tmod->variable() == & f_v )
    throw( std::logic_error(
		          "wrong VariableMod in PolyhedralFunctionBlock" ) );
   return;  // if it's not about v, none of my business
   }
  }

 // C05FunctionModLinRngd - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<C05FunctionModLinRngd>( mod );
  if( tmod ) {
   Index i = 0;
   auto ci = f_const.begin();
   for( ; ci != f_const.end() ; ++ci , ++i )
    if( ci->get_function() == tmod->function() )
     break;

   if( ci == f_const.end() )  // that's not in the linearized representation
    return;                   // none of my business

   RealVector ai( f_polyf.get_A()[ i ] );
   for( Index j = 0 ; j < tmod->delta().size() ; ++j )
    ai[ tmod->range().first + j - 1 ] += tmod->delta()[ j ];

   f_polyf.modify_row( i , std::move( ai ) , f_polyf.get_b()[ i ] ,
		       make_par( eNoBlck , chnl ) );
   return;
   }
  }

 // C05FunctionModLinSbst - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  const auto tmod = std::dynamic_pointer_cast<C05FunctionModLinSbst>( mod );
  if( tmod ) {
   Index i = 0;
   auto ci = f_const.begin();
   for( ; ci != f_const.end() ; ++ci , ++i )
    if( ci->get_function() == tmod->function() )
     break;

   if( ci == f_const.end() )  // that's not in the linearized representation
    return;                   // none of my business

   RealVector ai( f_polyf.get_A()[ i ] );
   for( Index j = 0 ; j < tmod->subset().size() ; ++j )
    ai[ tmod->subset()[ j ] - 1 ] += tmod->delta()[ j ];

   f_polyf.modify_row( i , std::move( ai ) , f_polyf.get_b()[ i ] ,
		       make_par( eNoBlck , chnl ) );
   return;
   }
  }

 // FunctionModVars - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // any addition/removal of Variables in the linearized representation is bad
 {
  const auto tmod = std::dynamic_pointer_cast<FunctionModVars>( mod );
  if( tmod ) {
   auto ci = f_const.begin();
   for( ; ci != f_const.end() ; ++ci )
    if( ci->get_function() == tmod->function() )
     break;

   if( ci != f_const.end() )  // it's in the linearized representation
    throw( std::logic_error(
	             "wrong FunctionModVars in PolyhedralFunctionBlock" ) );
 
   return;  // else, none of my business
   }
  }

 // FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // that's changing the constant, not good either
 {
  const auto tmod = std::dynamic_pointer_cast<FunctionMod>( mod );
  if( tmod ) {
   auto ci = f_const.begin();
   for( ; ci != f_const.end() ; ++ci )
    if( ci->get_function() == tmod->function() )
     break;

   if( ci != f_const.end() )  // it's in the linearized representation
    throw( std::logic_error(
			  "wrong FunctionMod in PolyhedralFunctionBlock" ) );
 
   return;  // else, none of my business
   }
  }
 }  // end( PolyhedralFunctionBlock::guts_of_add_Modification_LR )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::ConstructLPConstraint( Index i ,
						     FRowConstraint & ci )
{
 // if the PolyhedralFunction is convex, then the constraint is
 // b_i <= v - A_i x <= INF, otherwise it is - INF <= v - A_i x <= b_i
 ci.set_lhs( f_polyf.is_convex() ? f_polyf.get_b()[ i ]
	                         : - Inf< Function::FunctionValue >() ,
	     eNoMod );
 ci.set_rhs( f_polyf.is_convex() ? Inf< Function::FunctionValue >()
	                         : f_polyf.get_b()[ i ] ,
	     eNoMod );

 const auto nv = f_polyf.get_num_active_var();
 LinearFunction::v_coeff_pair vars( nv + 1 );
 auto vit = vars.begin();

 // v is the *first* Variable of the LinearFunction, since it is the only
 // one that "never moves"; as a consequence, x[ i ] is the (i+1)-th active
 // Variable in each constraint
 *(vit++) = std::make_pair( & f_v , 1 );

 auto Aiit = f_polyf.get_A()[ i ].begin(); 
 for( Index j = 0 ; j < nv ; ++j )
  *(vit++) = std::make_pair( static_cast< ColVariable * >(
					      f_polyf.get_active_var( j ) ) ,
			     - *(Aiit++) );

 ci.set_function( new LinearFunction( std::move( vars ) ) , eNoMod );

 }  // end( PolyhedralFunctionBlock::ConstructLPConstraint )

/*--------------------------------------------------------------------------*/
/*--------------- End File PolyhedralFunctionBlock.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
