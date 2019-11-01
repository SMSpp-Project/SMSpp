/*--------------------------------------------------------------------------*/
/*---------------------- File BendersBFunction.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the BendersBFunction class.
 *
 * \version 0.10
 *
 * \date 01 - 11 - 2019
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

#include "BendersBFunction.h"
#include "Objective.h"
#include "Observer.h"
#include "RowConstraint.h"
#include "SMSTypedefs.h"
#include "Solution.h"
#include <math.h>

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

 c_Index nvar = get_num_active_var(); // TODO should we deserialize the set of
                                      // active Variable?

 auto nv = group.getDim( "BendersBFunction_NumVar" );
 if( nv.isNull() )
  throw( std::logic_error( "BendersBFunction::deserialize: BendersBFunction_"
                           "NumVar dimension is required." ) );
 if( nv.getSize() != nvar )
  throw( std::invalid_argument( "BendersBFunction::deserialize: matrix A has a "
                                "wrong number of columns in the given "
                                "netCDF::NcGroup." ) );

 MultiVector tA;
 RealVector tb;

 netCDF::NcDim nr = group.getDim( "BendersBFunction_NumRow" );
 if( ( ! nr.isNull() ) && ( nr.getSize() ) ) {
   netCDF::NcVar ncdA = group.getVar( "BendersBFunction_A" );
   if( ncdA.isNull() )
    throw( std::logic_error( "BendersBFunction::deserialize: "
                             "BendersBFunction_A not found" ) );

   netCDF::NcVar ncdb = group.getVar( "BendersBFunction_b" );
   if( ncdb.isNull() )
    throw( std::logic_error( "BendersBFunction::deserialize: "
                             "BendersBFunction_b not found" ) );

  tA.resize( nr.getSize() );
  for( Index i = 0 ; i < tA.size() ; ++i ) {
   tA[ i ].resize( nvar );
   ncdA.getVar( { i , 0 } , { 1 , nvar } , tA[ i ].data() );
   }

  tb.resize( nr.getSize() );
  ncdb.getVar( tb.data() );
  }

 // TODO deserialize the vector of ConstraintSpecifier

 // set_mapping( std::move( tA ) , std::move( tb ) ,
 //              std::move( constraints ) , issueMod );

 }  // end( BendersBFunction::deserialize )

/*--------------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::set_variables( VarVector && x )
{
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
/*---- METHODS FOR HANDLING "ACTIVE" Variable IN THE BendersBFunction ------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::map_active( c_Vec_p_Var & vars , Subset & map ,
                                   const bool ordered ) const
{
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
                  std::vector< ConstraintSpecifier > && constraints ,
                  c_ModParam issueMod )
{

 if( ! A.empty() )
  if( v_x.size() != v_A[ 0 ].size() )
   throw( std::invalid_argument( "BendersBFunction::set_mapping: A and x must "
                                 "have the same number of columns." ) );

 if( constraints.size() != b.size() )
  throw( std::invalid_argument( "BendersBFunction::set_mapping: the number of "
                                "affected constraints must be equal to the "
                                "size of b." ) );

 if( A.size() != b.size() )
  throw( std::invalid_argument( "BendersBFunction::set_mapping: A has " +
                                std::to_string( A.size() ) + " rows and b " +
                                "has " + std::to_string( b.size() ) + ", but "
                                "they must have the same number of rows." ) );
 if( ! A.empty() ) {
  const auto m = A[ 0 ].size();
  for( const auto & a : A )
   if( a.size() != m )
    throw( std::invalid_argument( "BendersBFunction::set_mapping: all rows of "
                                  "A must have the same size." ) );
  }

 v_A = std::move( A );
 v_b = std::move( b );
 v_constraints = std::move( constraints );

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
                                      c_ModParam issueMod )
{
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
                                     c_RealVector & Aj , c_ModParam issueMod )
{
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

void BendersBFunction::remove_variable( c_Index i , c_ModParam issueMod )
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

 // erase the columns in v_A
 for( auto & ai : v_A )
  ai.erase( ai.begin() + range.first , ai.begin() + range.second );

 constraints_are_updated = false;

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

void BendersBFunction::remove_variables( Subset & nms ,
                                         const bool ordered ,
                                         c_ModParam issueMod )
{
 if( nms.empty() )  // actually nothing to remove
  return;           // cowardly (and silently) return

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
                                    Range range , c_ModParam issueMod )
{
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
                             C05FunctionMod::NaNshift ,
                             Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

 }  // end( BendersBFunction::modify_rows( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_rows( MultiVector && nA , c_RealVector & nb ,
                                    Subset && rows , bool ordered ,
                                    c_ModParam issueMod )
{
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
                             std::move( rows ) , true ,
                             C05FunctionMod::NaNshift ,
                             Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

 }  // end( BendersBFunction::modify_rows( subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_row( c_Index i , RealVector && Ai ,
                                   c_FunctionValue bi ,
                                   c_ModParam issueMod )
{
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
                             Range( i , i + 1 ) , C05FunctionMod::NaNshift ,
                             Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

 }  // end( BendersBFunction::modify_row )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants( c_RealVector & nb , Range range ,
                                         c_ModParam issueMod )
{
 range.second = std::min( range.second , Index( v_x.size() ) );
 if( range.second <= range.first )
  return;

 if( nb.size() != range.second - range.first )
  throw( std::invalid_argument( "BendersBFunction::modify_constants: range "
                                "and nb sizes do not match." ) );

 bool changed = false;
 for( Index i = 0 ; i < nb.size() ; ++i ) {
  if( v_b[ range.first + i ] != nb[ i ] ) {
   v_b[ range.first + i ] = nb[ i ];
   changed = true;
  }
 }

 if( ! changed )
  return;

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
                                      range , C05FunctionMod::NaNshift ,
                                      Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

 }  // end( BendersBFunction::modify_constants( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constants( c_RealVector & nb ,
                                         Subset && rows , bool ordered ,
                                         c_ModParam issueMod )
{
 if( rows.empty() )  // actually nothing to modify
  return;            // cowardly (and silently) return

 if( nb.size() != rows.size() )
  throw( std::invalid_argument( "BendersBFunction::modify_constants: rows and "
                                "nb sizes do not match." ) );

 for( const auto i : rows )
  if( i >= v_A.size() )
   throw( std::invalid_argument( "BendersBFunction::modify_constants: row " +
                                 std::to_string( i ) + " does not exist." ) );

 bool changed = false;
 for( Index i = 0 ; i < rows.size() ; ++i ) {
  if( v_b[ rows[ i ] ] != nb[ i ] ) {
   v_b[ rows[ i ] ] = nb[ i ];
   changed = true;
  }
 }

 if( ! changed )
  return;

 constraints_are_updated = false;

 // Dual solutions are still feasible. The g part of the linearizations are
 // still valid and only the linearization constants must be recomputed.

 global_pool.reset_linearization_constants();

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModSbst: note that ordered is unmodified
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModSbst>(
                                      this , C05FunctionMod::AlphaChanged ,
                                      BendersBFunctionMod::ModifyCnst ,
                                      std::move( rows ) , ordered ,
                                      C05FunctionMod::NaNshift ,
                                      Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

 }  // end( BendersBFunction::modify_constants )

/*--------------------------------------------------------------------------*/

