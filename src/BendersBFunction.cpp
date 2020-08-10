/*--------------------------------------------------------------------------*/
/*---------------------- File BendersBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BendersBFunction class.
 *
 * \version 0.10
 *
 * \date 02 - 08 - 2020
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
 * Copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato.
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "AbstractPath.h"
#include "BendersBFunction.h"
#include "FRowConstraint.h"
#include "Objective.h"
#include "Observer.h"
#include "OneVarConstraint.h"
#include "RowConstraint.h"
#include "SMSTypedefs.h"
#include "Solution.h"
#include <cmath>

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register BendersBFunction to the Block factory

SMSpp_insert_in_factory_cpp_1( BendersBFunction );

/*--------------------------------------------------------------------------*/
/*---------------------------------TODO-------------------------------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::load( std::istream &input ) {
 throw( std::logic_error( "BendersBFunction::load(): not implemented yet." ) );
 }

/*--------------------------------------------------------------------------*/
/*------------- CONSTRUCTING AND DESTRUCTING BendersBFunction --------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::deserialize( netCDF::NcGroup & group ,
                                    c_ModParam issueMod ) {

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
  throw std::logic_error( "BendersBFunction::deserialize: the '" +
                          BLOCK_NAME + "' group must be present." );

 auto inner_block = new_Block( inner_block_group , this );
 if( ! inner_block )
  throw std::logic_error( "BendersBFunction::deserialize: the '" +
                          BLOCK_NAME + "' group is present "
                          "but its description is incomplete." );

 set_inner_block( inner_block );

 inner_block->generate_abstract_variables();
 inner_block->generate_abstract_constraints();

 std::vector< ConstraintSide > sides;
 if( ! ::deserialize( group , "ConstraintSide" , tb.size() , sides , true ) ) {
  sides.resize( tb.size() );
  sides.assign( sides.size() , eBoth );
 }

 if( tA.size() != sides.size() )
  throw ( std::invalid_argument( "BendersBFunction::deserialize: The size of "
                                 "the 'ConstraintSide' vector  must be equal "
                                 "to the number of rows of the A matrix." ) );

 auto path_group = group.getGroup( "AbstractPath" );

 std::vector< AbstractPath > paths;

 if( ! path_group.isNull() )
  paths = AbstractPath::vector_deserialize( path_group );
 else if( sides.size() > 0 )
  throw ( std::invalid_argument( "BendersBFunction::deserialize: The group "
                                 "'AbstractPath' was not found." ) );

 if( paths.size() != sides.size() )
  throw ( std::invalid_argument( "BendersBFunction::deserialize: The number of "
                                 "AbstractPath to Constraint must be equal to "
                                 "the size of the 'ConstraintSide' vector." ) );

 std::vector< RowConstraint * > constraints;
 constraints.reserve( paths.size() );

 for( Index i = 0 ; i < sides.size() ; ++i ) {

  auto constraint = AbstractPath::get_element< RowConstraint >
   ( paths[ i ] , get_inner_block() );

  if( ! constraint )
   throw ( std::invalid_argument( "BendersBFunction::deserialize: Constraint " +
                                  std::to_string( i ) + " was not found." ) );

  constraints.push_back( constraint );
 }

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

 constraints_are_updated = false;
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

   if( f_Observer && value < old_size ) {
    // The size of the global pool is being reduced. We store in "which" the
    // indices of the deleted linearizations.
    Subset which( global_pool.size() - value );
    std::iota( which.begin() , which.end() , value );
    f_Observer->add_Modification
     ( std::make_shared<BendersBFunctionMod>
       ( this , C05FunctionMod::GlobalPoolRemoved , std::move( which ) , 0 ) );
   }

   break;
  }
  default: C05Function::set_par( par , value );
 }
}  // end( BendersBFunction::set_par )

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
                                    c_ModParam issueMod ) {

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

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // "nuclear modification" for Function: everything changed
 f_Observer->add_Modification( std::make_shared<FunctionMod>( this ,
                                         FunctionMod::NaNshift ,
                                         Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::set_mapping )

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_variables( VarVector && nx , MultiVector && nA ,
                                      c_ModParam issueMod ) {
 const auto nn = nx.size();
 if( ! nn )  // actually nothing to add
  return;    // cowardly (and silently) return

 if( ! nA.empty() && ! v_A.empty() && nA.size() != v_A.size() )
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

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;  // noone is listening: all done

 Vec_p_Var vars( nn );
 std::copy( v_x.begin() + n , v_x.end() , vars.begin() );

 // now issue the C05FunctionModVarsAddd
 f_Observer->add_Modification( std::make_shared<C05FunctionModVarsAddd>(
                                         this , std::move( vars ) , n , 0 ,
                                         Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::add_variables )

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_variable( ColVariable * const var ,
                                     c_RealVector & Aj , c_ModParam issueMod ) {
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

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // now issue the Modification
 // a Benders function is strongly quasi-additive
 f_Observer->add_Modification( std::make_shared<C05FunctionModVarsAddd>(
                                         this , Vec_p_Var( { var } ) ,
                                         v_x.size() - 1 , 0 ,
                                         Observer::par2concern( issueMod ) ) ,
                                 Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::add_variable )

/*--------------------------------------------------------------------------*/

