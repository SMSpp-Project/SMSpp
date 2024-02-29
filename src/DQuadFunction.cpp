/*--------------------------------------------------------------------------*/
/*------------------------ File DQuadFunction.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the DQuadFunction class.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "DQuadFunction.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE DQuadFunction -----------*/
/*--------------------------------------------------------------------------*/

int DQuadFunction::compute( bool changedvars )
{
 if( changedvars ) {
  f_value = f_constant_term;  // value of the function
  for( const auto & triple : v_triples ) {
   auto variable_value = std::get< 0 >( triple )->get_value();
   f_value += variable_value * ( std::get< 1 >( triple ) +
				 std::get< 2 >( triple ) * variable_value );
   }
  }

 return( kOK );
 }

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_convex( void ) const
{
 for( const auto & triple : v_triples )
  if( std::get< 2 >( triple ) < 0 )
   return( false );

 return( true );
 }

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_concave( void ) const
{
 for( const auto & triple : v_triples )
  if( std::get< 2 >( triple ) > 0 )
   return( false );

 return( true );
 }

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_linear( void ) const
{
 for( const auto & triple : v_triples )
  if( std::get< 2 >( triple ) != 0 )
   return( false );

 return( true );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_hessian_approximation( SparseHessian & hessian ) const
{
 int num_active_var = this->get_num_active_var();

 std::vector< Eigen::Triplet< FunctionValue > > tripletList;
 tripletList.reserve( num_active_var );

 int index = 0;
 for( const auto & triple : v_triples )
  tripletList.push_back(
   Eigen::Triplet< FunctionValue >( index , index, 2 * std::get< 2 >( triple )
				  ) );
 hessian.setZero();
 hessian.reserve( Eigen::VectorXi::Constant( num_active_var , 1 ) );
 hessian.setFromTriplets( tripletList.begin() , tripletList.end() );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_hessian_approximation( DenseHessian & hessian ) const
{
 int num_active_var = get_num_active_var();
 hessian.setZero( num_active_var , num_active_var );
 int index = 0;
 for( const auto & triple : v_triples ) {
  hessian( index , index ) = 2 * std::get< 2 >( triple );
  index++;
  }
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( FunctionValue * g ,
						    Range range , Index name )
{
 range.second = std::min( range.second , get_num_active_var() );
 if( range.second <= range.first )
  return;

 for( Index i = range.first ; i < range.second ; i++ )
  *(g++) = get_linearization_coefficient( i );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( SparseVector & g ,
						    Range range , Index name )
{
 c_Index num_active_var = get_num_active_var();
 range.second = std::min( range.second , num_active_var );
 if( range.second <= range.first )
  return;

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < num_active_var )
   g.resize( num_active_var );

  g.reserve( range.second - range.first );

  for( Index i = range.first ; i < range.second ; ++i ) {
   auto gi = get_linearization_coefficient( i );
   if( gi )
    g.insert( i ) = gi;
   }
  }
 else {                  // The given vector contains some non-zero elements
  if( g.size() != num_active_var )
   throw( std::invalid_argument( "DQuadFunction::get_linearization_"
                                 "coefficients: wrong size of nonempty "
                                 "SparseVector g: g has size " +
                                 std::to_string( g.size() ) + " but there is "
                                 + std::to_string( num_active_var ) +
                                 " active Variables" ) );

  for( Index i = range.first ; i < range.second ; ++i )
   g.coeffRef( i ) = get_linearization_coefficient( i );

  g.prune( 0 , 0 );
  }
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( FunctionValue * g ,
						    c_Subset & subset ,
						    const bool ordered ,
						    Index name )
{
 for( const auto & i : subset ) {
  if( i >= get_num_active_var() )
   throw( std::invalid_argument( "DQuadFunction::get_linearization_"
                                 "coefficients: wrong index in subset: " +
                                 std::to_string( i ) ) );
  *(g++) = get_linearization_coefficient( i );
  }
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( SparseVector & g ,
						    c_Subset & subset ,
						    bool ordered ,
						    Index name )
{
 c_Index num_active_var = get_num_active_var();

 if( g.nonZeros() == 0 ) {  // g contains no non-zero element
  if( g.size() < num_active_var )
   g.resize( num_active_var );

  g.reserve( subset.size() );

  for( const auto & i : subset ) {
   if( i >= num_active_var )
    throw( std::invalid_argument( "DQuadFunction::get_linearization_"
                                  "coefficients: wrong index in subset: " +
                                  std::to_string( i ) ) );
   auto gi = get_linearization_coefficient( i );
   if( gi )
    g.insert( i ) = gi;
   }
  }
 else {                  // g contains some non-zero elements
  if( g.size() != num_active_var )
   throw( std::invalid_argument( "DQuadFunction::get_linearization_"
                                 "coefficients: wrong size of nonempty "
                                 "SparseVector g: g has size " +
                                 std::to_string( g.size() ) + " but there are "
                                 + std::to_string( num_active_var ) +
                                 " active Variables" ) );

  for( const auto & i : subset ) {
   if( i >= num_active_var )
    throw( std::invalid_argument( "DQuadFunction::get_linearization_"
                                  "coefficients: wrong index in subset: " +
                                  std::to_string( i ) ) );
   g.coeffRef( i ) = get_linearization_coefficient( i );
   }

  g.prune( 0 , 0 );
  }
 }

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE DQuadFunction --------*/
/*--------------------------------------------------------------------------*/

void DQuadFunction::map_active( c_Vec_p_Var & vars , Subset & map ,
				bool ordered ) const
{
 if( vars.empty() )
  return;

 if( map.size() < vars.size() )
   map.resize( vars.size() );

 if( ordered )
  for( Index i = 0 ; i < v_triples.size() ; ++i ) {
   auto itvi = std::lower_bound( vars.begin() , vars.end() ,
				 std::get< 0 >( v_triples[ i ] ) );
   if( itvi != vars.end() )
    map[ std::distance( vars.begin() , itvi ) ] = i;
   else
    throw( std::invalid_argument( "DQuadFunction::map_active: "
				  "some Variable is not active" ) );
   }
 else {
  auto it = map.begin();
  for( auto var : vars ) {
   Index i = DQuadFunction::is_active( var );
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "DQuadFunction::map_active: "
                                  "some Variable is not active" ) );
   *(it++) = i;
   }
  }
 }  // end( DQuadFunction::map_active )

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE DQuadFunction -------------------*/
/*--------------------------------------------------------------------------*/

void DQuadFunction::add_variables( v_coeff_triple && vars ,
				   ModParam issueMod )
{
 if( vars.empty() )  // actually nothing to add
  return;            // cowardly (and silently) return

 auto added = & vars;
 Index k = v_triples.size();
 if( k )    // adding to a nonempty set
  v_triples.insert( v_triples.end() , vars.begin() , vars.end() );
 else {     // adding to nothing
  v_triples = std::move( vars );
  added = & v_triples;
  }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 Vec_p_Var vptr( added->size() );
 for( Index i = 0 ; i < added->size() ; ++i )
  vptr[ i ] = std::get< 0 >( (*added)[ i ] );

 // a diagonal quadratic function is additive ==> strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
					 this , std::move( vptr ) , k , 0 ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::add_variables )

/*--------------------------------------------------------------------------*/

void DQuadFunction::add_variable( ColVariable * var , Coefficient lin_coeff ,
				  Coefficient quad_coeff ,
				  ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to add
  return;              // cowardly (and silently) return

 v_triples.push_back( std::make_tuple( var , lin_coeff , quad_coeff ) );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a diagonal quadratic function is additive ==> strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
				         this , Vec_p_Var( { var } ) ,
				         v_triples.size() - 1 , 0 ,
				         Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::add_variable )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_term( Index i , Coefficient lin_coeff ,
                                 Coefficient quad_coeff ,
				 ModParam issueMod )
{
 if( i >= v_triples.size() )
  throw( std::invalid_argument( "DQuadFunction::modify_term: invalid "
                                "index: " + std::to_string( i ) ) );

 if( ( std::get< 1 >( v_triples[ i ] ) == lin_coeff ) &&
     ( std::get< 2 >( v_triples[ i ] ) == quad_coeff ) )
           // actually nothing to modify
  return;  // cowardly (and silently) return

 std::get< 1 >( v_triples[ i ] ) = lin_coeff;   // modify linear coefficient
 std::get< 2 >( v_triples[ i ] ) = quad_coeff;  // modify quadratic coeff.

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;  // noone is there: all done

 f_Observer->add_Modification( std::make_shared< C05FunctionModRngd >(
                                this , C05FunctionMod::AllLinearizationChanged ,
                                Vec_p_Var( { std::get< 0 >( v_triples[ i ] ) } ) ,
                                std::make_pair( i , i + 1 ) , Subset( {} ) ,
                                FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::modify_term )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_linear_coefficient( Index i , Coefficient coeff ,
                                               ModParam issueMod )
{
 if( i >= v_triples.size() )
  throw( std::invalid_argument( "DQuadFunction::modify_linear_coefficient: "
                                "invalid index: " + std::to_string( i ) ) );

 if( std::get< 1 >( v_triples[ i ] ) == coeff )  // nothing changes
  return;                                        // silently return

 auto diff = coeff - std::get< 1 >( v_triples[ i ] );
 std::get< 1 >( v_triples[ i ] ) = coeff;  // modify the linear coefficient

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                                  // noone is there: all done

 f_Observer->add_Modification( std::make_shared< C05FunctionModLinRngd >(
			   this , Vec_FunctionValue( { diff } ) ,
			   Vec_p_Var( { std::get< 0 >( v_triples[ i ] ) } ) ,
                           Range( i , i + 1 ) , FunctionMod::NaNshift ,
			   Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::modify_linear_coefficient )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_terms( c_v_coeff_it NQuadCoef ,
				  c_v_coeff_it NLinCoef , Subset && nms ,
				  bool ordered , ModParam issueMod )
{
 if( nms.empty() )
  return;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( nms.size() );
  auto vpit = vp.begin();

  for( auto i : nms ) {
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "DQuadFunction::modify_terms: invalid "
                                  "index: " + std::to_string( i ) ) );
   *(vpit++) = std::get< 0 >( v_triples[ i ] );
   std::get< 1 >( v_triples[ i ] ) = *(NLinCoef++);   // modify quad. coeff.
   std::get< 2 >( v_triples[ i ] ) = *(NQuadCoef++);  // modify linear coeff.
   }

  // now issue the Modification
  f_Observer->add_Modification( std::make_shared< C05FunctionModSbst >(
                                 this , C05FunctionMod::AllLinearizationChanged ,
                                 std::move( vp ) , std::move( nms ) ,
                                 ordered , Subset( {} ) ,
                                 FunctionMod::NaNshift ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  for( auto i : nms ) {
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "DQuadFunction::modify_terms: invalid "
                                  "index: " + std::to_string( i ) ) );
   std::get< 1 >( v_triples[ i ] ) = *(NLinCoef++);   // modify quad. coeff.
   std::get< 2 >( v_triples[ i ] ) = *(NQuadCoef++);  // modify linear coeff.
   }

 }  // end( DQuadFunction::modify_terms( subset ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_linear_coefficients( Vec_FunctionValue && NCoef ,
                                                Subset && nms , bool ordered ,
                                                ModParam issueMod )
{
 if( nms.empty() )
  return;

 if( NCoef.size() < nms.size() )
  throw( std::invalid_argument( "DQuadFunction::modify_linear_coefficients: "
                                "NCoef.size < nms.size" ) );
 auto NCit = NCoef.begin();
 
 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( nms.size() );
  auto vpit = vp.begin();

  for( auto i : nms ) {
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "DQuadFunction::modify_linear_coefficients: "
                                  "invalid index: " + std::to_string( i ) ) );
   *(vpit++) = std::get< 0 >( v_triples[ i ] );
   auto di = *NCit - std::get< 1 >( v_triples[ i ] );
   std::get< 1 >( v_triples[ i ] ) = *NCit;
   *(NCit++) = di;
   }

  // now issue the Modification
  f_Observer->add_Modification( std::make_shared< C05FunctionModLinSbst >(
                                 this , std::move( NCoef ) , std::move( vp ) ,
                                 std::move( nms ) , ordered ,
                                 FunctionMod::NaNshift ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
 }
 else  // noone is there: just do it
  for( auto i : nms ) {
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "DQuadFunction::modify_linear_coefficients: "
                                  "invalid index: " + std::to_string( i ) ) );
   std::get< 1 >( v_triples[ i ] ) = *(NCit++);
   }

 }  // end( DQuadFunction::modify_linear_coefficients( subset ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_terms( c_v_coeff_it NQuadCoef ,
				  c_v_coeff_it NLinCoef ,
				  Range range , ModParam issueMod )
{
 range.second = std::min( range.second , c_Index( v_triples.size() ) );
 if( range.second <= range.first )
  return;

 auto strtit = v_triples.begin() + range.first;
 const auto stopit = v_triples.begin() + range.second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( range.second - range.first );
  auto vpit = vp.begin();

  while( strtit < stopit ) {
   (*(vpit++)) = std::get< 0 >( *strtit );
   std::get< 1 >( *strtit ) = *(NLinCoef++);
   std::get< 2 >( *(strtit++) ) = *(NQuadCoef++);
   }

  // now issue the Modification
  f_Observer->add_Modification( std::make_shared< C05FunctionModRngd >(
                                 this , C05FunctionMod::AllLinearizationChanged ,
                                 std::move( vp ) , range , Subset( {} ) ,
                                 FunctionMod::NaNshift ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  while( strtit < stopit ) {
   std::get< 1 >( *strtit ) = *(NLinCoef++);
   std::get< 2 >( *(strtit++) ) = *(NQuadCoef++);
   }

 }  // end( DQuadFunction::modify_terms( range ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_linear_coefficients( Vec_FunctionValue && NCoef ,
						Range range ,
						ModParam issueMod )
{
 range.second = std::min( range.second , c_Index( v_triples.size() ) );
 if( range.second <= range.first )
  return;

 if( NCoef.size() < range.second - range.first )
  throw( std::invalid_argument( "DQuadFunction::modify_linear_coefficients: "
                                "NCoef.size too small" ) );

 auto NCit = NCoef.begin();
 auto strtit = v_triples.begin() + range.first;
 const auto stopit = v_triples.begin() + range.second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( range.second - range.first );
  auto vpit = vp.begin();

  while( strtit < stopit ) {
   auto di = *NCit - std::get< 1 >( *strtit );
   *(vpit++) = std::get< 0 >( *strtit );
   std::get< 1 >( *(strtit++) ) = *NCit;
   *(NCit++) = di;
   }

  // now issue the Modification
  f_Observer->add_Modification( std::make_shared< C05FunctionModLinRngd >(
                                 this , std::move( NCoef ) , std::move( vp ) ,
                                 range , FunctionMod::NaNshift ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  while( strtit < stopit )
   std::get< 1 >( *(strtit++) ) = *(NCit++);

 }  // end( DQuadFunction::modify_linear_coefficients( range ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::remove_variable( Index i , ModParam issueMod )
{
 if( v_triples.size() <= i )
  throw( std::logic_error( "less than i Variable are active" ) );

 auto itv = v_triples.begin() + i;
 auto var = std::get< 0 >( *itv );
 v_triples.erase( itv );       // erase it

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a diagonal quadratic function is additive ==> strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                this , Vec_p_Var( { var } ) ,
                                Range( i , i + 1 ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::remove_variable( index ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::remove_variables( Range range, ModParam issueMod )
{
 range.second = std::min( range.second , Index( v_triples.size() ) );
 if( range.second <= range.first )
  return;

 if( ( range.first == 0 ) && ( range.second >= v_triples.size() ) ) {
  // removing *all* variable
  if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
   // an Observer is there: copy the names of deleted Variable (all of them)
   Vec_p_Var vars( v_triples.size() );

   for( Index i = 0 ; i < v_triples.size() ; ++i )
    vars[ i ] = std::get< 0 >( v_triples[ i ] );

   clear();  // then clear the DQuadFunction

  // now issue the Modification: note that the subset is empty
  // a diagonal quadratic function is additive ==> strongly quasi-additive
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
                                  this , std::move( vars ) , Subset() , true ,
                                  0 , Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
   }
  else       // no-one is listening
   clear();  // just do it
  return;    // all done
  }

 // this is not a complete reset
 const auto strtit = v_triples.begin() + range.first;
 const auto stopit = v_triples.begin() + range.second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vars( range.second - range.first );
  auto vpit = vars.begin();
  for( auto tmpit = strtit ; strtit < stopit ; )
   *(vpit++) = std::get< 0 >( *(tmpit++) );

  v_triples.erase( strtit , stopit );

  // now issue the Modification
  // a diagonal quadratic function is additive ==> strongly quasi-additive
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                 this , std::move( vars ) , range , 0 ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  v_triples.erase( strtit , stopit );

 }  // end( DQuadFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::remove_variables( Subset && nms , bool ordered ,
				      ModParam issueMod )
{
 if( nms.empty() ) {      // removing *all* variable
  if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
   // an Observer is there: copy the names of deleted Variable (all of them)
   Vec_p_Var vars( v_triples.size() );

   for( Index i = 0 ; i < v_triples.size() ; ++i )
    vars[ i ] = std::get< 0 >( v_triples[ i ] );

   clear();  // then clear the DQuadFunction

  // now issue the Modification: note that the subset is empty
  // a diagonal quadratic function is additive ==> strongly quasi-additive
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
                                  this , std::move( vars ) , Subset() , true ,
                                  0 , Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );
   }
  else       // no-one is listening
   clear();  // just do it
  return;    // all done
  }

 // this is not a complete reset
 if( ! ordered )
  std::sort( nms.begin() , nms.end() );

 if( nms.back() >= v_triples.size() )  // the last name is wrong
  throw( std::invalid_argument(
	  "DQuadFunction::remove_variables: wrong Variable index in nms" ) );

 auto it = nms.begin();
 auto vi = *it;    // first element to be eliminated
 auto curr = v_triples.begin() + vi;   // position where to move stuff

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification
  // (as it will be destroyed during the process)

  Vec_p_Var vars( nms.size() );
  auto its = vars.begin();

  *(its++) = std::get< 0 >( v_triples[ *(it++) ] );
  ++vi;              // skip the first element, it will be overwritten

  for( ; it < nms.end() ; ++vi )
   if( *it == vi )                 // one element to be eliminated
    *(its++) = std::get< 0 >( v_triples[ *(it++) ] );
                                   // skip it, but record the Variable
   else
    *(curr++) = v_triples[ vi ];   // move in the current position

  auto itv = v_triples.begin() + vi;
  for( ; itv < v_triples.end(); )  // copy the last part
   *(curr++) = *(itv++);           // after the last of nms[]

  v_triples.erase( curr, itv );    // erase the last part

  // now issue the Modification
  // a diagonal quadratic function is additive ==> strongly quasi-additive
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
					 this , std::move( vars ) ,
					 std::move( nms ) , ordered , 0 ,
                                         Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else {  // noone is there: just do it
  ++it;              // skip the first element
  ++vi;              // as it will be overwritten

  for( ; it < nms.end() ; ++vi )
   if( *it == vi )                // one element to be eliminated
    ++it;                         // skip it
   else
    *(curr++) = v_triples[ vi ];  // move in the current position

  auto itv = v_triples.begin() + vi;
  for( ; itv < v_triples.end(); )  // copy the last part
   *(curr++) = *(itv++);       // after the last of v_var

  v_triples.erase( curr, itv );    // erase the last part
  }
 }  // end( DQuadFunction::remove_variables( subset ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::set_constant_term( FunctionValue constant_term ,
				       ModParam issueMod )
{
 if( f_constant_term == constant_term )  // actually nothing to change
  return;                                // cowardly (and silently) return

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  const FunctionValue delta = constant_term - f_constant_term;
  f_constant_term = constant_term;

  f_Observer->add_Modification( std::make_shared< C05FunctionMod >(
                                 this , C05FunctionMod::NothingChanged ,
                                 Subset( {} ) , delta ,
                                 Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
  }
 else
  f_constant_term = constant_term;
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File DQuadFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