void BendersBFunction::modify_constant( c_Index i , c_FunctionValue bi ,
                                        c_ModParam issueMod )
{
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

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModRngd>(
                                      this , C05FunctionMod::AlphaChanged ,
                                      BendersBFunctionMod::ModifyCnst ,
                                      Range( i , i + 1 ) ,
                                      C05FunctionMod::NaNshift ,
                                      Observer::par2concern( issueMod ) ) ,
                                Observer::par2chnl( issueMod ) );

 }  // end( BendersBFunction::modify_constant )

/*--------------------------------------------------------------------------*/

void BendersBFunction::add_rows( MultiVector && nA , c_RealVector & nb ,
                                 c_ModParam issueMod )
{
 const auto k = nA.size();
 if( k != nb.size() )
  throw( std::invalid_argument( "BendersBFunction::add_rows: nA and nb must "
                                "have the same size." ) );

 const auto n = v_x.size();
 for( const auto & a : nA )
  if( a.size() != n )
   throw( std::invalid_argument( "BendersBFunction::add_rows: some row of nA "
                                 "has a wrong size." ) );

 v_A.insert( v_A.end() , std::make_move_iterator( nA.begin() ) ,
                         std::make_move_iterator( nA.end() ) );

 v_b.insert( v_b.end() , nb.begin(), nb.end() );

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
                                c_ModParam issueMod )
{
 if( Ai.size() != v_x.size() )
  throw( std::invalid_argument( "BendersBFunction::add_row: given row Ai "
                                "has wrong size." ) );

 v_A.push_back( std::move( Ai ) );
 v_b.push_back( bi );

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

void BendersBFunction::delete_rows( Range range , c_ModParam issueMod )
{
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

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModRngd>(
                                this , mod_type ,
                                BendersBFunctionMod::DeleteRows , range ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( BendersBFunction::delete_rows( range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_rows( Subset && rows , bool ordered ,
                                    c_ModParam issueMod )
{
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

 if( mod_type == C05FunctionMod::AllLinearizationChanged )
  global_pool.reset_linearization_constants();

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModSbst; rows is ordered
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModSbst>(
                                this , mod_type ,
                                BendersBFunctionMod::DeleteRows ,
                                std::move( rows ) , true ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( BendersBFunction::delete_rows( subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_row( c_Index i , c_ModParam issueMod )
{
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

 v_A.erase( v_A.begin() + i );      // kill i in v_A[]
 v_b.erase( v_b.begin() + i );      // kill i in v_b[]

 constraints_are_updated = false;

 if( ( ! f_Observer ) || ( ! f_Observer->issue_mod( issueMod ) ) )
  return;                  // noone is there: all done

 // issue the BendersBFunctionModRngd
 f_Observer->add_Modification( std::make_shared<BendersBFunctionModRngd>(
                                this , mod_type ,
                                BendersBFunctionMod::DeleteRows ,
                                Range( i , i + 1 ) ,
                                C05FunctionMod::NaNshift ,
                                Observer::par2concern( issueMod ) ) ,
                               Observer::par2chnl( issueMod ) );

 }  // end( BendersBFunction::delete_row )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_rows( c_ModParam issueMod )
{
 v_A.clear();   // delete original rows
 v_b.clear();

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
                                         Observer::ChnlName chnl ) {
 // TODO
 throw( std::logic_error( "BendersBFunction::load(): not implemented yet." ) );
}

/*--------------------------------------------------------------------------*/
/*------------ METHODS FOR Saving THE DATA OF THE BendersBFunction ---------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::serialize( netCDF::NcGroup & group ) const {


}

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE BendersBFunction --------*/
/*--------------------------------------------------------------------------*/

void BendersBFunction::update_constraints() {
 for( Index i = 0 ; i < v_A.size() ; ++i ) {
  auto value = std::inner_product( v_x.begin() , v_x.end() , v_A[ i ].begin() ,
                                   v_b[ i ] , std::plus<>() ,
                                   []( ColVariable * var , FunctionValue val ) {
                                    return var->get_value() * val;
                                   } );
  if( v_constraints[ i ].second == eLHS )
   v_constraints[ i ].first->set_lhs( value );
  else if( v_constraints[ i ].second == eRHS )
   v_constraints[ i ].first->set_rhs( value );
  else
   v_constraints[ i ].first->set_both( value );
 }

 constraints_are_updated = true;
 }  // end( BendersBFunction::update_constraints )

/*--------------------------------------------------------------------------*/

int BendersBFunction::compute( bool changedvars )
{
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

bool BendersBFunction::compute_new_linearization( const bool diagonal )
{

 auto solver = get_solver<CDASolver>();

 if( ! solver )
  return false;

 if( diagonal )
  return solver->new_dual_solution();
 else
  return solver->new_dual_direction();
 }  // end ( BendersBFunction::compute_new_linearization )

/*--------------------------------------------------------------------------*/

Function::FunctionValue BendersBFunction::get_value( void ) const
{

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

void BendersBFunction::store_linearization( Index name )
{
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

  global_pool.store( Inf<FunctionValue>() , solution , name );
} // end BendersBFunction::store_linearization( Index )

/*--------------------------------------------------------------------------*/

void BendersBFunction::store_combination_of_linearizations(
                        LinearCombination & coefficients , const Index name )
{
  global_pool.store_combination_of_linearizations( coefficients , name );
 }  // end( BendersBFunction::store_combination_of_linearizations )

/*--------------------------------------------------------------------------*/

void BendersBFunction::rename_linearization( const Index current_name ,
                                             const Index new_name )
{
 global_pool.rename_linearization( current_name , new_name );
 }  // end( BendersBFunction::rename_linearization )

/*--------------------------------------------------------------------------*/

void BendersBFunction::delete_linearization( const Index name )
{
 global_pool.delete_linearization( name );
 }  // end( BendersBFunction::delete_linearization )

/*--------------------------------------------------------------------------*/

void BendersBFunction::write_dual_solution( Index name )
{

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
                                                       Index name )
{

 range.second = std::min( range.second , Index( v_constraints.size() ) );
 if( range.second <= range.first )
  return;

 write_dual_solution( name );

 for( Index i = range.first ; i < range.second ; ++i )
  g[ i ] = 0;

 for( Index j = 0; j < v_constraints.size(); ++j ) {
  const auto constraint = v_constraints[ j ].first;
  const auto side = v_constraints[ j ].second;
  const auto dual_value = get_dual_value( constraint , side );
  for( Index i = range.first ; i < range.second ; ++i )
   g[ i - range.first ] += dual_value * v_A[ j ][ i ];
 }

}  // end( BendersBFunction::get_linearization_coefficients( * , range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients( SparseVector & g ,
                                                       Range range ,
                                                       Index name )
{
 range.second = std::min( range.second , Index( v_x.size() ) );
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
  const auto constraint = v_constraints[ j ].first;
  const auto side = v_constraints[ j ].second;
  const auto dual_value = get_dual_value( constraint , side );
  for( Index i = range.first ; i < range.second ; ++i )
   g.coeffRef( i ) += dual_value * v_A[ j ][ i ];
 }
}  // end( BendersBFunction::get_linearization_coefficients( sv , range ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients( FunctionValue * g ,
                                                       c_Subset & subset ,
                                                       const bool ordered ,
                                                       Index name )
{
 write_dual_solution( name );

 for( auto i : subset ) {
  if( i >= v_x.size() )
   throw( std::invalid_argument( "BendersBFunction::get_linearization_"
                                 "coefficients: invalid index: " +
                                 std::to_string( i ) + "." ) );
  g[ i ] = 0;
 }

 for( Index j = 0; j < v_constraints.size(); ++j ) {
  const auto constraint = v_constraints[ j ].first;
  const auto side = v_constraints[ j ].second;
  const auto dual_value = get_dual_value( constraint , side );
  for( auto i : subset )
   g[ i ] += dual_value * v_A[ j ][ i ];
 }
}  // end( BendersBFunction::get_linearization_coefficients( * , subset ) )

/*--------------------------------------------------------------------------*/

void BendersBFunction::get_linearization_coefficients( SparseVector & g ,
                                                       c_Subset & subset ,
                                                       const bool ordered ,
                                                       Index name )
{

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
  const auto constraint = v_constraints[ j ].first;
  const auto side = v_constraints[ j ].second;
  const auto dual_value = get_dual_value( constraint , side );
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

Function::FunctionValue BendersBFunction::compute_linearization_constant()
{

 FunctionValue constant = 0.0;

 std::set< Constraint * > equality_constraints;

 for( Index j = 0; j < v_constraints.size(); ++j ) {

  const auto constraint = v_constraints[ j ].first;
  const auto side = v_constraints[ j ].second;
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

 return constant;
}  // end( BendersBFunction::compute_linearization_constant )

/*--------------------------------------------------------------------------*/

void BendersBFunction::write_dual_solution_from_global_pool( Index name )
{

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
                                                                  Index name )
{

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
}  // end( BendersBFunction::GlobalPool::resize )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::store( FunctionValue linearization_constant ,
                                          Solution * solution , Index name ) {
 if( name >= size() )
  throw( std::invalid_argument( "BenderdBFunction::GlobalPool::store: "
                                "invalid linearization name." ) );
 delete solutions[ name ];
 solutions[ name ] = solution;
 linearization_constants[ name ] = linearization_constant;
}  // end( BendersBFunction::GlobalPool::store )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::store_combination_of_linearizations(
                        LinearCombination & coefficients , const Index name )
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

 auto first_solution = get_solution( it->first );
 if( ! first_solution )
  throw( std::invalid_argument( "BendersBFunction::GlobalPool::store_combinati"
                                "on_of_linearizations: linearization with "
                                "name " +
                                std::to_string( coefficients[ 0 ].first ) +
                                ", given in the coefficients parameter, "
                                "does not exist." ) );

 auto solution = first_solution->scale( it->second );
 auto constant = it->second * linearization_constants[ it->first ];

 for( ++it ; it != coefficients.end() ; ++it ) {
  auto next_solution = get_solution( it->first );
  if( ! next_solution )
   throw( std::invalid_argument( "BendersBFunction::store_combination_of_"
                                 "linearizations: linearization with name " +
                                 std::to_string( it->first ) +
                                 ", given in the coefficients parameter, "
                                 "does not exist." ) );

  solution->sum( next_solution , it->second );
  constant += it->second * linearization_constants[ it->first ];
  }

 this->store( constant, solution , name );

 }  // end( BendersBFunction::GlobalPool::store_combination_of_linearizations )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::rename_linearization
( const Index current_name , const Index new_name ) {

 if( current_name == new_name )  // actually doing nothing
  return;                        // cowardly (and silently) return

 if( current_name >= size() )
  throw( std::invalid_argument( "GlobalPool::rename_linearization: invalid "
                                "linearization current_name: " +
                                std::to_string( current_name ) ) );

 if( new_name >= size() )
  throw( std::invalid_argument( "GlobalPool::rename_linearization: invalid "
                                "linearization new_name: " +
                                std::to_string( new_name ) ) );

 delete_linearization( new_name );
 solutions[ new_name ] = solutions[ current_name ];
 linearization_constants[ new_name ] = linearization_constants[ current_name ];
 linearization_constants[ current_name ] = NaN;

 }  // end( BendersBFunction::GlobalPool::rename_linearization )

/*--------------------------------------------------------------------------*/

void BendersBFunction::GlobalPool::delete_linearization( const Index name )
{
 if( name >= size() )
  throw( std::invalid_argument( "GlobalPool::delete_linearization: invalid "
                                "linearization name: " +
                                std::to_string( name ) ) );

 linearization_constants[ name ] = NaN;
 delete solutions[ name ];
 solutions[ name ] = nullptr;
 }  // end( BendersBFunction::GlobalPool::delete_linearization )

/*--------------------------------------------------------------------------*/

BendersBFunction::GlobalPool::~GlobalPool() {
for( auto solution : solutions )
 delete solution;
}

/*--------------------------------------------------------------------------*/
/*-------------------- End File BendersBFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
