/*--------------------------------------------------------------------------*/
/*---------------------- File BendersBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BendersBFunction class.
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

#include "AbstractPath.h"
#include "BendersBFunction.h"
#include "BlockSolverConfig.h"
#include "FRowConstraint.h"
#include "Objective.h"
#include "Observer.h"
#include "OneVarConstraint.h"
#include "RBlockConfig.h"
#include "RowConstraint.h"
#include "SMSTypedefs.h"
#include "Solution.h"

#include <cmath>
#include <functional>
#include <queue>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register BendersBFunction to the Block factory

SMSpp_insert_in_factory_cpp_1( BendersBFunction );
SMSpp_insert_in_factory_cpp_1( BendersBFunctionState );

/*--------------------------------------------------------------------------*/
/*------------- CONSTRUCTING AND DESTRUCTING BendersBFunction --------------*/
/*--------------------------------------------------------------------------*/

BendersBFunction::BendersBFunction( Block * inner_block , VarVector && x ,
				    MultiVector && A , RealVector && b ,
				    ConstraintVector && constraints ,
				    ConstraintSideVector && sides ,
				    Observer * const observer )
 : C05Function( observer ) , f_constraints_are_updated( false ) ,
   f_solver_status( kUnEval ) , f_diagonal_linearization_required( false ) ,
   f_id( this )
{
 set_inner_block( inner_block );
 set_variables( std::move( x ) );
 set_mapping( std::move( A ) , std::move( b ) , std::move( constraints ) ,
              std::move( sides ) , eNoMod );

 // default parameter values
 LinComp = get_dflt_int_par( intLinComp );
 AAccMlt = get_dflt_dbl_par( dblAAccMlt );
 set_par( intGPMaxSz , C05Function::get_dflt_int_par( intGPMaxSz ) );
 }

/*--------------------------------------------------------------------------*/

BendersBFunction::~BendersBFunction()
{
 if( ! v_Block.empty() ) {
  assert( v_Block.size() == 1 );
  delete v_Block.front();
  }

 delete f_get_dual_solution_partial_config;
 delete f_get_dual_direction_partial_config;
 delete f_get_dual_solution_config;
 delete f_get_dual_direction_config;
 }

/*--------------------------------------------------------------------------*/
/*      IMPORTANT NOTE ON THE DESERIALIZATION OF THE BendersBFunction
 *
 * The BendersBFunction maintains a vector of pointers to the Constraint that
 * are affected by its active Variable. Each of these Constraint is identified
 * by an AbstractPath having the inner Block of this BendersBFunction as the
 * reference Block (see AbstractPath for more details about the reference
 * Block). When a BendersBFunction is deserialized, the paths to these
 * Constraint are provided in the netCDF file. The issue is that after the
 * inner Block is constructed, it is very likely that its abstract
 * representation has not been generated yet, so that it is not possible to
 * retrieve the pointers to the Constraint (because these Constraint still do
 * not exist). After the inner Block is constructed, the BendersBFunction
 * could ask the inner Block to generate its abstract representation, so that
 * the BendersBFunction would then be able to retrieve the pointers to the
 * Constraint. The issue is that the abstract representation can be generated
 * only once. If one were to generate the abstract representation of the inner
 * Block at a later stage, after the BendersBFunction is deserialized (and,
 * therefore, after the abstract representation of the inner Block has been
 * generated), it would take no effect. This is particularly serious because
 * one may want to use a Configuration to generate the abstract representation
 * of the inner Block. This means that the abstract representation of the
 * inner Block may not be the desired one if it is generated during the
 * deserialization of the BendersBFunction (and, thus, without the appropriate
 * Configuration). In order to deal with this situation, the abstract
 * representation of the inner Block is not generated during the
 * deserialization of the BendersBFunction. Instead, the BendersBFunction asks
 * the inner Block to generate its abstract representation at the first time
 * the BendersBFunction needs access to the Constraint. Doing this, gives
 * someone a chance to generate the abstract representation of the inner Block
 * after the deserialization of the inner Block.
 *
 * Therefore, the vector of pointers to the affected Constraint (namely,
 * v_constraints) is lazy initialized and, thus,
 *
 *      THE CURRENT (VERY INCONVENIENT) SOLUTION REQUIRES THE METHOD
 *      retrieve_constraints() TO BE INVOKED RIGHT BEFORE THE
 *      POINTER TO A Constraint IS NEEDED.
 */

void BendersBFunction::deserialize( const netCDF::NcGroup & group ,
                                    ModParam issueMod ) {

 c_Index nvar = get_num_active_var();

 auto ncDim_NumVar = group.getDim( "NumVar" );

 if( ncDim_NumVar.isNull() )
  throw( std::logic_error( "BendersBFunction::deserialize: "
                           "NumVar dimension is required." ) );

 if( ncDim_NumVar.getSize() != nvar )
  throw( std::invalid_argument( "BendersBFunction::deserialize: matrix A has "
                                "a wrong number of columns in the given "
                                "netCDF::NcGroup." ) );

 MultiVector tA;
 RealVector tb;

 auto ncDim_NumRow = group.getDim( "NumRow" );

 if( ( ! ncDim_NumRow.isNull() ) && ( ncDim_NumRow.getSize() != 0 ) ) {

  auto nrow = ncDim_NumRow.getSize();

  tA.resize( nrow );
  for( Index i = 0 ; i < tA.size() ; ++i ) {
   tA[ i ].resize( nvar , 0 );
  }

  if( ! ::deserialize( group , "b" , nrow , tb , true , false ) ) {
   tb.resize( nrow );
   tb.assign( tb.size() , 0 );
  }

  auto ncDim_NumNonZero = group.getDim( "NumNonzero" );

  if( ncDim_NumNonZero.isNull() ) {
   // A is given in dense format

   netCDF::NcVar ncVar_A = group.getVar( "A" );
   if( ncVar_A.isNull() )
    throw( std::logic_error( "BendersBFunction::deserialize: The 'A' matrix "
                             "was not found." ) );

   if( ncVar_A.getDimCount() != 2 )
    throw( std::logic_error( "BendersBFunction::deserialize: The 'A' matrix "
                             "must be given as a two-dimensional array." ) );

   auto A_dims = ncVar_A.getDims();

   if( A_dims[ 0 ].getSize() != nrow || A_dims[ 1 ].getSize() != nvar )
    throw( std::logic_error( "BendersBFunction::deserialize: The 'A' matrix "
                             "must have 'NumRow' rows and 'NumVar' columns." ) );

   for( Index i = 0 ; i < tA.size() ; ++i )
    ncVar_A.getVar( { i , 0 } , { 1 , nvar } , tA[ i ].data() );
  }
  else {

   auto nnz = ncDim_NumNonZero.getSize();

   if( group.getVar( "NumNonzeroAtRow" ).isNull() ) {
    // A is the identity matrix

    if( ( nrow != nvar ) && ( nrow != nnz ) )
     throw( std::logic_error( "BendersBFunction::deserialize: The 'A' matrix "
                              "is given as the identity matrix but either "
                              "'NumRow' != 'NumVar' or 'NumRow' != "
                              "'NumNonzero'." ) );

    for( Index i = 0 ; i < tA.size() ; ++i ) {
     tA[ i ][ i ] = 1;
    }
   }
   else {

    // A is given in sparse row-major format

    std::vector< Index > num_nonzero_at_row;
    ::deserialize( group , "NumNonzeroAtRow" , nrow , num_nonzero_at_row ,
                   false , false );

    std::vector< Index > column;
    ::deserialize( group , "Column" , nnz , column , false , false );

    netCDF::NcVar ncVar_A = group.getVar( "A" );
    if( ncVar_A.isNull() )
     throw( std::logic_error( "BendersBFunction::deserialize: The 'A' matrix "
                              "was not found." ) );

    if( ncVar_A.getDimCount() != 1 )
     throw( std::logic_error( "BendersBFunction::deserialize: The 'A' variable "
                              "was expected to be a one-dimensional array." ) );

    if( ncVar_A.getDims()[ 0 ].getSize() != nnz )
     throw( std::logic_error( "BendersBFunction::deserialize: The 'A' variable "
                              "was expected to be indexed over the 'NumNonzero' "
                              "dimension." ) );

    Index k = 0;
    for( Index i = 0 ; i < nrow ; ++i ) {
     for( Index l = 0 ; l < num_nonzero_at_row[ i ] ; ++l , ++k ) {
      auto j = column[ k ];
      ncVar_A.getVar( { k } , & v_A[ i ][ j ] );
     }
    }
   }
  }
 }

 auto inner_block_group = group.getGroup( BLOCK_NAME );
 if( inner_block_group.isNull() )
  throw( std::logic_error( "BendersBFunction::deserialize: the '" +
                           BLOCK_NAME + "' group must be present." ) );

 auto inner_block = new_Block( inner_block_group , this );
 if( ! inner_block )
  throw( std::logic_error( "BendersBFunction::deserialize: the '" +
                           BLOCK_NAME + "' group is present but its " +
                           "description is incomplete." ) );

 set_inner_block( inner_block );

 std::vector< ConstraintSide > sides;
 if( ! ::deserialize( group , "ConstraintSide" , tb.size() , sides , true ) ) {
  sides.resize( tb.size() );
  sides.assign( sides.size() , eBoth );
 }

 if( tA.size() != sides.size() )
  throw( std::invalid_argument( "BendersBFunction::deserialize: The size of "
                                "the 'ConstraintSide' vector  must be equal "
                                "to the number of rows of the A matrix." ) );

 auto path_group = group.getGroup( "AbstractPath" );

 if( ! path_group.isNull() )
  AbstractPath::vector_deserialize( path_group , v_paths_to_constraints );
 else if( sides.size() > 0 )
  throw( std::invalid_argument( "BendersBFunction::deserialize: The group "
                                "'AbstractPath' was not found." ) );

 if( v_paths_to_constraints.size() != sides.size() )
  throw( std::invalid_argument( "BendersBFunction::deserialize: The number of "
                                "AbstractPath to Constraint must be equal to "
                                "the size of the 'ConstraintSide' vector." ) );

 // The pointers to the affected RowConstraint are retrieved at the first time
 // they are needed, so as to give someone a chance to generate the abstract
 // representation of the inner Block.
 std::vector< RowConstraint * > constraints;
 constraints.resize( v_paths_to_constraints.size() , nullptr );

 set_mapping( std::move( tA ) , std::move( tb ) ,
              std::move( constraints ) , std::move( sides ) , issueMod );

 Block::deserialize( group );

}  // end( BendersBFunction::deserialize )

/*--------------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::set_variables( VarVector && x ) {
 if( ! v_A.empty() )
  if( v_A[ 0 ].size() != x.size() )
   throw( std::logic_error("BendersBFunction::set_variables: wrong x.size(). "
                           "Matrix A has " + std::to_string( v_A[ 0 ].size() ) +
                           " row(s), but x has size " +
                           std::to_string( x.size() ) ) );

 v_x = std::move( x );

 f_constraints_are_updated = false;
}  // end( BendersBFunction::set_variables )

/*--------------------------------------------------------------------------*/

void BendersBFunction::set_par( const idx_type par , const int value ) {
 switch( par ) {
  case( intMaxIter ):
   set_solver_par( Solver::intMaxIter , value );
   break;

  case( intLPMaxSz ):
   if( value < 1 )
    throw( std::invalid_argument( "BendersBFunction::set_par: intLPMaxSz "
                                  "must be non-negative" ) );
   set_solver_par( CDASolver::intMaxDSol , value );
   break;

  case( intGPMaxSz ): {
   if( value < 0 )
    throw( std::invalid_argument( "BendersBFunction::set_par: intGPMaxSz "
                                  "must be non-negative" ) );

   auto old_size = global_pool.size();

   global_pool.resize( value );

   if( f_Observer && ( decltype( old_size )( value ) < old_size ) ) {
    // The size of the global pool is being reduced. We store in "which" the
    // indices of the deleted linearizations.
    Subset which( global_pool.size() - value );
    std::iota( which.begin() , which.end() , value );
    f_Observer->add_Modification( std::make_shared< BendersBFunctionMod >(
     this , C05FunctionMod::GlobalPoolRemoved , std::move( which ) , 0 ) );
   }

   break;
  }

  case( intLinComp ):
   LinComp = value;
   break;

  default: C05Function::set_par( par , value );
 }
}  // end( BendersBFunction::set_par )

/*--------------------------------------------------------------------------*/
/*--------- METHODS FOR HANDLING THE State OF THE BendersBFunction ---------*/
/*--------------------------------------------------------------------------*/

State * BendersBFunction::get_State( void ) const {
 return( new BendersBFunctionState( this ) );
}  // end( BendersBFunction::get_State )

