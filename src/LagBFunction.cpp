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

static const char VarIsDir = 0;  // Variable contain a direction
static const char VarIsSol = 1;  // Variable contain a solution
static const char NoVar = 2;  // No Variable is in the Solver

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

LagBFunction::LagBFunction( void ) :  C05Function()
{

 slag_p.clear();
 v_Block[0] = nullptr;

 // define global pool of intGPMaxSz size - - - - - - - - - - - - - - - - - -
 g_pool.resize( intGPMaxSz );

 // so far, the current solution is unknown - - - - - - - - - - - - - - - - -
 VarType = NoVar;
 LastSolution = Inf<LinearizationName>();

 FuncIsIntlzd = false;

 } // end LagBFunction::LagBFunction( )  - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

LagBFunction::LagBFunction( v_dual_pair && static_lagrangian_pairs ,
		 const bool static_is_ordered )
 :  C05Function() , slag_p( std::move( static_lagrangian_pairs ) )
{

 v_Block[0] = nullptr;

 // define global pool of intGPMaxSz size - - - - - - - - - - - - - - - - - -
 g_pool.resize( intGPMaxSz );

 // so far, the current solution is unknown - - - - - - - - - - - - - - - - -
 VarType = NoVar;
 LastSolution = Inf<LinearizationName>();

 FuncIsIntlzd = false;

 addStaticLagrangianPairs( std::move( static_lagrangian_pairs ) ,
		 static_is_ordered );


 } // end LagBFunction::LagBFunction( )  - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

LagBFunction::~LagBFunction() {

 dlag_p.clear();
 g_pool.clear(); //
 } // end LagBFunction::~LagBFunction( ) - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::setInnerBlock( Block* innerblock ) {

 // set the inner block  - - - - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 v_Block[0] = innerblock;

 // get the objective function pointer of the inner block  - - - - - - - - - -

 auto obj = boost::any_cast<FRealObjective *>( & v_Block[0]->get_objective() );
   if( obj == nullptr )
    throw( std::logic_error( "the objective is not a real function" ) );
  auto LFInnBlck = dynamic_cast<LinearFunction *>( (*obj)->get_function() );
  if( LFInnBlck == nullptr )
   throw( std::logic_error( "the objective is not a linear function" ) );

 if( !FuncIsIntlzd && !slag_p.empty() ) {
  FuncIsIntlzd = true;
  InitStaticPartOfFunction();
  }

 } // end LagBFunction::generate_objective( )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::addStaticLagrangianPairs( v_dual_pair &&
   static_lagrangian_pairs ,  const bool static_is_ordered  ) {

 slag_p = std::move( static_lagrangian_pairs ); //?????

 if( ! static_is_ordered ) {
  std::sort( slag_p.begin() , slag_p.end() ,
		      []( const auto & p1, const auto & p2 ) {
		       return( p1.first < p2.first );
		       }
	        );
  }

 // check for the relaxed constraints  - - - - - - - - - - - - - - - - - - - -

 for( const auto irelax : slag_p ) { // for each relaxed constraints

  // get the relaxed constraint  - - - - - - - - - - - - - - - - - - - - - - -

  auto LinFunc = dynamic_cast<const LinearFunction *>( irelax.second );
  if( LinFunc == nullptr )
   throw( std::logic_error( "the objective is not a linear function" ) );

  }

 InitStaticDataStructure( );

 if( !FuncIsIntlzd && v_Block[0] ) {
  FuncIsIntlzd = true;
  InitStaticPartOfFunction();
  }

 } // end LagBFunction::addStaticLagrangianPairs( )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE LagBFunction ------------*/
/*--------------------------------------------------------------------------*/

bool LagBFunction::has_linearization( const bool diagonal )
{
 // get the Solver from inner Block  - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 // true if a linearization of the related type exists   - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 bool SlvHasNewLin;

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

 if( !SlvHasNewLin )
  VarType = NoVar; // set the current solution as unknown

 // last solution is the current solution  - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LastSolution = Inf<Index>();

 return( SlvHasNewLin );

 }  // end LagBFunction::has_linearization( )  - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

bool LagBFunction::compute_new_linearization( const bool diagonal )
{
 // get the Solver from inner Block - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

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

 if( !SlvHasNewLin )
  VarType = NoVar; // set the current solution as unknown

 // last solution is the current solution  - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LastSolution = Inf<Index>();

 return( SlvHasNewLin );

 } // end LagBFunction::compute_new_linearization( ) - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::store_linearization( const LinearizationName name )
{
 if( LastSolution < Inf<Index>() ) // the solution to be solved is not a new one
  throw( std::logic_error( "the linearization is not available anymore" ) );

 if( VarType == NoVar )            // if the current solution is unknown ...
  throw( std::logic_error( "there is no solution to save" ) );

 // get the current solution   - - - - - - - - - - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( g_pool[ name ].first == nullptr )
  g_pool[ name ].first = v_Block[0]->get_Solution();

 g_pool[ name ].first->read( v_Block[0] );

 if( VarType == VarIsSol )
  g_pool[ name ].second = VarIsSol;
 else
  g_pool[ name ].second = VarIsDir;

 } // end LagBFunction::store_linearization( ) - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

