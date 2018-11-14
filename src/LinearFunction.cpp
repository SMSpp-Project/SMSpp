/*--------------------------------------------------------------------------*/
/*----------------------- File LinearFunction.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the LinearFunction class.
 *
 * \version 0.10
 *
 * \date 05 - 04 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Department of Applied Mathematics \n
 *         State University of Campinas, Brazil \n
 *
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Observer.h"
#include "SMSTypedefs.h"
#include "LinearFunction.h"
#include <math.h>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void LinearFunction::register_Observer( Observer * const observer )
{
 if( f_Observer == observer )  // actually changing nothing
  return;                      // cowardly (and silently) return

 // if there was a previous Observer and it was a ThinVarDepInterface, then
 // un-register it from all the Variable of the LinearFunction
 auto TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 if( TVDIO )
  for( auto pair : v_pairs )
   pair.first->remove_active( TVDIO );

 f_Observer = observer;

 // if the new Observer is a ThinVarDepInterface, then register it to all
 // the Variable of the LinearFunction
 TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 if( TVDIO )
  for( auto pair : v_pairs )
   pair.first->add_active( TVDIO );

 }  // end( register_Observer )

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
					  c_Vec_Index * const indices ,
					  c_Index start , c_Index end ) const
{
 c_Index tend = std::min( end , get_num_active_var() );
 for( const auto & i : *indices )
  if( ( i >= start ) && ( i < tend ) )
   g[ i - start ] = v_pairs[ i ].second;
 }

/*--------------------------------------------------------------------------*/

void LinearFunction::get_linearization_coefficients( FunctionValue * g ,
					  c_Index start , c_Index end ) const
{
 c_Index tend = std::min( end , get_num_active_var() );
 for( Index i = start ; i < tend ; i++ )
  g[ i - start ] = v_pairs[ i ].second;
 }

/*--------------------------------------------------------------------------*/

void LinearFunction::get_linearization_coefficients( FunctionValue * g ,
				 const LinearizationName name ,
                                 const std::vector<Index> * const indices ,
                                 const Index start , const Index end ) const
{
 if( indices != nullptr )
  get_linearization_coefficients( g , indices , start , end );
 else
  get_linearization_coefficients( g , start , end );
 }

/*--------------------------------------------------------------------------*/

void LinearFunction::get_linearization_coefficients( SparseVector & g ,
				         const LinearizationName name ,
                                         c_Vec_Index * const indices ,
                                         c_Index start , c_Index end ) const
{
 c_Index num_active_var = get_num_active_var();
 c_Index tend = std::min( end , num_active_var );
 if( tend <= start )
  return;

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < num_active_var )
   g.resize( num_active_var );

  g.reserve( tend - start );

  if( indices != nullptr ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < tend ) )
     g.insert( i ) = v_pairs[ i ].second;
   }
  else
   for( Index i = start ; i < tend ; ++i )
    g.insert( i ) = v_pairs[ i ].second;
  }
 else {                  // The given vector contains some non-zero elements
  if( g.size() != num_active_var )
   throw( std::invalid_argument(
	    "LinearFunction::get_linearization_coefficients: "
	    "the size of the sparse vector must be equal to the number "
	    "of active Variables of the Function" ) );
  
  if( indices ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < tend ) )
     g.coeffRef( i ) = v_pairs[ i ].second;
   }
  else
   for( Index i = start ; i < tend ; ++i )
    g.coeffRef( i ) = v_pairs[ i ].second;
  }
 }  // end( LinearFunction::get_linearization_coefficients( SparseVector ) )

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LinearFunction -------*/
/*--------------------------------------------------------------------------*/

