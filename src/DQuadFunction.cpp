/*--------------------------------------------------------------------------*/
/*------------------------ File DQuadFunction.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the DQuadFunction class.
 *
 * \version 0.20
 *
 * \date 14 - 03 - 2019
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
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
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
  for( const auto &triple : v_triples ) {
   auto variable_value = std::get<0>(triple)->get_value();
   f_value += variable_value * ( std::get<1>(triple) +
				 std::get<2>(triple) * variable_value );
   }
  }

 return( kOK );
 }

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_convex( void ) const
{
 for( const auto &triple : v_triples )
  if( std::get<2>( triple ) < 0 )
   return( false );

 return( true );
 }

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_concave( void ) const
{
 for( const auto &triple : v_triples )
  if( std::get<2>(triple) > 0 )
   return( false );

 return( true );
 }

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_linear( void ) const
{
 for( const auto &triple : v_triples )
  if( std::get<2>(triple) != 0 )
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

double DQuadFunction::get_linearization_coefficient( c_Index i ) const
{
 if( ( i < 0 ) || ( i >= v_triples.size() ) )
  throw( std::invalid_argument( "wrong index in "
	                  "DQuadFunction::get_linearization_coefficient" ) );

 return( 2 * std::get<0>( v_triples[ i ] )->get_value() *
	 std::get<2>( v_triples[ i ] ) + std::get<1>( v_triples[ i ] ) );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( FunctionValue * g ,
				  c_Vec_Index * const indices ,
				  c_Index start , c_Index end ) const
{
 c_Index tend = std::min( end , get_num_active_var() );
 for( const auto & i : *indices )
  if( ( i >= start ) && ( i < tend ) )
    *(g++) = get_linearization_coefficient( i );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( FunctionValue * g ,
					  c_Index start , c_Index end ) const
{
 c_Index tend = std::min( end , get_num_active_var() );
 for( Index i = start ; i < tend ; i++ )
    g[ i - start ] = get_linearization_coefficient( i );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( FunctionValue * g ,
				 const LinearizationName name ,
                                 const std::vector<Index> * const indices ,
                                 const Index start , const Index end )
{
 if( indices != nullptr )
  get_linearization_coefficients( g , indices , start , end );
 else
  get_linearization_coefficients( g , start , end );
 }

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_linearization_coefficients( SparseVector & g ,
				         const LinearizationName name ,
                                         c_Vec_Index * const indices ,
                                         c_Index start , c_Index end )
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
     if( ( i >= start ) && ( i < tend ) ) {
       g.insert( i ) = get_linearization_coefficient( i );
     }
   }
  else
    for( Index i = start ; i < tend ; ++i ) {
      g.insert( i ) = get_linearization_coefficient( i );
    }
  }
 else {                  // The given vector contains some non-zero elements
  if( g.size() != num_active_var )
   throw( std::invalid_argument(
	    "DQuadFunction::get_linearization_coefficients: "
	    "the size of the sparse vector must be equal to the number "
	    "of active Variables of the Function" ) );

  if( indices ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < tend ) )
     g.coeffRef( i ) = get_linearization_coefficient( i );
   }
  else
   for( Index i = start ; i < tend ; ++i )
    g.coeffRef( i ) = get_linearization_coefficient( i );
  }
 }  // end( DQuadFunction::get_linearization_coefficients( SparseVector ) )

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE DQuadFunction --------*/
/*--------------------------------------------------------------------------*/

