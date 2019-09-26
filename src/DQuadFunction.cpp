/*--------------------------------------------------------------------------*/
/*------------------------ File DQuadFunction.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the DQuadFunction class.
 *
 * \version 0.30
 *
 * \date 15 - 09 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

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
   auto variable_value = std::get<0>( triple )->get_value();
   f_value += variable_value * ( std::get<1>(triple) +
				 std::get<2>(triple) * variable_value );
   }
  }

 return( kOK );
 }

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_convex( void ) const
{
 for( const auto & triple : v_triples )
  if( std::get<2>( triple ) < 0 )
   return( false );

 return( true );
 }

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_concave( void ) const
{
 for( const auto & triple : v_triples )
  if( std::get<2>( triple ) > 0 )
   return( false );

 return( true );
 }

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_linear( void ) const
{
 for( const auto & triple : v_triples )
  if( std::get<2>( triple ) != 0 )
   return( false );

 return( true );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_hessian_approximation( SparseHessian &hessian ) const
{
 int num_active_var = this->get_num_active_var();

 std::vector< Eigen::Triplet<FunctionValue> > tripletList;
 tripletList.reserve( num_active_var );

 int index = 0;
 for(const auto &triple : v_triples)
  tripletList.push_back(
   Eigen::Triplet<FunctionValue>( index , index, 2 * std::get<2>( triple ) ) );

 hessian.setZero();
 hessian.reserve( Eigen::VectorXi::Constant( num_active_var , 1 ) );
 hessian.setFromTriplets( tripletList.begin() , tripletList.end() );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_hessian_approximation( DenseHessian &hessian ) const
{
 int num_active_var = get_num_active_var();
 hessian.setZero( num_active_var , num_active_var );
 int index = 0;
 for(const auto &triple : v_triples) {
  hessian( index , index ) = 2 * std::get<2>( triple );
  index++;
  }
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( FunctionValue * g ,
						    Range range ,
						    Index name )
{
 range.second = std::min( end , get_num_active_var() );
 if( range.second <= range.first )
  return( 0 );

 for( Index i = range.first ; i < range.second ; i++ )
  *(g++) = get_linearization_coefficient( i );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( SparseVector & g ,
						    Range range ,
						    Index name )
{
 c_Index num_active_var = get_num_active_var();
 range.second = std::min( end , num_active_var );
 if( range.second <= range.first )
  return( 0 );

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < num_active_var )
   g.resize( num_active_var );

  g.reserve( range.second - range.first );

  for( Index i = range.first ; i < range.second ; ++i )
   g.insert( i ) = get_linearization_coefficient( i );
  }
 else {                  // The given vector contains some non-zero elements
  if( g.size() != num_active_var )
   throw( std::invalid_argument( "wrong size of nonempty SparseVector g" ) );

  for( Index i = range.first ; i < range.second ; ++i )
   g.coeffRef( i ) = get_linearization_coefficient( i );
  }
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( FunctionValue * g ,
						    c_Subset & subset ,
						    Index name )
{
 for( const auto & i : indices ) {
  if( i >= get_num_active_var() )
   throw( std::invalid_argument( "wrong index in subset" ) );
  *(g++) = get_linearization_coefficient( i );
  }
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( SparseVector & g ,
						    c_Subset & subset ,
						    Index name )
{
 c_Index num_active_var = get_num_active_var();

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < num_active_var )
   g.resize( num_active_var );

  g.reserve( range.second - range.first );

  for( const auto & i : indices ) {
   if( i >= num_active_var )
    throw( std::invalid_argument( "wrong index in subset" ) );
   g.insert( i ) = get_linearization_coefficient( i );
   }
  }
 else {                  // The given vector contains some non-zero elements
  if( g.size() != num_active_var )
   throw( std::invalid_argument( "wrong size of nonempty SparseVector g" ) );

  for( const auto & i : indices ) {
   if( i >= num_active_var )
    throw( std::invalid_argument( "wrong index in subset" ) );
   g.coeffRef( i ) = get_linearization_coefficient( i );
   }
  }
 }

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE DQuadFunction --------*/
/*--------------------------------------------------------------------------*/