void LinearFunction::map_active( c_Vec_p_Var & vars , Vec_Index & map ,
				 const bool ordered ) const
{
 if( ! vars.size() )
  return;

 if( ! ordered ) {
  ThinVarDepInterface::map_active( vars , map );
  return;
  }
   
 if( map.size() < vars.size() )
  map.resize( vars.size() );

 auto itvb = vars.begin();
 auto itvv = std::lower_bound( v_pairs.begin() , v_pairs.end() ,
                               std::make_pair( *itvb , 0 ) ,
                               []( const auto & p1 , const auto & p2 ) {
				return( p1.first < p2.first );
			        }
			       );
 auto itve = std::upper_bound( itvv , v_pairs.end() ,
			       std::make_pair( *(--vars.end()) , 0 ) ,
			       []( const auto & p1 , const auto & p2 ) {
				return( p1.first < p2.first );
			        }
			       );
 auto itm = map.begin();
 while( itvb < vars.end() ) {
  if( itvv >= itve )
   throw( std::invalid_argument( "some Variable is not active" ) );

  *(itm++) = std::distance( itvv , v_pairs.begin() );
  itvv = std::lower_bound( itvv , itve , std::make_pair( *(++itvb) , 0 ) ,
			   []( const auto & p1, const auto & p2 ) {
			    return( p1.first < p2.first );
			    }
			   );
  }
 }  // end( LinearFunction::map_active )

/*--------------------------------------------------------------------------*/

