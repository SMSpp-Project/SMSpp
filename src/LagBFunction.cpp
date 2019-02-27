/*--------------------------------------------------------------------------*/
/*------------------------ File LagBFunction.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the LagBFunction class.
 *
 * \version 0.02
 *
 * \date 18 - 02 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Enrico Gorgone \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Enrico Gorgone
 *
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Observer.h"
#include "SMSTypedefs.h"
#include "LagBFunction.h"
#include <math.h>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

static const char VarIsDir = 0;   // a direction is stored
static const char VarIsSol = 1;   // a solution is stored
static const char UnknownVar = 2; // variables contain unknown values

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

LagBFunction::LagBFunction( v_dual_pair && v_lag_pair , const bool static_is_ordered ,
		Block* innerblock  )
 :  C05Function() , lag_p( std::move( v_lag_pair ) )
{

 // LastSolution is the current solution but, so far, it is unknown
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LastSolution = Inf<LinearizationName>();
 VarType = UnknownVar;

 // set the sub-Block pointer - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( innerblock )
  set_inner_block( innerblock );

 // set a bunch of *static* dual pairs <y, g(x)>   - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( v_lag_pair.size() )
  set_dual_pairs( std::move( v_lag_pair ) , static_is_ordered );

 } // end ( LagBFunction::LagBFunction( ) )  - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::clear( ) {

 for( const auto & lagdual : lag_p )
  delete[] lagdual.second;
 lag_p.clear();

 } //end ( LagBFunction::clear( ) )  - - - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::set_inner_block( Block* innerblock ) {

 // set the sub-block  - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( v_Block.size() )
  v_Block.clear();

 v_Block[0] = innerblock;

 // set the objective  - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 set_objective_and_solver( );

 // if the static relaxed constraints <y, g(x)> have been written,
 // do the following:
 // (i) copy the coefficients c of the inner block (B) in order to allow
 // the changes of the Lagrangian cost vector c^y = c + yA
 // (ii) add to that linear function of (B) the variables which
 // are involved in the definition of g(x) with coefficient zero

 if( lag_p.size() )
  store_function();

 } // end ( LagBFunction::set_inner_block( Block* )  ) - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_dual_pairs( v_dual_pair && v_lag_pair ,
		 const bool static_is_ordered  ) {

 clear();

 // if not already ordered by ColVariable "name = pointer" sort the vector of
 // "static" dual Lagrangian pairs, and construct the structure LagMatrix for
 //  updating the Lagrangian cost vector
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 set_structure( v_lag_pair , static_is_ordered );

 // if the sub-block (B) has been set, do the following:
 // (i) copy the coefficients c_i of the inner block (B) in order to allow
 // the changes of the Lagrangian cost vector c^y = c + yA
 // (ii) add to the linear function of (B) the variables which
 // are involved in the definition of g(x) with coefficient zero

 if( v_Block.size() )
  store_function();

 // assign the dual Lagrangian pairs to lag_p  - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 lag_p = std::move( v_lag_pair );

 } // end ( LagBFunction::set_dual_pairs( ) )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_par( const idx_type par , const int value )
{
 switch( par ) {
  case( intLPMaxSz ):
   LPMaxSz = value;
   break;
  case( intGPMaxSz ):
   GPMaxSz = value;
   g_pool.resize( GPMaxSz );
   break;
  default: Function::set_par( par , value );
  }
 }  // end ( LagBFunction::set_par( int ) )  - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_par( const idx_type par , const double value )
{
 switch( par ) {
  case( dblRAccLin ):
   RAccLin = value;
   break;
  case( dblAAccLin ):
   AAccLin = value;
   break;
  default: Function::set_par( par , value );
  }
 } // end ( LagBFunction::set_par( double ) )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::add_dual_pairs( v_dual_pair && v_lag_pair ,
		 const bool static_is_ordered ) {

 // update the structure LagMatrix for changing the Lagrangian cost vector
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 std::move( v_lag_pair );
 set_structure( v_lag_pair , static_is_ordered );

 // copy the coefficients c_i of the inner block (B) in order to allow
 // the changes of the Lagrangian cost vector c^y = c + yA and add to
 // the linear function of (B) the variables which are involved in the
 // definition of g(x) with coefficient zero

 store_function();

 // merge the list of dual Lagrangian pairs, both containers shall already be
 // ordered  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 std::merge( lag_p.begin() , lag_p.end() , v_lag_pair.begin() , v_lag_pair.end() ,
		 lag_p.begin() ,
		 []( const auto & p1, const auto & p2 ) { return( p1.first < p2.first ); }  );

 } // end ( LagBFunction::add_dual_pairs( ) )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ------------*/