/*--------------------------------------------------------------------------*/

void BendersBFunction::put_State( const State & state ) {

 const auto & s = dynamic_cast< const BendersBFunctionState & >( state );

 const bool global_pool_was_empty = global_pool.empty();

 global_pool.clone( s.global_pool );

 if( ! f_Observer )
  return;

 // If the global pool was not initially empty, issue a Modification telling
 // that all previous linearizations have been removed.

 if( ! global_pool_was_empty )
  f_Observer->add_Modification( std::make_shared< BendersBFunctionMod >(
   this , C05FunctionMod::GlobalPoolRemoved , Subset() , 0 , 0 ) );

 // Collect the indices of all linearizations that were added and issue the
 // Modification.

 Subset added;
 added.reserve( global_pool.size() );
 for( Index i = 0 ; i < global_pool.size() ; ++i )
  if( global_pool.get_solution( i ) )
   added.push_back( i );

 if( ! added.empty() )
  f_Observer->add_Modification( std::make_shared< BendersBFunctionMod >(
   this , C05FunctionMod::GlobalPoolAdded , std::move( added ) , 0 , 0 ) );

}  // end( BendersBFunction::put_State )

/*--------------------------------------------------------------------------*/

void BendersBFunction::put_State( State && state ) {

 auto && s = dynamic_cast< BendersBFunctionState && >( state );

 const bool global_pool_was_empty = global_pool.empty();

 global_pool.clone( std::move( s.global_pool ) );

 if( ! f_Observer )
  return;

 // If the global pool was not initially empty, issue a Modification telling
 // that all previous linearizations have been removed.

 if( ! global_pool_was_empty )
  f_Observer->add_Modification( std::make_shared< BendersBFunctionMod >(
   this , C05FunctionMod::GlobalPoolRemoved , Subset() , 0 , 0 ) );

 // Collect the indices of all linearizations that were added and issue the
 // Modification.

 Subset added;
 added.reserve( global_pool.size() );
 for( Index i = 0 ; i < global_pool.size() ; ++i )
  if( global_pool.get_solution( i ) )
   added.push_back( i );

 if( ! added.empty() )
  f_Observer->add_Modification( std::make_shared< BendersBFunctionMod >(
   this , C05FunctionMod::GlobalPoolAdded , std::move( added ) , 0 , 0 ) );
}  // end( BendersBFunction::put_State )

/*--------------------------------------------------------------------------*/

void BendersBFunction::serialize_State
( netCDF::NcGroup & group , const std::string & sub_group_name ) const {

 if( ! sub_group_name.empty() ) {
  auto g = group.addGroup( sub_group_name );
  serialize_State( g );
  return;
 }

 group.putAtt( "type" , "BendersBFunctionState" );
 global_pool.serialize( group );

}  // end( BendersBFunction::serialize_State )

/*--------------------------------------------------------------------------*/
/*---- METHODS FOR HANDLING "ACTIVE" Variable IN THE BendersBFunction ------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::map_active( c_Vec_p_Var & vars , Subset & map ,
                                   const bool ordered ) const {
 if( v_x.empty() )
  return;

 if( map.size() < vars.size() )
  map.resize( vars.size() );

 if( ordered ) {
  Index found = 0;
  for( Index i = 0 ; i < v_x.size() ; ++i ) {
   auto itvi = std::lower_bound( vars.begin() , vars.end() , v_x[ i ] );
   if( itvi != vars.end() ) {
    map[ std::distance( vars.begin() , itvi ) ] = i;
    ++found;
   }
  }
  if( found < vars.size() )
   throw( std::invalid_argument( "BendersBFunction::map_active: some Variable "
                                 "is not active." ) );
 }
 else {
  auto it = map.begin();
  for( auto var : vars ) {
   auto i = this->is_active( var );
   if( i >= v_x.size() )
    throw( std::invalid_argument( "BendersBFunction::map_active: some Variable "
                                  "is not active" ) );
   *(it++) = i;
  }
 }
}  // end( BendersBFunction::map_active )

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE BendersBFunction ----------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::set_mapping( MultiVector && A , RealVector && b ,
                                    ConstraintVector && constraints ,
                                    ConstraintSideVector && sides ,
                                    ModParam issueMod ) {

 if( constraints.size() != b.size() )
  throw( std::invalid_argument( "BendersBFunction::set_mapping: the number of "
                                "affected constraints must be equal to the "
                                "size of b." ) );

 if( sides.size() != constraints.size() )
  throw( std::invalid_argument( "BendersBFunction::set_mapping: the number of "
                                "affected sides must be equal to the number of "
                                "affected constraints." ) );

 if( A.size() != b.size() )
  throw( std::invalid_argument( "BendersBFunction::set_mapping: A has " +
                                std::to_string( A.size() ) + " rows and b " +
                                "has " + std::to_string( b.size() ) + ", but "
                                "they must have the same number of rows." ) );
 if( ! A.empty() ) {
  if( v_x.size() != A[ 0 ].size() )
   throw( std::invalid_argument( "BendersBFunction::set_mapping: A and x must "
                                 "have the same number of columns." ) );

  const auto m = A[ 0 ].size();
  for( const auto & a : A )
   if( a.size() != m )
    throw( std::invalid_argument( "BendersBFunction::set_mapping: all rows of "
                                  "A must have the same size." ) );
 }

 v_A = std::move( A );
 v_b = std::move( b );
 v_constraints = std::move( constraints );
 v_sides = std::move( sides );

 f_constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // "nuclear modification" for Function: everything changed
 f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                this , FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::set_mapping )

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_variables( VarVector && nx , MultiVector && nA ,
                                      ModParam issueMod ) {
 const auto nn = nx.size();
 if( ! nn )  // actually nothing to add
  return;    // cowardly (and silently) return

 if( ( ! nA.empty() ) && ( ! v_A.empty() ) && ( nA.size() != v_A.size() ) )
  throw( std::invalid_argument( "BendersBFunction::add_variables: current A "
                                "matrix has " + std::to_string( v_A.size() ) +
                                "row(s), but provided new columns have " +
                                std::to_string( nA.size() ) + " row(s). They "
                                "must have the same number of rows." ) );

 for( const auto & a : nA )
  if( a.size() != nn )
   throw( std::invalid_argument( "BendersBFunction::add_variables: all columns "
                                 "of nA must have the size of nx." ) );

 if( v_A.empty() )
  v_A.resize( nA.size() );

 const auto n = v_x.size();

 if( ! n ) {    // very easy case: adding to nothing
  v_x = std::move( nx );
  assert( v_A.empty() );
  if( ! nA.empty() )
   v_A = std::move( nA );
 }
 else {         // not much more difficult: append at the end
  v_x.insert( v_x.end() , nx.begin() , nx.end() );
  if( ! nA.empty() )
   for( Index i = 0 ; i < v_A.size() ; ++i )
    v_A[ i ].insert( v_A[ i ].end() , nA[ i ].begin() , nA[ i ].end() );
 }

 f_constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;  // noone is listening: all done

 Vec_p_Var vars( nn );
 std::copy( v_x.begin() + n , v_x.end() , vars.begin() );

 // now issue the C05FunctionModVarsAddd
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
                                         this , std::move( vars ) , n , 0 ,
                                         Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::add_variables )

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_variable( ColVariable * const var ,
                                     c_RealVector & Aj , ModParam issueMod ) {
 if( var == nullptr )  // actually nothing to add
  return;              // cowardly (and silently) return

 if( v_A.empty() )
  v_A.resize( Aj.size() );
 else if( ! Aj.empty() && Aj.size() != v_A.size() )
  throw( std::invalid_argument( "BendersBFunction::add_variable: The size of Aj"
                                " must be equal to the number of rows of the A"
                                " matrix." ) );

 if( ! Aj.empty() )
  for( Index j = 0 ; j < v_A.size() ; ++j )
   v_A[ j ].push_back( Aj[ j ] );

 v_x.push_back( var );

 f_constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // now issue the Modification
 // a Benders function is strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsAddd >(
                                         this , Vec_p_Var( { var } ) ,
                                         v_x.size() - 1 , 0 ,
                                         Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::add_variable )

/*--------------------------------------------------------------------------*/

void BendersBFunction::remove_variable( Index i , ModParam issueMod )
{
 if( i >= v_x.size() )
  throw( std::logic_error( "BendersBFunction::remove_variable: invalid "
                           "Variable index " + std::to_string( i ) + "." ) );

 auto var = v_x[ i ];
 v_x.erase( v_x.begin() + i );    // erase it in v_x
 for( auto & ai : v_A )           // erase the column in A
  ai.erase( ai.begin() + i );

 f_constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // now issue the Modification
 // a Benders function is strongly quasi-additive
 f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                    this , Vec_p_Var( { var } ) ,
                                    Range( i , i + 1 ) , 0 ,
                                    Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::remove_variable( index ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::remove_variables( Range range , ModParam issueMod )
{
 range.second = std::min( range.second , Index( v_x.size() ) );
 if( range.second <= range.first )
  return;

 f_constraints_are_updated = false;

 if( ( range.first == 0 ) && ( range.second == Index( v_x.size() ) ) ) {
  // removing *all* Variables
  Vec_p_Var vars( v_x.size() );

  for( decltype( v_x )::size_type i = 0 ; i < v_x.size() ; ++i )
   vars[ i ] = v_x[ i ];

  v_x.clear();            // clear v_x
  for( auto & ai : v_A )  // erase all v_A
   ai.clear();

  // now issue the Modification
  // a Benders function is strongly quasi-additive
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
				      this , std::move( vars ) , range , 0 ,
				      Observer::par2concern( issueMod ) ) ,
				 Observer::par2chnl( issueMod ) );
  return;
  }

 // removing *some* Variables

 // erase the columns in v_A
 for( auto & ai : v_A )
  ai.erase( ai.begin() + range.first , ai.begin() + range.second );

 // erase the elements in v_x
 const auto strtit = v_x.begin() + range.first;
 const auto stopit = v_x.begin() + range.second;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  // somebody is there: meanwhile, prepare data for the Modification

  Vec_p_Var vars( range.second - range.first );
  std::copy( strtit , stopit , vars.begin() );
  v_x.erase( strtit , stopit );

  // now issue the Modification
  // a Benders function is strongly quasi-additive
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                    this , std::move( vars ) , range , 0 ,
                                    Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
 }
 else  // noone is there: just do it
  v_x.erase( strtit , stopit );

}  // end( BendersBFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