void LinearFunction::remove_variable( Variable * var , c_ModParam issueMod )
{
 if( ! var )  // actually nothing to remove
  return;     // cowardly (and silently) return

 if( v_pairs.empty() )  // deleting from nothing
  throw( std::logic_error( "deleting from an empty set" ) );

 // search where the variable lives
 auto itv = std::find_if( v_pairs.begin() , v_pairs.end() ,
			  [ var ]( const coeff_pair & p ) {
			   return( p.first == var );
			   }
			  );

 if( itv == v_pairs.end() )  // if the variable is not there
  throw( std::invalid_argument( "Variable is not active" ) );

 v_pairs.erase( itv );       // erase it

 if( ! f_Observer )
  return;

 // if the Observer is a ThinVarDepInterface, un-register it from var
 auto TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 if( TVDIO )
  var->remove_active( TVDIO );

 if( ! f_Observer->issue_mod( issueMod ) )
  return;

 f_Observer->add_Modification( std::make_shared<LinearFunctionModSbst>( this ,
                                       FunctionModVars::RemoveVar ,
				       Vec_p_Var( { var } ) , 0 ,
				       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::remove_variable( pointer ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::remove_variable( c_Index i , c_ModParam issueMod )
{
 if( v_pairs.size() >= i )
  throw( std::logic_error( "less than i Variable are active" ) );

 auto itv = v_pairs.begin() + i;
 auto var = (*itv).first;
 v_pairs.erase( itv );       // erase it

 if( ! f_Observer )
  return;

 // if the Observer is a ThinVarDepInterface, un-register it from var
 auto TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 if( TVDIO )
  var->remove_active( TVDIO );

 if( ! f_Observer->issue_mod( issueMod ) )
  return;

 f_Observer->add_Modification( std::make_shared<LinearFunctionModSbst>( this ,
                                       FunctionModVars::RemoveVar ,
				       Vec_p_Var( { var } ) , 0 ,
				       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::remove_variable( index ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::remove_variables( c_Index strt , Index stop ,
				       c_ModParam issueMod )
{
 stop = std::min( stop , c_Index( v_pairs.size() ) );
 if( stop <= strt )
  return;

 const auto strtit = v_pairs.begin() + strt;
 const auto stopit = v_pairs.begin() + stop;

 // if the Observer is a ThinVarDepInterface, un-register it from the vars
 auto TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 if( TVDIO )
  for( auto it = strtit ; it < stopit ; )
   (*(it++)).first->remove_active( TVDIO );
 
 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  Variable * const vstrt = strt ? v_pairs[ strt ].first : nullptr;
  Variable * const vstop = stop < v_pairs.size() ?
                                  v_pairs[ stop ].first : nullptr;

  v_pairs.erase( strtit , stopit );

  f_Observer->add_Modification( std::make_shared<LinearFunctionModRngd>(
			    this , FunctionModVars::RemoveVar , vstrt ,
			    vstop , 0 , Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );
  }
 else
  v_pairs.erase( strtit , stopit );

 }  // end( LinearFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::remove_variables( Vec_p_Var && vars ,
				       const bool ordered ,
				       c_ModParam issueMod )
{
 if( vars.empty() )  // actually nothing to remove
  return;            // cowardly (and silently) return

 if( v_pairs.empty() )  // deleting from nothing
  throw( std::logic_error( "deleting from an empty set" ) );

 if( ! ordered )
  std::sort( vars.begin() , vars.end() );

 auto it = vars.begin();
 auto itv = v_pairs.begin();

 // search the first variable to be eliminated
 itv = std::find_if( itv , v_pairs.end() ,
                     [ &it ]( const coeff_pair &p )
                            { return( p.first == *it ); } );

 if( itv >= v_pairs.end() )  // if the variable is not there
  throw( std::invalid_argument( "a Variable is not active" ) );

 auto curr = itv;  // position where to move stuff
 ++it;             // skip the first elements
 ++itv;            // as they have been processed already
 for( ; it < vars.end() ; ++itv ) {
  if( *it < itv->first )
   throw( std::invalid_argument( "a Variable is not active" ) );

  if( *it == itv->first )  // one element to be eliminated
   ++it;                   // skip it
  else
   *(curr++) = *itv;       // move in the current position
  }

 for( ; itv < v_pairs.end() ; )  // copy the last part
  *(curr++) = *(itv++);              // after the last of v_var

 v_pairs.erase( curr , itv );    // erase the last part

 if( ! f_Observer )
  return;

 // if the Observer is a ThinVarDepInterface, un-register it from the vars
 auto TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 if( TVDIO )
  for( auto var :  vars )
   var->remove_active( TVDIO );

 if( ! f_Observer->issue_mod( issueMod ) )
  return;

 f_Observer->add_Modification( std::make_shared<LinearFunctionModSbst>(
					 this , FunctionModVars::RemoveVar ,
					 std::move( vars ) , 0 ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::remove_variables( pointers ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::remove_variables( c_Vec_Index & nms ,
				       c_ModParam issueMod )
{
 if( nms.empty() )  // actually nothing to remove
  return;           // cowardly (and silently) return

 if( v_pairs.empty() )  // deleting from nothing
  throw( std::logic_error( "deleting from an empty set" ) );

 auto it = nms.begin();
 if( *it >= v_pairs.size() )  // if the first name is wrong
  throw( std::invalid_argument( "wrong index in LinearFunction" ) );

 if( nms.back() >= v_pairs.size() )  // if the last name is wrong
  throw( std::invalid_argument( "wrong index in LinearFunction" ) );

 auto vi = *it;    // first element to be eliminated
 auto curr = v_pairs.begin() + vi;   // position where to move stuff

 // if the Observer is a ThinVarDepInterface, un-register it from the vars
 auto TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 if( TVDIO )
  v_pairs[ *(it++) ].first->remove_active( TVDIO );
 else
  ++it;             // skip the first elements
 ++vi;              // as they have been processed already

 if( TVDIO )
  for( ; it < nms.end() ; ++vi ) {
   if( *it == vi )                    // one element to be eliminated
    v_pairs[ *(it++) ].first->remove_active( TVDIO );  // skip it
                                      // and meanwhile un-register it
   else
    *(curr++) = v_pairs[ vi ];    // move in the current position
   }
 else
  for( ; it < nms.end() ; ++vi ) {
   if( *it == vi )                    // one element to be eliminated
    ++it;                             // skip it
   else
    *(curr++) = v_pairs[ vi ];    // move in the current position
   }

 auto itv = v_pairs.begin() + vi;
 for( ; itv < v_pairs.end() ; )   // copy the last part
  *(curr++) = *(itv++);               // after the last of v_var

 v_pairs.erase( curr , itv );     // erase the last part

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 Vec_p_Var vars( nms.size() );
 auto its = vars.begin();
 for( auto nm : nms )
  *(its++) = v_pairs[ nm ].first;

 f_Observer->add_Modification( std::make_shared<LinearFunctionModSbst>( this ,
				       FunctionModVars::RemoveVar ,
				       std::move( vars ) , 0 ,
				       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::remove_variables( indices ) )

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LinearFunction ------------------*/
/*--------------------------------------------------------------------------*/

void LinearFunction::add_variables( v_coeff_pair && vars ,
				    const bool ordered , c_ModParam issueMod )
{
 if( vars.empty() )  // actually nothing to add
  return;            // cowardly (and silently) return

 if( ! ordered )
  std::sort( vars.begin() , vars.end() , []( const coeff_pair & x ,
                                             const coeff_pair & y ) {
                                          return( x.first < y.first );
                                          }
	     );

 // if the Observer is a ThinVarDepInterface, register it with the vars
 auto TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 if( TVDIO )
  for( auto pair : vars )
   pair.first->add_active( TVDIO );

 if( v_pairs.empty() ) {    // adding to nothing
  v_pairs = std::move( vars );

  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   issue_add_variables_modification( v_pairs , issueMod );
  }
 else {                         // adding to a nonempty set
  v_coeff_pair join( vars.size() + v_pairs.size() );
  auto newit = vars.begin();
  auto oldit = v_pairs.begin();
  auto joinit = join.begin();
  for( ; ( newit != vars.end() ) && ( oldit != v_pairs.end() ) ; ) {
   if( newit->first == oldit->first )
    throw( std::invalid_argument(
       "add_variables: some Variable is already active in the Function" ) );

   if( newit->first < oldit->first )
    *(joinit++) = *(newit++);
   else
    *(joinit++) = *(oldit++);
   }

  for( ; newit != vars.end() ; )
   *(joinit++) = *(newit++);

  for( ; oldit != v_pairs.end() ; )
   *(joinit++) = *(oldit++);

  v_pairs = std::move( join );

  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   issue_add_variables_modification( vars , issueMod );
  }
 }  // end( LinearFunction::add_variables )

/*--------------------------------------------------------------------------*/

void LinearFunction::add_variable( ColVariable * const var ,
                                   const Coefficient coeff ,
				   c_ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to add
  return;              // cowardly (and silently) return

 // if the Observer is a ThinVarDepInterface, register it with var
 auto TVDIO = dynamic_cast<ThinVarDepInterface *>( f_Observer );

 if( TVDIO )
  var->add_active( TVDIO );

 auto pair = std::make_pair( var , coeff );

 if( v_pairs.empty() )  // adding to nothing
  v_pairs.push_back( pair );
 else {                     // adding to a nonempty set
  // search where the variable lives
  auto itv = std::upper_bound( v_pairs.begin() , v_pairs.end() ,
			       pair ,
			       []( const coeff_pair &a , const coeff_pair &b )
			         { return( a.first < b.first ); } );
  if( itv->first == var )
   throw( std::invalid_argument(
                    "add_variables: Variable is already in the Function" ) );

  v_pairs.insert( itv , pair );
  }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared<LinearFunctionModSbst>( this ,
					 FunctionModVars::AddVar ,
					 Vec_p_Var( { var } ) , 0 ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::add_variable )

/*--------------------------------------------------------------------------*/

void LinearFunction::modify_coefficient( ColVariable * const var ,
					 const Coefficient coeff ,
					 c_ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to modify
  return;              // cowardly (and silently) return

 // look for position of var
 auto itv = std::find_if( v_pairs.begin() , v_pairs.end() ,
			  [ var ]( const coeff_pair &p )
                                 { return( p.first == var ); } );

 if( itv == v_pairs.end() ) // if the Variable is not there
  throw( std::invalid_argument( "Variable is not active" ) );

 itv->second = coeff;  // modify the coefficient

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 f_Observer->add_Modification( std::make_shared<LinearFunctionModSbst>( this ,
                                    LinearFunctionModSbst::SomeEntriesChange ,
			            Vec_p_Var( { var } ) , 0 ,
				    Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::modify_coefficient )

/*--------------------------------------------------------------------------*/

void LinearFunction::modify_coefficients( v_coeff_pair && vars ,
                                          const bool ordered  ,
					  c_ModParam issueMod )
{
 if( vars.empty() )  // actually nothing to modify
  return;            // all done

 if( v_pairs.empty() )  // modifying nothing
  throw( std::logic_error( "modifying an empty set" ) );

 if( ! ordered )
  std::sort( vars.begin() , vars.end() , []( const coeff_pair & x ,
                                             const coeff_pair & y ) {
                                          return( x.first < y.first );
                                          }
	     );

 auto itv = v_pairs.begin();
 for( auto it = vars.begin() ; it != vars.end() ; ++it , ++itv ) {
  // look for position of next variable to be modified
  itv = std::find_if( itv , v_pairs.end() ,
                      [ &it ]( const coeff_pair &p )
                             { return( p.first == it->first ); } );

  if( itv == v_pairs.end() )  // if the variable is not there
   throw( std::invalid_argument( "some Variable is not active" ) );

  itv->second = it->second;  // modify the coefficient
  }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 Vec_p_Var vp( vars.size() );
 for( Index i = 0 ; i < vars.size() ; ++i )
  vp[ i ] = vars[ i ].first;

 f_Observer->add_Modification( std::make_shared<LinearFunctionModSbst>(
			       this ,
			       LinearFunctionModSbst::SomeEntriesChange ,
			       std::move( vp ) , 0 ,
			       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( LinearFunction::modify_coefficients( subset ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::modify_coefficients( c_v_coeff_it NCoef ,
					  c_Vec_Index nms ,
					  c_ModParam issueMod )
{
 if( ! nms.size() )
  return;

 if( v_pairs.empty() )  // deleting from nothing
  throw( std::logic_error( "modifying an empty set" ) );

 auto it = nms.begin();
 if( *it >= v_pairs.size() )  // if the first name is wrong
  throw( std::invalid_argument( "wrong index in LinearFunction" ) );

 if( nms.back() >= v_pairs.size() )  // if the last name is wrong
  throw( std::invalid_argument( "wrong index in LinearFunction" ) );

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  Vec_p_Var vp( nms.size() );
  auto vpit = vp.begin();

  while( it < nms.end() ) {
   (*(vpit++)) = v_pairs[ *it ].first;
   v_pairs[ *(it++) ].second = *(NCoef++);
   }

  f_Observer->add_Modification( std::make_shared<LinearFunctionModSbst>(
			        this ,
			        LinearFunctionModSbst::SomeEntriesChange ,
			        std::move( vp ) , 0 ,
				Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else
  while( it < nms.end() )
   v_pairs[ *(it++) ].second = *(NCoef++);

 }  // end( LinearFunction::modify_coefficients( indices ) )

/*--------------------------------------------------------------------------*/

void LinearFunction::modify_coefficients( c_v_coeff_it NCoef , c_Index strt ,
					  Index stop , c_ModParam issueMod )
{
 stop = std::min( stop , c_Index( v_pairs.size() ) );
 if( stop <= strt )
  return;

 auto strtit = v_pairs.begin() + strt;
 const auto stopit = v_pairs.begin() + stop;
 
 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  Variable * const vstrt = strt ? v_pairs[ strt ].first : nullptr;
  Variable * const vstop = stop < v_pairs.size() ?
                                  v_pairs[ stop ].first : nullptr;

  while( strtit < stopit )
   (*(strtit++)).second = *(NCoef++);

  f_Observer->add_Modification( std::make_shared<LinearFunctionModRngd>(
			    this , C05FunctionModVarsRngd::SomeEntriesChange ,
			    vstrt , vstop , 0 ,
			    Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );
  }
 else
  while( strtit < stopit )
   (*(strtit++)).second = *(NCoef++);

 }  // end( LinearFunction::modify_coefficients( range ) )

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

void LinearFunction::issue_add_variables_modification( v_coeff_pair & pairs ,
						       c_ModParam issueMod )
{
 Vec_p_Var vars( pairs.size() );
 for( Index i = 0 ; i < pairs.size() ; ++i )
  vars[ i ] = pairs[ i ].first;

 f_Observer->add_Modification( std::make_shared<LinearFunctionModSbst>( this ,
                                         FunctionModVars::AddVar ,
					 std::move( vars ) , 0 ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- End File LinearFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