/*--------------------------------------------------------------------------*/

bool LagBFunction::has_linearization( const bool diagonal )
{
 // check for a new linearization and set its type   - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 bool SlvHasNewLin;  // true if a linearization of the related type exists

 if( diagonal ) {
  SlvHasNewLin = slv->has_var_solution();
  if( SlvHasNewLin )
   VarType = VarIsSol;
  }
 else {
  SlvHasNewLin = slv->has_var_direction();
  if( SlvHasNewLin )
   VarType = VarIsDir;
  }

 if( !SlvHasNewLin )     // if no linearization has been found, the solution
  VarType = UnknownVar;  // is unavailable

 // LastSolution is the current solution   - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LastSolution = Inf<Index>();
 return( SlvHasNewLin );

 }  // end LagBFunction::has_linearization( )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

bool LagBFunction::compute_new_linearization( const bool diagonal )
{
 // true if a new linearization of the related type exists in the local pool
 // which is kept in the Solver   - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 bool SlvHasNewLin;

 if( diagonal ) {
  SlvHasNewLin = slv->new_var_solution();
  if( SlvHasNewLin )
   VarType = VarIsSol;
  }
 else {
  SlvHasNewLin = slv->new_var_direction();
  if( SlvHasNewLin )
   VarType = VarIsDir;
  }

 if( !SlvHasNewLin )    // if no linearization has been found, the solution
  VarType = UnknownVar; // is unavailable

 // the previous LastSolution is lost unless it has been stored, LastSolution
 // is the current solution  - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LastSolution = Inf<Index>();
 return( SlvHasNewLin );

 } // end LagBFunction::compute_new_linearization( ) - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::store_linearization( const LinearizationName name )
{
 if( LastSolution < Inf<Index>() ) // the solution has been already stored
  throw( std::logic_error( "the linearization is not available anymore" ) );

 if( VarType == UnknownVar )       // the solution to be stored is unavailable
  throw( std::logic_error( "there is no solution to save" ) );

 // get the current solution   - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( g_pool[ name ].first == nullptr )
  g_pool[ name ].first = v_Block[0]->get_Solution();
 g_pool[ name ].first->read( v_Block[0] );

 // set the solution type  - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( VarType == VarIsSol )
  g_pool[ name ].second = VarIsSol;
 else
  g_pool[ name ].second = VarIsDir;

 } // end LagBFunction::store_linearization( ) - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

int LagBFunction::compute( bool changedvars )
{
 LastSolution = Inf<Index>();	// set LastSolution as the current solution, i.e.
                                // that solution which is going to be computed

 VarType = UnknownVar;          //so far, the current solution is unavailable

 // update the Lagrangian cost vector  - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 update_function();

 // return the status of the optimization process  - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( slv->compute( false ) );

 } // end ( LagBFunction::compute( ) ) - - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

Function::FunctionValue LagBFunction::get_value( void ) const
{
 if( obj->get_sense() == Objective::eMax )
  return( slv->get_ub() );
 else
  return( slv->get_lb() );

 } // end ( LagBFunction::get_value( ) ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
	  const LinearizationName name , const std::vector<Index> * const indices ,
      const Index start , const Index end )

