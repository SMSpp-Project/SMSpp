/*--------------------------------------------------------------------------*/
/*--------------------- File PolyhedralFunction.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the PolyhedralFunction class.
 *
 * \version 0.10
 *
 * \date 16 - 07 - 2019
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

#include "SMSTypedefs.h"
#include "PolyhedralFunction.h"
#include <math.h>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LinearFunction ----------*/
/*--------------------------------------------------------------------------*/

int PolyhedralFunction::compute( bool changedvars )
{
 if( changedvars ) {
  f_next = 0;
  f_value = - Inf<FunctionValue>();
  if( A.empty() )
   return( kOK );

  if( v_ord.size() > 1 ) {
   std::vector<FunctionValue> v;
   for( Index i = 0 ; i < A.size() ; ++i ) {
    v[ i ] = b[ i ];
    for( Index j = 0 ; j < x.size() ; ++j )
     v[ i ] += x[ j ]->get_value() * A[ i ][ j ];
    }

   if( f_is_convex )
    std::sort( v_ord.begin() , v_ord.end() ,
	       [ & v ]( const Index x , const Index y ) {
		return( v[ x ] > v[ y ] );
	       } );
   else
    std::sort( v_ord.begin() , v_ord.end() ,
	       [ & v ]( const Index x , const Index y ) {
		return( v[ x ] < v[ y ] );
	       } );

   f_value = v[ v_ord[ 0 ] ];
   }
  else {
   for( Index i = 0 ; i < A.size() ; ++i ) {
    FunctionValue vi = b[ i ];
    for( Index j = 0 ; j < x.size() ; ++j ) {
     vi += x[ j ]->get_value() * A[ i ][ j ];

     if( f_is_convex ) {
      if( vi > f_value ) {
       f_value = vi;
       v_ord[ 0 ] = i;
       }
      }
     else {
      if( vi < f_value ) {
       f_value = vi;
       v_ord[ 0 ] = i;
       }
      }
     }
    }
   }
  }

 return( kOK );
 }

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::store_combination_of_linearizations(
	    LinearCombination & coefficients , const LinearizationName name )
{
 if( name >= v_glob.size() )
  throw( std::invalid_argument( "invalid global pool name" ) );

 // construct the aggregated linearization in a new vector
 std::vector<FunctionValue> a;
 a.resize( x.size() , 0 );
 FunctionValue b = 0;

 for( const auto & coef : coefficients ) {
  if( v_glob[ coef.first ] == Inf<Index>() )
   throw( std::invalid_argument( "invalid name in coefficients" ) );

  std::vector<FunctionValue> & ai;
  if( v_glob[ coef.first ] < v_A.size() ) {
   ai = & v_A[ v_glob[ coef.first ] ];
   b += v_b[ v_glob[ coef.first ] ] * coef.second;
   }
  else {
   ai = & v_aA[ v_glob[ coef.first ] - v_A.size() ];
   b += v_b[ v_glob[ coef.first ] - v_A.size() ] * coef.second;
   }

  for( Index i = 0 ; i < x.size() ; ++i )
   a[ i ] += ai[ i ] * coef.second;
  }

 // now put the vector in the right place
 
 Index pos = 0;
 if( ( v_glob[ name ] < v_A.size() ) || ( v_glob[ name ] == Inf<Index>() ) ) {
  // a new aggregated linearization must be created
  // serach for a free position in aA[], ab[]
  for( ; pos < v_ab.size() ; ++pos )
   if( v_ab[ pos ] == Inf<FunctionValue>() )
    break;

  if( pos == v_ab.size() ) {
   // no free position is found, thus create one
   v_ab.push_back();
   v_aA.push_back();
   }
  }
 else
  // the aggregated linearization replaces an already aggregated one
  pos = v_glob[ name ] - A.size();

 aA[ pos ] = std::move( a );
 ab[ pos ] = b;

 }  // end( PolyhedralFunction::store_combination_of_linearizations )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::rename_linearization(
				       const LinearizationName current_name ,
				       const LinearizationName new_name )
{
 if( current_name == new_name )  // actually doing nothing
  return;                        // cowardly (and silently) return

 if( current_name >= v_glob.size() )
  throw( std::invalid_argument( "invalid global pool current_name" ) );
 if( v_glob[ current_name ] == Inf<Index>() )
  throw( std::invalid_argument( "no current_name in global pool " ) );
 if( new_name >= v_glob.size() )
  throw( std::invalid_argument( "invalid global pool new_name" ) );

 PolyhedralFunction::delete_linearization( new_name );

 v_glob[ new_name ] = v_glob[ current_name ];
 v_glob[ current_name ] = Inf<Index>();
 
 }  // end( PolyhedralFunction::rename_linearization )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::delete_linearization( const LinearizationName name )
{
 if( name >= v_glob.size() )
  throw( std::invalid_argument( "invalid global pool name" ) );

 if( v_glob[ name ] == Inf<Index>() )  // no item with that name
  return;                              // cowardly and silently return

 if( v_glob[ name ] >= v_A.size() ) {  // it is an aggregated item
  // mark its position in v_ab[] with INF to signal it's not needed
  v_ab[ v_glob[ name ] - v_A.size() ] = Inf<FunctionValue>();
  // until the last position is not needed, shorten v_aA[] and v_ab[]
  while( ! v_ab.empty() ) {
   auto last = --v_ab.end();
   if( *last == Inf<FunctionValue>() ) {
    v_aA.pop_back();
    v_ab.pop_back();
    }
   }
  }

 v_glob[ name ] = Inf<Index>();        // mark the item as deleted already

 }  // end( PolyhedralFunction::delete_linearization )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::get_linearization_coefficients( FunctionValue * g ,
	const LinearizationName name ,
	c_Vec_Index & indices  , const Index start , const Index end )
{
 c_Index tend = std::min( end , v_x.size() );
 if( tend <= start )
  return;

 std::vector<FunctionValue> & ai;
 if( name >= v_glob.size() )
  ai = & v_A[ v_ord[ f_next ] ];
 else {
  auto pos = v_glob[ name ];
  if( pos < v_A.size() )
   ai = & v_A[ pos ];
  else
   ai = & v_aA[ pos - v_A.size() ];
  }

 if( indices.empty() )
  for( ; start < end ; ++start )
   *(g++) = ai[ start ];
 else {
  auto ti = indices.begin();
  while( ( *ti < start ) && ( ti != indices.end() ) )
   ++ti;

  while( ( *ti < emd ) && ( ti != indices.end() ) )
   *(g++) = ai[ *(ti++) ];
  }
 }  // end( PolyhedralFunction::get_linearization_coefficients( array ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::get_linearization_coefficients( SparseVector & g ,
	 const LinearizationName name , c_Vec_Index & indices ,
     c_Index start , c_Index end )
{
 c_Index tend = std::min( end , v_x.size() );
 if( tend <= start )
  return;

 std::vector<FunctionValue> & ai;
 if( name >= v_glob.size() )
  ai = & v_A[ v_ord[ f_next ] ];
 else {
  auto pos = v_glob[ name ];
  if( pos < v_A.size() )
   ai = & v_A[ pos ];
  else
   ai = & v_aA[ pos - v_A.size() ];
  }

 if( g.nonZeros() == 0 ) {
  // the given vector contains no non-zero element
  if( g.size() < x.size() )
   g.resize( x.size() );

  g.reserve( tend - start );

  if( indices.empty() ) {
   for( ; start < end ; ++start ) {
    if( ai[ start ] != 0 )
     g.insert( start ) = ai[ start ];
    }
   }
  else {
   auto ti = indices.begin();
   while( ( *ti < start ) && ( ti != indices.end() ) )
    ++ti;

   for( ; ( *ti < emd ) && ( ti != indices.end() ) ; ++ti )
    if( ai[ *ti ] != 0 )
     g.insert( *ti ) = ai[ *ti ];
   }
  }
 else {
  // the given vector contains some non-zero elements
  if( g.size() != x.size() )
   throw( std::invalid_argument(
	    "PolyhedralFunction::get_linearization_coefficients: "
	    "the size of the sparse vector must be equal to the number "
	    "of active Variables of the Function" ) );

  if( indices.empty() ) {
   for( ; start < end ; ++start ) {
    if( ai[ start ] != 0 )
     g.coeffRef( start ) = ai[ start ];
    }
   }
  else {
   auto ti = indices.begin();
   while( ( *ti < start ) && ( ti != indices.end() ) )
    ++ti;

   for( ; ( *ti < emd ) && ( ti != indices.end() ) ; ++ti )
    if( ai[ *ti ] != 0 )
     g.coeffRef( *ti ) = ai[ *ti ];
   }
  }
 }  // end( PolyhedralFunction::get_linearization_coefficients( sparse ) )

