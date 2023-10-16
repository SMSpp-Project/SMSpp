/*--------------------------------------------------------------------------*/
/*------------------- File PolyhedralFunctionBlock.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the PolyhedralFunctionBlock class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
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
 auto tstvv = dynamic_cast< SimpleConfiguration< int > * >( stvv );

 if( ( ! tstvv ) && f_BlockConfig &&
     f_BlockConfig->f_static_variables_Configuration )
  tstvv = dynamic_cast< SimpleConfiguration< int > * >(
			    f_BlockConfig->f_static_variables_Configuration );
 if( tstvv )
  wsol = tstvv->f_value;

 if( wsol )
  f_rep |= 1;

 if( f_rep & 1 ) {  // use linearized representation
  // note: the static ColVariable "v" is added "in front"
  f_1st_stat_var = 1;
  add_static_variable( f_v , "PolyF_v" , true );
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
  f_1st_dyn_cnst = 1;
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

 set_objective( obj , eNoMod );

 f_rep |= 8;

 }  // end( PolyhedralFunctionBlock::generate_objective )

/*--------------------------------------------------------------------------*/
/*------- Methods for reading the data of the PolyhedralFunctionBlock ------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------- Methods for R3 Blocks --------------------------*/
/*--------------------------------------------------------------------------*/