{
 c_Index end_p = std::min( Index( lag_p.size()) , end );
 if( end_p <= start )
  return;

 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf<LinearizationName>() ) { // asking for the last computed
	                                      // linearization  - - - - - - - - -

  if( LastSolution < Inf<Index>() ) // LastSolution is not in the local pool
   throw( std::logic_error( "the linearization is not available anymore" ) );

  // get solution/direction from the solver - - - - - - - - - - - - - - - - -

  if( VarType == UnknownVar )            // if the current solution is unavailable
   throw( std::logic_error( "there is no solution to save" ) );
  else
   if( VarType == VarIsSol )
    slv->get_var_solution();
   else
    slv->get_var_direction();

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the sub-Block in such a way the related linearization
  // < name > will be recovered

  if( name != LastSolution )
   g_pool[ name ].first->write( v_Block[0] );

  LastSolution = name ;    // update LastSolution

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier, the objective value of the relaxed constraint
 // is the corresponding entry of g  - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( indices != nullptr ) {
  for( const auto & i : *indices )
   if( ( i >= start ) && ( i < end_p ) )
    *(g++) = lag_p[ i ].second->get_value();
  }
 else
  for( Index i = start ; i < end_p ; ++i )
	*(g++) = lag_p[ i ].second->get_value();

 } // end( LagBFunction::get_linearization_coefficients( DenseVector ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( SparseVector & g ,
			const LinearizationName name , c_Vec_Index * const indices ,
            c_Index start , c_Index end )
{
 c_Index end_p = std::min( Index( lag_p.size()) , end );
 if( end_p <= start )
  return;

 // the solution shall be written in the Variable of the Block - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( name == Inf<LinearizationName>() ) { // asking for the last computed
	                                      // linearization  - - - - - - - - -

  if( LastSolution < Inf<Index>() ) // the saved solution is not in the local pool
   throw( std::logic_error( "the linearization is not available anymore" ) );

  // get solution/direction from the solver - - - - - - - - - - - - - - - - -

  if( VarType == UnknownVar )            // if the current solution is unavailable
   throw( std::logic_error( "there is no solution to save" ) );
  else
   if( VarType == VarIsSol )
    slv->get_var_solution();
   else
    slv->get_var_direction();

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the sub-Block in such a way the related linearization
  // < name > will be recovered

  if( name != LastSolution )
   g_pool[ name ].first->write( v_Block[0] );

  LastSolution = name ;    // update LastSolution

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier, the objective value of the relaxed constraint
 // is the corresponding entry of g  - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < Index( lag_p.size()) )
   g.resize( Index( lag_p.size()) );

  g.reserve( end_p - start );

  if( indices != nullptr ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < end_p ) )
     g.insert( i ) = lag_p[ i ].second->get_value();
   }
  else
   for( Index i = start ; i < end_p ; ++i )
    g.insert( i ) = lag_p[ i ].second->get_value();

  }
 else {  // the given vector contains some non-zero elements
  if( g.size() != Index( lag_p.size()) )
   throw( std::invalid_argument(
      "LagBFunction::get_linearization_coefficients: "
      "the size of the sparse vector must be equal to the number "
      "of Lagrangian multipliers" ) );

  if( indices != nullptr ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < end_p ) )
     g.coeffRef( i ) = lag_p[ i ].second->get_value();  //*?????
     }
    else
     for( Index i = start ; i < end_p ; ++i )
      g.coeffRef( i ) = lag_p[ i ].second->get_value();
  }

 }  // end( LagBFunction::get_linearization_coefficients( SparseVector ) )

/*--------------------------------------------------------------------------*/

double LagBFunction::get_linearization_constant( const LinearizationName name ) const
{
 double alpha = 0;
 if( name == Inf<LinearizationName>() ) {

  if( LastSolution < Inf<Index>() ) // the saved solution is not in the local pool
   throw( std::logic_error( "the linearization is not available anymore" ) );

  if( VarType == UnknownVar )       // if the current solution is unavailable
   throw( std::logic_error( "there is no solution to save" ) );

  }
 else
  if( name != LastSolution )
   throw( std::logic_error( "he linearization is not available anymore" ) );

 for( auto it = LagMatrix.begin() ; it != LagMatrix.end() ; ++it )
  alpha += (it->first)->get_value() * (it->second).first;

 return( alpha );

 } // end( LagBFunction::get_linearization_constant() )  - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*------------------- METHODS FOR HANDLING THE PARAMETERS ------------------*/
/*--------------------------------------------------------------------------*/

int LagBFunction::get_dflt_int_par( const idx_type par ) const
{
 switch( par ) {
  case( intLPMaxSz ):
   return( RAccLin );
	break;
  case( intGPMaxSz ):
   return( RAccLin );
   break;
  default:
   return( C05Function::get_dflt_dbl_par( par ) ) ;
  }

} // end( LagBFunction::get_dflt_int_par( idx_type ) )  - - - - - - - - - - -

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