template< class T >
static void compact( std::vector< T > & x ,
                     const BendersBFunction::Subset & nms )
{
 BendersBFunction::Index i = nms.front();
 auto xit = x.begin() + (i++);
 for( auto nit = ++(nms.begin()) ; nit != nms.end() ; ++i )
  if( *nit == i )
   ++nit;
  else
   *(xit++) = std::move( x[ i ] );

 for( ; i < x.size() ; ++i )
  *(xit++) = std::move( x[ i ] );

 x.resize( x.size() - nms.size() );
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::remove_variables( Subset && nms , bool ordered ,
                                         ModParam issueMod )
{
 if( nms.empty() ) {      // removing *all* Variables

  if( v_x.empty() )       // there is no Variable to be removed
   return;                // cowardly (and silently) return

  Vec_p_Var vars( v_x.size() );

  for( Index i = 0 ; i < v_x.size() ; ++i )
   vars[ i ] = v_x[ i ];

  v_x.clear();            // clear v_x
  for( auto & ai : v_A )  // erase all v_A
   ai.clear();

  f_constraints_are_updated = false;

  // now issue the Modification: note that the subset is empty
  // a BendersBFunction is strongly quasi-additive, and nms is ordered
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
				 this , std::move( vars ) , Subset() , true ,
				 0 , Observer::par2concern( issueMod ) ) ,
				 Observer::par2chnl( issueMod ) );
  return;
 }

 // removing *some* Variables

 if( ! ordered )
  std::sort( nms.begin() , nms.end() );

 if( nms.back() >= v_x.size() )  // the last name is wrong
  throw( std::invalid_argument( "BendersBFunction::remove_variables: wrong "
                                "Variable index in the Subset nms." ) );

 for( auto & ai : v_A )          // erase the columns in A
  compact( ai , nms );

 f_constraints_are_updated = false;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  Vec_p_Var vars( nms.size() );
  auto its = vars.begin();
  for( auto nm : nms )
   *(its++) = v_x[ nm ];

  compact( v_x , nms );

  // now issue the Modification
  // a Benders function is strongly quasi-additive, and nms is ordered
  f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
                             this , std::move( vars ) , std::move( nms ) ,
                             true , 0 , Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
 }
 else  // noone is there: just do it
  compact( v_x , nms );

}  // end( BendersBFunction::remove_variables( subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_rows( MultiVector && nA , c_RealVector & nb ,
                                    Range range , ModParam issueMod ) {
 range.second = std::min( range.second , Index( v_A.size() ) );
 if( range.second <= range.first )
  return;

 if( nb.size() != range.second - range.first )
  throw( std::invalid_argument( "BendersBFunction::modify_rows: range and nb "
                                "sizes do not match." ) );

 if( nA.size() != range.second - range.first )
  throw( std::invalid_argument( "BendersBFunction::modify_rows: range and nA "
                                "sizes do not match." ) );

 // copy rows
 for( Index i = 0 ; i < nA.size() ; ++i ) {
  if( nA[ i ].size() != v_x.size() )
   throw( std::invalid_argument( "BendersBFunction::modify_rows: given row " +
                                 std::to_string( i ) + " (associated with " +
                                 "row " + std::to_string( range.first + i ) +
                                 " of the A matrix) has wrong size." ) );

  v_A[ range.first + i ] = std::move( nA[ i ] );
  v_b[ range.first + i ] = nb[ i ];
 }

 f_constraints_are_updated = false;

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModRngd >(
                                this , C05FunctionMod::AllLinearizationChanged ,
                                BendersBFunctionMod::ModifyRows , range ,
                                Subset( {} ) , C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::modify_rows( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_rows( MultiVector && nA , c_RealVector & nb ,
                                    Subset && rows , bool ordered ,
                                    ModParam issueMod ) {
 if( rows.empty() )  // actually nothing to modify
  return;            // cowardly (and silently) return

 if( nb.size() != rows.size() )
  throw( std::invalid_argument( "BendersBFunction::modify_rows: rows and nb "
                                "sizes do not match." ) );

 if( nA.size() != rows.size() )
  throw( std::invalid_argument( "BendersBFunction::modify_row: rows and nA "
                                "sizes do not match." ) );

 for( Index i = 0 ; i < rows.size() ; ++i ) {
  if( rows[ i ] >= v_A.size() )
   throw( std::invalid_argument( "BendersBFunction::modify_row: row " +
                                 std::to_string( rows[ i ] ) +
                                 " does not exist." ) );
  if( nA[ i ].size() != v_x.size() )
   throw( std::invalid_argument( "BendersBFunction::modify_row: given row " +
                                 std::to_string( i ) + " (associated with " +
                                 "row " + std::to_string( rows[ i ] ) +
                                 " of the A matrix) has wrong size." ) );

  v_A[ rows[ i ] ] = std::move( nA[ i ] );
  v_b[ rows[ i ] ] = nb[ i ];
 }

 f_constraints_are_updated = false;

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModSbst; note that rows is ordered
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModSbst >(
                                this , C05FunctionMod::AllLinearizationChanged ,
                                BendersBFunctionMod::ModifyRows ,
                                std::move( rows ) , ordered ,
                                Subset( {} ) , C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::modify_rows( subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_row( c_Index i , RealVector && Ai ,
                                   c_FunctionValue bi ,
                                   ModParam issueMod ) {
 if( i >= v_A.size() )
  throw( std::invalid_argument( "BendersBFunction::modify_row: row " +
                                std::to_string( i ) + " does not exist." ) );

 if( Ai.size() != v_x.size() )
  throw( std::invalid_argument( "BendersBFunction::modify_row: given row has "
                                "size different from that of x." ) );

 // actually change things
 v_A[ i ] = std::move( Ai );
 v_b[ i ] = bi;

 f_constraints_are_updated = false;

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModRngd >(
                                this , C05FunctionMod::AllLinearizationChanged ,
                                BendersBFunctionMod::ModifyRows ,
                                Range( i , i + 1 ) , Subset( {} ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::modify_row )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants( MF_dbl_it nb , Range range ,
					 c_ModParam issuePMod ,
					 c_ModParam issueAMod ) {

 range.second = std::min( range.second , Index( v_b.size() ) );
 if( range.second <= range.first )
  return;

 bool changed = false;
 for( Index i = 0 ; i < range.second - range.first ; ++i ) {
  const auto new_b = * ( nb + i );
  if( v_b[ range.first + i ] != new_b ) {
   v_b[ range.first + i ] = new_b;
   changed = true;
  }
 }

 if( ! changed )
  return;

 f_constraints_are_updated = false;

 // Dual solutions are still feasible. The g part of the linearizations are
 // still valid and only the linearization constants must be recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueAMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModRngd >(
                                this , C05FunctionMod::AlphaChanged ,
                                BendersBFunctionMod::ModifyCnst ,
                                range , Subset( {} ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueAMod ) ) ,
                               Observer::par2chnl( issueAMod ) );

}  // end( BendersBFunction::modify_constants( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants( c_RealVector & nb , Range range ,
                                         ModParam issueMod ) {
 modify_constants( nb.cbegin() , range , issueMod , issueMod );
}  // end( BendersBFunction::modify_constants( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants( MF_dbl_it nb , Subset && rows ,
					 const bool ordered ,
					 c_ModParam issuePMod ,
					 c_ModParam issueAMod )
{
 if( rows.empty() )  // actually nothing to modify
  return;            // cowardly (and silently) return

 for( const auto i : rows )
  if( i >= v_A.size() )
   throw( std::invalid_argument( "BendersBFunction::modify_constants: row " +
                                 std::to_string( i ) + " does not exist." ) );

 bool changed = false;
 for( Index i = 0 ; i < rows.size() ; ++i ) {
  const auto new_b = * ( nb + i );
  if( v_b[ rows[ i ] ] != new_b ) {
   v_b[ rows[ i ] ] = new_b;
   changed = true;
  }
 }

 if( ! changed )
  return;

 f_constraints_are_updated = false;

 // Dual solutions are still feasible. The g part of the linearizations are
 // still valid and only the linearization constants must be recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueAMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModSbst: note that ordered is unmodified
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModSbst >(
                                this , C05FunctionMod::AlphaChanged ,
                                BendersBFunctionMod::ModifyCnst ,
                                std::move( rows ) , ordered ,
                                Subset( {} ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueAMod ) ) ,
                               Observer::par2chnl( issueAMod ) );

}  // end( BendersBFunction::modify_constants )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants( c_RealVector & nb , Subset && rows ,
					 bool ordered , ModParam issueMod )
{
 modify_constants( nb.cbegin() , std::move( rows ) , ordered ,
                   issueMod , issueMod );
}  // end( BendersBFunction::modify_constants )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constant( c_Index i , c_FunctionValue bi ,
                                        ModParam issueMod ) {
 if( i >= v_A.size() )
  throw( std::invalid_argument( "BendersBFunction::modify_constant: row " +
                                std::to_string( i ) + " does not exist." ) );

 if( bi == v_b[ i ] )  // actually nothing is changing
  return;              // cowardly (and silently) return

 // actually change the constant
 v_b[ i ] = bi;

 f_constraints_are_updated = false;

 // Dual solutions are still feasible. The g part of the linearizations are
 // still valid and only the linearization constants must be recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModRngd >(
                                this , C05FunctionMod::AlphaChanged ,
                                BendersBFunctionMod::ModifyCnst ,
                                Range( i , i + 1 ) ,
                                Subset( {} ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::modify_constant )

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_rows( MultiVector && nA , c_RealVector & nb ,
                                 const ConstraintVector & nc ,
                                 const ConstraintSideVector & ns ,
                                 ModParam issueMod ) {
 const auto k = nA.size();
 if( k != nb.size() )
  throw( std::invalid_argument( "BendersBFunction::add_rows: nA and nb must "
                                "have the same size." ) );

 if( ns.size() != nc.size() )
  throw( std::invalid_argument( "BendersBFunction::add_rows: nc and ns must "
                                "have the same size." ) );

 if( k != nc.size() )
  throw( std::invalid_argument( "BendersBFunction::add_rows: nA and nc must "
                                "have the same size." ) );

 const auto n = v_x.size();
 for( const auto & a : nA )
  if( a.size() != n )
   throw( std::invalid_argument( "BendersBFunction::add_rows: some row of nA "
                                 "has a wrong size." ) );

 for( const auto & c : nc )
  if( c == nullptr )
   throw( std::invalid_argument( "BendersBFunction::add_rows: the pointer to "
                                 "the RowConstraint must be non-null." ) );

 v_A.insert( v_A.end() , std::make_move_iterator( nA.begin() ) ,
                         std::make_move_iterator( nA.end() ) );

 v_b.insert( v_b.end() , nb.begin(), nb.end() );

 add_constraints( nc );

 v_sides.insert( v_sides.end() , ns.begin(), ns.end() );

 f_constraints_are_updated = false;

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 auto mod_type = C05FunctionMod::AllLinearizationChanged;
 if( std::all_of( nb.cbegin(), nb.cend(),
                  []( FunctionValue i ) {
                   return( i == FunctionValue( 0 ) ); } ) ) {
  // The new part nb of b is zero. So, the linearization constants (alpha) do
  // not change.
  mod_type = C05FunctionMod::AllEntriesChanged;
 }
 else {
  global_pool.reset_linearization_constants();
 }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModAddd

 f_Observer->add_Modification( std::make_shared< BendersBFunctionModAddd >(
                                this , mod_type , k ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::add_rows )

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_row( RealVector && Ai , FunctionValue bi ,
                                RowConstraint * ci , ConstraintSide si ,
                                ModParam issueMod ) {
 if( Ai.size() != v_x.size() )
  throw( std::invalid_argument( "BendersBFunction::add_row: given row Ai "
                                "has wrong size." ) );

 if( ci == nullptr )
  throw( std::invalid_argument( "BendersBFunction::add_row: the pointer to "
                                "the RowConstraint must be non-null." ) );

 v_A.push_back( std::move( Ai ) );
 v_b.push_back( bi );
 v_sides.push_back( si );
 add_constraint( ci );

 f_constraints_are_updated = false;

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 auto mod_type = C05FunctionMod::AllLinearizationChanged;
 if( bi == FunctionValue( 0 ) ) {
  // The new entry bi of b is zero. So, the linearization constants (alpha) do
  // not change.
  mod_type = C05FunctionMod::AllEntriesChanged;
 }
 else {
  global_pool.reset_linearization_constants();
 }

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModAddd
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModAddd >(
                                this , mod_type , 1 ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::add_row )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_rows( Range range , ModParam issueMod ) {
 range.second = std::min( range.second , Index( v_b.size() ) );
 if( range.second <= range.first )
  return;

 if( range.second - range.first == 1 ) {
  delete_row( range.first , issueMod );
  return;
 }

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 auto mod_type = C05FunctionMod::AllLinearizationChanged;
 if( std::all_of( v_b.cbegin() + range.first , v_b.cbegin() + range.second ,
                  []( FunctionValue i ) {
                   return( i == FunctionValue( 0 ) ); } ) ) {
  // The entries of b to be deleted are all zero. So, the linearization
  // constants (alpha) do not change.
  mod_type = C05FunctionMod::AllEntriesChanged;
 }
 else {
  global_pool.reset_linearization_constants();
 }

 v_A.erase( v_A.begin() + range.first , range.second < v_A.size() ?
                                        v_A.begin() + range.second :
                                        v_A.end() );

 v_b.erase( v_b.begin() + range.first , v_b.begin() + range.second );

 remove_constraints( range );

 v_sides.erase( v_sides.begin() + range.first ,
                v_sides.begin() + range.second );

 f_constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModRngd >(
                                this , mod_type ,
                                BendersBFunctionMod::DeleteRows , range ,
                                Subset( {} ) , C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::delete_rows( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_rows( Subset && rows , bool ordered ,
                                    ModParam issueMod ) {
 if( rows.empty() )  // actually nothing to remove
  return;            // cowardly (and silently) returning

 if( rows.size() == 1 ) {
  delete_row( rows.front() , issueMod );
  return;
 }

 if( ! ordered )
  std::sort( rows.begin() , rows.end() );

 if( rows.back() >= v_b.size() )
  throw( std::invalid_argument( "BendersBFunction::delete_rows: given row "
                                "index " + std::to_string( rows.back() ) +
                                " does not exist." ) );

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 auto mod_type = C05FunctionMod::AllEntriesChanged;

 // mark stuff to be killed in v_A[] and v_b[]
 for( auto idx : rows ) {
  if( v_b[ idx ] != FunctionValue( 0 ) )
    mod_type = C05FunctionMod::AllLinearizationChanged;

  v_A[ idx ].clear();
  v_b[ idx ] = std::numeric_limits< FunctionValue >::quiet_NaN();
  v_constraints[ idx ] = nullptr;
  v_sides[ idx ] = ConstraintSide::eNone;
 }

 // kill stuff in v_A[]
 v_A.erase( std::remove_if( v_A.begin() + rows.front() , v_A.end() ,
                            []( RealVector & ai ) { return( ai.empty() ); } ) ,
            v_A.end() );

 // kill stuff in v_b[]
 v_b.erase( std::remove_if( v_b.begin() + rows.front() , v_b.end() ,
                            []( FunctionValue bi ) { return( std::isnan( bi ) );
                            } ) ,
            v_b.end() );

 // kill stuff in v_constraints[]
 remove_constraints( rows );

 // kill stuff in v_sides[]
 v_sides.erase
  ( std::remove_if( v_sides.begin() + rows.front() , v_sides.end() ,
                    []( ConstraintSide si ) {
                     return( si == ConstraintSide::eNone ); } ) ,
    v_sides.end() );

 if( mod_type == C05FunctionMod::AllLinearizationChanged )
  global_pool.reset_linearization_constants();

 f_constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModSbst; rows is ordered
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModSbst >(
                                this , mod_type ,
                                BendersBFunctionMod::DeleteRows ,
                                std::move( rows ) , true , Subset( {} ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::delete_rows( subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_row( c_Index i , ModParam issueMod ) {
 if( i >= v_b.size() )
  throw( std::invalid_argument( "BendersBFunction::delete_row: given row " +
                                std::to_string( i ) + " does not exist." ) );

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 auto mod_type = C05FunctionMod::AllLinearizationChanged;
 if( v_b[ i ] == FunctionValue( 0 ) ) {
  // The i-th entry of b is zero. So, the linearization constants (alpha) do
  // not change.
  mod_type = C05FunctionMod::AllEntriesChanged;
 }
 else {
  global_pool.reset_linearization_constants();
 }

 v_A.erase( v_A.begin() + i );                     // kill i in v_A[]
 v_b.erase( v_b.begin() + i );                     // kill i in v_b[]
 remove_constraint( i );                           // kill i in v_constraints[]
 v_sides.erase( v_sides.begin() + i );             // kill i in v_sides[]

 f_constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared< BendersBFunctionModRngd >(
                                this , mod_type ,
                                BendersBFunctionMod::DeleteRows ,
                                Range( i , i + 1 ) , Subset( {} ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::delete_row )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_rows( ModParam issueMod ) {
 v_A.clear();   // delete original rows
 v_b.clear();
 remove_constraints();
 v_sides.clear();

 global_pool.invalidate();
 f_constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // "nuclear modification" for Function: everything changed
 f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                this , FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::delete_rows( all ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::set_default_inner_Block_BlockConfig() {
 if( auto inner_block = get_inner_block() ) {
  auto config = new OCRBlockConfig( inner_block );
  config->clear();
  config->apply( inner_block );
  delete config;
 }
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::set_default_inner_Block_BlockSolverConfig() {
 if( auto inner_block = get_inner_block() ) {
  auto solver_config = new RBlockSolverConfig( inner_block );
  solver_config->clear();
  solver_config->apply( inner_block );
  delete solver_config;
 }
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::set_ComputeConfig( ComputeConfig * scfg ) {

 ThinComputeInterface::set_ComputeConfig( scfg );

 auto inner_block = get_inner_block();

 if( ! inner_block )
  return;
 else if( ! scfg ) {
  // scfg is nullptr
  set_default_inner_Block_configuration();
  return;
 }
 else if( ! scfg->f_extra_Configuration ) {
  // scfg->f_extra_Configuration is nullptr
  if( ! scfg->f_diff )
   set_default_inner_Block_configuration();
  return;
 }

 auto config_map = dynamic_cast
  < SimpleConfiguration< std::map< std::string , Configuration * > > * >
  ( scfg->f_extra_Configuration );

 if( ! config_map )
  // An invalid extra Configuration has not been provided.
  throw( std::invalid_argument( "BendersBFunction::set_ComputeConfig: "
                                "invalid extra_Configuration." ) );

 for( const auto & [ key , config ] : config_map->f_value ) {

  if( key == "BlockConfig" ) {
   if( ! config ) {
    if( ! scfg->f_diff )
     // A BlockConfig for the inner Block was not provided. The inner Block is
     // configured to its default configuration.
     set_default_inner_Block_BlockConfig();
   }
   else if( auto block_config = dynamic_cast< BlockConfig * >( config ) )
    // A BlockConfig for the inner Block has been provided. Apply it.
    block_config->apply( inner_block );
   else
    // An invalid Configuration has been provided.
    throw( std::invalid_argument
           ( "BendersBFunction::set_ComputeConfig: the Configuration "
             "associated with key \"BlockConfig\" is not a BlockConfig." ) );
  }
  else if( key == "BlockSolverConfig" ) {
   if( ! config ) {
    if( ! scfg->f_diff )
     // A BlockSolverConfig for the inner Block was not provided. The Solver
     // of the inner Block (and their sub-Block, recursively) are unregistered
     // and deleted.
     set_default_inner_Block_BlockSolverConfig();
   }
   else if( auto bsc = dynamic_cast< BlockSolverConfig * >( config ) )
    // A BlockSolverConfig for the inner Block has been provided. Apply it.
    bsc->apply( inner_block );
   else
    // An invalid Configuration has been provided.
    throw( std::invalid_argument
           ( "BendersBFunction::set_ComputeConfig: the Configuration "
             "associated with key \"BlockSolverConfig\" is not a "
             "BlockSolverConfig." ) );
  }
  else if( key == "get_dual_solution" ) {
   delete f_get_dual_solution_config;
   f_get_dual_solution_config = config->clone();
  }
  else if( key == "get_dual_direction" ) {
   delete f_get_dual_direction_config;
   f_get_dual_direction_config = config->clone();
  }
  else if( key == "get_dual" ) {
   delete f_get_dual_solution_config;
   delete f_get_dual_direction_config;
   f_get_dual_solution_config = config->clone();
   f_get_dual_direction_config = config->clone();
  }
  else if( key == "get_dual_solution_partial" ) {
   delete f_get_dual_solution_partial_config;
   f_get_dual_solution_partial_config = config->clone();
  }
  else if( key == "get_dual_direction_partial" ) {
   delete f_get_dual_direction_partial_config;
   f_get_dual_direction_partial_config = config->clone();
  }
  else if( key == "get_dual_partial" ) {
   delete f_get_dual_solution_partial_config;
   delete f_get_dual_direction_partial_config;
   f_get_dual_solution_partial_config = config->clone();
   f_get_dual_direction_partial_config = config->clone();
  }
  else {
   // An invalid key has been provided.
   throw( std::invalid_argument( "BendersBFunction::set_ComputeConfig: "
                                 "invalid key: " + key ) );
  }
 }
}  // end( BendersBFunction::set_ComputeConfig )

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::add_Modification( sp_Mod mod ,
                                         Observer::ChnlName chnl )
{

 if( f_ignore_modifications )
  return;

 // GroupModification - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( const auto tmod = std::dynamic_pointer_cast< GroupModification >( mod ) ) {
  for( const auto & submod : tmod->sub_Modifications() )
   this->add_Modification( submod , chnl );
  return;
  }

 // FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( const auto tmod = std::dynamic_pointer_cast< FunctionMod >( mod ) ) {

  auto observer = tmod->function()->get_Observer();

  if( const auto constraint = dynamic_cast< Constraint * >( observer ) ) {
   if( this->has_constraint( constraint ) )
    send_nuclear_modification( chnl );
   else {
    /* Dual solutions are still feasible and linearizations are still
     * valid. However, the value of this BendersBFunction changes
     * unpredictably. */
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< FunctionMod >(
      this , FunctionMod::NaNshift ) , chnl );
    }
   }
  else {
   /* Send a "nuclear Function Modification" considering the Observer is:
   *
   * - An Objective. Dual solutions may become infeasible and the value of
   *   this BendersBFunction may change unpredictably.
   *
   * - Unknown.
   */
   send_nuclear_modification( chnl );
   }
  return;
  }

 // FunctionModVars - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( const auto tmod = std::dynamic_pointer_cast< FunctionModVars >( mod ) ) {

  auto observer = tmod->function()->get_Observer();

  if( const auto constraint = dynamic_cast< Constraint * >( observer ) ) {
   if( this->has_constraint( constraint ) ) {
    // The Constraint is being handled by this BendersBFunction.
    if( tmod->added() ) {
     /* Variables were added to the Constraint. Dual solutions may become
      * infeasible and the value of this BendersBFunction may change
      * unpredictably. */
     send_nuclear_modification( chnl );
     }
    else {
     /* Variables were removed from the Constraint. Dual solutions are still
      * feasible but the value of this BendersBFunction may change
      * unpredictably. */
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared< FunctionMod >(
       this , FunctionMod::NaNshift ) , chnl );
     }
    }
   else {
    // The Constraint is not being handled by this BendersBFunction.
    /* Dual solutions are still feasible and linearizations are still
     * valid. However, the value of this BendersBFunction may change
     * unpredictably. */
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< FunctionMod >(
      this , FunctionMod::NaNshift ) , chnl );
    }
   }
  else {
   /* Send a "nuclear Function Modification" considering the Observer is:
    *
    * - An Objective. Variables were added to the Constraint. Dual solutions
    *   may become infeasible and the value of this BendersBFunction may
    *   change unpredictably.
    *
    * - Unknown.
    */
   send_nuclear_modification( chnl );
   }
  return;
  }

 // ConstraintMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( const auto tmod = std::dynamic_pointer_cast< ConstraintMod >( mod ) ) {

  if( tmod->type() == ConstraintMod::eRelaxConst ||
      tmod->type() == ConstraintMod::eEnforceConst ) {

   auto behaviour = get_behaviour( tmod );
   if( behaviour == function_value_behaviour::unknown )
    send_nuclear_modification( chnl );
   else
    if( behaviour == function_value_behaviour::increase && f_Observer )
     f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                    this , Inf< FunctionValue >() ) ,
                                   chnl );
    else
     if( behaviour == function_value_behaviour::decrease && f_Observer )
      f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                     this , -Inf< FunctionValue >() ) ,
                                    chnl );
   return;
   }

  // actually a RowConstraintMod

  if( const auto tmod = std::dynamic_pointer_cast< RowConstraintMod >( mod ) ) {
   switch( tmod->type() ) {
    case( RowConstraintMod::eChgRHS ):
    case( RowConstraintMod::eChgLHS ):
    case( RowConstraintMod::eChgBTS ): {
     if( this->has_constraint( tmod->constraint() ) ) {
      /* Dual solution is still feasible and linearizations are still
       * valid. But since this Constraint is being handled by this
       * BendersBFunction, it must be updated. */
      f_constraints_are_updated = false;
      }
     else  // this BendersBFunction may change unpredictably.
      send_nuclear_modification( chnl );

     return;
     }
    default:  // unknown modification
     send_nuclear_modification( chnl );
    }
   return;
   }

  // actually a FRowConstraintMod

  if( const auto tmod = std::dynamic_pointer_cast< FRowConstraintMod >( mod ) ) {
   if( tmod->type() == FRowConstraintMod::eFunctionChanged ) {
    // Pointer to the Function has changed
    if( this->has_constraint( tmod->constraint() ) )
     /* Dual solutions may not be feasible and this BendersBFunction may
      * change unpredictably. */
     send_nuclear_modification( chnl );
    else
     /* Dual solutions are still feasible and linearizations are still
      * valid. However, the value of this BendersBFunction changes
      * unpredictably. */
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                     this , FunctionMod::NaNshift ) ,
                                    chnl );
    }
   else  // unknown modification
    send_nuclear_modification( chnl );

   return;
   }

  // actually a OneVarConstraintMod

  if( const auto tmod =
                    std::dynamic_pointer_cast< OneVarConstraintMod >( mod ) ) {

   if( tmod->type() == OneVarConstraintMod::eVariableChanged ) {
    // Pointer to the Variable of a OneVarConstraint has changed.
    if( ! this->has_constraint( tmod->constraint() ) ) {
     /* Dual solutions are still feasible and linearizations are still
      * valid. However, the value of this BendersBFunction changes
      * unpredictably. */
     if( f_Observer )
      f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                     this , FunctionMod::NaNshift ) ,
                                    chnl );
     return;
     }
    }
   }

  // unknown modification
  send_nuclear_modification( chnl );
  return;

  }  // end ConstraintMod - - - - - - - - - - - - - - - - - - - - - - - - - -

 // BlockModAD- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( const auto tmod = std::dynamic_pointer_cast< BlockModAD >( mod ) ) {
  if( tmod->is_variable() ) {
   if( tmod->is_added() )
    // Variables were added. This BendersBFunction may change unpredictably.
    send_nuclear_modification( chnl );
   else
    /* Variables were removed. Dual solutions are still feasible. But the
     * value of this BendersBFunction may change unpredictably. */
    if( f_Observer )
     f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                    this , FunctionMod::NaNshift ) ,
                                   chnl );
   }
  else {
   /* Constraints were added or removed. Since these Constraints must not be
    * any of those handled by this BendersBFunction, dual solutions are still
    * feasible. We now check how the behaviour of this BendersBFunction
    * changes. */

   auto behaviour = get_behaviour( tmod );
   if( behaviour == function_value_behaviour::unknown )
    send_nuclear_modification( chnl );
   else
    if( behaviour == function_value_behaviour::increase && f_Observer )
    f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                   this , Inf< FunctionValue >() ) ,
                                  chnl );
    else
     if( behaviour == function_value_behaviour::decrease && f_Observer )
      f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                     this , -Inf< FunctionValue >() ) ,
                                    chnl );
  }

  return;
  }

 /* If all else fails, send a "nuclear Function Modification" considering the
  * Modification is:
   *
   * - VariableMod
   *
   *   The type of a Variable has changed. Dual solutions may become
   *   infeasible and the value of this BendersBFunction may change
   *   unpredictably.
   *
   * - ObjectiveMod
   *
   *   An Objective has changed. Dual solutions may become infeasible and this
   *   BendersBFunction may change unpredictably.
   *
   * - BlockMod
   *
   * - NModification
   *
   * - Unknown modification
   */

 send_nuclear_modification( chnl );

 }  // end( BendersBFunction::add_Modification )