Block * PolyhedralFunctionBlock::get_R3_Block( Configuration *r3bc ,
					       Block * base , Block * father )
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
  PFB = new PolyhedralFunctionBlock( father );

 PFB->f_polyf.set_PolyhedralFunction( MultiVector( f_polyf.get_A() ) ,
				      RealVector( f_polyf.get_b() ) ,
				      f_polyf.get_global_bound() ,
				      f_polyf.is_convex() , eNoMod );
 return( PFB );

 }  // end( MCFBlock::get_R3_Block )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::map_forward_Modification(
			   Block * R3B , c_p_Mod mod , Configuration * r3bc ,
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

 /* Use a Lambda to define a "guts" of the method that can be called
    recursively without having to pass "local globals". Note the trick of
    defining the std::function object and "passing" it to the lambda,
    which allows recursive calls. Note the need to explicitly capture
    "this" to use fields/methods of the class. */

 std::function< bool( c_p_Mod , ModParam ) > guts_of_mfM;
 guts_of_mfM = [ this , & guts_of_mfM , & PFB ]( c_p_Mod mod ,
						 ModParam iPM ) {
  // process Modification- - - - - - - - - - - - - - - - - - - - - - - - - - -
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  /* This requires to patiently sift through the possible Modification types
     to find what this Modification exactly is, and call the appropriate
     method changing the "physical representation" of PFB. */

  // GroupModification - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const GroupModification * >( mod ) ) {
   auto niPM = make_par( par2mod( iPM ) ,
			 PFB->open_channel( par2chnl( iPM ) ) );
   bool ok = true;
   for( const auto & submod : tmod->sub_Modifications() )  // for each sub-Mod
    if( ! guts_of_mfM( submod.get() , niPM ) )             // make the call
     ok = false;

   PFB->close_channel( par2chnl( niPM ) );  // close it
   return( ok );
   }

  // PolyhedralFunctionModAddd - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const PolyhedralFunctionModAddd * >( mod ) ) {
   if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
    return( false );                    // none of my business

   Index nr = f_polyf.get_A().size();
   MultiVector nA( tmod->addedrows() );
   RealVector nb( tmod->addedrows() );
   Index j = 0;
   for( Index i = nr - tmod->addedrows() ; i < nr ; ) {
    nA[ j ] = f_polyf.get_A()[ i ];
    nb[ j++ ] = f_polyf.get_b()[ i++ ];
    }
       
   PFB->f_polyf.add_rows( std::move( nA ) , nb , iPM );
   return( true );
   }

  // PolyhedralFunctionModRngd - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const PolyhedralFunctionModRngd * >( mod ) ) {
   if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
    return( false );                    // none of my business

   Index n = tmod->range().second - tmod->range().first;
   switch( tmod->PFtype() ) {
    case( PolyhedralFunctionMod::ModifyRows ):
     if( n == 1 )
      PFB->f_polyf.modify_row( tmod->range().first ,
			       RealVector(
				   f_polyf.get_A()[ tmod->range().first ] ) ,
			      f_polyf.get_b()[ tmod->range().first ] , iPM );
     else {
      MultiVector nA( n );
      RealVector nb( n );
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
				    f_polyf.get_b()[ tmod->range().first ] ,
				    iPM );
     else {
      RealVector nb( n );
      auto bit = nb.begin();
      for( Index i = tmod->range().first ; i < tmod->range().second ; )
       *(bit++) = f_polyf.get_b()[ i++ ];

      PFB->f_polyf.modify_constants( std::move( nb ) , tmod->range() , iPM );
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

  // PolyhedralFunctionModSbst - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const PolyhedralFunctionModSbst * >( mod ) ) {
   if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
    return( false );                    // none of my business

   Index n = tmod->rows().size();
   switch( tmod->PFtype() ) {
    case( PolyhedralFunctionMod::ModifyRows ):
     if( n == 1 )
      PFB->f_polyf.modify_row( tmod->rows()[ 0 ] ,
			       RealVector(
				    f_polyf.get_A()[ tmod->rows()[ 0 ] ] ) ,
				f_polyf.get_b()[ tmod->rows()[ 0 ] ] , iPM );
     else {
      MultiVector nA( n );
      RealVector nb( n );
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
      RealVector nb( n );
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

  // C05FunctionModVarsAddd- - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const C05FunctionModVarsAddd * >( mod ) ) {
   if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
    return( false );                    // none of my business

   return( true );  // pretend we have done it, which is impossible
                    // see comments for rationale
   }

  // C05FunctionModVarsRngd- - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const C05FunctionModVarsRngd * >( mod ) ) {
   if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
    return( false );                    // none of my business

   if( tmod->range().second == tmod->range().first + 1 )
    PFB->f_polyf.remove_variable( tmod->range().first , iPM );
   else
    PFB->f_polyf.remove_variables( tmod->range() , iPM );

   return( true );
   }

  // C05FunctionModVarsSbst- - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const C05FunctionModVarsSbst * >( mod ) ) {
   if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
    return( false );                    // none of my business

   PFB->f_polyf.remove_variables( Subset( tmod->subset() ) , iPM );     
   return( true );
   }

  // PolyhedralFunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const PolyhedralFunctionMod * >( mod ) ) {
   if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
    return( false );                    // none of my business

   if( tmod->type() != C05FunctionMod::NothingChanged )
    throw( std::invalid_argument( "unexpected type() in C05FunctionMod" ) );

   PFB->f_polyf.set_is_convex( f_polyf.is_convex() , iPM );
     
   return( true );
   }

  // FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  if( auto tmod = dynamic_cast< const FunctionMod * >( mod ) ) {
   // "nuclear Modification for Function": everything changed

   if( tmod->function() != & f_polyf )  // not my PolyhedralFunction
    return( false );                    // none of my business

   if( ! std::isnan( tmod->shift() ) )
    throw( std::invalid_argument( "unexpected shift() in FunctionMod" ) );

   PFB->f_polyf.set_PolyhedralFunction( MultiVector( f_polyf.get_A() ) ,
					RealVector( f_polyf.get_b() ) ,
					f_polyf.get_global_bound() ,
					f_polyf.is_convex() , iPM );
   return( true );
   }

  return( false );

  };  // end( guts_of_mfM )- - - - - - - - - - - - - - - - - - - - - - - - - -
      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // finally, call the "guts of"- - - - - - - - - - - - - - - - - - - - - - - -
 return( guts_of_mfM( mod , issuePMod ) );

 }  // end( PolyhedralFunctionBlock::map_forward_Modification )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::map_back_Modification(
			      Block *R3B , c_p_Mod mod , Configuration *r3bc ,
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
/*------- METHODS FOR PRINTING & SAVING THE PolyhedralFunctionBlock --------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::print( std::ostream & output , char vlvl ) const
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

 if( vlvl ) {
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
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

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
  Constraint::clear( f_const );
  f_bcv.clear();

  // then nothing, they will be deleted when f_const/f_bcv are

  // ensure that the LinearFunction inside the Objective is deleted
  if( obj )
   obj->set_function( nullptr , eNoMod , true );
  }
 else {             // use natural representation
  // ensure that the PolyhedralFunction inside the Objective is NOT deleted
  if( obj )
   obj->set_function( nullptr , eNoMod , false );
  }

 // finally delete the Objective
 delete obj;

 }  // end( PolyhedralFunctionBlock::guts_of_destructor )

/*--------------------------------------------------------------------------*/