void DQuadFunction::map_active( c_Vec_p_Var & vars , Vec_Index & map ,
				const bool ordered ) const
{
 if( vars.empty() )
  return;

 if( map.size() < vars.size() )
   map.resize( vars.size() );

 if( ordered ) {
  Index found = 0;
  for( Index i = 0 ; i < v_triples.size() ; ++i ) {
   auto itvi = std::lower_bound( vars.begin() , vars.end() ,
				 std::get<0>( v_triples[ i ] ) );
   if( itvi != vars.end() ) {
    map[ std::distance( vars.begin() , itvi ) ] = i;
    ++found;
    }
   }
  if( found < vars.size() )
   throw( std::invalid_argument( "map_active: some Variable is not active" )
	  );
  }
 else {
  auto it = map.begin();
  for( auto var : vars ) {
   Index i = DQuadFunction::::is_active( var );
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "map_active: some Variable is not active" )
	   );
   *(it++) = i;
   }
  }
 }  // end( DQuadFunction::map_active )

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE DQuadFunction -------------------*/
/*--------------------------------------------------------------------------*/

void DQuadFunction::add_variables( v_coeff_triple && vars ,
				   c_ModParam issueMod )
{
 if( vars.empty() )  // actually nothing to add
  return;            // cowardly (and silently) return

 if( v_triples.empty() ) {    // adding to nothing
  v_triples = std::move( vars );

  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   issue_add_variables_modification( v_triples , issueMod );
  }
 else {                         // adding to a nonempty set
  v_triples.insert( v_triples.begin() , vars.begin() , vars.end() );

  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   issue_add_variables_modification( vars , issueMod );
  }
 }  // end( DQuadFunction::add_variables )

/*--------------------------------------------------------------------------*/