/*--------------------------------------------------------------------------*/
/*------------ METHODS FOR Saving THE DATA OF THE BendersBFunction ---------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::serialize( netCDF::NcGroup & group ) const {

 Block::serialize( group );

 c_Index nvar = get_num_active_var();

 auto NcDim_NumVar = group.addDim( "NumVar" , nvar );

 if( v_A.size() ) {
  auto NcDim_NumRow = group.addDim( "NumRow" , v_A.size() );

  BendersBFunction::SparseMatrix< FunctionValue > sparse_A( v_A.size() );
  if( is_A_sparse( sparse_A ) ) {
   // Store A in sparse format
   sparse_A.serialize( group , NcDim_NumRow );
  }
  else {
   // Store A in dense format

   auto NcVar_A = group.addVar( "A" , netCDF::NcDouble() ,
                                { NcDim_NumRow , NcDim_NumVar } );

   for( Index i = 0 ; i < v_A.size() ; ++i )
    NcVar_A.putVar( { i , 0 } , { 1 , nvar } , v_A[ i ].data() );
  }

  ::serialize( group , "b" , netCDF::NcDouble() , NcDim_NumRow , v_b );

  ::serialize( group , "ConstraintSide" , netCDF::NcByte() ,
               NcDim_NumRow , v_sides );
 }

 auto abstract_path_group = group.addGroup( "AbstractPath" );

 if( v_paths_to_constraints.empty() ) {
  // Construct the paths to the Constraint
  std::vector< AbstractPath > paths;
  paths.reserve( v_constraints.size() );
  for( const auto constraint : v_constraints ) {
   paths.emplace_back( constraint , get_inner_block() );
  }
  AbstractPath::serialize( paths , abstract_path_group );
 }
 else {
  // Use the paths we have
  AbstractPath::serialize( v_paths_to_constraints , abstract_path_group );
 }

 if( auto inner_block = get_inner_block() ) {
  auto inner_block_group = group.addGroup( BLOCK_NAME );
  inner_block->serialize( inner_block_group );
 }
}

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE BendersBFunction --------*/
/*--------------------------------------------------------------------------*/