void BendersBFunction::remove_variable( Index i , c_ModParam issueMod )
{
 if( i >= v_x.size() )
  throw( std::logic_error( "BendersBFunction::remove_variable: invalid "
                           "Variable index " + std::to_string( i ) + "." ) );

 auto var = v_x[ i ];
 v_x.erase( v_x.begin() + i );    // erase it in v_x
 for( auto & ai : v_A )           // erase the column in A
  ai.erase( ai.begin() + i );

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 // now issue the Modification
 // a Benders function is strongly quasi-additive
 f_Observer->add_Modification( std::make_shared<C05FunctionModVarsRngd>(
                                    this , Vec_p_Var( { var } ) ,
                                    Range( i , i + 1 ) , 0 ,
                                    Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::remove_variable( index ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::remove_variables( Range range , c_ModParam issueMod )
{
 range.second = std::min( range.second , Index( v_x.size() ) );
 if( range.second <= range.first )
  return;

 constraints_are_updated = false;

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
   f_Observer->add_Modification( std::make_shared<C05FunctionModVarsRngd>(
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
  f_Observer->add_Modification( std::make_shared<C05FunctionModVarsRngd>(
                                    this , std::move( vars ) , range , 0 ,
                                    Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
 }
 else  // noone is there: just do it
  v_x.erase( strtit , stopit );

}  // end( BendersBFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

template< class T >
static void compact( std::vector< T > x ,
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
                                         c_ModParam issueMod )
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

  constraints_are_updated = false;

  // now issue the Modification: note that the subset is empty
  // a BendersBFunction is strongly quasi-additive, and nms is ordered
  if( f_Observer && f_Observer->issue_mod( issueMod ) )
   f_Observer->add_Modification( std::make_shared<C05FunctionModVarsSbst>(
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

 constraints_are_updated = false;

 if( f_Observer && f_Observer->issue_mod( issueMod ) ) {
  Vec_p_Var vars( nms.size() );
  auto its = vars.begin();
  for( auto nm : nms )
   *(its++) = v_x[ nm ];

  compact( v_x , nms );

  // now issue the Modification
  // a Benders function is strongly quasi-additive, and nms is ordered
  f_Observer->add_Modification( std::make_shared<C05FunctionModVarsSbst>(
                             this , std::move( vars ) , std::move( nms ) ,
                             true , 0 , Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );
 }
 else  // noone is there: just do it
  compact( v_x , nms );

}  // end( BendersBFunction::remove_variables( subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_rows( MultiVector && nA , c_RealVector & nb ,
                                    Range range , c_ModParam issueMod ) {
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

 constraints_are_updated = false;

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModRngd>(
                             this , C05FunctionMod::AllLinearizationChanged ,
                             BendersBFunctionMod::ModifyRows , range ,
                             Subset( {} ) , C05FunctionMod::NaNshift ,
                             Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::modify_rows( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_rows( MultiVector && nA , c_RealVector & nb ,
                                    Subset && rows , bool ordered ,
                                    c_ModParam issueMod ) {
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

 constraints_are_updated = false;

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModSbst; note that rows is ordered
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModSbst>(
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
                                   c_ModParam issueMod ) {
 if( i >= v_A.size() )
  throw( std::invalid_argument( "BendersBFunction::modify_row: row " +
                                std::to_string( i ) + " does not exist." ) );

 if( Ai.size() != v_x.size() )
  throw( std::invalid_argument( "BendersBFunction::modify_row: given row has "
                                "size different from that of x." ) );

 // actually change things
 v_A[ i ] = std::move( Ai );
 v_b[ i ] = bi;

 constraints_are_updated = false;

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModRngd>(
                             this , C05FunctionMod::AllLinearizationChanged ,
                             BendersBFunctionMod::ModifyRows ,
                             Range( i , i + 1 ) , Subset( {} ) ,
                             C05FunctionMod::NaNshift ,
                             Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::modify_row )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants
( std::vector< double >::const_iterator nb , Range range ,
  c_ModParam issuePMod , c_ModParam issueAMod ) {

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

 constraints_are_updated = false;

 // Dual solutions are still feasible. The g part of the linearizations are
 // still valid and only the linearization constants must be recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueAMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModRngd>(
                                      this , C05FunctionMod::AlphaChanged ,
                                      BendersBFunctionMod::ModifyCnst ,
                                      range , Subset( {} ) ,
                                      C05FunctionMod::NaNshift ,
                                      Observer::par2concern( issueAMod ) ) ,
                                Observer::par2chnl( issueAMod ) );

}  // end( BendersBFunction::modify_constants( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants( c_RealVector & nb , Range range ,
                                         c_ModParam issueMod ) {
 modify_constants( nb.cbegin() , range , issueMod , issueMod );
}  // end( BendersBFunction::modify_constants( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants
( std::vector< double >::const_iterator nb , Subset && rows , bool ordered ,
  c_ModParam issuePMod , c_ModParam issueAMod ) {

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

 constraints_are_updated = false;

 // Dual solutions are still feasible. The g part of the linearizations are
 // still valid and only the linearization constants must be recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueAMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModSbst: note that ordered is unmodified
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModSbst>(
                                      this , C05FunctionMod::AlphaChanged ,
                                      BendersBFunctionMod::ModifyCnst ,
                                      std::move( rows ) , ordered ,
                                      Subset( {} ) ,
                                      C05FunctionMod::NaNshift ,
                                      Observer::par2concern( issueAMod ) ) ,
                                Observer::par2chnl( issueAMod ) );

}  // end( BendersBFunction::modify_constants )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants
( c_RealVector & nb , Subset && rows , bool ordered , c_ModParam issueMod ) {
 modify_constants( nb.cbegin() , std::move( rows ) , ordered ,
                   issueMod , issueMod );
}  // end( BendersBFunction::modify_constants )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constant( c_Index i , c_FunctionValue bi ,
                                        c_ModParam issueMod ) {
 if( i >= v_A.size() )
  throw( std::invalid_argument( "BendersBFunction::modify_constant: row " +
                                std::to_string( i ) + " does not exist." ) );

 if( bi == v_b[ i ] )  // actually nothing is changing
  return;              // cowardly (and silently) return

 // actually change the constant
 v_b[ i ] = bi;

 constraints_are_updated = false;

 // Dual solutions are still feasible. The g part of the linearizations are
 // still valid and only the linearization constants must be recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModRngd>(
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
                                 c_ModParam issueMod ) {
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

 v_constraints.insert( v_constraints.end() , nc.begin(), nc.end() );

 v_sides.insert( v_sides.end() , ns.begin(), ns.end() );

 constraints_are_updated = false;

 // Dual solutions are still feasible. Linearizations need only to be
 // recomputed.

 auto mod_type = C05FunctionMod::AllLinearizationChanged;
 if( std::all_of( nb.cbegin(), nb.cend(),
                  []( FunctionValue i ) {
                   return i == FunctionValue( 0 ); } ) ) {
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

 f_Observer->add_Modification( std::make_shared<BendersBFunctionModAddd>(
                                  this , mod_type , k ,
                                  C05FunctionMod::NaNshift ,
                                  Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::add_rows )

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_row( RealVector && Ai , FunctionValue bi ,
                                RowConstraint * ci , ConstraintSide si ,
                                c_ModParam issueMod ) {
 if( Ai.size() != v_x.size() )
  throw( std::invalid_argument( "BendersBFunction::add_row: given row Ai "
                                "has wrong size." ) );

 if( ci == nullptr )
  throw( std::invalid_argument( "BendersBFunction::add_row: the pointer to "
                                "the RowConstraint must be non-null." ) );

 v_A.push_back( std::move( Ai ) );
 v_b.push_back( bi );
 v_constraints.push_back( ci );
 v_sides.push_back( si );

 constraints_are_updated = false;

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
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModAddd>(
                                  this , mod_type , 1 ,
                                  C05FunctionMod::NaNshift ,
                                  Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::add_row )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_rows( Range range , c_ModParam issueMod ) {
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
                   return i == FunctionValue( 0 ); } ) ) {
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

 v_constraints.erase( v_constraints.begin() + range.first ,
                      v_constraints.begin() + range.second );

 v_sides.erase( v_sides.begin() + range.first ,
                v_sides.begin() + range.second );

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModRngd>(
                                this , mod_type ,
                                BendersBFunctionMod::DeleteRows , range ,
                                Subset( {} ) , C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::delete_rows( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_rows( Subset && rows , bool ordered ,
                                    c_ModParam issueMod ) {
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
  v_sides[ idx ] = std::numeric_limits< ConstraintSide >::quiet_NaN();
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
 v_constraints.erase
  ( std::remove_if( v_constraints.begin() + rows.front() , v_constraints.end() ,
                    []( RowConstraint * ci ) {
                     return( ci == nullptr ); } ) ,
    v_constraints.end() );

 // kill stuff in v_sides[]
 v_sides.erase
  ( std::remove_if( v_sides.begin() + rows.front() , v_sides.end() ,
                    []( ConstraintSide si ) {
                     return( std::isnan< typename std::underlying_type
                             < ConstraintSide >::type >( si ) ); } ) ,
    v_sides.end() );

 if( mod_type == C05FunctionMod::AllLinearizationChanged )
  global_pool.reset_linearization_constants();

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModSbst; rows is ordered
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModSbst>(
                                this , mod_type ,
                                BendersBFunctionMod::DeleteRows ,
                                std::move( rows ) , true , Subset( {} ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::delete_rows( subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_row( c_Index i , c_ModParam issueMod ) {
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
 v_constraints.erase( v_constraints.begin() + i ); // kill i in v_constraints[]
 v_sides.erase( v_sides.begin() + i );             // kill i in v_sides[]

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModRngd>(
                                this , mod_type ,
                                BendersBFunctionMod::DeleteRows ,
                                Range( i , i + 1 ) , Subset( {} ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::delete_row )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_rows( c_ModParam issueMod ) {
 v_A.clear();   // delete original rows
 v_b.clear();
 v_constraints.clear();
 v_sides.clear();

 global_pool.invalidate();
 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // "nuclear modification" for Function: everything changed
 f_Observer->add_Modification( std::make_shared<FunctionMod>( this ,
                                         FunctionMod::NaNshift ,
                                         Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::delete_rows( all ) )

/*--------------------------------------------------------------------------*/
/*-------------------- Methods for handling Modification -------------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::add_Modification( sp_Mod mod ,
                                         Observer::ChnlName chnl )
{
 // GroupModification - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( const auto tmod = std::dynamic_pointer_cast<GroupModification>( mod ) ) {
  for( const auto & submod : tmod->sub_Modifications() )
   this->add_Modification( submod , chnl );
  return;
  }

 // FunctionMod - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( const auto tmod = std::dynamic_pointer_cast<FunctionMod>( mod ) ) {

  auto observer = tmod->function()->get_Observer();

  if( const auto constraint = dynamic_cast<Constraint *>( observer ) ) {
   if( this->has_constraint( constraint ) )
    send_nuclear_modification( chnl );
   else {
    /* Dual solutions are still feasible and linearizations are still
     * valid. However, the value of this BendersBFunction changes
     * unpredictably. */
    if( f_Observer )
     f_Observer->add_Modification
      ( std::make_shared<FunctionMod>( this , FunctionMod::NaNshift ) , chnl );
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

 if( const auto tmod = std::dynamic_pointer_cast<FunctionModVars>( mod ) ) {

  auto observer = tmod->function()->get_Observer();

  if( const auto constraint = dynamic_cast<Constraint *>( observer ) ) {
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
      f_Observer->add_Modification
       ( std::make_shared<FunctionMod>( this , FunctionMod::NaNshift ) , chnl );
     }
    }
   else {
    // The Constraint is not being handled by this BendersBFunction.
    /* Dual solutions are still feasible and linearizations are still
     * valid. However, the value of this BendersBFunction may change
     * unpredictably. */
    if( f_Observer )
     f_Observer->add_Modification
      ( std::make_shared<FunctionMod>( this , FunctionMod::NaNshift ) , chnl );
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

 if( const auto tmod = std::dynamic_pointer_cast<ConstraintMod>( mod ) ) {

  if( tmod->type() == ConstraintMod::eRelaxConst ||
      tmod->type() == ConstraintMod::eEnforceConst ) {

   auto behaviour = get_behaviour( tmod );
   if( behaviour == function_value_behaviour::unknown )
    send_nuclear_modification( chnl );
   else
    if( behaviour == function_value_behaviour::increase && f_Observer )
     f_Observer->add_Modification(
	       std::make_shared<FunctionMod>( this , Inf<FunctionValue>() ) ,
	       chnl );
    else
     if( behaviour == function_value_behaviour::decrease && f_Observer )
      f_Observer->add_Modification(
	     std::make_shared<FunctionMod>( this , - Inf<FunctionValue>() ) ,
	     chnl );
   return;
   }

  // actually a RowConstraintMod

  if( const auto tmod = std::dynamic_pointer_cast<RowConstraintMod>( mod ) ) {
   switch( tmod->type() ) {
    case( RowConstraintMod::eChgRHS ):
    case( RowConstraintMod::eChgLHS ):
    case( RowConstraintMod::eChgBTS ): {
     if( this->has_constraint( tmod->constraint() ) ) {
      /* Dual solution is still feasible and linearizations are still
       * valid. But since this Constraint is being handled by this
       * BendersBFunction, it must be updated. */
      constraints_are_updated = false;
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

  if( const auto tmod = std::dynamic_pointer_cast<FRowConstraintMod>( mod ) ) {
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
      f_Observer->add_Modification(
		std::make_shared<FunctionMod>( this , FunctionMod::NaNshift ) ,
		chnl );
    }
   else  // unknown modification
    send_nuclear_modification( chnl );

   return;
   }

  // actually a OneVarConstraintMod

  if( const auto tmod =
                    std::dynamic_pointer_cast<OneVarConstraintMod>( mod ) ) {

   if( tmod->type() == OneVarConstraintMod::eVariableChanged ) {
    // Pointer to the Variable of a OneVarConstraint has changed.
    if( ! this->has_constraint( tmod->constraint() ) ) {
     /* Dual solutions are still feasible and linearizations are still
      * valid. However, the value of this BendersBFunction changes
      * unpredictably. */
     if( f_Observer )
      f_Observer->add_Modification(
	       std::make_shared<FunctionMod>( this , FunctionMod::NaNshift ) ,
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

 if( const auto tmod = std::dynamic_pointer_cast<BlockModAD>( mod ) ) {
  if( tmod->is_variable() ) {
   if( tmod->is_added() )
    // Variables were added. This BendersBFunction may change unpredictably.
    send_nuclear_modification( chnl );
   else
    /* Variables were removed. Dual solutions are still feasible. But the
     * value of this BendersBFunction may change unpredictably. */
    if( f_Observer )
     f_Observer->add_Modification(
	       std::make_shared<FunctionMod>( this , FunctionMod::NaNshift ) ,
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
    f_Observer->add_Modification(
		std::make_shared<FunctionMod>( this , Inf<FunctionValue>() ) ,
		chnl );
    else
     if( behaviour == function_value_behaviour::decrease && f_Observer )
      f_Observer->add_Modification(
	      std::make_shared<FunctionMod>( this , - Inf<FunctionValue>() ) ,
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

 std::vector< AbstractPath > paths;
 paths.reserve( v_constraints.size() );
 for( const auto constraint : v_constraints ) {
  paths.push_back( AbstractPath::build_path< Constraint >
                   ( constraint , get_inner_block() ) );
 }

 AbstractPath::serialize( paths , group );

 if( auto inner_block = get_inner_block() ) {
  auto inner_block_group = group.addGroup( BLOCK_NAME );
  inner_block->serialize( inner_block_group );
 }
}

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE BendersBFunction --------*/
/*--------------------------------------------------------------------------*/

int BendersBFunction::compute( bool changedvars ) {
 if( ( ! changedvars ) && constraints_are_updated )
  // TODO We need another flag telling whether the sub-Block has changed since
  // the last call.
  return( solver_status ); //  nothing changed since last call, nothing to do

 if( v_Block.size() != 1 )
  throw( std::logic_error( "BendersBFunction::compute: there must be exactly "
                           "one sub-Block, but there is (are) " +
                           std::to_string( v_Block.size() ) + "." ) );

 auto solver = get_solver();

 if( ! solver )
  throw( std::logic_error( "BendersBFunction::compute: It is not possible to "
                           "compute. The sub-Block has no Solver attached to "
                           "it." ) );

 if( changedvars || ! constraints_are_updated )
  update_constraints();

 // TODO can we assume that the variables of the sub-Block haven't changed?
 solver_status = solver->compute( true );

 return solver_status;

 }  // end( BendersBFunction::compute )

/*--------------------------------------------------------------------------*/

bool BendersBFunction::is_convex( void ) const {
 if( v_Block.empty() ) return false;
 return( v_Block[ 0 ]->get_objective_sense() == Objective::eMin );
}

/*--------------------------------------------------------------------------*/

bool BendersBFunction::is_concave( void ) const {
 if( v_Block.empty() )
  return false;
 return( v_Block[ 0 ]->get_objective_sense() == Objective::eMax );
}

/*--------------------------------------------------------------------------*/

bool BendersBFunction::has_linearization( const bool diagonal ) {

 auto solver = get_solver<CDASolver>();

 if( ! solver )
  return false;

 if( diagonal ) {
  diagonal_linearization_required = true;
  return solver->has_dual_solution();
 }
 else {
  diagonal_linearization_required = false;
  return solver->has_dual_direction();
 }

}  // end( BendersBFunction::has_linearization )

/*--------------------------------------------------------------------------*/

bool BendersBFunction::compute_new_linearization( const bool diagonal ) {
 auto solver = get_solver<CDASolver>();
 if( ! solver )
  return false;
 if( diagonal )
  return solver->new_dual_solution();
 else
  return solver->new_dual_direction();
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

 if( v_Block[ 0 ]->get_objective_sense() == Objective::eMin )
  return solver->get_ub();
 return solver->get_lb();

} // end ( BendersBFunction::get_value )

/*--------------------------------------------------------------------------*/

/* The last linearization that was computed by calling
 * compute_new_linearization() (which returned true) can be permanently stored
 * in the global pool of linearizations by calling this method. */

void BendersBFunction::store_linearization( Index name , c_ModParam issueMod ) {
 if( name >= global_pool.size() )
  throw( std::invalid_argument( "BendersBFunction::store_linearization: "
                                "invalid global pool name: " +
                                std::to_string( name ) ) );

 auto solver = get_solver<CDASolver>();

 if( ! solver )
  throw( std::logic_error( "BendersBFunction::store_linearization: It is not "
                           "possible to store the linearization. The sub-Block"
                           " (if present) has no Solver attached to it." ) );

 // TODO check whether the solution has already been written into the Block

 if( diagonal_linearization_required )
  solver->get_dual_solution();
 else
  solver->get_dual_direction();

 Solution * solution = nullptr;

 if( ! solution )
  solution = v_Block[ 0 ]->get_Solution();
 solution->read( v_Block[ 0 ] );

 // Lazy computation of the linearization constant

 global_pool.store( Inf<FunctionValue>() , solution , name ,
                    diagonal_linearization_required );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared<BendersBFunctionMod>( this ,
				      C05FunctionMod::GlobalPoolAdded ,
				      Subset( { name } ) , 0 ,
				      Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

} // end BendersBFunction::store_linearization( Index )

/*--------------------------------------------------------------------------*/

void BendersBFunction::store_combination_of_linearizations
( LinearCombination & coefficients , const Index name , c_ModParam issueMod ) {
 global_pool.store_combination_of_linearizations( coefficients , name ,
                                                  AAccMlt );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared<BendersBFunctionMod>( this ,
					C05FunctionMod::GlobalPoolAdded ,
					Subset( { name } ) , 0 ,
					Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );

}  // end( BendersBFunction::store_combination_of_linearizations )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_linearization( const Index name ,
                                             c_ModParam issueMod ) {
 global_pool.delete_linearization( name );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared<BendersBFunctionMod>( this ,
				      C05FunctionMod::GlobalPoolRemoved ,
				      Subset( { name } ) , 0 ,
				      Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );
}  // end( BendersBFunction::delete_linearization )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_linearizations( Subset && which , bool ordered ,
                                              c_ModParam issueMod ) {
 global_pool.delete_linearizations( which , ordered );

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;

 f_Observer->add_Modification( std::make_shared<BendersBFunctionMod>( this ,
				      C05FunctionMod::GlobalPoolRemoved ,
				      std::move( which ) , 0 ,
				      Observer::par2concern( issueMod ) ) ,
			       Observer::par2chnl( issueMod ) );
}

/*--------------------------------------------------------------------------*/

void BendersBFunction::write_dual_solution( Index name ) {

 auto solver = get_solver<CDASolver>();

 if( ! solver )
  throw( std::logic_error( "BendersBFunction::write_dual_solution: The sub-Blo"
                           "ck (if present) has no Solver attached to it." ) );

 if( name == Inf<Index>() ) {
  // Last computed linearization

  // TODO check whether the solution has already been written into the Block

  if( diagonal_linearization_required )
   solver->get_dual_solution();
  else
   solver->get_dual_direction();
 }
 else
  // Linearization stored in the global pool
  write_dual_solution_from_global_pool( name );
}  // end( BendersBFunction::write_dual_solution )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients( FunctionValue * g ,
                                                       Range range ,
                                                       Index name ) {

 range.second = std::min( range.second , Index( v_constraints.size() ) );
 if( range.second <= range.first )
  return;

 write_dual_solution( name );

 for( Index i = range.first ; i < range.second ; ++i )
  g[ i ] = 0;

 for( Index j = 0; j < v_constraints.size(); ++j ) {
  const auto dual_value = get_dual_value( v_constraints[ j ] , v_sides[ j ] );
  for( Index i = range.first ; i < range.second ; ++i )
   g[ i - range.first ] += dual_value * v_A[ j ][ i ];
 }
}  // end( BendersBFunction::get_linearization_coefficients( * , range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients( SparseVector & g ,
                                                       Range range ,
                                                       Index name ) {
 range.second = std::min( range.second , Index( v_constraints.size() ) );
 if( range.second <= range.first )
  return;

 write_dual_solution( name );

 if( g.nonZeros() == 0 ) {  // g contains no non-zero element
  g.resize( v_x.size() );
  g.reserve( range.second - range.first );
  for( Index i = range.first ; i < range.second ; ++i )
   g.insert( i ) = 0;
 }
 else {                     // g contains some non-zero elements
  if( static_cast<decltype( v_x.size() )>( g.size() ) != v_x.size() )
   throw( std::invalid_argument( "BendersBFunction::get_linearization_"
                                 "coefficients: invalid SparseVector size" ) );

  for( Index i = range.first ; i < range.second ; ++i )
   g.coeffRef( i ) = 0;
 }

 for( Index j = 0; j < v_constraints.size(); ++j ) {
  const auto dual_value = get_dual_value( v_constraints[ j ] , v_sides[ j ] );
  for( Index i = range.first ; i < range.second ; ++i )
   g.coeffRef( i ) += dual_value * v_A[ j ][ i ];
 }
}  // end( BendersBFunction::get_linearization_coefficients( sv , range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients( FunctionValue * g ,
                                                       c_Subset & subset ,
                                                       const bool ordered ,
                                                       Index name ) {
 write_dual_solution( name );

 for( auto i : subset ) {
  if( i >= v_x.size() )
   throw( std::invalid_argument( "BendersBFunction::get_linearization_"
                                 "coefficients: invalid index: " +
                                 std::to_string( i ) + "." ) );
  g[ i ] = 0;
 }

 for( Index j = 0; j < v_constraints.size(); ++j ) {
  const auto dual_value = get_dual_value( v_constraints[ j ] , v_sides[ j ] );
  for( auto i : subset )
   g[ i ] += dual_value * v_A[ j ][ i ];
 }
}  // end( BendersBFunction::get_linearization_coefficients( * , subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients( SparseVector & g ,
                                                       c_Subset & subset ,
                                                       const bool ordered ,
                                                       Index name ) {

 write_dual_solution( name );

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
  if( static_cast<decltype( v_x.size() )>( g.size() ) != v_x.size() )
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
  const auto dual_value = get_dual_value( v_constraints[ j ] , v_sides[ j ] );
  for( auto i : subset )
   g.coeffRef( i ) += dual_value * v_A[ j ][ i ];
 }
}  // end( BendersBFunction::get_linearization_coefficients( sv, subset ) )

/*--------------------------------------------------------------------------*/

Function::FunctionValue BendersBFunction::get_dual_value
( const RowConstraint * constraint , const ConstraintSide & side ) {

 const auto dual_value = constraint->get_dual();

 if( constraint->get_lhs() == constraint->get_rhs() )
  return dual_value;

 else if( constraint->get_lhs() > - Inf<RowConstraint::RHSValue>() &&
          constraint->get_rhs() <   Inf<RowConstraint::RHSValue>() ) {
  // Two constraints (lower and upper)

  RowConstraint::RHSValue sign =
   ( v_Block[ 0 ]->get_objective_sense() == Objective::eMin ) ? - 1 : 1;

  assert( side != eBoth );

  if( ( side == eLHS && sign * dual_value > 0 ) ||
      ( side == eRHS && sign * dual_value < 0 ) )
   return std::abs( dual_value );
  else
   return 0;
 }

 return std::abs( dual_value );
}  // end( BendersBFunction::get_dual_value )

/*--------------------------------------------------------------------------*/

Function::FunctionValue BendersBFunction::compute_linearization_constant() {

 FunctionValue constant = 0.0;

 std::set< Constraint * > equality_constraints;

 for( Index j = 0; j < v_constraints.size(); ++j ) {

  const auto constraint = v_constraints[ j ];
  const auto side = v_sides[ j ];
  const auto dual_value = get_dual_value( constraint , side );
  const auto b = v_b[ j ];

  if( constraint->get_lhs() == constraint->get_rhs() && ! ( side == eBoth ) ) {
   /* This is an equality Constraint, but it is stated as being an
    * inequality. In this case, there could be another element in the
    * v_constraints vector that is associated with the same Constraint but
    * with a different side. The term associated with this Constraint is,
    * therefore, added only once. */

   if( equality_constraints.find( constraint ) == equality_constraints.end() ) {
    equality_constraints.insert( constraint );
    constant += dual_value * b;
   }
  }
  else {
   constant += dual_value * b;
  }
 }

 // TODO add the term associated with the PolyhedralFunction

 return constant;
}  // end( BendersBFunction::compute_linearization_constant )

/*--------------------------------------------------------------------------*/

void BendersBFunction::write_dual_solution_from_global_pool( Index name ) {

 // Take the dual solution from the global pool and write it into the sub-Block.

 auto solution = global_pool.get_solution( name );

 if( name >= global_pool.size() || solution == nullptr )
  throw( std::invalid_argument( "BendersBFunction::write_dual_solution_from_"
                                "global_pool: linearization with name " +
                                std::to_string( name ) + " was not found." ) );

 // TODO check whether the solution has already been written into the Block

 solution->write( v_Block[ 0 ] );
}  // end( BendersBFunction::write_dual_solution_from_global_pool )

/*--------------------------------------------------------------------------*/

Function::FunctionValue BendersBFunction::get_linearization_constant(
                                                                  Index name ) {

 auto solver = get_solver<CDASolver>();

 if( ! solver )
  throw( std::logic_error( "BendersBFunction::get_linearization_constant: The "
                           "sub-Block (if present) has no Solver attached "
                           "to it." ) );

 if( name == Inf<Index>() ) {
  // Linearization just computed and not in the global pool yet.

  // TODO check whether the solution has already been written into the Block

  if( diagonal_linearization_required )
   solver->get_dual_solution();
  else
   solver->get_dual_direction();

  // TODO Maybe store it for (perhaps) inserting it later in the global pool
  return compute_linearization_constant();
 }
 else {
  auto constant = global_pool.get_linearization_constant( name );
  if( constant == Inf<FunctionValue>() ) {
   // the linearization constant must be recomputed
   write_dual_solution_from_global_pool( name );
   constant = compute_linearization_constant();
   global_pool.set_linearization_constant( constant , name );
  }
  return constant;
 }
}  // end( BendersBFunction::get_linearization_constant )

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::update_constraints() {
 for( Index i = 0 ; i < v_A.size() ; ++i ) {
  auto value = std::inner_product( v_x.begin() , v_x.end() , v_A[ i ].begin() ,
                                   v_b[ i ] , std::plus<>() ,
                                   []( ColVariable * var , FunctionValue val ) {
                                    return var->get_value() * val;
                                   } );
  if( v_sides[ i ] == eLHS )
   v_constraints[ i ]->set_lhs( value );
  else if( v_sides[ i ] == eRHS )
   v_constraints[ i ]->set_rhs( value );
  else
   v_constraints[ i ]->set_both( value );
 }

 constraints_are_updated = true;
}  // end( BendersBFunction::update_constraints )

/*--------------------------------------------------------------------------*/

BendersBFunction::function_value_behaviour
BendersBFunction::get_behaviour( Objective::of_type sense ,
                                 bool added_or_enforced_constraint ) const {

 if( sense == Objective::eMin ) { // minimization problem
  if( added_or_enforced_constraint ) // function value increases
   return function_value_behaviour::increase;
  else // constraint was removed or relaxed: function value decreases
   return function_value_behaviour::decrease;
 }
 else { // maximization problem
  if( added_or_enforced_constraint ) // function value increases
   return function_value_behaviour::decrease;
  else // constraint was removed or relaxed: function value increases
   return function_value_behaviour::increase;
 }
}  // end( BendersBFunction::get_behaviour )

/*--------------------------------------------------------------------------*/

BendersBFunction::function_value_behaviour
BendersBFunction::get_behaviour( std::shared_ptr<BlockModAD> mod ) const {

 auto behaviour = function_value_behaviour::unchanged;

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
 return behaviour;
}  // end( BendersBFunction::get_behaviour )

/*--------------------------------------------------------------------------*/

BendersBFunction::function_value_behaviour
BendersBFunction::get_behaviour( std::shared_ptr<ConstraintMod> mod ) const {

 if( mod->type() != ConstraintMod::eRelaxConst &&
     mod->type() != ConstraintMod::eEnforceConst )
  return function_value_behaviour::unknown;

 auto modified_constraint = mod->constraint();

 for( const auto constraint : v_constraints ) {
  /* If the affected Constraint is present in this BendersBFunction, the
   * behaviour is unpredictable. */
  if( modified_constraint == constraint )
   return function_value_behaviour::unknown;
 }

 return get_behaviour
  ( Objective::of_type
    ( modified_constraint->get_Block()->get_objective_sense() ) ,
    ( mod->type() == ConstraintMod::eEnforceConst ) );
}  // end( BendersBFunction::get_behaviour )

/*--------------------------------------------------------------------------*/

void BendersBFunction::send_nuclear_modification
( const Observer::ChnlName chnl ) {
 // "nuclear modification" for Function: everything changed
 global_pool.invalidate();
 constraints_are_updated = false;
 if( f_Observer )
  f_Observer->add_Modification
   ( std::make_shared<FunctionMod>( this , FunctionMod::NaNshift ) , chnl );
}  // end( BendersBFunction::send_nuclear_modification )

/*--------------------------------------------------------------------------*/

bool BendersBFunction::has_constraint( Constraint * constraint ) const {
 for( const auto affected_constraint : v_constraints ) {
  if( constraint == affected_constraint )
   return true;
 }
 return false;
}  // end( BendersBFunction::has_constraint )

/*--------------------------------------------------------------------------*/

template< class T >
bool BendersBFunction::is_A_sparse( SparseMatrix<T> & matrix ) const {
 matrix.clear();
 if( v_A.empty() ) {
  return true;
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
     return false;
    }
   }
  }
 }
 return true;
}  // end( BendersBFunction::is_A_sparse )

/*--------------------------------------------------------------------------*/
/*----------------------------- GLOBALPOOL ---------------------------------*/
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
  return false;
 return true;
}  // end( BendersBFunction::GlobalPool::is_linearization_there )

/*--------------------------------------------------------------------------*/

bool BendersBFunction::GlobalPool::is_linearization_vertical( Index name )
 const {
 if( name >= size() || std::isnan( linearization_constants[ name ] ) )
  return false;
 return( ! is_diagonal[ name ] );
}  // end( BendersBFunction::GlobalPool::is_linearization_vertical )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::store_combination_of_linearizations(
                        LinearCombination & coefficients , const Index name ,
                        const FunctionValue AAccMlt ) {
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
/*-------------------- End File BendersBFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
