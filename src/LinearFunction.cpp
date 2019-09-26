/*--------------------------------------------------------------------------*/
/*----------------------- File LinearFunction.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the LinearFunction class.
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

#include "SMSTypedefs.h"

#include "LinearFunction.h"

#include <math.h>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LinearFunction ----------*/
/*--------------------------------------------------------------------------*/

int LinearFunction::compute( bool changedvars )
{
 if( changedvars ) {
  f_value = f_constant_term;  // value of the function
  for( const auto el : v_pairs )
   f_value += el.first->get_value() * el.second;
  }

 return( kOK );
 }

/*--------------------------------------------------------------------------*/

Function::FunctionValue LinearFunction::get_Lipschitz_constant( void )
{
 FunctionValue L = 0;
 for( const auto el : v_pairs )
  L += el.second * el.second;

 return( sqrt( double( L ) ) );
 }

/*--------------------------------------------------------------------------*/

void LinearFunction::get_hessian_approximation( SparseHessian &hessian ) const
{
 hessian.setZero();
 }

/*--------------------------------------------------------------------------*/

void LinearFunction::get_hessian_approximation( DenseHessian &hessian ) const
{
 const Eigen::Index num_active_var = get_num_active_var();
 hessian.setZero( num_active_var , num_active_var );
 }

/*--------------------------------------------------------------------------*/

void LinearFunction::get_linearization_coefficients( FunctionValue * g ,
						     Range range ,
						     Index name )
{
 range.second = std::min( range.second , get_num_active_var() );
 if( range.second <= range.first )
  return;

 for( Index i = range.first ; i < range.second ; i++ )
  *(g++) = v_pairs[ i ].second;
 }

/*--------------------------------------------------------------------------*/

void LinearFunction::get_linearization_coefficients( SparseVector & g ,
						     Range range ,
						     Index name )
{
 c_Index num_active_var = get_num_active_var();
 range.second = std::min( range.second , num_active_var );
 if( range.second <= range.first )
  return;

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < num_active_var )
   g.resize( num_active_var );

  g.reserve( range.second - range.first );

  for( Index i = range.first ; i < range.second ; ++i )
   g.insert( i ) = v_pairs[ i ].second;
  }
 else {                  // The given vector contains some non-zero elements
  if( g.size() != num_active_var )
   throw( std::invalid_argument( "wrong size of nonempty SparseVector g" ) );

  for( Index i = range.first ; i < range.second ; ++i )
   g.coeffRef( i ) = v_pairs[ i ].second;
  }
 }

/*--------------------------------------------------------------------------*/

void LinearFunction::get_linearization_coefficients( FunctionValue * g ,
						     c_Subset & subset ,
						     Index name )
{
 for( const auto & i : subset ) {
  if( i >= get_num_active_var() )
   throw( std::invalid_argument( "wrong index in subset" ) );
  *(g++) = v_pairs[ i ].second;
  }
 }

/*--------------------------------------------------------------------------*/

void LinearFunction::get_linearization_coefficients( SparseVector & g ,
						     c_Subset & subset ,
						     Index name )
{
 c_Index num_active_var = get_num_active_var();

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < num_active_var )
   g.resize( num_active_var );

  g.reserve( subset.size() );

  for( const auto & i : subset ) {
   if( i >= num_active_var )
    throw( std::invalid_argument( "wrong index in subset" ) );
   g.insert( i ) = v_pairs[ i ].second;
   }
  }
 else {                  // The given vector contains some non-zero elements
  if( g.size() != num_active_var )
   throw( std::invalid_argument( "wrong size of nonempty SparseVector g" ) );

  for( const auto & i : subset ) {
   if( i >= num_active_var )
    throw( std::invalid_argument( "wrong index in subset" ) );
   g.coeffRef( i ) = v_pairs[ i ].second;
   }
  }
 }

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LinearFunction -------*/
/*--------------------------------------------------------------------------*/