int BendersBFunction::compute( bool changedvars ) {

 if( ( ! changedvars ) && f_constraints_are_updated )
  // TODO We need another flag telling whether the sub-Block has changed since
  // the last call.
  return( f_solver_status ); //  nothing changed since last call, nothing to do

 if( v_Block.size() != 1 )
  throw( std::logic_error( "BendersBFunction::compute: there must be exactly "
                           "one sub-Block, but there is (are) " +
                           std::to_string( v_Block.size() ) + "." ) );

 auto solver = get_solver();

 if( ! solver )
  throw( std::logic_error(
             "BendersBFunction::compute: no Solver attached to sub-Block" ) );

 if( changedvars || ( ! f_constraints_are_updated ) ) {
  // update the constraints

  // try to lock the inner Block: if this does not work
  auto owned = v_Block.front()->is_owned_by( f_id );
  if( ( ! owned ) && ( ! v_Block.front()->lock( f_id ) ) )
   return( kError );     // that's clearly an error

  update_constraints();

  if( ! owned )
   v_Block.front()->unlock( f_id );  // unlock the inner Block
 }

 // TODO can we assume that the variables of the sub-Block haven't changed?
 f_solver_status = solver->compute( true );

 return( f_solver_status );

 }  // end( BendersBFunction::compute )

/*--------------------------------------------------------------------------*/

