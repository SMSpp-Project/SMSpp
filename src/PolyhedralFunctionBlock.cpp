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

 if( ( ! tstvv ) && f_BlockConfig && f_BlockConfig->f_solution_Configuration )
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
/*------------------------- Methods for R3 Blocks --------------------------*/
/*--------------------------------------------------------------------------*/

Block * PolyhedralFunctionBlock::get_R3_Block( Configuration *r3bc ,
					       Block * base )
{
 if( r3bc != nullptr )
  throw( std::invalid_argument( "non-nullptr R3B Configuration" ) );

 PolyhedralFunctionBlock *PFB;
 if( base ) {
  PFB = dynamic_cast< PolyhedralFunctionBlock * >( base );
  if( ! PFB )
   throw( std::invalid_argument( "base is not a PolyhedralFunctionBlock" ) );
  }
 else
  PFB = new PolyhedralFunctionBlock();

 PFB->f_polyf.set_PolyhedralFunction(
		   PolyhedralFunction::MultiVector( f_polyf.get_A() ) ,
		   PolyhedralFunction::RealVector( f_polyf.get_b() ) ,
		   f_polyf.get_global_bound() , f_polyf.is_convex() ,
		   eNoMod );
 return( PFB );

 }  // end( MCFBlock::get_R3_Block )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::map_forward_Modification(
			      Block *R3B , sp_Mod mod , Configuration *r3bc ,
			      ModParam issuePMod , ModParam issueAMod )
{
 if( mod->concerns_Block() )  // an abstract Modification
  return( false );            // none of my business

 auto PFB = dynamic_cast< PolyhedralFunctionBlock * >( R3B );
 if( ! PFB )
  throw( std::invalid_argument( "R3B is not a PolyhedralFunctionBlock" ) );
 if( r3bc != nullptr )
  throw( std::invalid_argument( "non-nullptr R3B Configuration" ) );

 /* Note that issueAMod is completely ignored because we only perform
    physical Modification; the "translation" to "abstract" Modification, if
    ever, will be done by PFB->add_Modification() when it receives the
    physical one generated here. */

 /* When a GroupModification is processed, if no channel is provided, then
    one is opened. This only happens "at root", after which in guts_of_mfM()
    whenever a GroupModification is processed, then the channel is nested.
    Indeed, if the "root" Modification is not a GroupModification, then there
    cannot be any GroupModification in it. */

 ModParam iPM = issuePMod;

 /* Use a Lambda to define a "guts" of the method that can be called
    recursively without having to pass "local globals". Note the trick of
    defining the std::function object and "passing" it to the lambda,
    which allows recursive calls. Note the need to explicitly capture
    "this" to use fields/methods of the class. */

 std::function< bool( sp_Mod ) > guts_of_mfM;
 guts_of_mfM = [ this , & guts_of_mfM , & PFB , & iPM ]( sp_Mod mod ) {
  // process Modification- - - - - - - - - - - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  /* This requires to patiently sift through the possible Modification types
     to find what this Modification exactly is, and call the appropriate
     method changing the "physical representation" of PFB. */

  // GroupModification - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod = std::dynamic_pointer_cast<GroupModification>( mod );
   if( tmod ) {
    PFB->nest_channel( par2chnl( iPM ) );     // nest the channel

    bool ok = true;
    for( const auto & submod : tmod->sub_Modifications() )
     if( ! guts_of_mfM( submod ) )
      ok = false;

    PFB->un_nest_channel( par2chnl( iPM ) );  // un-nest the channel

    return( ok );
    }
   }

  // PolyhedralFunctionModRngd - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod =
                std::dynamic_pointer_cast< PolyhedralFunctionModRngd >( mod );
   if( tmod ) {
    if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
     return( false );                    // none of my business

    Index n = tmod->range().second - tmod->range().first;
    switch( tmod->PFtype() ) {
     case( PolyhedralFunctionMod::ModifyRows ):
      if( n == 1 )
       PFB->f_polyf.modify_row( tmod->range().first ,
				 PolyhedralFunction::RealVector(
				   f_polyf.get_A()[ tmod->range().first ] ) ,
				 f_polyf.get_b()[ tmod->range().first ] ,
				 iPM );
      else {
       PolyhedralFunction::MultiVector nA( n );
       PolyhedralFunction::RealVector nb( n );
       Index j = 0;
       for( Index i = tmod->range().first ; i < tmod->range().second ; ) {
	nA[ j ] = f_polyf.get_A()[ i ];
	nb[ j++ ] = f_polyf.get_b()[ i++ ];
        }
       
       PFB->f_polyf.modify_rows( std::move( nA ) , std::move( nb ) ,
				  tmod->range() , iPM );
       }
      break;
     case( PolyhedralFunctionMod::ModifyCnst ):
      if( n == 0 ) {
       PFB->f_polyf.modify_bound(  f_polyf.get_global_bound() , iPM );
       break;
       }
       
      if( n == 1 )
       PFB->f_polyf.modify_constant( tmod->range().first ,
			    f_polyf.get_b()[ tmod->range().first ] , iPM );
      else {
       PolyhedralFunction::RealVector nb( n );
       auto bit = nb.begin();
       for( Index i = tmod->range().first ; i < tmod->range().second ; )
	*(bit++) = f_polyf.get_b()[ i++ ];
       
       PFB->f_polyf.modify_constants( std::move( nb ) ,
				       tmod->range() , iPM );
       }
      break;
     case( PolyhedralFunctionMod::DeleteRows ):
      if( n == 1 )
       PFB->f_polyf.delete_row( tmod->range().first , iPM );
      else
       PFB->f_polyf.delete_rows( tmod->range() , iPM );
      break;
     default:
      throw( std::invalid_argument(
			      "unknown PolyhedralFunctionModRngd PFtype" ) );
     }
    return( true );
    }
   }

  // PolyhedralFunctionModSbst - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod =
                std::dynamic_pointer_cast< PolyhedralFunctionModSbst >( mod );
   if( tmod ) {
    if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
     return( false );                    // none of my business

    Index n = tmod->rows().size();
    switch( tmod->PFtype() ) {
     case( PolyhedralFunctionMod::ModifyRows ):
      if( n == 1 )
       PFB->f_polyf.modify_row( tmod->rows()[ 0 ] ,
				 PolyhedralFunction::RealVector(
				    f_polyf.get_A()[ tmod->rows()[ 0 ] ] ) ,
				 f_polyf.get_b()[ tmod->rows()[ 0 ] ] ,
				 iPM );
      else {
       PolyhedralFunction::MultiVector nA( n );
       PolyhedralFunction::RealVector nb( n );
       Index j = 0;
       for( auto i : tmod->rows() ) {
	nA[ j ] = f_polyf.get_A()[ i ];
	nb[ j++ ] = f_polyf.get_b()[ i ];
        }
       
       PFB->f_polyf.modify_rows( std::move( nA ) , std::move( nb ) ,
				 Subset( tmod->rows() ) , true , iPM );
       }
      break;
     case( PolyhedralFunctionMod::ModifyCnst ):
      if( n == 1 )
       PFB->f_polyf.modify_constant( tmod->rows()[ 0 ] ,
				     f_polyf.get_b()[ tmod->rows()[ 0 ] ] ,
				     iPM );
      else {
       PolyhedralFunction::RealVector nb( n );
       auto bit = nb.begin();
       for( auto i : tmod->rows() )
	*(bit++) = f_polyf.get_b()[ i ];
       
       PFB->f_polyf.modify_constants( std::move( nb ) ,
				      Subset( tmod->rows() ) , true , iPM );
       }
      break;
     case( PolyhedralFunctionMod::DeleteRows ):
      if( n == 1 )
       PFB->f_polyf.delete_row( tmod->rows()[ 0 ] , iPM );
      else
       PFB->f_polyf.delete_rows( Subset( tmod->rows() ) , true , iPM );
      break;
     default:
      throw( std::invalid_argument(
			      "unknown PolyhedralFunctionModRngd PFtype" ) );
     }
    return( true );
    }
   }

  // PolyhedralFunctionModAddd - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod =
                std::dynamic_pointer_cast< PolyhedralFunctionModAddd >( mod );
   if( tmod ) {
    if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
     return( false );                    // none of my business

    return( true );  // pretend we have done it, which is impossible
                     // see comments for rationale
    }
   }

  // C05FunctionModVarsRngd- - - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod =
                   std::dynamic_pointer_cast< C05FunctionModVarsRngd >( mod );
   if( tmod ) {
    if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
     return( false );                    // none of my business

    if( tmod->range().second == tmod->range().first + 1 )
     PFB->f_polyf.remove_variable( tmod->range().first , iPM );
    else
     PFB->f_polyf.remove_variables( tmod->range() , iPM );
     
    return( true );
    }
   }

  // C05FunctionModVarsSbst- - - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod =
                   std::dynamic_pointer_cast< C05FunctionModVarsSbst >( mod );
   if( tmod ) {
    if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
     return( false );                    // none of my business

    PFB->f_polyf.remove_variables( Subset( tmod->subset() ) , iPM );     
    return( true );
    }
   }

  // PolyhedralFunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod =
                    std::dynamic_pointer_cast< PolyhedralFunctionMod >( mod );
   if( tmod ) {
    if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
     return( false );                    // none of my business

    if( tmod->type() != C05FunctionMod::NothingChanged )
     throw( std::invalid_argument( "unexpected type() in C05FunctionMod" ) );

    PFB->f_polyf.set_is_convex( f_polyf.is_convex() , iPM );
     
    return( true );
    }
   }

  // FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  {
   const auto tmod = std::dynamic_pointer_cast< FunctionMod >( mod );
   if( tmod ) {
    // "nuclear Modification for Function": everything changed

    if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
     return( false );                    // none of my business

    if( ! std::isnan( tmod->shift() ) )
     throw( std::invalid_argument( "unexpected shift() in FunctionMod" ) );

    PFB->f_polyf.set_PolyhedralFunction(
		    PolyhedralFunction::MultiVector( f_polyf.get_A() ) ,
		    PolyhedralFunction::RealVector( f_polyf.get_b() ) ,
		    f_polyf.get_global_bound() , f_polyf.is_convex() , iPM );
    return( true );
    }
   }

  return( false );

  };  // end( guts_of_mfM )- - - - - - - - - - - - - - - - - - - - - - - - - -
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // finally, call the "guts of"- - - - - - - - - - - - - - - - - - - - - - - -
 // this is done differently if mod is a GroupModification, since at the root
 // a channel has to be opened while further down it has to be nested

 bool ok = true;  // final return value

 const auto tmod = std::dynamic_pointer_cast< GroupModification >( mod );
 if( tmod ) {                    // this is a GroupModification
  if( ! par2chnl( issuePMod ) )  // and the channel is the default one
                                 // open a new channel and use it instead
   iPM = make_par( par2concern( issuePMod ) , PFB->open_channel() );

  for( const auto & submod : tmod->sub_Modifications() )  // for each sub-Mod
   if( ! guts_of_mfM( submod ) )                          // make the call
    ok = false;

  if( iPM != issuePMod )                   // a channel had been opened
   PFB->close_channel( par2chnl( iPM ) );  // close it
 }
 else                            // any other Modification
  ok = guts_of_mfM( mod );       // just make the call

 return( ok );

 }  // end( PFBlock::map_forward_Modification )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::map_back_Modification(
			      Block *R3B , sp_Mod mod , Configuration *r3bc ,
			      ModParam issuePMod , ModParam issueAMod )
{
 /* Fantastically dirty trick: because the two objects are copies, mapping
    back a Modification to this from R3B is the same as mapping forward a
    Modification from R3B to this. */

 auto PFB = dynamic_cast< PolyhedralFunctionBlock * >( R3B );
 if( ! PFB )
  throw( std::invalid_argument( "R3B is not a PolyhedralFunctionBlock" ) );

 return( PFB->map_forward_Modification( this , mod , r3bc , issuePMod ,
					issueAMod ) );

 }  // end( PolyhedralFunctionBlock::map_back_Modification )

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

   if( tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
    // delete rows
    remove_dynamic_constraints( f_const , tmod->range() , par );
    }
   else {
    auto cit = f_const.size() - strt < strt ?
	       std::prev( f_const.end() , f_const.size() - strt ) :
               std::next( f_const.begin() , strt );

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
    remove_dynamic_constraints( f_const , Subset( tmod->rows() ) , par );
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
   if( & tmod->whc() != & f_const )   // if it's not about f_const
    return;                           // none of my business

   const auto & arr = tmod->added();

   if( arr.empty() )  // should not happen, but in case
    return;           // nothing to do
   PolyhedralFunction::MultiVector A( arr.size() );
   PolyhedralFunction::RealVector b( arr.size() );

   Index i = 0;
   for( auto ci : arr ) {
    // recover the constant = RHS (easy)
    b[ i ] = f_polyf.is_convex() ? ci->get_lhs() : ci->get_rhs();

    // now the though part: recover the linearization
    auto lf = dynamic_cast< LinearFunction * >( ci->get_function() );
    if( ! lf )
     throw( std::logic_error( "FRowConstraint with no LinearFunction" ) );

    const auto & coeff = lf->get_v_var();

    if( coeff.size() != f_polyf.get_num_active_var() )
     throw( std::logic_error( "incorrect LinearFunction in FRowConstraint" ) );

    #ifndef NDEBUG
    // TODO: check that the Variables actually are the same
    #endif

    for( Index j = 0 ; j < coeff.size() ; ++j )
     A[ i ][ j ] = - coeff[ j ].second;

    ++i;
    }

   f_polyf.add_rows( std::move( A ) , b , make_par( eNoBlck , chnl ) );

   throw( std::logic_error( "adding FRowConstraint is not handled yet" ) );
   }
  }

 // BlockModRmv< FRowConstraint > - - - - - - - - - - - - - - - - - - - - - -
 // removing a dynamic constraint
 {
  const auto tmod =
            std::dynamic_pointer_cast< BlockModRmv< FRowConstraint > >( mod );
  if( tmod ) {
   if( & tmod->whc() != & f_const )   // if it's not about f_const
    return;                           // none of my business

   const auto & rmvd = tmod->removed();
   if( rmvd.empty() )  // should not happen, but in case
    return;            // nothing to do
   /*
   Subset nms( rmvd.size() );
   for( auto rit = rmvd.begin() ; rit != rmvd.end() ; ++rit ) {
    auto it = std::find();
    }
   */
   throw( std::logic_error( "removing FRowConstraint is not handled yet" ) );
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