int LagBFunction::compute( bool changedvars )
{
 LastSolution = Inf<Index>();	// set LastSolution as the current solution, i.e.
                                // that solution which has computed in compute();

 VarType = NoVar;               // the solution is unknown so far

 // get the Solver from inner Block  - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 UpdateFunction();

 return( slv->compute( false ) );

 } // end LagBFunction::compute( ) - - - - - - - - - - - - - - - - - - - - - -

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( FunctionValue * g ,
				 const LinearizationName name ,
                                 const std::vector<Index> * const indices ,
                                 const Index start , const Index end )

{

 c_Index end_p = std::min( Index(slag_p.size()) , end );
 if( end_p <= start )
  return;

 // get the Solver from inner Block - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 if( name == Inf<LinearizationName>() ) { // asking for the last computed - -
	                                      // linearization  - - - - - - - - -

  if( LastSolution < Inf<Index>() ) // the saved solution is not in the local pool
   throw( std::logic_error( "the linearization is not available anymore" ) );

  // get solution/direction from the solver - - - - - - - - - - - - - - - - -

  if( VarType == NoVar )            // if the current solution is unknown ...
   throw( std::logic_error( "there is no solution to save" ) );
  else
   if( VarType == VarIsSol )
    slv->get_var_solution();
   else
    slv->get_var_direction();

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the inner Block from which the related linearization
  // <name> can be recovered

  if( name != LastSolution )
   g_pool[ name ].first->write( v_Block[0] );

  LastSolution = name ;    // update last solution

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier, the value of the relaxed constraint
 // is the component of g  - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( indices != nullptr ) {
  for( const auto & i : *indices )
   if( ( i >= start ) && ( i < end_p ) )
    *(g++) = slag_p[ i ].second->get_value();
  }
 else
  for( Index i = start ; i < end_p ; ++i )
	*(g++) = slag_p[ i ].second->get_value();

 } // end( LagBFunction::get_linearization_coefficients( DenseVector ) )

/*--------------------------------------------------------------------------*/