void DQuadFunction::add_variable( ColVariable * const var ,
				  const Coefficient lin_coeff ,
				  const Coefficient quad_coeff ,
				  c_ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to add
  return;              // cowardly (and silently) return

 v_triples.push_back( std::make_tuple( var , lin_coeff , quad_coeff ) );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a diagonal quadratic function is additive ==> strongly quasi-additive
 f_Observer->add_Modification( std::make_shared<C05FunctionModVarsAddd>(
				      this , { var } ,
				      v_triples.size() - 1 , 0 , true ,
				      Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::add_variable )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_term( ColVariable * const var,
                                 const Coefficient lin_coeff,
                                 const Coefficient quad_coeff,
                                 c_ModParam issueMod )
{
 if( i >= v_triples.size() )
  throw ( std::invalid_argument( "modify_coefficient: invalid index" ) );

 if( ( std::get< 1 >( v_triples[ i ] ) == lin_coeff ) &&
     ( std::get< 2 >( v_triples[ i ] ) == quad_coeff ) )
           // actually nothing to modify
  return;  // cowardly (and silently) return

 std::get< 1 >( v_triples[ i ] ) = lin_coeff;   // modify linear coefficient
 std::get< 2 >( v_triples[ i ] ) = quad_coeff;  // modify quadratic coeff.

 if( !f_Observer || !f_Observer->issue_mod( issueMod ) )
  return;  // noone is there: all done

 f_Observer->add_Modification( std::make_shared< C05FunctionModRngd >( this ,
                                     C05FunctionMod::AllLinearizationChanged ,
				     { var } , std::make_pair( i , i + 1 ) ,
				     FunctionMod::NaNshift ,
				     Observer::par2concern( issueMod ) ),
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::modify_term )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_linear_coefficient( Index i , Coefficient coeff ,
                                               c_ModParam issueMod )
{
 if( i >= v_triples.size() )
  throw( std::invalid_argument( "modify_coefficient: invalid index" ) );

 if( std::get< 1 >( v_triples[ i ] ) == coeff )  // nothing changes
  return;                                        // silently return

 auto diff = coeff - std::get< 1 >( v_triples[ i ] );
 std::get< 1 >( v_triples[ i ] ) = coeff;  // modify the linear coefficient

 if( !f_Observer || !f_Observer->issue_mod( issueMod ) )
  return;                                  // noone is there: all done

 f_Observer->add_Modification( std::make_shared< C05FunctionModLinRngd >(
				        this, { diff } ,
				        { std::get< 0 >( v_triples[ i ] ) ) ,
                                        std::make_pair< i , i + 1 > ,
				        FunctionMod::NaNshift ,
				  Observer::par2concern( issueMod ) ),
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::modify_linear_coefficient )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_terms( c_v_coeff_it NQuadCoef ,
				  c_v_coeff_it NLinCoef ,
				  Vec_Index && nms , c_ModParam issueMod )
{
 if( nms.empty() )
  return;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( nms.size() );
  auto vpit = vp.begin();

  for( auto i : nms ) {
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "modify_terms: invalid index" ) );
   *(vpit++) = std::get< 0 >( v_triples[ i ] );
   std::get< 1 >( v_triples[ i ] ) = *(NQuadCoef++);  // modify linear coeff.
   std::get< 2 >( v_triples[ i ] ) = *(NLinCoef++);   // modify quad. coeff.
   }

  // now issue the Modification
  f_Observer->add_Modification( std::make_shared< C05FunctionModSbst >( this ,
                                     C05FunctionMod::AllLinearizationChanged ,
                                     std::move( vp ) , std::move( nms ) ,
				     FunctionMod::NaNshift,
				     Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );

  }
 else  // noone is there: just do it
  for( auto i : nms ) {
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "modify_terms: invalid index" ) );
   std::get< 1 >( v_triples[ i ] ) = *(NQuadCoef++);  // modify linear coeff.
   std::get< 2 >( v_triples[ i ] ) = *(NLinCoef++);   // modify quad. coeff.
   }

 }  // end( DQuadFunction::modify_terms( subset ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_linear_coefficients( Vec_FunctionValue && NCoef ,
                                                Vec_Index && nms ,
                                                c_ModParam issueMod )
{
 if( nms.empty() )
  return;

 if( NCoef.size() < nms.size() )
  throw( std::invalid_argument( "modify_coefficient: NCoef.size < nms.size"
				) );

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( nms.size() );
  auto vpit = vp.begin();

  for( auto i : nms ) {
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "modify_coefficients: invalid index" ) );
   *(vpit++) = std::get< 0 >( v_triples[ i ] );
   auto di = std::get< 2 >( v_triples[ i ] ) - *NCit;
   std::get< 2 >( v_triples[ i ] ) = *NCit;
   *(NCit++) = di;
   }

  // now issue the Modification
  f_Observer->add_Modification( std::make_shared<C05FunctionModLinSbst>(
				 this , std::move( NCoef ) , std::move( vp ) ,
				 std::move( nms ) , FunctionMod::NaNshift ,
				 Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  for( auto i : nms ) {
   if( i >= v_triples.size() )
    throw( std::invalid_argument( "modify_coefficients: invalid index" ) );
   std::get< 2 >( v_triples[ i ] ) = *(NCit++);
   }

 }  // end( DQuadFunction::modify_linear_coefficients )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_terms( c_v_coeff_it NQuadCoef ,
				  c_v_coeff_it NLinCoef ,
				  Range range , c_ModParam issueMod )
{
 range.second = std::min( range.second , c_Index( v_pairs.size() ) );
 if( range.second <= range.first )
  return;

 if( NCoef.size() < range.second - range.first )
  throw( std::invalid_argument(
	               "modify_linear_coefficients: NCoef.size too small" ) );

 auto strtit = v_triples.begin() + range.first;
 const auto stopit = v_triples.begin() + range.second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( range.second - range.first );
  auto vpit = vp.begin();

  while( strtit < stopit ) {
   (*(vpit++)) = std::get< 0 >( *strtit );
   std::get< 1 >( *strtit ) = *(NQuadCoef++);
   std::get< 2 >( *(strtit++) ) = *(NLinCoef++);
   }

  // now issue the Modification
  f_Observer->add_Modification( std::make_shared<C05FunctionModRngd>(
			     this , C05FunctionMod::AllLinearizationChanged ,
			     std::move( vp ) , range , FunctionMod::NaNshift ,
			     Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  while( strtit < stopit ) {
   std::get< 1 >( *strtit ) = *(NQuadCoef++);
   std::get< 2 >( *(strtit++) ) = *(NLinCoef++);
   }

 }  // end( DQuadFunction::modify_terms( range ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_linear_coefficients( Vec_FunctionValue && NCoef ,
						Range range ,
						c_ModParam issueMod )
{
 range.second = std::min( range.second , c_Index( v_pairs.size() ) );
 if( range.second <= range.first )
  return;

 if( NCoef.size() < range.second - range.first )
  throw( std::invalid_argument(
	               "modify_linear_coefficients: NCoef.size too small" ) );

 auto NCit = NCoef.begin();
 auto strtit = v_triples.begin() + range.first;
 const auto stopit = v_triples.begin() + range.second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( range.second - range.first );
  auto vpit = vp.begin();

  while( strtit < stopit ) {
   auto di = *NCit - std::get< 2 >( *strtit )
   (*(vpit++)) = std::get< 0 >( *strtit );
   std::get< 2 >( *(strtit++) ) = *NCit;
   *(*NCit++) = di;
   }

  // now issue the Modification
  f_Observer->add_Modification( std::make_shared<C05FunctionModLinSbst>(
				 this , std::move( NCoef ) , std::move( vp ) ,
				 range , FunctionMod::NaNshift ,
				 Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  while( strtit < stopit )
   std::get< 2 >( *(strtit++) ) = *(NCit++);

 }  // end( DQuadFunction::modify_linear_coefficients( range ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::remove_variable( Variable * var, c_ModParam issueMod ) {
 if( !var )  // actually nothing to remove
  return;     // cowardly (and silently) return

 if( v_triples.empty() )  // deleting from nothing
  throw ( std::logic_error( "deleting from an empty set" ) );

 // search where the variable lives
 auto itv = std::lower_bound( v_triples.begin(), v_triples.end(),
                              var,
                              []( const coeff_triple & p,
                                  const Variable * v ) {
                               return ( std::get< 0 >( p ) < v );
                              } );

 if( itv == v_triples.end() )  // if the variable is not there
  throw ( std::invalid_argument( "Variable is not active" ) );

 v_triples.erase( itv );       // erase it

 if( !f_Observer || !f_Observer->issue_mod( issueMod ) )
  return;

 // a diagonal quadratic function is additive ==> strongly quasi-additive
 // note that there is only one Variable, hence it is ordered
 f_Observer->add_Modification(
  std::make_shared< C05FunctionModVars >( this,
                                          FunctionModVars::RemoveVar,
                                          Vec_p_Var( { var } ), true, 0, true,
                                          Observer::par2concern( issueMod ) ),
  Observer::par2chnl( issueMod ) );

}  // end( DQuadFunction::remove_variable( pointer ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::remove_variable( c_Index i , c_ModParam issueMod )
{
 if( v_triples.size() >= i )
  throw( std::logic_error( "less than i Variable are active" ) );

 auto itv = v_triples.begin() + i;
 auto var = std::get<0>( *itv );
 v_triples.erase( itv );       // erase it

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a diagonal quadratic function is additive ==> strongly quasi-additive
 // note that there is only one Variable, hence it is ordered
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                   FunctionModVars::RemoveVar ,
				    Vec_p_Var( { var } ) , true , 0 , true ,
				    Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::remove_variable( index ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::remove_variables( c_Index strt , Index stop ,
				      c_ModParam issueMod )
{
 stop = std::min( stop , c_Index( v_triples.size() ) );
 if( stop <= strt )
  return;

 const auto strtit = v_triples.begin() + strt;
 const auto stopit = v_triples.begin() + stop;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vars( stop - strt );
  auto vpit = vars.begin();
  for( auto tmpit = strtit ; strtit < stopit ; )
   *(vpit++) = std::get<0>( *(tmpit++) );

  v_triples.erase( strtit , stopit );

  // now issue the Modification
  // a diagonal quadratic function is additive ==> strongly quasi-additive
  // note that the Variable are ordered by construction
  f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                       FunctionModVars::RemoveVar ,
				       std::move( vars ) , true , 0 , true ,
				       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  v_triples.erase( strtit , stopit );

 }  // end( DQuadFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::remove_variables( Vec_Index & nms , const bool ordered ,
				      c_ModParam issueMod )
{
 if( nms.empty() )  // actually nothing to remove
  return;           // cowardly (and silently) return

 if( v_triples.empty() )  // deleting from nothing
  throw( std::logic_error( "deleting from an empty set" ) );

 auto it = nms.begin();
 if( ! ordered )
  std::sort( it , nms.end() );

 if( *it >= v_triples.size() )  // if the first name is wrong
  throw( std::invalid_argument( "wrong index in DQuadFunction" ) );

 if( nms.back() >= v_triples.size() )  // if the last name is wrong
  throw( std::invalid_argument( "wrong index in DQuadFunction" ) );

 auto vi = *it;    // first element to be eliminated
 auto curr = v_triples.begin() + vi;   // position where to move stuff

 // if the Observer is a ThinVarDepInterface, un-register it from the vars
 auto TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 ++it;              // skip the first elements
 ++vi;              // as they have been processed already

 for( ; it < nms.end() ; ++vi )
  if( *it == vi )                  // one element to be eliminated
   ++it;                           // skip it
  else
   *(curr++) = v_triples[ vi ];    // move in the current position

 auto itv = v_triples.begin() + vi;
 for( ; itv < v_triples.end() ; )   // copy the last part
  *(curr++) = *(itv++);             // after the last of v_var

 v_triples.erase( curr , itv );     // erase the last part

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 Vec_p_Var vars( nms.size() );
 auto its = vars.begin();
 for( auto nm : nms )
   *(its++) = std::get<0>( v_triples[ nm ] );

 // now issue the Modification
 // a diagonal quadratic function is additive ==> strongly quasi-additive
 // note that the Variable have been ordered (if they were not so already)
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                       FunctionModVars::RemoveVar ,
				       std::move( vars ) , true , 0 , true ,
				       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::remove_variables( indices ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::set_constant_term( const FunctionValue constant_term  ,
				       c_ModParam issueMod )
{
 if( f_constant_term == constant_term )  // actually nothing to change
  return;                                // cowardly (and silently) return

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  const FunctionValue delta = f_constant_term - constant_term;
  f_constant_term = constant_term;

  f_Observer->add_Modification( std::make_shared<FunctionMod>( this , delta ,
				        Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else
  f_constant_term = constant_term;
 }

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

void DQuadFunction::issue_add_variables_modification
( v_coeff_triple & triples , c_ModParam issueMod ) {

 Vec_p_Var vars( triples.size() );
 for( Index i = 0 ; i < triples.size() ; ++i )
  vars[ i ] = std::get<0>( triples[ i ] );

 // a diagonal quadratic function is additive ==> strongly quasi-additive
 // note that triples is always ordered
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                         FunctionModVars::AddVar ,
					 std::move( vars ) , true , 0 , true ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File DQuadFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