static RealObjective::OFValue get_recours_obj( const Block * blck )
{
 RealObjective::OFValue rv = 0;
 if( auto obj = dynamic_cast< RealObjective * >( blck->get_objective() ) )
  rv = obj->get_constant_term();
 for( const auto bk : blck->get_nested_Blocks() )
  rv += get_recours_obj( bk );

 return( rv );
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

Function::FunctionValue BendersBFunction::get_constant_term( void ) const
{
 if( auto bk = get_inner_block() )
  return( get_recours_obj( bk ) );
 else
  return( 0 );
 }

/*--------------------------------------------------------------------------*/

bool BendersBFunction::is_convex( void ) const {
 if( v_Block.empty() ) return( false );
 return( v_Block.front()->get_objective_sense() == Objective::eMin );
}

/*--------------------------------------------------------------------------*/

bool BendersBFunction::is_concave( void ) const {
 if( v_Block.empty() )
  return( false );
 return( v_Block.front()->get_objective_sense() == Objective::eMax );
}

/*--------------------------------------------------------------------------*/

bool BendersBFunction::has_linearization( const bool diagonal ) {

 auto solver = get_solver< CDASolver >();

 if( ! solver )
  return( false );

 if( diagonal ) {
  f_diagonal_linearization_required = true;
  return( solver->has_dual_solution() );
 }
 else {
  f_diagonal_linearization_required = false;
  return( solver->has_dual_direction() );
 }

}  // end( BendersBFunction::has_linearization )

/*--------------------------------------------------------------------------*/

bool BendersBFunction::compute_new_linearization( const bool diagonal ) {
 auto solver = get_solver< CDASolver >();
 if( ! solver )
  return( false );
 if( diagonal )
  return( solver->new_dual_solution() );
 else
  return( solver->new_dual_direction() );
}  // end ( BendersBFunction::compute_new_linearization )

/*--------------------------------------------------------------------------*/

Function::FunctionValue BendersBFunction::get_value( void ) const {
 if( v_Block.size() != 1 )
  throw( std::logic_error( "BendersBFunction::get_value: there must be exactly "
                           "one sub-Block, but there is (are) " +
                           std::to_string( v_Block.size() ) + "." ) );

 auto solver = get_solver();

 if( ! solver )
  throw( std::logic_error( "BendersBFunction::get_value: It is not possible to "
                           "get the value. The sub-Block has no Solver "
                           "attached to it." ) );

 if( v_Block.front()->get_objective_sense() == Objective::eMin )
  return( solver->get_ub() );
 return( solver->get_lb() );

} // end ( BendersBFunction::get_value )

/*--------------------------------------------------------------------------*/

/* The last linearization that was computed by calling
 * compute_new_linearization() (which returned true) can be permanently stored
 * in the global pool of linearizations by calling this method. */

void BendersBFunction::store_linearization( Index name , ModParam issueMod ) {
 if( name >= global_pool.size() )
  throw( std::invalid_argument( "BendersBFunction::store_linearization: "
                                "invalid global pool name: " +
                                std::to_string( name ) ) );

 auto solver = get_solver< CDASolver >();

 if( ! solver )
  throw( std::logic_error( "BendersBFunction::store_linearization: It is not "
                           "possible to store the linearization. The sub-Block"
                           " (if present) has no Solver attached to it." ) );

 // TODO check whether the solution has already been written into the Block

 if( f_diagonal_linearization_required )
  solver->get_dual_solution( f_get_dual_solution_config );
 else
  solver->get_dual_direction( f_get_dual_direction_config );

 Solution * solution = nullptr;

 if( ! solution )
  solution = v_Block.front()->get_Solution();
 solution->read( v_Block.front() );

 // Lazy computation of the linearization constant

 // TODO If the linearization constant has just been computed, use this
 // computed value instead of Inf.

 global_pool.store( Inf< FunctionValue >() , solution , name ,
                    f_diagonal_linearization_required );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared< BendersBFunctionMod >(
                                this , C05FunctionMod::GlobalPoolAdded ,
                                Subset( { name } ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

} // end BendersBFunction::store_linearization( Index )

/*--------------------------------------------------------------------------*/

void BendersBFunction::store_combination_of_linearizations(
		            c_LinearCombination & coefficients , Index name ,
			    ModParam issueMod )
{
 global_pool.store_combination_of_linearizations( coefficients , name ,
                                                  AAccMlt );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared< BendersBFunctionMod >(
                                this , C05FunctionMod::GlobalPoolAdded ,
                                Subset( { name } ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::store_combination_of_linearizations )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_linearization( const Index name ,
                                             ModParam issueMod ) {
 global_pool.delete_linearization( name );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared< BendersBFunctionMod >(
                                this , C05FunctionMod::GlobalPoolRemoved ,
                                Subset( { name } ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );
}  // end( BendersBFunction::delete_linearization )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_linearizations( Subset && which , bool ordered ,
                                              ModParam issueMod ) {
 global_pool.delete_linearizations( which , ordered );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared< BendersBFunctionMod >(
                                this , C05FunctionMod::GlobalPoolRemoved ,
                                std::move( which ) , 0 ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::write_dual_solution( Index name ) {

 auto solver = get_solver< CDASolver >();

 if( ! solver )
  throw( std::logic_error( "BendersBFunction::write_dual_solution: The sub-Blo"
                           "ck (if present) has no Solver attached to it." ) );

 if( name == Inf< Index >() ) {
  // Last computed linearization. Notice that only the dual solution
  // associated with the Constraint handled by this BendersBFunction are being
  // required.

  // TODO check whether the solution has already been written into the Block

  if( f_diagonal_linearization_required )
   solver->get_dual_solution( f_get_dual_solution_partial_config );
  else
   solver->get_dual_direction( f_get_dual_direction_partial_config );
 }
 else
  // Linearization stored in the global pool
  write_dual_solution_from_global_pool( name );
}  // end( BendersBFunction::write_dual_solution )

/*--------------------------------------------------------------------------*/

bool ignore_constraint( RowConstraint * constraint ) {
 if( constraint->is_relaxed() )
  return( true );
 if( constraint->get_lhs() == -Inf< RowConstraint::RHSValue >() &&
     constraint->get_rhs() ==  Inf< RowConstraint::RHSValue >() )
  return( true );
 return( false );
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients
( FunctionValue * g , Range range , Index name ) {

 range.second = std::min( range.second , Index( v_x.size() ) );
 if( range.second <= range.first )
  return;

 write_dual_solution( name );

 const auto obj_sign =
  ( v_Block.front()->get_objective_sense() == Objective::eMin ) ? - 1 : 1;

 std::fill_n( g , range.second - range.first , 0 );

 for( Index j = 0 ; j < v_constraints.size() ; ++j ) {

  const auto constraint = get_constraint( j );
  if( ignore_constraint( constraint ) )
   continue;

  const auto dual_value = constraint->get_dual();

  if( dual_value == 0 )
   continue;

  if( obj_sign * dual_value >= 0 && v_sides[ j ] == eRHS )
   continue;

  if( obj_sign * dual_value <= 0 && v_sides[ j ] == eLHS )
   continue;

  for( Index i = range.first ; i < range.second ; ++i ) {
   g[ i - range.first ] += - dual_value * v_A[ j ][ i ];
  }
 }
}  // end( BendersBFunction::get_linearization_coefficients( * , range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients
( SparseVector & g , Range range , Index name ) {

 range.second = std::min( range.second , Index( v_x.size() ) );
 if( range.second <= range.first )
  return;

 write_dual_solution( name );

 const auto obj_sign =
  ( v_Block.front()->get_objective_sense() == Objective::eMin ) ? - 1 : 1;

 if( g.nonZeros() == 0 ) {  // g contains no non-zero element
  g.resize( v_x.size() );
  g.reserve( range.second - range.first );
  for( Index i = range.first ; i < range.second ; ++i )
   g.insert( i ) = 0;
 }
 else {                     // g contains some non-zero elements
  if( static_cast< decltype( v_x.size() ) >( g.size() ) != v_x.size() )
   throw( std::invalid_argument( "BendersBFunction::get_linearization_"
                                 "coefficients: invalid SparseVector size" ) );

  for( Index i = range.first ; i < range.second ; ++i )
   g.coeffRef( i ) = 0;
 }

 for( Index j = 0; j < v_constraints.size(); ++j ) {

  const auto constraint = get_constraint( j );
  if( ignore_constraint( constraint ) )
   continue;

  const auto dual_value = constraint->get_dual();

  if( dual_value == 0 )
   continue;

  if( obj_sign * dual_value >= 0 && v_sides[ j ] == eRHS )
   continue;

  if( obj_sign * dual_value <= 0 && v_sides[ j ] == eLHS )
   continue;

  for( Index i = range.first ; i < range.second ; ++i )
   g.coeffRef( i ) += - dual_value * v_A[ j ][ i ];
 }
}  // end( BendersBFunction::get_linearization_coefficients( sv , range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients
( FunctionValue * g , c_Subset & subset , const bool ordered , Index name ) {

 write_dual_solution( name );

 const auto obj_sign =
  ( v_Block.front()->get_objective_sense() == Objective::eMin ) ? - 1 : 1;

 std::fill_n( g , subset.size() , 0 );

 for( auto i : subset ) {
  if( i >= v_x.size() )
   throw( std::invalid_argument( "BendersBFunction::get_linearization_"
                                 "coefficients: invalid index: " +
                                 std::to_string( i ) + "." ) );
 }

 for( Index j = 0; j < v_constraints.size(); ++j ) {

  const auto constraint = get_constraint( j );
  if( ignore_constraint( constraint ) )
   continue;

  const auto dual_value = constraint->get_dual();

  if( dual_value == 0 )
   continue;

  if( obj_sign * dual_value >= 0 && v_sides[ j ] == eRHS )
   continue;

  if( obj_sign * dual_value <= 0 && v_sides[ j ] == eLHS )
   continue;

  Index k = 0;
  for( auto i : subset )
   g[ k++ ] += - dual_value * v_A[ j ][ i ];
 }
}  // end( BendersBFunction::get_linearization_coefficients( * , subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients
( SparseVector & g , c_Subset & subset , const bool ordered , Index name ) {

 write_dual_solution( name );

 const auto obj_sign =
  ( v_Block.front()->get_objective_sense() == Objective::eMin ) ? - 1 : 1;

 if( g.nonZeros() == 0 ) {  // g contains no non-zero element
  g.resize( v_x.size() );
  g.reserve( subset.size() );
  for( auto i : subset ) {
   if( i >= v_x.size() )
    throw( std::invalid_argument("BendersBFunction::get_linearization_"
                                 "coefficients: wrong index in subset: " +
                                 std::to_string( i ) ) );
   g.insert( i ) = 0;
  }
 }
 else {                     // g contains some non-zero elements
  if( static_cast< decltype( v_x.size() ) >( g.size() ) != v_x.size() )
   throw( std::invalid_argument( "BendersBFunction::get_linearization_"
                                 "coefficients: invalid SparseVector size" ) );

  for( auto i : subset ) {
   if( i >= v_x.size() )
    throw( std::invalid_argument("BendersBFunction::get_linearization_"
                                 "coefficients: wrong index in subset: " +
                                 std::to_string( i ) ) );
   g.coeffRef( i ) = 0;
  }
 }

 for( Index j = 0; j < v_constraints.size(); ++j ) {

  const auto constraint = get_constraint( j );
  if( ignore_constraint( constraint ) )
   continue;

  const auto dual_value = constraint->get_dual();

  if( dual_value == 0 )
   continue;

  if( obj_sign * dual_value >= 0 && v_sides[ j ] == eRHS )
   continue;

  if( obj_sign * dual_value <= 0 && v_sides[ j ] == eLHS )
   continue;

  for( auto i : subset )
   g.coeffRef( i ) += dual_value * v_A[ j ][ i ];
 }
}  // end( BendersBFunction::get_linearization_coefficients( sv, subset ) )

/*--------------------------------------------------------------------------*/

Function::FunctionValue
BendersBFunction::compute_linearization_constant_from_bound() {

 // Since a diagonal linearization has been required, the sub-Block must be
 // feasible and, therefore, we can compute the linearization constant as
 // f(x) - g'x, where x are the active Variables of this BendersBFunction
 // and g are the coefficients of the linearization.

 std::vector< FunctionValue > g( v_x.size() );
 get_linearization_coefficients( g.data() );

 auto solver = get_solver< CDASolver >();

 if( ! solver )
  throw( std::logic_error( "BendersBFunction::compute_linearization_constant_"
                           "from_bound: The sub-Block (if present) has no "
                           "Solver attached to it." ) );

 FunctionValue linearization_constant = 0;
 if( v_Block.front()->get_objective_sense() == Objective::eMin )
  linearization_constant = solver->get_lb();
 else
  linearization_constant = solver->get_ub();

 for( Index i = 0 ; i < v_x.size() ; ++i )
  linearization_constant -= g[ i ] * v_x[ i ]->get_value();

 return( linearization_constant );
}

/*--------------------------------------------------------------------------*/

Function::FunctionValue BendersBFunction::compute_linearization_constant() {

 Function::FunctionValue alpha = 0;

 // The given dual value is associated with either the lower bound or the
 // upper bound constraint. This will help determine to which bound the given
 // dual is associated with.
 const auto obj_sign =
  ( v_Block.front()->get_objective_sense() == Objective::eMin ) ? - 1 : 1;

 auto update_alpha =
  [ &alpha , this , obj_sign ]( auto & c ) {

   if( ignore_constraint( & c ) )
    return;

   const auto dual_value = c.get_dual();

   if( dual_value == 0 )
    return;

   if( c.get_lhs() == c.get_rhs() ) { // Equality constraint

    auto b = c.get_lhs();

    Index index = Inf< Index >();
    if( obj_sign * dual_value > 0 )
     index = get_constraint_index( &c , eLHS );
    if( obj_sign * dual_value < 0 )
     index = get_constraint_index( &c , eRHS );
    if( index == Inf< Index >() )
     index = get_constraint_index( &c );

    if( index < Inf< Index >() ) {
     // This is a RowConstraint that is handled by this BendersBFunction
     b = v_b[ index ];
    }

    alpha += - dual_value * b;
    return;
   }
   else if( c.get_lhs() > -Inf< RowConstraint::RHSValue >() &&
            c.get_rhs() <  Inf< RowConstraint::RHSValue >() ) {
    // Two constraints (lower and upper bound).

    RowConstraint::RHSValue b = 0;
    if( obj_sign * dual_value >= 0 ) { // lower bound constraint
     auto index = get_constraint_index( &c , eLHS );
     if( index < Inf< Index >() )
      // Constraint handled by the BendersBFunction
      b = v_b[ index ];
     else
      b = c.get_lhs();
    }
    else { // upper bound constraint
     auto index = get_constraint_index( &c , eRHS );
     if( index < Inf< Index >() )
      // Constraint handled by the BendersBFunction
      b = v_b[ index ];
     else
      b = c.get_rhs();
    }

    alpha += - dual_value * b;
    return;
   }
   else { // Single inequality constraint
    const auto side = ( c.get_rhs() < Inf< RowConstraint::RHSValue >() ) ?
     eRHS : eLHS;

    RowConstraint::RHSValue b;
    const auto index = get_constraint_index( &c , side );
    if( index < Inf< Index >() )
     // Constraint handled by the BendersBFunction
     b = v_b[ index ];
    else
     b = ( side == eRHS ) ? c.get_rhs() : c.get_lhs();

    alpha += - dual_value * b;
    return;
   }
  };

 std::queue< Block * > Q;
 Q.push( v_Block.front() );
 while( ! Q.empty() ) {
  auto block = Q.front();
  Q.pop();
  for( auto * sub_block : block->get_nested_Blocks() )
   Q.push( sub_block );

  for( const auto & i : block->get_static_constraints() )
   un_any_const_static( i , update_alpha , un_any_type< FRowConstraint >() )
    || un_any_const_static( i , update_alpha , un_any_type< BoxConstraint >() )
    || un_any_const_static( i , update_alpha , un_any_type< ZOConstraint >() )
    || un_any_const_static( i , update_alpha , un_any_type< LB0Constraint >() )
    || un_any_const_static( i , update_alpha , un_any_type< UB0Constraint >() )
    || un_any_const_static( i , update_alpha , un_any_type< LBConstraint >() )
    || un_any_const_static( i , update_alpha , un_any_type< UBConstraint >() )
    || un_any_const_static( i , update_alpha , un_any_type< NNConstraint >() )
    || un_any_const_static( i , update_alpha , un_any_type< NPConstraint >() );

  for( const auto & i : block->get_dynamic_constraints() )
   un_any_const_dynamic( i , update_alpha , un_any_type< FRowConstraint >() )
    || un_any_const_dynamic( i , update_alpha , un_any_type< BoxConstraint >() )
    || un_any_const_dynamic( i , update_alpha , un_any_type< ZOConstraint >() )
    || un_any_const_dynamic( i , update_alpha , un_any_type< LB0Constraint >() )
    || un_any_const_dynamic( i , update_alpha , un_any_type< UB0Constraint >() )
    || un_any_const_dynamic( i , update_alpha , un_any_type< LBConstraint >() )
    || un_any_const_dynamic( i , update_alpha , un_any_type< UBConstraint >() )
    || un_any_const_dynamic( i , update_alpha , un_any_type< NNConstraint >() )
    || un_any_const_dynamic( i , update_alpha , un_any_type< NPConstraint >() );
 }

 return( alpha );
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::write_dual_solution_from_global_pool( Index name ) {

 // Take the dual solution from the global pool and write it into the sub-Block.

 auto solution = global_pool.get_solution( name );

 if( name >= global_pool.size() || solution == nullptr )
  throw( std::invalid_argument( "BendersBFunction::write_dual_solution_from_"
                                "global_pool: linearization with name " +
                                std::to_string( name ) + " was not found." ) );

 // TODO check whether the solution has already been written into the Block

 solution->write( v_Block.front() );
}  // end( BendersBFunction::write_dual_solution_from_global_pool )

/*--------------------------------------------------------------------------*/

Function::FunctionValue BendersBFunction::get_linearization_constant(
                                                                  Index name ) {

 if( name == Inf< Index >() ) {
  // Linearization just computed and not in the global pool yet.

  auto solver = get_solver< CDASolver >();

  if( ! solver )
   throw( std::logic_error( "BendersBFunction::get_linearization_constant: "
                            "The sub-Block (if present) has no Solver "
                            "attached to it." ) );

  // TODO check whether the dual solution has already been written into the
  // Block so that it is not written again.

  // TODO Maybe store the linearization computed below in case this
  // linearization is later inserted into the global pool

  if( f_diagonal_linearization_required ) {
   if( is_linearization_constant_computed_from_bound() ) {
    // Compute the linearization constant from the linearization coefficients
    return( compute_linearization_constant_from_bound() );
   }
   else {
    // Compute the linearization constant from the dual solution
    solver->get_dual_solution( f_get_dual_solution_config );
    return( compute_linearization_constant() );
   }
  }
  else {
   // "vertical" linearization
   solver->get_dual_direction( f_get_dual_direction_config );
   return( compute_linearization_constant() );
  }
 }
 else {
  auto constant = global_pool.get_linearization_constant( name );
  if( constant == Inf< FunctionValue >() ) {
   // the linearization constant must be recomputed
   write_dual_solution_from_global_pool( name );
   constant = compute_linearization_constant();
   global_pool.set_linearization_constant( constant , name );
  }
  return( constant );
 }
}  // end( BendersBFunction::get_linearization_constant )

/*--------------------------------------------------------------------------*/

ComputeConfig * BendersBFunction::get_ComputeConfig
( bool all , ComputeConfig * ocfg ) const {

 bool default_config = false;

 auto ccfg = ThinComputeInterface::get_ComputeConfig( all , ocfg );

 if( ! ccfg ) {
  default_config = true;
  ccfg = new ComputeConfig();
  ccfg->f_diff = ! all;
 }

 auto extra_config = dynamic_cast< SimpleConfiguration<
  std::map< std::string , Configuration * > > * >
  ( ccfg->f_extra_Configuration );

 if( ! extra_config ) {
  // The given extra Configuration pointer is either nullptr or does not have
  // the right type. Replace the extra Configuration with an appropriate one.
  delete ccfg->f_extra_Configuration;
  extra_config =
   new SimpleConfiguration< std::map< std::string , Configuration * > >;
  ccfg->f_extra_Configuration = extra_config;
 }

 auto inner_block = get_inner_block();

 // Retrieve the BlockConfig of the inner Block

 BlockConfig * bc = nullptr;

 auto bc_it = extra_config->f_value.find( "BlockConfig" );
 if( bc_it != extra_config->f_value.end() ) {
  // The map has a "BlockConfig" key. Try to cast the corresponding value.
  if( ( bc = dynamic_cast< BlockConfig * >( bc_it->second ) ) ) {
   bc->get( inner_block ); // use the given BlockConfig
  }
  else {
   // Whatever value is there, it is not a pointer to a BlockConfig.
   // Delete it.
   delete bc_it->second;
   bc_it->second = nullptr;
   if( inner_block ) {
    // If this BendersBFunction has an inner Block, extract a BlockConfig
    // from it.
    bc = OCRBlockConfig::get_right_BlockConfig( inner_block );
    bc_it->second = bc;
   }
  }
 }
 else {
  // The map has no "BlockConfig" key.
  if( inner_block ) {
   // If this BendersBFunction has an inner Block, extract a BlockConfig
   // from it and add it to the map.
   if( ( bc = OCRBlockConfig::get_right_BlockConfig( inner_block ) ) )
    extra_config->f_value[ "BlockConfig" ] = bc;
  }
 }

 // Retrieve the BlockSolverConfig of the inner Block

 BlockSolverConfig * bsc = nullptr;

 auto bsc_it = extra_config->f_value.find( "BlockSolverConfig" );
 if( bsc_it != extra_config->f_value.end() ) {
  // The map has a "BlockSolverConfig" key. Try to cast the corresponding
  // value.
  if( ( bsc = dynamic_cast< BlockSolverConfig * >( bsc_it->second ) ) ) {
   bsc->get( inner_block ); // use the given BlockSolverConfig
  }
  else {
   // Whatever value is there, it is not a pointer to a BlockSolverConfig.
   // Delete it.
   delete bsc_it->second;
   bsc_it->second = nullptr;
   if( inner_block ) {
    // If this BendersBFunction has an inner Block, extract a
    // BlockSolverConfig from it.
    bsc = RBlockSolverConfig::get_right_BlockSolverConfig( inner_block , ! all );
    bsc_it->second = bsc;
   }
  }
 }
 else {
  // The map has no "BlockSolverConfig" key.
  if( inner_block ) {
   // If this BendersBFunction has an inner Block, extract a BlockSolverConfig
   // from it and add it to the map.
   bsc = RBlockSolverConfig::get_right_BlockSolverConfig( inner_block , ! all );
   if( bsc )
    extra_config->f_value[ "BlockSolverConfig" ] = bsc;
  }
 }

 // Configuration for dual solution/direction

 if( f_get_dual_solution_config )
  extra_config->f_value[ "get_dual_solution" ] =
   f_get_dual_solution_config->clone();

 if( f_get_dual_direction_config )
  extra_config->f_value[ "get_dual_direction" ] =
   f_get_dual_direction_config->clone();

 if( f_get_dual_solution_partial_config )
  extra_config->f_value[ "get_dual_solution_partial" ] =
   f_get_dual_solution_partial_config->clone();

 if( f_get_dual_direction_partial_config )
  extra_config->f_value[ "get_dual_direction_partial" ] =
   f_get_dual_direction_partial_config->clone();

 // If this is the default Configuration, return nullptr

 if( default_config && extra_config->f_value.empty() ) {
  delete ccfg;
  ccfg = nullptr;
 }

 return( ccfg );
}  // end( BendersBFunction::get_ComputeConfig )

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::update_constraints() {

 const auto initial_f_ignore_modifications = f_ignore_modifications;

 f_ignore_modifications = true;

 for( Index i = 0 ; i < v_A.size() ; ++i ) {
  auto value = std::inner_product( v_x.begin() , v_x.end() , v_A[ i ].begin() ,
                                   v_b[ i ] , std::plus<>() ,
                                   []( ColVariable * var , FunctionValue val ) {
                                    return( var->get_value() * val );
                                   } );
  if( v_sides[ i ] == eLHS )
   get_constraint( i )->set_lhs( value );
  else if( v_sides[ i ] == eRHS )
   get_constraint( i )->set_rhs( value );
  else
   get_constraint( i )->set_both( value );
 }

 f_ignore_modifications = initial_f_ignore_modifications;
 f_constraints_are_updated = true;
}  // end( BendersBFunction::update_constraints )

/*--------------------------------------------------------------------------*/

BendersBFunction::function_value_behaviour
BendersBFunction::get_behaviour( Objective::of_type sense ,
                                 bool added_or_enforced_constraint ) const {

 if( sense == Objective::eMin ) { // minimization problem
  if( added_or_enforced_constraint ) // function value increases
   return( function_value_behaviour::increase );
  else // constraint was removed or relaxed: function value decreases
   return( function_value_behaviour::decrease );
 }
 else { // maximization problem
  if( added_or_enforced_constraint ) // function value increases
   return( function_value_behaviour::decrease );
  else // constraint was removed or relaxed: function value increases
   return( function_value_behaviour::increase );
 }
}  // end( BendersBFunction::get_behaviour )

/*--------------------------------------------------------------------------*/

BendersBFunction::function_value_behaviour
BendersBFunction::get_behaviour( std::shared_ptr< BlockModAD > mod ) {

 auto behaviour = function_value_behaviour::unchanged;

 retrieve_constraints();

 std::vector< Constraint * > affected_constraints;
 mod->get_elements( affected_constraints );

 for( auto affected_constraint : affected_constraints ) {
  for( const auto constraint : v_constraints ) {

   // Check whether the removed Constraint is still present in this
   // BendersBFunction.
   if( constraint == affected_constraint )
    throw( std::logic_error( "BendersBFunction::add_Modification(): "
                             "Some Constraint of the sub-Block was removed "
                             "from its own Block without firstly being "
                             "removed from this BendersBFunction." ) );

   if( behaviour == function_value_behaviour::unknown )
    continue; // We already know that the behaviour is unknown. Now, we only
              // check consistency, i.e., if this BendersBFunction has some of
              // the removed Constraints.

   auto new_behaviour = get_behaviour
    ( Objective::of_type
      ( affected_constraint->get_Block()->get_objective_sense() ) ,
      mod->is_added() );

   if( behaviour != function_value_behaviour::unchanged &&
       behaviour != new_behaviour )
    behaviour = function_value_behaviour::unknown;
   else
    behaviour = new_behaviour;
  }
 }
 return( behaviour );
}  // end( BendersBFunction::get_behaviour )

/*--------------------------------------------------------------------------*/

BendersBFunction::function_value_behaviour
BendersBFunction::get_behaviour( std::shared_ptr< ConstraintMod > mod ) {

 if( mod->type() != ConstraintMod::eRelaxConst &&
     mod->type() != ConstraintMod::eEnforceConst )
  return( function_value_behaviour::unknown );

 retrieve_constraints();

 auto modified_constraint = mod->constraint();

 for( const auto constraint : v_constraints ) {
  /* If the affected Constraint is present in this BendersBFunction, the
   * behaviour is unpredictable. */
  if( modified_constraint == constraint )
   return( function_value_behaviour::unknown );
 }

 return( get_behaviour
  ( Objective::of_type
     ( modified_constraint->get_Block()->get_objective_sense() ) ,
    ( mod->type() == ConstraintMod::eEnforceConst ) ) );
}  // end( BendersBFunction::get_behaviour )

/*--------------------------------------------------------------------------*/

void BendersBFunction::send_nuclear_modification
( const Observer::ChnlName chnl ) {
 // "nuclear modification" for Function: everything changed
 global_pool.invalidate();
 f_constraints_are_updated = false;
 if( f_Observer )
  f_Observer->add_Modification( std::make_shared< FunctionMod >(
                                 this , FunctionMod::NaNshift ) ,
                                chnl );
}  // end( BendersBFunction::send_nuclear_modification )

/*--------------------------------------------------------------------------*/

bool BendersBFunction::has_constraint( Constraint * constraint ) {
 retrieve_constraints();
 if( std::find( std::begin( v_constraints ) , std::end( v_constraints ) ,
                constraint ) != std::end( v_constraints ) )
  return( true );
 return( false );
}  // end( BendersBFunction::has_constraint )

/*--------------------------------------------------------------------------*/

template< class T >
bool BendersBFunction::is_A_sparse( SparseMatrix< T > & matrix ) const {
 matrix.clear();
 if( v_A.empty() ) {
  return( true );
 }
 Index nnz = 0;
 auto max_nnz_for_sparsity = ( v_A.size() * v_A[ 0 ].size() ) / 4;
 matrix.reserve( max_nnz_for_sparsity );
 for( Index i = 0 ; i < v_A.size() ; ++i ) {
  for( Index j = 0 ; j < v_A[ i ].size() ; ++j ) {
   if( v_A[ i ][ j ] != T( 0 ) ) {
    ++nnz;
    matrix.insert( i , j , v_A[ i ][ j ] );
    if( nnz > max_nnz_for_sparsity ) {
     matrix.clear();
     return( false );
    }
   }
  }
 }
 return( true );
}  // end( BendersBFunction::is_A_sparse )

/*--------------------------------------------------------------------------*/

void BendersBFunction::retrieve_constraints() {
 if( v_paths_to_constraints.empty() )
  return; // no Constraint needs to be retrieved

 auto inner_block = get_inner_block();

 assert( inner_block );

 inner_block->generate_abstract_variables();
 inner_block->generate_abstract_constraints();

 v_constraints.resize( v_paths_to_constraints.size() );

 for( Index i = 0 ; i < v_sides.size() ; ++i ) {

  RowConstraint * constraint = nullptr;

  if( v_paths_to_constraints[ i ] )
   constraint = v_paths_to_constraints[ i ]->
    get_element< RowConstraint >( inner_block );

  if( ! constraint )
   throw( std::logic_error( "BendersBFunction::retrieve_constraints: "
                            "Constraint " + std::to_string( i ) +
                            " was not found." ) );

  v_constraints[ i ] = constraint;
 }

 v_paths_to_constraints.clear(); // paths are no longer needed
 f_constraints_are_updated = false;
}   // end( BendersBFunction::retrieve_constraints )

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_constraints( const ConstraintVector & nc ) {
 if( v_paths_to_constraints.empty() ) {
  // There is no path to Constraint. We can add the given pointers to
  // Constraint directly.
  v_constraints.insert( v_constraints.end() , nc.begin() , nc.end() );
 }
 else {
  // This BendersBFunction still holds paths to Constraint.
  v_paths_to_constraints.reserve( v_paths_to_constraints.size() + nc.size() );
  for( auto & constraint : nc )
   v_paths_to_constraints.push_back
    ( std::make_unique< AbstractPath >( constraint , get_inner_block() ) );
  v_constraints.resize( v_paths_to_constraints.size() , nullptr );
 }
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_constraint( RowConstraint * constraint ) {
 add_constraints( std::vector< RowConstraint * >{ constraint } );
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::remove_constraints( Range range ) {
 if( v_paths_to_constraints.empty() ) {
  // There is no path to Constraint. We can remove the given pointers to
  // Constraint directly.
  v_constraints.erase( v_constraints.begin() + range.first ,
                       v_constraints.begin() + range.second );
 }
 else {
  // This BendersBFunction still holds paths to Constraint.
  v_paths_to_constraints.erase
   ( v_paths_to_constraints.begin() + range.first ,
     v_paths_to_constraints.begin() + range.second );
  v_constraints.resize( v_paths_to_constraints.size() );
 }
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::remove_constraints( const Subset & rows ) {

 // Assumption: "rows" is ordered

 if( ( ! rows.empty() ) && rows.back() >
     std::max( v_paths_to_constraints.size() , v_constraints.size() ) )
  throw( std::invalid_argument( "BendersBFunction::remove_constraints: "
                                "invalid constraint index " +
                                std::to_string( rows.back() ) + "." ) );

 auto remove = [ &rows ]( auto & v ) {
                auto it = v.begin();
                Index curr_index = 0;
                for( Subset::size_type i = 0 ; i < rows.size() ; ++i ) {
                 if( i > 0 && rows[ i ] == rows[ i - 1 ] )
                  continue; // already removed
                 it = v.erase( std::next( it , rows[ i ] - curr_index ) );
                 curr_index = rows[ i ] + 1;
                }
               };

 if( v_paths_to_constraints.empty() ) {
  // There is no path to Constraint. We can remove the pointers to the given
  // set of Constraint directly.
  remove( v_constraints );
 }
 else {
  // This BendersBFunction still holds paths to Constraint.
  remove( v_paths_to_constraints );
  v_constraints.resize( v_paths_to_constraints.size() );
 }
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::remove_constraints() {
 v_constraints.clear();
 v_paths_to_constraints.clear();
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::remove_constraint( Block::Index index ) {
 remove_constraints( Range( index , index + 1 ) );
}

/*--------------------------------------------------------------------------*/
/*----------------------------- GlobalPool ---------------------------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::resize( Index size ) {
 if( size < this->size() ) {
  for( auto it = solutions.begin() + size ; it != solutions.end() ; ++it ) {
   delete *it;
   *it = nullptr;
  }
 }
 solutions.resize( size , nullptr );
 linearization_constants.resize( size , NaN );
 is_diagonal.resize( size );
}  // end( BendersBFunction::GlobalPool::resize )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::store( FunctionValue linearization_constant ,
                                          Solution * solution , Index name ,
                                          bool diagonal_linearization ) {
 if( name >= size() )
  throw( std::invalid_argument( "BendersBFunction::GlobalPool::store: "
                                "invalid linearization name." ) );
 delete solutions[ name ];
 solutions[ name ] = solution;
 linearization_constants[ name ] = linearization_constant;
 is_diagonal[ name ] = diagonal_linearization;
}  // end( BendersBFunction::GlobalPool::store )

/*--------------------------------------------------------------------------*/

bool BendersBFunction::GlobalPool::is_linearization_there( Index name ) const {
 if( name >= size() || std::isnan( linearization_constants[ name ] ) )
  return( false );
 return( true );
}  // end( BendersBFunction::GlobalPool::is_linearization_there )

/*--------------------------------------------------------------------------*/

bool BendersBFunction::GlobalPool::is_linearization_vertical( Index name )
 const {
 if( name >= size() || std::isnan( linearization_constants[ name ] ) )
  return( false );
 return( ! is_diagonal[ name ] );
}  // end( BendersBFunction::GlobalPool::is_linearization_vertical )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::store_combination_of_linearizations(
                                         c_LinearCombination & coefficients ,
			                 Index name , FunctionValue AAccMlt )
{
 if( name >= size() )
  throw( std::invalid_argument( "BendersBFunction::GlobalPool::store_combinati"
                                "on_of_linearizations: invalid global pool "
                                "name." ) );

 if( coefficients.empty() )
  throw( std::invalid_argument( "BendersBFunction::GlobalPool::store_combinati"
                                "on_of_linearizations: coefficients is "
                                "empty." ) );

 auto it = coefficients.begin();
 auto linearization_name = it->first;
 auto coeff = it->second;

 if( coeff < - AAccMlt )
  throw( std::invalid_argument( "BendersBFunction::GlobalPool::store_combinati"
                                "on_of_linearizations: invalid coefficient for"
                                " linearization with name " +
                                std::to_string( linearization_name ) +
                                ": " + std::to_string( coeff ) ) );

 auto first_solution = get_solution( linearization_name );
 if( ! first_solution )
  throw( std::invalid_argument( "BendersBFunction::GlobalPool::store_combinati"
                                "on_of_linearizations: linearization with "
                                "name " +
                                std::to_string( linearization_name ) +
                                ", given in the coefficients parameter, "
                                "does not exist." ) );

 auto solution = first_solution->scale( coeff );
 auto constant = coeff * linearization_constants[ linearization_name ];

 FunctionValue coeff_sum_diagonal = 0;
 if( is_diagonal[ linearization_name ] )
  coeff_sum_diagonal = coeff;

 bool diagonal_linearization = false;

 for( ++it ; it != coefficients.end() ; ++it ) {
  linearization_name = it->first;
  coeff = it->second;

  auto next_solution = get_solution( linearization_name );
  if( ! next_solution ) {
   delete solution;
   throw( std::invalid_argument( "BendersBFunction::store_combination_of_"
                                 "linearizations: linearization with name " +
                                 std::to_string( linearization_name ) +
                                 ", given in the coefficients parameter, "
                                 "does not exist." ) );
  }

  if( coeff < - AAccMlt ) {
   delete solution;
   throw( std::invalid_argument( "BendersBFunction::GlobalPool::store_combinati"
                                 "on_of_linearizations: invalid coefficient for"
                                 " linearization with name " +
                                 std::to_string( linearization_name ) +
                                 ": " + std::to_string( coeff ) ) );
  }

  solution->sum( next_solution , coeff );
  constant += coeff * linearization_constants[ linearization_name ];

  if( is_diagonal[ linearization_name ] ) {
   coeff_sum_diagonal += coeff;
   diagonal_linearization = true;
  }
 }

 if( diagonal_linearization &&
     std::abs( FunctionValue( 1 ) - coeff_sum_diagonal ) >
     AAccMlt * coefficients.size() ) {

  delete solution;
  throw( std::invalid_argument( "BendersBFunction::GlobalPool::store_combinati"
                                "on_of_linearizations: a non-convex "
                                "combination of diagonal linearizations has "
                                "been provided." ) );
 }

 this->store( constant , solution , name , diagonal_linearization );

}  // end( BendersBFunction::GlobalPool::store_combination_of_linearizations )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::delete_linearization( const Index name ) {
 if( name >= size() )
  throw( std::invalid_argument( "GlobalPool::delete_linearization: invalid "
                                "linearization name: " +
                                std::to_string( name ) ) );

 linearization_constants[ name ] = NaN;
 delete solutions[ name ];
 solutions[ name ] = nullptr;
}  // end( BendersBFunction::GlobalPool::delete_linearization )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::delete_linearizations( Subset & which ,
                                                          bool ordered )
{
 if( which.empty() ) {  // delete them all
  for( Index i = 0 ; i < size() ; ++i )
   if( is_linearization_there( i ) )
    delete_linearization( i );
  }
 else {                 // delete the given subset
  if( ! ordered )
   std::sort( which.begin() , which.end() );

  if( which.back() >= size() )
   throw( std::invalid_argument( "BendersBFunction::GlobalPool::delete_linea"
                                 "rizations: invalid linearization name." ) );

  for( auto i : which )
   if( is_linearization_there( i ) )
    delete_linearization( i );
  }
 }

/*--------------------------------------------------------------------------*/

BendersBFunction::GlobalPool::~GlobalPool() {
 for( auto solution : solutions )
  delete solution;
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::deserialize
( const netCDF::NcGroup & group ) {

 auto gs = group.getDim( "BendersBFunction_MaxGlob" );
 const auto global_pool_size = gs.isNull() ? 0 : gs.getSize();

 for( auto & solution : solutions )
  delete solution;

 solutions.assign( global_pool_size , nullptr );
 linearization_constants.assign( global_pool_size , NaN );
 is_diagonal.assign( global_pool_size , true );

 if( global_pool_size ) {

  ::deserialize( group , "BendersBFunction_Constants" , { global_pool_size } ,
                 linearization_constants , false , false );

  auto nct = group.getVar( "BendersBFunction_Type" );
  if( nct.isNull() )
   throw( std::logic_error( "BendersBFunction::GlobalPool::deserialize: "
                            "BendersBFunction_Type was not found." ) );

  for( Index i = 0 ; i < global_pool_size ; ++i ) {
   int type;
   nct.getVar( { i } , &type );
   is_diagonal[ i ] = ( type != 0 );

   auto gi = group.getGroup( "BendersBFunction_Sol_" + std::to_string( i ) );
   if( ! gi.isNull() )
    solutions[ i ] = Solution::new_Solution( gi );
  }
 }

 auto nic = group.getDim( "BendersBFunction_ImpCoeffNum" );
 if( ( ! nic.isNull() ) && ( nic.getSize() ) ) {
  important_linearization_lin_comb.resize( nic.getSize() );

  auto ncCI = group.getVar( "BendersBFunction_ImpCoeffInd" );
  if( ncCI.isNull() )
   throw( std::logic_error( "BendersBFunction::GlobalPool::deserialize: "
                            "BendersBFunction_ImpCoeffInd was not found." ) );

  auto ncCV = group.getVar( "BendersBFunction_ImpCoeffVal" );
  if( ncCV.isNull() )
   throw( std::logic_error( "BendersBFunction::GlobalPool::deserialize: "
                            "BendersBFunction_ImpCoeffVal was not found." ) );

  for( Index i = 0 ; i < important_linearization_lin_comb.size() ; ++i ) {
   ncCI.getVar( { i } , &( important_linearization_lin_comb[ i ].first ) );
   ncCV.getVar( { i } , &( important_linearization_lin_comb[ i ].second ) );
  }
 }
 else
  important_linearization_lin_comb.clear();

}  // end( BendersBFunction::GlobalPool::deserialize )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::serialize( netCDF::NcGroup & group ) const {

 const auto global_pool_size = size();

 if( global_pool_size ) {
  auto size_dim = group.addDim( "BendersBFunction_MaxGlob" , global_pool_size );

  std::vector< int > type( global_pool_size );
  for( Index i = 0 ; i < global_pool_size ; ++i )
   type[ i ] = is_diagonal[ i ] ? 1 : 0;

  group.addVar( "BendersBFunction_Type" , netCDF::NcByte() , size_dim )
   .putVar( { 0 } , { global_pool_size } , type.data() );

  group.addVar( "BendersBFunction_Constants" , netCDF::NcDouble() , size_dim )
   .putVar( { 0 } , { global_pool_size } , linearization_constants.data() );

  for( Index i = 0 ; i < global_pool_size ; ++i ) {
   if( ! solutions[ i ] )
    continue;

   auto gi = group.addGroup( "BendersBFunction_Sol_" + std::to_string( i ) );
   solutions[ i ]->serialize( gi );
  }
 }

 if( ! important_linearization_lin_comb.empty() ) {
  auto linearization_dim = group.addDim
   ( "BendersBFunction_ImpCoeffNum" , important_linearization_lin_comb.size() );

  auto linearization_coeff_index = group.addVar
   ( "BendersBFunction_ImpCoeffInd" , netCDF::NcInt() , linearization_dim );

  auto linearization_coeff_value = group.addVar
   ( "BendersBFunction_ImpCoeffVal" , netCDF::NcDouble() , linearization_dim );

  for( Index i = 0 ; i < important_linearization_lin_comb.size() ; ++i ) {
   linearization_coeff_index.putVar
    ( { i } , important_linearization_lin_comb[ i ].first );
   linearization_coeff_value.putVar
    ( { i } , important_linearization_lin_comb[ i ].second );
  }
 }
}  // end( BendersBFunction::GlobalPool::serialize )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::clone( const GlobalPool & global_pool ) {

 for( auto & solution : solutions ) {
  delete solution;
  solution = nullptr;
 }

 if( this->size() < global_pool.size() ) {
  // resize this GlobalPool to accomodate the given elements
  this->resize( global_pool.size() );
 }

 std::copy( global_pool.is_diagonal.cbegin() ,
            global_pool.is_diagonal.cend() ,
            is_diagonal.begin() );

 std::copy( global_pool.linearization_constants.cbegin() ,
            global_pool.linearization_constants.cend() ,
            linearization_constants.begin() );

 important_linearization_lin_comb =
  global_pool.important_linearization_lin_comb;

 auto this_solution = solutions.begin();
 for( const auto & given_solution : global_pool.solutions ) {
  if( given_solution )
   *this_solution = given_solution->clone();
  else
   *this_solution = nullptr;
  ++this_solution;
 }
}  // end( BendersBFunction::GlobalPool::clone )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::clone( GlobalPool && global_pool ) {

 for( auto & solution : solutions ) {
  delete solution;
  solution = nullptr;
 }

 // The size of the GlobalPool will be at least the size it currently has.
 const auto size = std::max( this->size() , global_pool.size() );

 is_diagonal = std::move( global_pool.is_diagonal );

 linearization_constants = std::move( global_pool.linearization_constants );

 important_linearization_lin_comb =
  std::move( global_pool.important_linearization_lin_comb );

 auto this_solution = solutions.begin();
 for( auto & given_solution : global_pool.solutions ) {
  *this_solution++ = given_solution;
  given_solution = nullptr;
 }

 // Possibly resize this GlobalPool so that it has at least the same size it
 // had before.

 this->resize( size );

}  // end( BendersBFunction::GlobalPool::clone )

/*--------------------------------------------------------------------------*/
/*------------------------- BendersBFunctionState --------------------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunctionState::deserialize( const netCDF::NcGroup & group ) {
 global_pool.deserialize( group );
}

/*--------------------------------------------------------------------------*/

void BendersBFunctionState::serialize( netCDF::NcGroup & group ) const {
 State::serialize( group );
 global_pool.serialize( group );
}

/*--------------------------------------------------------------------------*/

BendersBFunctionState::BendersBFunctionState( const BendersBFunction * f ) {
 if( ! f )
  return;

 global_pool.clone( f->global_pool );
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File BendersBFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