/*--------------------------------------------------------------------------*/
/*--- METHODS FOR HANDLING "ACTIVE" Variable IN THE PolyhedralFunction -----*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunction::map_active( c_Vec_p_Var & vars , Vec_Index & map ,
				     const bool ordered ) const
{
 if( ! v_x.size() )
  return;

 if( ! ordered ) {
  ThinVarDepInterface::map_active( vars , map );
  return;
  }

 if( map.size() < vars.size() )
  map.resize( vars.size() );

 auto itvb = vars.begin();
 auto itvv = std::lower_bound( v_x.begin() , v_x.end() , *itvb );
 auto itve = std::upper_bound( itvv , v_x.end() , *(--vars.end()) );
 auto itm = map.begin();
 while( itvb < vars.end() ) {
  if( itvv >= itve )
   throw( std::invalid_argument( "some Variable is not active" ) );

  *(itm++) = std::distance( v_x.begin() , itvv );
  itvv = std::lower_bound( itvv , itve , *(++itvb) );
  }
 }  // end( PolyhedralFunction::map_active )

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR MODIFYING THE PolyhedralFunction ---------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunction::add_variables( Vec_p_Var && nx , MultiVector && nA ,
				        c_ModParam issueMod )
{
 const Index nn = nx.size();
 if( ! nn )  // actually nothing to add
  return;    // cowardly (and silently) return

 if( nA.size() != v_A.size() )
  throw( std::invalid_argument( "wrong number of rows in nA" ) );

 for( auto & a : nA )
  if( a.size() != nn )
   throw( std::invalid_argument( "all rows nA must have the size of nx" ) );

 const Index n = v_x.size();

 if( ! n ) {    // very easy case: adding to nothing
  v_A = std::move( nA );
  v_x.resize( nm );
  auto tvx = v_x.begin();
  for( const auto nxi : nx )
   *(tvx++) = nxi;

  // now issue the C05FunctionModVars
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                         FunctionModVars::AddVar ,
					 std::move( nx ) , true , 0 , true ,
					 Observer::par2concern( issueMod ) ) ,
				 Observer::par2chnl( issueMod ) );
  return;  // all done
  }

 if( nx.front() > v_x.back() )  {  // easy case: adding at the end
  for( Index i = 0 ; i < v_A.size() ; ++i ) {
   v_A[ i ].resize( n + nn );
   std::copy( nA[ i ].begin() , nA[ i ].end() , v_A[ i ].begin() + n );
   }

  // now issue the C05FunctionModVars
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                         FunctionModVars::AddVar ,
					 std::move( nx ) , true , 0 , true ,
					 Observer::par2concern( issueMod ) ) ,
				 Observer::par2chnl( issueMod ) );
  return;  // all done
  }

 if( nx.back() < v_x.front() )  {  // easy-ish case: adding at the beginning
  for( Index i = 0 ; i < v_A.size() ; ++i )
   v_A[ i ].insert( v_A.begin() , nA[ i ].begin() , nA[ i ].end() );

  // now issue the C05FunctionModVars
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                         FunctionModVars::AddVar ,
					 std::move( nx ) , true , 0 , true ,
					 Observer::par2concern( issueMod ) ) ,
				 Observer::par2chnl( issueMod ) );
  return;  // all done
  }

 // nasty general case: add somewhere in the middle

 // newpos[ i ] = position of nx[ i ] in the merged vector
 // oldpos[ i ] = position of v_x[ i ] in the merged vector
 std::vector<Index> newpos;  // position of new variables in new vectors
 std::vector<Index> oldpos;  // position of old variables in new vectors
 
 auto npi = newpos.begin();
 auto opi = oldpos.begin();
 auto nxi = nx.begin();
 auto xi = v_x.begin();
 Index i = 0;
 for( ; ( nxi != nx.end ) && ( xi != v_x.end() ) ; )
  if( *nxi == *xi )
   throw( std::invalid_argument( "some variable already present" ) );
  else
   if( *nxi < *xi ) {
    *(npi++) = i++;
    ++nxi;
    }
   else {
    *(opi++) = i++;
    ++xi;
    }
   
 for( ; nxi != nx.end ; ) {
  *(npi++) = i++;
  ++nxi;
  }

 for( ; xi != v_x.end() ; ) {
  *(opi++) = i++;
  ++xi;
  }

 assert( i == n + nn );

 // merge v_x and nx
 VarVector t_x( n + nn );
 for( i = n ; i-- ; )
  t_x[ oldpos[ i ] ] = v_x[ i ];
 for( i = 0 ; i < n ; ++i )
  t_x[ newpos[ i ] ] = nx[ i ];
 
 v_x = std::move( t_x );

 // merge v_A and nA
 for( Index j = 0 ; j < v_A.size() ; ++j ) {
  std::vector < FunctionValue > Aj( n + nn );
  for( i = n ; i-- ; )
   Aj[ oldpos[ i ] ] = v_A[ i ];
  for( i = 0 ; i < n ; ++i )
   Aj[ newpos[ i ] ] = nA[ i ];

  v_A[ j ] = std::move( Aj );
  }

 // now issue the C05FunctionModVars
 if( f_Observer && f_Observer->issue_mod( issueMod ) )
  f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                         FunctionModVars::AddVar ,
					 std::move( nx ) , true , 0 , true ,
					 Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::add_variables )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::add_variable( ColVariable * const var ,
				       const std::vector<FunctionValue> & Aj ,
				       c_ModParam issueMod )
{
 if( var == nullptr )  // actually nothing to add
  return;              // cowardly (and silently) return

 auto itr = std::lower_bound( v_x.begin() , v_x.end() , var );
 if( *itr == var ) 
  throw( std::invalid_argument( "var is already present" ) );

 auto pos = std::distance( v_x.begin() , itr );

 v_x.insert( itr , var );


 for( Index j = 0 ; j < v_A.size() ; ++j )
  v_A[ j ].insert( v_A[ j ].begin() + pos , Aj[ j ] );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                         FunctionModVars::AddVar ,
					 { var } , true , 0 , true ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::add_variable )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::modify_row( c_Index i ,
				     std::vector<FunctionValue> && Ai ,
				     c_FunctionValue bi , c_ModParam issueMod )
{
 if( i >= v_A.size() )
  throw( std::invalid_argument( "wrong row name" ) );

 v_A[ i ] = std::move( Ai );
 v_b[ i ] = bi;

 f_value = - Inf<FunctionValue>();  // the function value has changed

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the C05FunctionMod
 f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				     C05FunctionMod::AllLinearizationChanged ,
			             NANshift ,
				     Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::modify_row )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::modify_constant( c_Index i , c_FunctionValue bi ,
					  c_ModParam issueMod )
{
 if( i >= v_A.size() )
  throw( std::invalid_argument( "wrong row name" ) );

 v_b[ i ] = bi;

 f_value = - Inf<FunctionValue>();  // the function value has changed

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the C05FunctionMod
 f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				         C05FunctionMod::AlphaChanged ,
			                 NANshift ,
				         Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::modify_constant )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::add_rows( MultiVector && nA ,
				   std::vector<FunctionValue> & nb ,
				   c_ModParam issueMod )
{
 const Index k = nA.size();
 if( k != nb.size() )
  throw( std::invalid_argument( "nA and nb must have the same size" ) );

 const Index n = v_x.size();
 for( auto & a : nA )
  if( a.size() != n )
   throw( std::invalid_argument( "some rows of nA have a wrong size" ) );

 v_A.insert( v_A.end() , std::make_move_iterator( nA.begin() ) , 
                         std::make_move_iterator( nA.end() ) );

 v_b.insert( v_b.end() , nb.begin(), nb.end() );
 
 f_value = - Inf<FunctionValue>();  // the function value has changed

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the C05FunctionMod
 f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				      C05FunctionMod::NothingChanged ,
				      f_is_convex ? FunctionMod::INFshift :
				                  - FunctionMod::INFshift ,
				      Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::add_rows )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::add_row( std::vector<FunctionValue> && Ai ,
				  c_FunctionValue bi , c_ModParam issueMod )
{
 if( Ai.size() != v_x.size() )
  throw( std::invalid_argument( "Ai has a wrong size" ) );

 v_A.push_back( std::move( Ai ) );
 v_b.push_back( bi );

 f_value = - Inf<FunctionValue>();  // the function value has changed

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the C05FunctionMod
 f_Observer->add_Modification( std::make_shared<C05FunctionMod>( this ,
				      C05FunctionMod::NothingChanged ,
				      f_is_convex ? FunctionMod::INFshift :
				                  - FunctionMod::INFshift ,
				      Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::add_row )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::remove_variable( Variable *var , c_ModParam issueMod )
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

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a linear function is additive ==> strongly quasi-additive
 // note that there is only one Variable, hence it is ordered
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                    FunctionModVars::RemoveVar ,
				    Vec_p_Var( { var } ) , true , 0 , true ,
				    Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::remove_variable( pointer ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::remove_variable( c_Index i , c_ModParam issueMod )
{
 if( v_pairs.size() >= i )
  throw( std::logic_error( "less than i Variable are active" ) );

 auto itv = v_pairs.begin() + i;
 auto var = (*itv).first;
 v_pairs.erase( itv );       // erase it

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // a linear function is additive ==> strongly quasi-additive
 // note that there is only one Variable, hence it is ordered
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                    FunctionModVars::RemoveVar ,
				    Vec_p_Var( { var } ) , true , 0 , true ,
				    Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::remove_variable( index ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::remove_variables( c_Index strt , Index stop ,
				       c_ModParam issueMod )
{
 stop = std::min( stop , c_Index( v_pairs.size() ) );
 if( stop <= strt )
  return;

 const auto strtit = v_pairs.begin() + strt;
 const auto stopit = v_pairs.begin() + stop;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vars( stop - strt );
  auto vpit = vars.begin();
  for( auto tmpit = strtit ; tmpit < stopit ; )
   *(vpit++) = (*(tmpit++)).first;

  v_pairs.erase( strtit , stopit );

  // now issue the Modification
  // a linear function is additive ==> strongly quasi-additive
  // note that the Variable are ordered by construction
  f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                       FunctionModVars::RemoveVar ,
				       std::move( vars ) , true , 0 , true ,
				       Observer::par2concern( issueMod ) ) ,
				Observer::par2chnl( issueMod ) );
  }
 else  // noone is there: just do it
  v_pairs.erase( strtit , stopit );

 }  // end( PolyhedralFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::remove_variables( Vec_p_Var && vars ,
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
  *(curr++) = *(itv++);          // after the last of v_var

 v_pairs.erase( curr , itv );    // erase the last part

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // now issue the Modification
 // a linear function is additive ==> strongly quasi-additive
 // note that the Variable have been ordered (if they were not so already)
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                       FunctionModVars::RemoveVar ,
				       std::move( vars ) , true , 0 , true ,
				       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::remove_variables( pointers ) )

/*--------------------------------------------------------------------------*/