void DQuadFunction::map_active( c_Vec_p_Var & vars , Vec_Index & map ,
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
 auto itvv = std::lower_bound( v_triples.begin() , v_triples.end() ,
			       std::make_tuple( *itvb , 0 , 0 ) ,
                               []( const auto & p1 , const auto & p2 ) {
				 return( std::get<0>( p1 ) < std::get<0>( p2 ) );
			        }
			       );
 auto itve = std::upper_bound( itvv , v_triples.end() ,
			       std::make_tuple( *(--vars.end()) , 0 , 0 ) ,
			       []( const auto & p1 , const auto & p2 ) {
				 return( std::get<0>( p1 ) < std::get<0>( p2 ) );
			        }
			       );
 auto itm = map.begin();
 while( itvb < vars.end() ) {
  if( itvv >= itve )
   throw( std::invalid_argument( "some Variable is not active" ) );

  *(itm++) = std::distance( v_triples.begin() , itvv );
  itvv = std::lower_bound( itvv , itve , std::make_tuple( *(++itvb) , 0 , 0 ) ,
			   []( const auto & p1, const auto & p2 ) {
			     return( std::get<0>( p1 ) < std::get<0>( p2 ) );
			    }
			   );
  }
 }  // end( DQuadFunction::map_active )

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE DQuadFunction -------------------*/
/*--------------------------------------------------------------------------*/

void DQuadFunction::add_variables( v_var_coeff_coeff_triple && vars ,
				   const bool ordered , c_ModParam issueMod )
{
 if( vars.empty() )  // actually nothing to add
  return;            // cowardly (and silently) return

 if( ! ordered )
  std::sort( vars.begin() , vars.end() ,
	     []( const var_coeff_coeff_triple & x ,
		 const var_coeff_coeff_triple & y ) {
	       return( std::get<0>( x ) < std::get<0>( y ) );
	     }
	     );

 if( v_triples.empty() ) {    // adding to nothing
  v_triples = std::move( vars );

  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   issue_add_variables_modification( v_triples , issueMod );
  }
 else {                         // adding to a nonempty set
  v_var_coeff_coeff_triple join( vars.size() + v_triples.size() );
  auto newit = vars.begin();
  auto oldit = v_triples.begin();
  auto joinit = join.begin();
  for( ; ( newit != vars.end() ) && ( oldit != v_triples.end() ) ; ) {
    if( std::get<0>( *newit ) == std::get<0>( *oldit ) )
     throw( std::invalid_argument(
       "add_variables: some Variable is already active in the Function" ) );

   if( std::get<0>( *newit ) < std::get<0>( *oldit ) )
    *(joinit++) = *(newit++);
   else
    *(joinit++) = *(oldit++);
   }

  for( ; newit != vars.end() ; )
   *(joinit++) = *(newit++);

  for( ; oldit != v_triples.end() ; )
   *(joinit++) = *(oldit++);

  v_triples = std::move( join );

  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   issue_add_variables_modification( vars , issueMod );
  }
 }  // end( DQuadFunction::add_variables )

/*--------------------------------------------------------------------------*/