bool PolyhedralFunctionBlock::guts_of_add_Modification_PF(
				    const FunctionMod * mod , ChnlName chnl )
{
 // process a FunctionMod produced by the PolyhedralFunction- - - - - - - - -
 /* This requires to patiently sift through the possible Modification types
  * (but only those derived from FunctionMod) to find what this Modification
  * exactly is, and appropriately mirror the changes to the PolyhedralFunction
  * (which in this case counts as the "physical representation") into the
  * "abstract" one, i.e., performing the corresponding changes on the LP. */

 // C05FunctionModVarsAddd- - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod =  dynamic_cast< const C05FunctionModVarsAddd * >( mod ) ) {
  c_Index frst = tmod->first();
  c_Index nav = f_polyf.get_num_active_var();

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , f_const.size() );

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

  close_if_needed( par , f_const.size() );
  return( false );
  }

 // C05FunctionModVarsRngd- - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionModVarsRngd * >( mod ) ) {
  // this is "remove Variables, ranged"
  auto rng = tmod->range();
  rng.first++;   // variables names in the constraints are +1 w.r.t. those
  rng.second++;  // of the PolyhedralFunction

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , f_const.size() );

  for( auto & ci : f_const )
   static_cast< LinearFunction * >( ci.get_function() )->
                                             remove_variables( rng , par );
  close_if_needed( par , f_const.size() );
  return( false );
  }

 // C05FunctionModVarsSbst- - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionModVarsSbst * >( mod ) ) {
  // this is "remove Variables, subset"
  Subset sbst( tmod->subset() );
  for( auto & si : sbst )  // variables names in the constraints are +1
   si++;                   // w.r.t. those of the PolyhedralFunction

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , f_const.size() );

  for( auto & ci : f_const )
   static_cast< LinearFunction * >( ci.get_function() )->
              remove_variables( std::move( Subset( sbst ) ) , true , par );

  close_if_needed( par , f_const.size() );
  return( false );
  }

 // PolyhedralFunctionModRngd - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const PolyhedralFunctionModRngd * >( mod ) ) {
  // this is "modify/delete a range of rows"
  Index strt = tmod->range().first;
  Index stop = tmod->range().second;

  if( strt == stop ) {  // special case: the lower/upper bound
   if( f_polyf.is_convex() )  // convex ==> lower bound
    f_bcv.set_lhs( f_polyf.get_global_bound() , make_par( eNoBlck , chnl ) );
   else                       // concave ==> upper bound
    f_bcv.set_rhs( f_polyf.get_global_bound() , make_par( eNoBlck , chnl ) );
   return( false );
   }

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  // unless it's deleting or only one row and *not* also its constant
  Index nc = tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ? 0 :
             ( tmod->PFtype() == PolyhedralFunctionMod::ModifyCnst ? 2 :
	       stop - strt );
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , nc );

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
     RealVector Ai( f_polyf.get_A()[ i ] );
     for( auto & aij : Ai )
      aij = -aij;
     static_cast< LinearFunction * >( cit->get_function() )->
                          modify_coefficients( std::move( Ai ) , rng , par );
     if( f_polyf.is_convex() )
      (cit++)->set_lhs( f_polyf.get_b()[ i++ ] , par );
     else
      (cit++)->set_rhs( f_polyf.get_b()[ i++ ] , par );
     }
    }
   else  // modify constants only
    if( f_polyf.is_convex() )
     for( Index i = strt ; i < stop ; )
      (cit++)->set_lhs( f_polyf.get_b()[ i++ ] , par );
    else
     for( Index i = strt ; i < stop ; )
      (cit++)->set_rhs( f_polyf.get_b()[ i++ ] , par );
   }

  close_if_needed( par , nc );
  return( false );
  }

 // PolyhedralFunctionModSbst - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const PolyhedralFunctionModSbst * >( mod ) ) {
  // this is "modify/delete a subset of rows"
  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  // unless it's deleting or only one row and *not* also its constant
  Index nc = tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ? 0 :
             ( tmod->PFtype() == PolyhedralFunctionMod::ModifyCnst ? 2 :
	       tmod->rows().size() );
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , nc );

  Index prev = 0;
  auto cit = f_const.begin();
  auto rit = tmod->rows().begin();
  if( tmod->PFtype() == PolyhedralFunctionMod::DeleteRows ) {
   // delete rows
   remove_dynamic_constraints( f_const , Subset( tmod->rows() ) , true ,
			       par );
   }
  else
   if( tmod->PFtype() == PolyhedralFunctionMod::ModifyRows ) {
    // modify rows & constants
    Range rng = Range( 1 , f_polyf.get_num_active_var() + 1 );
    for( ; rit != tmod->rows().end() ; ) {
     cit = std::next( cit , *rit - prev );
     RealVector Ai( f_polyf.get_A()[ *rit ] );
     for( auto & aij : Ai )
      aij = -aij;
     static_cast< LinearFunction * >( cit->get_function() )->
                          modify_coefficients( std::move( Ai ) , rng , par );
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
 
  close_if_needed( par , nc );
  return( false );
  }

 // PolyhedralFunctionModAddd - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const PolyhedralFunctionModAddd * >( mod ) ) {
  // this is "add new rows"
  Index nr = f_polyf.get_A().size();
  std::list< FRowConstraint > newc( tmod->addedrows() );
  auto cit = newc.begin();
  for( Index i = nr - tmod->addedrows() ; i < nr ; )
   ConstructLPConstraint( i++ , *(cit++) );

  add_dynamic_constraints( f_const , newc , make_par( eNoBlck , chnl ) );
  return( false );
  }

 // C05FunctionMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionMod * >( mod ) ) {
  // this is a change of the "verse" of the PolyhedralFunction
  if( tmod->type() != C05FunctionMod::NothingChanged )
   throw( std::logic_error( "wrong C05FunctionMod in PolyhedralFunction" ) );

  // open a new GroupModification, not concerning PolyhedralFunctionBlock
  auto par = open_if_needed( make_par( eNoBlck , chnl ) , 2 );
  Index i = 0;

  if( f_polyf.is_convex() ) {
   // change the "verse" of the objective accordingly
   get_objective()->set_sense( Objective::eMin , par );

   // set upper/lower bound on v
   f_bcv.set_lhs( f_polyf.get_global_lower_bound() , par );
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
   f_bcv.set_lhs( -Inf< Function::FunctionValue >() , par );
   f_bcv.set_rhs( f_polyf.get_global_upper_bound() , par );

   // properly set the lhs/rhs of the constraints
   for( auto & ci : f_const ) {
    ci.set_lhs( -Inf< Function::FunctionValue >() , par );
    ci.set_rhs( f_polyf.get_b()[ i++ ] , par );
    }
   }

  close_if_needed( par , 2 );
  return( false );
  }

 // FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // if all else fails, this must be a "simple" FunctionMod, whose
 // meaning is "everything is changed", hence change everything

 assert( std::isnan( mod->shift() ) );

 // set upper/lower bound on v
 f_bcv.set_lhs( f_polyf.get_global_lower_bound() , eNoMod );
 f_bcv.set_rhs( f_polyf.get_global_upper_bound() , eNoMod );

 // clear out the linear constraints
 f_const.clear();

 // now add the linear constraints back again
 f_const.resize( f_polyf.get_A().size() );
 auto cit = f_const.begin();
 for( Index i = 0 ; i < f_polyf.get_A().size() ; ) {
  cit->set_Block( this );
  ConstructLPConstraint( i++ , *(cit++) );
  }
 
 // finally issue a NBModification
 AbstractBlock::add_Modification( std::make_shared< NBModification >( this )
				  );
 return( true );

 }  // end( PolyhedralFunctionBlock::guts_of_add_Modification_PF )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::guts_of_add_Modification_LR( c_p_Mod mod ,
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
 if( auto tmod = dynamic_cast< const BlockModAdd< FRowConstraint > * >( mod )
     ) {
  if( & tmod->whc() != & f_const )   // if it's not about f_const
   return;                           // none of my business

  const auto & arr = tmod->added();

  if( arr.empty() )  // should not happen, but in case
   return;           // nothing to do
  MultiVector A( arr.size() );
  RealVector b( arr.size() );

  Index i = 0;
  for( auto ci : arr ) {
   // recover the constant = RHS (easy)
   b[ i ] = f_polyf.is_convex() ? ci->get_lhs() : ci->get_rhs();

   // now the though part: recover the linearization
   auto lf = dynamic_cast< LinearFunction * >( ci->get_function() );
   if( ! lf )
    throw( std::logic_error( "FRowConstraint with no LinearFunction" ) );

   const auto & coeff = lf->get_v_var();

   // note that the LinearFunction has exactly one active Variable more than
   // the PolyhedralFunction, the first one being "v"
   if( coeff.size() != f_polyf.get_num_active_var() + 1 )
    throw( std::logic_error( "incorrect LinearFunction in FRowConstraint" ) );

   #ifndef NDEBUG
   // TODO: check that the Variables actually are the same
   #endif
   A[ i ].resize( f_polyf.get_num_active_var() );

   for( Index j = 1 ; j < coeff.size() ; ++j )
    A[ i ][ j - 1 ] = - coeff[ j ].second;

   ++i;
   }

  f_polyf.add_rows( std::move( A ) , b , make_par( eNoBlck , chnl ) );
  return;
  }

 // BlockModRmvRngd< FRowConstraint > - - - - - - - - - - - - - - - - - - - -
 // removing a range of dynamic Constraint = rows of PolyhedralFunction
 if( auto tmod = dynamic_cast< const BlockModRmvRngd< FRowConstraint > *
                               >( mod ) ) {
  if( & tmod->whc() != & f_const )   // if it's not about f_const
   return;                           // none of my business

  f_polyf.delete_rows( tmod->range() , make_par( eNoBlck , chnl ) );
  return;
  }

 // BlockModRmvSbst< FRowConstraint > - - - - - - - - - - - - - - - - - - - -
 // removing a subset of dynamic Constraint = rows of PolyhedralFunction
 if( const auto tmod =
     dynamic_cast< BlockModRmvSbst< FRowConstraint > * const >( mod ) ) {
  if( & tmod->whc() != & f_const )   // if it's not about f_const
   return;                           // none of my business

  f_polyf.delete_rows( Subset( tmod->subset() ) , true ,
		       make_par( eNoBlck , chnl ) );
  return;
  }

 // ObjectiveMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const ObjectiveMod * >( mod ) )
  throw( std::logic_error(
		   "ObjectiveMod not allowed in PolyhedralFunctionBlock" ) );

 // RowConstraintMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( const auto tmod = dynamic_cast< RowConstraintMod * const >( mod ) ) {
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

 // VariableMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const VariableMod * >( mod ) ) {
  if( tmod->variable() == & f_v )
   throw( std::logic_error(
		          "wrong VariableMod in PolyhedralFunctionBlock" ) );
  return;  // if it's not about v, none of my business
  }

 // C05FunctionModLinRngd - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionModLinRngd * >( mod ) ) {
  Index i = 0;
  auto ci = f_const.begin();
  for( ; ci != f_const.end() ; ++ci , ++i )
   if( ci->get_function() == tmod->function() )
    break;

  if( ci == f_const.end() )  // that's not in the linearized representation
   return;                   // none of my business

  // note that the LinearFunction has exactly one active Variable more than
  // the PolyhedralFunction, the first one being "v", whence the "- 1"
  // also, note that the coefficients are the opposite of the entries in A;
  // hence, if the coefficients are changed by adding them tmod->delta(),
  // the entries of A must change by subtracting them tmod->delta()
  RealVector ai( f_polyf.get_A()[ i ] );
  for( Index j = 0 ; j < tmod->delta().size() ; ++j )
   ai[ tmod->range().first + j - 1 ] -= tmod->delta()[ j ];

  f_polyf.modify_row( i , std::move( ai ) , f_polyf.get_b()[ i ] ,
		      make_par( eNoBlck , chnl ) );
  return;
  }

 // C05FunctionModLinSbst - - - - - - - - - - - - - - - - - - - - - - - - - -
 if( auto tmod = dynamic_cast< const C05FunctionModLinSbst * >( mod ) ) {
  Index i = 0;
  auto ci = f_const.begin();
  for( ; ci != f_const.end() ; ++ci , ++i )
   if( ci->get_function() == tmod->function() )
    break;

  if( ci == f_const.end() )  // that's not in the linearized representation
   return;                   // none of my business

  // note that the LinearFunction has exactly one active Variable more than
  // the PolyhedralFunction, the first one being "v", whence the "- 1"
  // also, note that the coefficients are the opposite of the entries in A;
  // hence, if the coefficients are changed by adding them tmod->delta(),
  // the entries of A must change by subtracting them tmod->delta()
  RealVector ai( f_polyf.get_A()[ i ] );
  for( Index j = 0 ; j < tmod->subset().size() ; ++j )
   ai[ tmod->subset()[ j ] - 1 ] -= tmod->delta()[ j ];

  f_polyf.modify_row( i , std::move( ai ) , f_polyf.get_b()[ i ] ,
		      make_par( eNoBlck , chnl ) );
  return;
  }

 // FunctionModVars - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // any addition/removal of Variables in the linearized representation is bad
 if( auto tmod = dynamic_cast< const FunctionModVars * >( mod ) ) {
  auto ci = f_const.begin();
  for( ; ci != f_const.end() ; ++ci )
   if( ci->get_function() == tmod->function() )
    break;

  if( ci != f_const.end() )  // it's in the linearized representation
   throw( std::logic_error(
	             "wrong FunctionModVars in PolyhedralFunctionBlock" ) );
 
  return;  // else, none of my business
  }

 // C05FunctionMod- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // that's changing the constant, not good either
 if( auto tmod = dynamic_cast< const C05FunctionMod * >( mod ) ) {
  auto ci = f_const.begin();
  for( ; ci != f_const.end() ; ++ci )
   if( ci->get_function() == tmod->function() )
    break;

  if( ci != f_const.end() )  // it's in the linearized representation
   throw( std::logic_error(
		       "wrong C05FunctionMod in PolyhedralFunctionBlock" ) );
 
  return;  // else, none of my business
  }
 }  // end( PolyhedralFunctionBlock::guts_of_add_Modification_LR )

/*--------------------------------------------------------------------------*/

void PolyhedralFunctionBlock::ConstructLPConstraint( Index i ,
						     FRowConstraint & ci )
{
 // if the PolyhedralFunction is convex, then the constraint is
 // b_i <= v - A_i x <= INF, otherwise it is -INF <= v - A_i x <= b_i
 ci.set_lhs( f_polyf.is_convex() ? f_polyf.get_b()[ i ]
	                         : -Inf< Function::FunctionValue >() ,
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