void PolyhedralFunction::remove_variables( Vec_Index & nms , const bool ordered ,
				       c_ModParam issueMod )
{
 if( nms.empty() )  // actually nothing to remove
  return;           // cowardly (and silently) return

 if( v_pairs.empty() )  // deleting from nothing
  throw( std::logic_error( "deleting from an empty set" ) );

 auto it = nms.begin();
 if( ! ordered )
  std::sort( it , nms.end() );
 
 if( *it >= v_pairs.size() )  // if the first name is wrong
  throw( std::invalid_argument( "wrong index in LinearFunction" ) );

 auto vi = *it;    // first element to be eliminated
 auto curr = v_pairs.begin() + vi;   // position where to move stuff

 ++it;              // skip the first elements
 ++vi;              // as they have been processed already

 for( ; it < nms.end() ; ++vi )
  if( *it == vi )                 // one element to be eliminated
   ++it;                          // skip it
  else
   *(curr++) = v_pairs[ vi ];     // move in the current position

 auto itv = v_pairs.begin() + vi;
 for( ; itv < v_pairs.end() ; )   // copy the last part
  *(curr++) = *(itv++);           // after the last of v_var

 v_pairs.erase( curr , itv );     // erase the last part

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 Vec_p_Var vars( nms.size() );
 auto its = vars.begin();
 for( auto nm : nms )
  *(its++) = v_pairs[ nm ].first;

 // now issue the Modification
 // a linear function is additive ==> strongly quasi-additive
 // note that the Variable have been ordered (if they were not so already)
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                       FunctionModVars::RemoveVar ,
				       std::move( vars ) , true , 0 , true ,
				       Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

 }  // end( PolyhedralFunction::remove_variables( indices ) )

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

void PolyhedralFunction::issue_add_variables_modification( v_coeff_pair & pairs ,
						       c_ModParam issueMod )
{
 Vec_p_Var vars( pairs.size() );
 for( Index i = 0 ; i < pairs.size() ; ++i )
  vars[ i ] = pairs[ i ].first;

 // a linear function is additive ==> strongly quasi-additive
 // note that pairs is always ordered
 f_Observer->add_Modification( std::make_shared<C05FunctionModVars>( this ,
                                         FunctionModVars::AddVar ,
					 std::move( vars ) , true , 0 , true ,
					 Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------- End File PolyhedralFunction.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