void LagBFunction::get_linearization_coefficients( SparseVector & g ,
				         const LinearizationName name ,
                                         c_Vec_Index * const indices ,
                                         c_Index start , c_Index end )
{

 c_Index end_p = std::min( Index(slag_p.size()) , end );
 if( end_p <= start )
  return;

 // get the Solver from inner Block - - - - - - - - - - - - - - - - - - - - -
 Solver* slv = v_Block[0]->get_registered_solvers().back();

 if( name == Inf<LinearizationName>() ) { // asking for the last computed - -
	                                      // linearization  - - - - - - - - -

  if( LastSolution < Inf<Index>() ) // the saved solution is not in the local pool
   throw( std::logic_error( "the linearization is not available anymore" ) );

  // get solution/direction from the solver - - - - - - - - - - - - - - - - -

  if( VarType == NoVar )            // if the current solution is unknown ...
   throw( std::logic_error( "there is no solution to save" ) );
  else
   if( VarType == VarIsSol )
    slv->get_var_solution();
   else
    slv->get_var_direction();

  }
 else {  // asking for a linearization of the global pool  - - - - - - - - - -

  // assign Solution to the inner Block from which the related linearization
  // <name> can be recovered

  if( name != LastSolution )
   g_pool[ name ].first->write( v_Block[0] );

  LastSolution = name ;    // update last solution

  } // end else  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // for each Lagrangian multiplier, the value of the relaxed constraint
 // is the component of g  - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( g.nonZeros() == 0 ) {  // the given vector contains no non-zero element

  if( g.size() < Index(slag_p.size()) )
   g.resize( Index(slag_p.size()) );

  g.reserve( end_p - start );

  if( indices != nullptr ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < end_p ) )
     g.insert( i ) = slag_p[ i ].second->get_value();
   }
  else
   for( Index i = start ; i < end_p ; ++i )
    g.insert( i ) = slag_p[ i ].second->get_value();

  }
 else {  // the given vector contains some non-zero elements
  if( g.size() != Index(slag_p.size()) )
   throw( std::invalid_argument(
      "LagBFunction::get_linearization_coefficients: "
      "the size of the sparse vector must be equal to the number "
      "of Lagrangian multipliers" ) );

  if( indices != nullptr ) {
   for( const auto & i : *indices )
    if( ( i >= start ) && ( i < end_p ) )
     g.coeffRef( i ) = slag_p[ i ].second->get_value();
     }
    else
     for( Index i = start ; i < end_p ; ++i )
      g.coeffRef( i ) = slag_p[ i ].second->get_value();
  }

 }  // end( LagBFunction::get_linearization_coefficients( SparseVector ) )

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE LagBFunction ---------*/
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE LagBFunction --------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void LagBFunction::InitStaticDataStructure( void )
{

 // construct the vector of the Lagrangian costs - - - - - - - - - - - - - - -
 //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 for( const auto & relaxed : slag_p ) { // read the dual vector slag_p, getting
              // the relaxed constraint <a,x> and the dual variable y thereof

  auto LinFunc = static_cast<const LinearFunction *>( relaxed.second );
  LinearFunction::v_c_coeff_pair & LinFunCoeffs = LinFunc->get_v_var();

  for( const auto & monomial : LinFunCoeffs ) { // for each Variable x_j
                        // of the relaxed constraint, add to LagMatrix
	                    // the pair < y_i , a_{ij} >

   // construct the pair of the form < y_i , a_{ij} > to be added to
   // the related column of x_j  - - - - - - - - - - - - - - - - - - - - - - - -

   const auto pairC = std::make_pair( relaxed.first , monomial.second );

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

   if( itA.second ) { // a new variable x_j was inserted - - - - - - - - - - - -

	// so far, c_i is unknown
	(itA.first->second).first = Inf<LinearFunction::Coefficient>();
	curr_column.insert( curr_column.begin() , pairC );

    }
   else {  // variable x_j already existed - - - - - - - - - - - - - - - - - - -

	auto itB = std::upper_bound( curr_column.begin() ,
			              curr_column.end() , pairC ,
		 	     		  []( const LinearFunction::coeff_pair &a ,
		 	     			  const LinearFunction::coeff_pair &b )
		 	     		  { return( a.first < b.first ); } );

	curr_column.insert( itB , pairC );

    }

   } // end linear function examination  - - - - - - - - - - - - - - - - - - -
  } // end for each relaxed constraints  - - - - - - - - - - - - - - - - - - -

 } // end LagBFunction::InitLagrangianDataStructure()

/*--------------------------------------------------------------------------*/

void LagBFunction::InitStaticPartOfFunction( void )
{

 // get the objective function pointer of the inner block   - - - - - - - - -

 auto obj = boost::any_cast<FRealObjective *>( & v_Block[0]->get_objective() );
 auto LFInnBlck = static_cast<LinearFunction *>( (*obj)->get_function() );

 LinearFunction::v_c_coeff_pair & LFInnBCoeff = LFInnBlck->get_v_var();
 LinearFunction::v_coeff_pair PairsToAdd;  // this array is created to add
      // the pairs which are not active in LFInnBlck

 auto itv = LFInnBCoeff.begin();
 for( auto it = LagMatrix.begin() ;
		 it != LagMatrix.end() ;  ++it ) {

  if( it->first == itv->first ) {
   (it->second).first = LFInnBlck->get_coefficient( it->first );
   itv++;
   }
  else {
   (it->second).first = LinearFunction::Coefficient(0);
   const auto pair = std::make_pair( it->first , LinearFunction::Coefficient(0) );
   PairsToAdd.push_back( pair );
   }

  LFInnBlck->add_variables( std::move(PairsToAdd) , true );

  } // end for - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 } // end LagBFunction::InitStaticPartOfFunction()

/*--------------------------------------------------------------------------*/

void LagBFunction::UpdateFunction( void )
{
 // get the objective function pointer of the inner block   - - - - - - - - -

 auto obj = boost::any_cast<FRealObjective *>( & v_Block[0]->get_objective() );
 auto LFInnBlck = static_cast<LinearFunction *>( (*obj)->get_function() );

 LinearFunction::v_c_coeff_pair & LFInnBCoeff = LFInnBlck->get_v_var();
 LinearFunction::Coefficient FVCoeff;

 auto itv = LFInnBCoeff.begin();
 for( auto it = LagMatrix.begin() ;
		 it != LagMatrix.end() ;  ++it ) {

  FVCoeff = (it->second).first;
  for( const auto & el : (it->second).second )
	  FVCoeff -= el.first->get_value() * el.second;

  FVCoeff += (it->second).first;
  LFInnBlck->modify_coefficient( it->first , FVCoeff );

  } // end for - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 } // end LagBFunction::UpdateFunction()

/*--------------------------------------------------------------------------*/
/*---------------------- End File LagBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