void LinearFunction::map_active( c_Vec_p_Var & vars , Subset & map ,
				 const bool ordered ) const
{
 if( vars.empty() )
  return;

 if( map.size() < vars.size() )
   map.resize( vars.size() );

 if( ordered ) {
  Index found = 0;
  for( Index i = 0 ; i < v_pairs.size() ; ++i ) {
   auto itvi = std::lower_bound( vars.begin() , vars.end() ,
				 v_pairs[ i ].first );
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
   Index i = LinearFunction::is_active( var );
   if( i >= v_pairs.size() )
    throw( std::invalid_argument( "map_active: some Variable is not active" )
	   );
   *(it++) = i;
   }
  }
 }  // end( LinearFunction::map_active )

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LinearFunction ------------------*/
/*--------------------------------------------------------------------------*/

void LinearFunction::add_variables( v_coeff_pair && vars ,
				    c_ModParam issueMod )
{
 if( vars.empty() )  // actually nothing to add
  return;            // cowardly (and silently) return

 auto added = & vars;
 Index k = v_pairs.size();
 if( k )  // adding to a nonempty set
  v_pairs.insert( v_pairs.end() , vars.begin() , vars.end() );
 else {     // adding to nothing
  v_pairs = std::move( vars );
  added = & v_pairs;
  }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 Vec_p_Var vptr( added->size() );
 for( Index i = 0 ; i < added->size() ; ++i )
  vptr[ i ] = (*added)[ i ].first;

 // a linear function is additive ==> strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
					 this , std::move( vptr ) , k , 0 ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::add_variables )

/*--------------------------------------------------------------------------*/