double LagBFunction::get_dflt_dbl_par( const idx_type par ) const
{
 switch( par ) {
  case( dblRAccLin ):
   return( RAccLin );
   break;
  case( intGPMaxSz ):
   return( RAccLin );
	break;
  default:
	return( C05Function::get_dflt_dbl_par( par ) ) ;
  }
} // end( LagBFunction::get_dflt_dbl_par( idx_type ) ) - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/

ThinVarDepInterface::Index LagBFunction::is_active( const Variable * const var )
const
{
 auto idx = std::lower_bound( lag_p.begin() , lag_p.end() ,
                                std::make_pair( var , 0 ) ,
                                []( const auto & p1, const auto & p2 )
                                  { return p1.first < p2.first; } );
 if( idx < lag_p.end() )
  return( std::distance( lag_p.begin() , idx ) );
 else
  return( Inf<Index>() );

 } // end( LagBFunction::is_active( Variable* ) )  - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::map_active( c_Vec_p_Var & vars , Vec_Index & map ,
		const bool ordered ) const
{
 if( ! vars.size() )
  return;

 // the basic implementation of the method is used whenever vars is not ordered
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( ! ordered ) {
  ThinVarDepInterface::map_active( vars , map );
  return;
  }

 if( map.size() < vars.size() )
  map.resize( vars.size() );

 // construct map vector - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 auto itvb = vars.begin();
 auto itvv = std::lower_bound( lag_p.begin() , lag_p.end() ,
     std::make_pair( *itvb , 0 ) , []( const auto & p1 , const auto & p2 ) {
	 return( p1.first < p2.first ); } );
 auto itve = std::upper_bound( itvv , lag_p.end() ,
	 std::make_pair( *(--vars.end()) , 0 ) , []( const auto & p1 , const auto & p2 ) {
	 return( p1.first < p2.first ); } );

 auto itm = map.begin();
 while( itvb < vars.end() ) {
  if( itvv >= itve )
   throw( std::invalid_argument( "some Variable is not active" ) );

  *(itm++) = std::distance( lag_p.begin() , itvv );
  itvv = std::lower_bound( itvv , itve , std::make_pair( *(++itvb) , 0 ) ,
     []( const auto & p1, const auto & p2 ) { return( p1.first < p2.first ); } );

  } // end while - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 }  // end( LagBFunction::map_active( Variable* ) )  - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LagBFunction --------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED METHODS ----------------------------*/
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::set_structure( v_dual_pair & v_lag_pair ,
		const bool static_is_ordered )
{
 // the objective functions of the relaxed constraints must be linear
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( const auto lagdual : v_lag_pair ) { // for each relaxed constraints
  auto LinFunc = dynamic_cast<const LinearFunction *>( lagdual.second );
  if( LinFunc == nullptr )
   throw( std::logic_error( "the objective is not a linear function" ) );
  }

 // if not already ordered by ColVariable "name = pointer", sort the vector
 // of dual Lagrangian pairs - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( ! static_is_ordered )
  std::sort( v_lag_pair.begin() , v_lag_pair.end() ,
	  []( const auto & p1, const auto & p2 ) { return( p1.first < p2.first ); } );

 // construct the vector of the Lagrangian costs - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( const auto & lagdual : v_lag_pair ) { // read the dual vector lag_p,
	          // getting the relaxed constraint <a,x> and the dual variable y thereof

  auto LinFunc = static_cast<const LinearFunction *>( lagdual.second );
  LinearFunction::v_c_coeff_pair & LinFunCoeffs = LinFunc->get_v_var();

  for( const auto & monomial : LinFunCoeffs ) { // for each Variable x_j
                        // of the relaxed constraint, add to LagMatrix
	                    // the pair < y_i , a_{ij} >

   // construct the pair of the form < y_i , a_{ij} > to be added to
   // the related column of x_j  - - - - - - - - - - - - - - - - - - - - - - - -

   const auto pair = std::make_pair( lagdual.first , monomial.second );

   auto itA = LagMatrix.insert( std::pair<ColVariable * , col_pair>
                         ( monomial.first , col_pair() ) );

   // itA.first is an iterator pointing to either the newly
   // inserted jth column or to the jth column already in the map,
   // in particular (itA.first->first) is the variable xj and
   // (itA.first->second) the pair <col_pair> thereof. This means
   // that (itA.first->second).first is c_j and (itA.first->second).second
   // is the array < y_i , a_{ij}j > for all i in the active index set
   // of the Lagrangian multipliers

   auto &curr_column = (itA.first->second).second;

   // note that itA is a pair <iterator,bool>, the pair::second element
   // is set to false if an equivalent key already existed, true otherwise

   if( itA.second ) { // a new variable x_j was inserted  - - - - - - - - - -

    // so far. c_i is unknown
	(itA.first->second).first = Inf<LinearFunction::Coefficient>();
	curr_column.insert( curr_column.begin() , pair );

    }
   else {  // variable x_j already existed - - - - - - - - - - - - - - - - - - -

	auto itB = std::upper_bound( curr_column.begin() ,
			              curr_column.end() , pair ,
		 	     		  []( const LinearFunction::coeff_pair &a ,
		 	     			  const LinearFunction::coeff_pair &b )
		 	     		  { return( a.first < b.first ); } );

	curr_column.insert( itB , pair );

    }

   }
  } // end for each relaxed constraints  - - - - - - - - - - - - - - - - - - -

 } // end ( LagBFunction::set_structure() )  - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::set_objective_and_solver( )
{
 assert( ! ( v_Block[0]->get_objective() ).empty() );  // ... which must exist

 // the objective function of the inner block must be linear - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 obj = boost::any_cast<FRealObjective *>( v_Block[0]->get_objective() );
 if( obj == nullptr )
  throw( std::logic_error( "the objective is not a real function" ) );

 auto LFInnBlck = dynamic_cast<LinearFunction *>( (obj)->get_function() );
 if( LFInnBlck == nullptr )
  throw( std::logic_error( "the objective is not a linear function" ) );

 // get the Solver of the sub-Block  - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 Solver* slv = v_Block[0]->get_registered_solvers().back();
 assert( ! slv );

 } // end ( LagBFunction::set_objective_and_solver( ) )  - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::store_function( )
{

 // get the objective function pointer of the inner block   - - - - - - - - -

 auto LFInnBlck = static_cast<LinearFunction *>( obj->get_function() );

 LinearFunction::v_c_coeff_pair & LFInnBCoeff = LFInnBlck->get_v_var();
 LinearFunction::v_coeff_pair PairsToAdd;  // this array is created to add the
                                           // pairs which are not active in LFInnBlck

 auto itv = LFInnBCoeff.begin();
 for( auto it = LagMatrix.begin() ;
		 it != LagMatrix.end() ;  ++it ) {

  if( it->first == itv->first ) {
   // set c_i if it is unknown
   if( (it->second).first < Inf<LinearFunction::Coefficient>() )
	(it->second).first = LFInnBlck->get_coefficient( it->first );
   itv++;
   }
  else {
   // set c_i if it is unknown
   if( (it->second).first < Inf<LinearFunction::Coefficient>() ) // this check should be
	(it->second).first = LinearFunction::Coefficient(0);         // necessary if remotion is
                                                                 // performed
   const auto pair = std::make_pair( it->first , LinearFunction::Coefficient(0) );
   PairsToAdd.push_back( pair );
   }

  LFInnBlck->add_variables( std::move(PairsToAdd) , true );

  } // end for - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 } // end ( LagBFunction::store_function( ) )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::update_function( )
{
 // get the objective function pointer of the inner block   - - - - - - - - -

 auto LFInnBlck = static_cast<LinearFunction *>( obj->get_function() );

 LinearFunction::Coefficient FVCoeff;
 LinearFunction::v_coeff_pair PairsToAdd; // this array is created to change the
                                          // pairs in LFInnBlck

 for( auto it = LagMatrix.begin() ; it != LagMatrix.end() ; ++it ) {

  FVCoeff = (it->second).first;
  for( const auto & el : (it->second).second )
   FVCoeff -= el.first->get_value() * el.second;

  PairsToAdd.push_back( std::make_pair( it->first , FVCoeff ) );

  } // end for - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LFInnBlck->modify_coefficients( move(PairsToAdd) , true );

 } // end ( LagBFunction::update_function() )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::guts_of_destructor( )
{
 clear();

 LagMatrix.clear();

 for( int i = 0 ; i < g_pool.size() ; ++i )
  delete[] g_pool[ i ].first;
 g_pool.clear();

 } // end ( LagBFunction::guts_of_destructor() ) - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*---------------------- End File LagBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