void DQuadFunction::add_variable( ColVariable * const var ,
				  const Coefficient linear_coeff ,
				  const Coefficient quadratic_coeff ,
				  c_ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to add
  return;              // cowardly (and silently) return

 auto triple = std::make_tuple( var , linear_coeff , quadratic_coeff );

 if( v_triples.empty() )  // adding to nothing
  v_triples.push_back( triple );
 else {                     // adding to a nonempty set
  // search where the variable lives
  auto itv = std::lower_bound( v_triples.begin() , v_triples.end() , triple ,
			       []( const var_coeff_coeff_triple &a ,
				   const var_coeff_coeff_triple &b )
			       { return( std::get<0>( a ) < std::get<0>( b ) );
			       } );
  if( itv != v_triples.end() ) {  // in the middle of the list
   if( std::get<0>( *itv ) == var )
    throw( std::invalid_argument(
                    "add_variables: Variable is already in the Function" ) );

   v_triples.insert( itv , triple );
   }
  else                           // at the end of the list
   v_triples.push_back( triple );
  }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a diagonal quadratic function is additive ==> strongly quasi-additive
 // note that there is only one Variable, hence it is ordered
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
				      FunctionModVars::AddVar ,
				      Vec_p_Var( { var } ) , true , 0 , true ,
				      Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::add_variable )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_term( ColVariable * const var ,
				 const Coefficient linear_coeff ,
				 const Coefficient quadratic_coeff ,
				 c_ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to modify
  return;              // cowardly (and silently) return

 // look for position of var
 auto itv = std::find_if( v_triples.begin() , v_triples.end() ,
			  [ var ]( const var_coeff_coeff_triple &p )
			  { return( std::get<0>( p ) == var ); } );

 if( itv == v_triples.end() ) // if the Variable is not there
  throw( std::invalid_argument( "Variable is not active" ) );

 if( ( std::get<1>( *itv ) == linear_coeff ) &&
     ( std::get<2>( *itv ) == quadratic_coeff ) ) // actually nothing to modify
  return;                                    // cowardly (and silently) return

 std::get<1>( *itv ) = linear_coeff;     // modify the linear coefficient in
 std::get<2>( *itv ) = quadratic_coeff;  // modify the quadratic coefficient

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>( this ,
				    C05FunctionMod::AllLinearizationChanged ,
				    Vec_p_Var( { var } ) , true ,
				    FunctionMod::NaNshift ,
				    Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::modify_term )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_linear_coefficient( ColVariable * const var ,
					       const Coefficient coeff ,
					       c_ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to modify
  return;              // cowardly (and silently) return

 // look for position of var
 auto itv = std::find_if( v_triples.begin() , v_triples.end() ,
			  [ var ]( const var_coeff_coeff_triple &p )
			  { return( std::get<0>( p ) == var ); } );

 if( itv == v_triples.end() ) // if the Variable is not there
  throw( std::invalid_argument( "Variable is not active" ) );

 if( std::get<1>( *itv ) == coeff )  // actually nothing to modify
  return;                            // cowardly (and silently) return

 auto diff = coeff - std::get<1>( *itv );
 std::get<1>( *itv ) = coeff;        // modify the linear coefficient

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                            // noone is there: all done

 f_Observer->add_Modification( std::make_shared<C05FunctionModLin>( this ,
			                Vec_FunctionValue( { diff } ),
			                Vec_p_Var( { var } ) , true ,
			                FunctionMod::NaNshift ,
			                Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::modify_linear_coefficient )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_terms( v_var_coeff_coeff_triple && vars ,
				  const bool ordered , c_ModParam issueMod )
{
 if( vars.empty() )  // actually nothing to modify
  return;            // all done

 if( v_triples.empty() )  // modifying nothing
  throw( std::logic_error( "modifying an empty set" ) );

 if( ! ordered )
  std::sort( vars.begin() , vars.end() ,
	     []( const var_coeff_coeff_triple & x ,
		 const var_coeff_coeff_triple & y ) {
	       return( std::get<0>( x ) < std::get<0>( y ) );
	     }
	     );

 auto itv = v_triples.begin();
 for( auto it = vars.begin() ; it != vars.end() ; ++it , ++itv ) {
  // look for position of next variable to be modified
  itv = std::find_if( itv , v_triples.end() ,
                      [ &it ]( const var_coeff_coeff_triple &p )
		      { return( std::get<0>( p ) == std::get<0>( *it ) ); } );

  if( itv == v_triples.end() )  // if the variable is not there
   throw( std::invalid_argument( "some Variable is not active" ) );

  std::get<1>( *itv ) = std::get<1>( *it );  // modify the linear coefficient
  std::get<2>( *itv ) = std::get<2>( *it );  // modify the quadratic coefficient
  }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 Vec_p_Var vp( vars.size() );
 for( Index i = 0 ; i < vars.size() ; ++i )
   vp[ i ] = std::get<0>( vars[ i ] );

 f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>( this ,
				    C05FunctionMod::AllLinearizationChanged ,
				    std::move( vp ) , true ,
				    FunctionMod::NaNshift ,
				    Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::modify_terms( subset ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_linear_coefficients( Vec_FunctionValue && NCoef ,
						Vec_p_Var && vars ,
						const bool ordered ,
						c_ModParam issueMod )
{
 if( vars.size() != NCoef.size() )
  throw( std::invalid_argument( "vars and NCoef sizes do not match" ) );

 if( ! ordered ) {
  std::vector<Index> ord( vars.size() );
  std::iota( ord.begin() , ord.end() , 0 );
  std::sort( ord.begin() , ord.end() ,
	     [ & vars ]( Function::Index i , Function::Index j ) {
	      return( vars[ i ] < vars[ j ] ); }
	      );
  Vec_FunctionValue nc( vars.size() );
  Vec_p_Var v( vars.size() );
  for( Function::Index i = 0 ; i < ord.size() ; ++i ) {
   nc[ i ] = NCoef[ ord[ i ] ];
   v[ i ] = vars[ ord[ i ] ];
   }
  NCoef = std::move( nc );
  vars = std::move( v );
  }

 auto itv = v_triples.begin();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) ) {
  // noone is there: just do it

  for( Index i = 0 ; i < vars.size() ; ) {
   // look for position of next variable to be modified
   auto var = vars[ i ];
   itv = std::find_if( itv , v_triples.end() ,
		       [ &var ]( const var_coeff_coeff_triple &p )
		       { return( std::get<0>( p ) == var ); } );

   if( itv == v_triples.end() )  // if the variable is not there
    throw( std::invalid_argument( "some Variable is not active" ) );

   std::get<1>( *itv ) = NCoef[ i++ ];  // modify the linear coefficient
   }
  }
 else {
  // somebody is there: meanwhile, prepare data for the Modification

  for( Index i = 0 ; i < vars.size() ; ) {
   // look for position of next variable to be modified
   auto var = vars[ i ];
   itv = std::find_if( itv , v_triples.end() ,
		       [ &var ]( const var_coeff_coeff_triple &p )
		       { return( std::get<0>( p ) == var ); } );

   if( itv == v_triples.end() )  // if the variable is not there
    throw( std::invalid_argument( "some Variable is not active" ) );

   auto diff = NCoef[ i ] - std::get<1>( *itv );
   std::get<1>( *itv ) = NCoef[ i ];  // modify the linear coefficient
   NCoef[ i++ ] = diff;
   }

  // now issue the Modification
  f_Observer->add_Modification( std::make_shared<C05FunctionModLin>( this ,
					std::move( NCoef ) ,
					std::move( vars ) , true ,
			                FunctionMod::NaNshift ,
			                Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 }  // end( DQuadFunction::modify_linear_coefficients )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_terms( c_v_coeff_coeff_it NCoef , c_Vec_Index &nms ,
				  c_ModParam issueMod )
{
 if( ! nms.size() )
  return;

 if( v_triples.empty() )  // modifying nothing
  throw( std::logic_error( "modifying an empty set" ) );

 auto it = nms.begin();
 if( *it >= v_triples.size() )  // if the first name is wrong
  throw( std::invalid_argument( "wrong index in DQuadFunction" ) );

 if( nms.back() >= v_triples.size() )  // if the last name is wrong
  throw( std::invalid_argument( "wrong index in DQuadFunction" ) );

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  Vec_p_Var vp( nms.size() );
  auto vpit = vp.begin();

  while( it < nms.end() ) {
   (*(vpit++)) = std::get<0>( v_triples[ *it ] );
   std::get<1>( v_triples[ *it ] ) = (*(NCoef)).first;
   std::get<2>( v_triples[ *(it++) ] ) = (*(NCoef++)).second;
   }

  f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>( this ,
				    C05FunctionMod::AllLinearizationChanged ,
				    std::move( vp ) , true ,
				    FunctionMod::NaNshift ,
				    Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else
  while( it < nms.end() ) {
   std::get<1>( v_triples[ *it ] ) = (*(NCoef)).first;
   std::get<2>( v_triples[ *(it++) ] ) = (*(NCoef++)).second;
   }

 }  // end( DQuadFunction::modify_terms( indices ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_terms( c_v_coeff_coeff_it NCoef ,
				  c_Index strt , Index stop ,
				  c_ModParam issueMod )
{
 stop = std::min( stop , c_Index( v_triples.size() ) );
 if( stop <= strt )
  return;

 auto strtit = v_triples.begin() + strt;
 const auto stopit = v_triples.begin() + stop;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  Vec_p_Var vp( stop - strt );

  for( auto vpit = vp.begin() ; strtit < stopit ; ) {
   *(vpit++) = std::get<0>( *strtit );
   std::get<1>( *strtit ) = (*(NCoef)).first;
   std::get<2>( *(strtit++) ) = (*(NCoef++)).second;
   }

  f_Observer->add_Modification( std::make_shared<C05FunctionModSbst>( this ,
				    C05FunctionMod::AllLinearizationChanged ,
				    std::move( vp ) , true ,
				    FunctionMod::NaNshift ,
				    Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else
  while( strtit < stopit ) {
   std::get<1>( *strtit ) = (*(NCoef)).first;
   std::get<2>( *(strtit++) ) = (*(NCoef++)).second;
   }

 }  // end( DQuadFunction::modify_terms( range ) )

/*--------------------------------------------------------------------------*/

void DQuadFunction::remove_variable( Variable *var , c_ModParam issueMod )
{
 if( ! var )  // actually nothing to remove
  return;     // cowardly (and silently) return

 if( v_triples.empty() )  // deleting from nothing
  throw( std::logic_error( "deleting from an empty set" ) );

 // search where the variable lives
 auto itv = std::find_if( v_triples.begin() , v_triples.end() ,
			  [ var ]( const var_coeff_coeff_triple & p ) {
			    return( std::get<0>( p ) == var );
			   }
			  );

 if( itv == v_triples.end() )  // if the variable is not there
  throw( std::invalid_argument( "Variable is not active" ) );

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

void DQuadFunction::remove_variables( Vec_p_Var && vars ,
				      const bool ordered ,
				      c_ModParam issueMod )
{
 if( vars.empty() )  // actually nothing to remove
  return;            // cowardly (and silently) return

 if( v_triples.empty() )  // deleting from nothing
  throw( std::logic_error( "deleting from an empty set" ) );

 if( ! ordered )
  std::sort( vars.begin() , vars.end() );

 auto it = vars.begin();
 auto itv = v_triples.begin();

 // search the first variable to be eliminated
 itv = std::find_if( itv , v_triples.end() ,
                     [ &it ]( const var_coeff_coeff_triple &p )
		     { return( std::get<0>( p ) == *it ); } );

 if( itv >= v_triples.end() )  // if the variable is not there
  throw( std::invalid_argument( "a Variable is not active" ) );

 auto curr = itv;  // position where to move stuff
 ++it;             // skip the first elements
 ++itv;            // as they have been processed already
 for( ; it < vars.end() ; ++itv ) {
  if( *it < std::get<0>( *itv ) )
   throw( std::invalid_argument( "a Variable is not active" ) );

  if( *it == std::get<0>( *itv ) )  // one element to be eliminated
   ++it;                   // skip it
  else
   *(curr++) = *itv;       // move in the current position
  }

 for( ; itv < v_triples.end() ; )  // copy the last part
  *(curr++) = *(itv++);              // after the last of v_var

 v_triples.erase( curr , itv );    // erase the last part

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // now issue the Modification
 // a diagonal quadratic function is additive ==> strongly quasi-additive
 // note that the Variable have been ordered (if they were not so already)
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                       FunctionModVars::RemoveVar ,
				       std::move( vars ) , true , 0 , true ,
				       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( DQuadFunction::remove_variables( pointers ) )

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
( v_var_coeff_coeff_triple & triples , c_ModParam issueMod ) {

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