void LinearFunction::add_variable( ColVariable * const var ,
                                   const Coefficient coeff ,
				   c_ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to add
  return;              // cowardly (and silently) return

 v_pairs.push_back( std::make_pair( var , coeff ) );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a linear function is additive ==> strongly quasi-additive
 f_Observer->add_Modification( std::make_shared<C05FunctionModVarsAddd>(
				      this , Vec_p_Var( { var } ) ,
				      v_pairs.size() - 1 , 0 ,
				      Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::add_variable )

/*--------------------------------------------------------------------------*/

void LinearFunction::modify_coefficient( Index i , Coefficient coeff,
                                         c_ModParam issueMod )
{
 if( i >= v_pairs.size() )
  throw( std::invalid_argument( "modify_coefficient: invalid index" ) );
  
 if( v_pairs[ i ].second == coeff )  // actually nothing to modify
  return;                            // cowardly (and silently) return

 auto diff = coeff - v_pairs[ i ].second;
 v_pairs[ i ].second = coeff;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return; // no one is there: all done

 f_Observer->add_Modification( std::make_shared< C05FunctionModLinRngd >(
				 this, Vec_FunctionValue( { diff } ) ,
				 Vec_p_Var( { v_pairs[ i ].first } ) ,
                                 Range( i , i + 1 ) , FunctionMod::NaNshift ,
				 Observer::par2concern( issueMod ) ),
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::modify_coefficient )

/*--------------------------------------------------------------------------*/

void LinearFunction::modify_coefficients( Vec_FunctionValue && NCoef ,
					  Subset && nms ,
					  c_ModParam issueMod )
{
 if( nms.empty() )
  return;

 if( NCoef.size() < nms.size() )
  throw( std::invalid_argument( "modify_coefficient: NCoef.size < nms.size"
				) );

 auto NCit = NCoef.begin();

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( nms.size() );
  auto vpit = vp.begin();

  for( auto i : nms ) {
   if( i >= v_pairs.size() )
    throw( std::invalid_argument( "modify_coefficients: invalid index" ) );
   *(vpit++) = v_pairs[ i ].first;
   auto di = v_pairs[ i ].second - *NCit;
   v_pairs[ i ].second = *NCit;
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
   if( i >= v_pairs.size() )
    throw( std::invalid_argument( "modify_coefficients: invalid index" ) );
   v_pairs[ i ].second = *(NCit++);
   }

 }  // end( LinearFunction::modify_coefficients( subset ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::modify_coefficients( Vec_FunctionValue && NCoef ,
					  Range range , c_ModParam issueMod )
{
 range.second = std::min( range.second , c_Index( v_pairs.size() ) );
 if( range.second <= range.first )
  return;

 if( NCoef.size() < range.second - range.first )
  throw( std::invalid_argument( "modify_coefficients: NCoef.size too small"
				) );

 auto NCit = NCoef.begin();
 auto strtit = v_pairs.begin() + range.first;
 const auto stopit = v_pairs.begin() + range.second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vp( range.second - range.first );
  auto vpit = vp.begin();

  while( strtit < stopit ) {
   *(vpit++) = strtit->first;
   auto di = *NCit - strtit->second;
   (strtit++)->second = *NCit;
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
   (*(strtit++)).second = *(NCit++);

 }  // end( LinearFunction::modify_coefficients( range ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::remove_variable( c_Index i , c_ModParam issueMod )
{
 if( v_pairs.size() >= i )
  throw( std::logic_error( "less than i Variable are active" ) );

 auto itv = v_pairs.begin() + i;
 auto var = (*itv).first;
 v_pairs.erase( itv );       // erase it

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a linear function is additive ==> strongly quasi-additive
 f_Observer->add_Modification( std::make_shared<C05FunctionModVarsRngd>(
					this , Vec_p_Var( { var } ) ,
			                Range( i , i + 1 ) , 0 ,
				        Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::remove_variable( index ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::remove_variables( Range range , c_ModParam issueMod )
{
 range.second = std::min( range.second , c_Index( v_pairs.size() ) );
 if( range.second <= range.first )
  return;

 const auto strtit = v_pairs.begin() + range.first;
 const auto stopit = v_pairs.begin() + range.second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vars( range.second - range.first );
  auto vpit = vars.begin();
  for( auto tmpit = strtit ; tmpit < stopit ; )
   *(vpit++) = (*(tmpit++)).first;

  v_pairs.erase( strtit , stopit );

  // now issue the Modification
  // a linear function is additive ==> strongly quasi-additive
  f_Observer->add_Modification( std::make_shared<C05FunctionModVarsRngd>(
				       this , std::move( vars ) , range , 0 ,
				       Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  v_pairs.erase( strtit , stopit );

 }  // end( LinearFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::remove_variables( Subset && nms , bool ordered ,
				       c_ModParam issueMod )
{
 if( nms.empty() )  // actually nothing to remove
  return;           // cowardly (and silently) return

 if( v_pairs.empty() )  // deleting from nothing
  throw( std::logic_error( "deleting from an empty set" ) );

 if( ! ordered )
  std::sort( nms.begin() , nms.end() );
 
 auto it = nms.begin();
 if( ( *it >= v_pairs.size() ) || ( nms.back() >= v_pairs.size() ) )
  throw( std::invalid_argument( "wrong index in LinearFunction" ) );

 auto vi = *it;    // first element to be eliminated
 auto curr = v_pairs.begin() + vi;   // position where to move stuff

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification
  // (as it will be destroyed during the process)

  Vec_p_Var vars( nms.size() );
  auto its = vars.begin();

  *(its++) = v_pairs[ *(it++) ].first;
  ++vi;              // skip the first element, it will be overwritten

  for( ; it < nms.end() ; ++vi )
   if( *it == vi )                // one element to be eliminated
    *(its++) = v_pairs[ *(it++) ].first;  // skip it, but record the Variable
   else
    *(curr++) = v_pairs[ vi ];   // move in the current position

  auto itv = v_pairs.begin() + vi;
  for( ; itv < v_pairs.end(); )  // copy the last part
   *(curr++) = *(itv++);         // after the last of nms[]

  v_pairs.erase( curr, itv );    // erase the last part

  // now issue the Modification
  // a linear function is additive ==> strongly quasi-additive
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
					 this , std::move( vars ) ,
					 std::move( nms ) , 0 ,
                                         Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else {  // noone is there: just do it
  ++it;              // skip the first element
  ++vi;              // as it will be overwritten

  for( ; it < nms.end() ; ++vi )
   if( *it == vi )               // one element to be eliminated
    ++it;                        // skip it
   else
    *(curr++) = v_pairs[ vi ];   // move in the current position

  auto itv = v_pairs.begin() + vi;
  for( ; itv < v_pairs.end(); )  // copy the last part
   *(curr++) = *(itv++);         // after the last of v_var

  v_pairs.erase( curr, itv );    // erase the last part
  }
 }  // end( LinearFunction::remove_variables( subset ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::set_constant_term( const FunctionValue constant_term  ,
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

/*--------------------------------------------------------------------------*/
/*---------------------- End File LinearFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
