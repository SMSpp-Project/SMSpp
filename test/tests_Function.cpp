/** @file
 * Unit tests for Function. Since it's an interface,
 * is it tested through its closest implementations.
 *
 * \author Niccolo' Iardella \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Niccolo' Iardella, Donato Meoli
 */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "LinearFunction.h"

#include <stdexcept>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

// TODO: ThinComputeInterface::set/get_ComputeConfig()
// TODO: C05Function linearization stuff
// TODO: C15Function hessian approximation stuff

void runAllTests()
{
 LinearFunction fun;

 // test GetsNumberOfParameters
 assert( fun.ThinComputeInterface::get_num_int_par() == 0 );
 assert( fun.ThinComputeInterface::get_num_dbl_par() == 0 );
 assert( fun.ThinComputeInterface::get_num_str_par() == 0 );
 assert( fun.Function::get_num_int_par() == Function::intLastParFun );
 assert( fun.Function::get_num_dbl_par() == Function::dblLastParFun );
 assert( fun.C05Function::get_num_int_par() == C05Function::intLastParC05F );
 assert( fun.C05Function::get_num_dbl_par() == C05Function::dblLastParC05F );

 // test GetsParameterDefaultValues
 assert( fun.Function::get_dflt_int_par( Function::intMaxIter ) ==
         Inf< int >() );
 assert( fun.Function::get_dflt_int_par( Function::intMaxThread ) == 0 );
 assert( fun.Function::get_dflt_dbl_par( Function::dblMaxTime ) ==
         Inf< double >() );
 assert( fun.Function::get_dflt_dbl_par( Function::dblRelAcc ) == 1e-6 );
 assert( fun.Function::get_dflt_dbl_par( Function::dblAbsAcc ) ==
         Inf< double >() );
 assert( fun.Function::get_dflt_dbl_par( Function::dblUpCutOff ) ==
         Inf< double >() );
 assert( fun.Function::get_dflt_dbl_par( Function::dblLwCutOff ) ==
         -Inf< double >() );
 assert( fun.C05Function::get_dflt_int_par( C05Function::intLPMaxSz ) == 1 );
 assert( fun.C05Function::get_dflt_int_par( C05Function::intGPMaxSz ) == 0 );
 assert( fun.C05Function::get_dflt_dbl_par( C05Function::dblRAccLin ) == 0 );
 assert( fun.C05Function::get_dflt_dbl_par( C05Function::dblAAccLin ) == 0 );
 assert( fun.C05Function::get_dflt_dbl_par( C05Function::dblAAccMlt ) == 1e-10 );

 // test ChecksParameterValues
 for( int i = 0 ; i < Function::intLastParFun ; ++i )
  assert( fun.Function::get_int_par( i ) ==
          fun.Function::get_dflt_int_par( i ) );
 for( int i = 0 ; i < Function::dblLastParFun ; ++i )
  assert( fun.Function::get_dbl_par( i ) ==
          fun.Function::get_dflt_dbl_par( i ) );
 for( int i = 0 ; i < C05Function::intLastParC05F ; ++i )
  assert( fun.C05Function::get_int_par( i ) ==
          fun.C05Function::get_dflt_int_par( i ) );
 for( int i = 0 ; i < C05Function::dblLastParC05F ; ++i )
  assert( fun.C05Function::get_dbl_par( i ) ==
          fun.C05Function::get_dflt_dbl_par( i ) );

 // test ChecksParameterIndices
 assert( fun.Function::int_par_str2idx( "intMaxIter" ) ==
         Function::intMaxIter );
 assert( fun.Function::int_par_str2idx( "intMaxThread" ) ==
         Function::intMaxThread );
 assert( fun.Function::dbl_par_str2idx( "dblMaxTime" ) ==
         Function::dblMaxTime );
 assert( fun.Function::dbl_par_str2idx( "dblRelAcc" ) ==
         Function::dblRelAcc );
 assert( fun.Function::dbl_par_str2idx( "dblAbsAcc" ) ==
         Function::dblAbsAcc );
 assert( fun.Function::dbl_par_str2idx( "dblUpCutOff" ) ==
         Function::dblUpCutOff );
 assert( fun.Function::dbl_par_str2idx( "dblLwCutOff" ) ==
         Function::dblLwCutOff );
 assert( fun.C05Function::int_par_str2idx( "intLPMaxSz" ) ==
         C05Function::intLPMaxSz );
 assert( fun.C05Function::int_par_str2idx( "intGPMaxSz" ) ==
         C05Function::intGPMaxSz );
 assert( fun.C05Function::dbl_par_str2idx( "dblRAccLin" ) ==
         C05Function::dblRAccLin );
 assert( fun.C05Function::dbl_par_str2idx( "dblAAccLin" ) ==
         C05Function::dblAAccLin );
 assert( fun.C05Function::dbl_par_str2idx( "dblAAccMlt" ) ==
         C05Function::dblAAccMlt );

 // test ChecksParameterNames
 assert( fun.Function::int_par_idx2str( Function::intMaxIter ) ==
         "intMaxIter" );
 assert( fun.Function::int_par_idx2str( Function::intMaxThread ) ==
         "intMaxThread" );
 assert( fun.Function::dbl_par_idx2str( Function::dblMaxTime ) ==
         "dblMaxTime" );
 assert( fun.Function::dbl_par_idx2str( Function::dblRelAcc ) ==
         "dblRelAcc" );
 assert( fun.Function::dbl_par_idx2str( Function::dblAbsAcc ) ==
         "dblAbsAcc" );
 assert( fun.Function::dbl_par_idx2str( Function::dblUpCutOff ) ==
         "dblUpCutOff" );
 assert( fun.Function::dbl_par_idx2str( Function::dblLwCutOff ) ==
         "dblLwCutOff" );
 assert( fun.C05Function::int_par_idx2str( C05Function::intLPMaxSz ) ==
         "intLPMaxSz" );
 assert( fun.C05Function::int_par_idx2str( C05Function::intGPMaxSz ) ==
         "intGPMaxSz" );
 assert( fun.C05Function::dbl_par_idx2str( C05Function::dblRAccLin ) ==
         "dblRAccLin" );
 assert( fun.C05Function::dbl_par_idx2str( C05Function::dblAAccLin ) ==
         "dblAAccLin" );
 assert( fun.C05Function::dbl_par_idx2str( C05Function::dblAAccMlt ) ==
         "dblAAccMlt" );
}

/*--------------------------------------------------------------------------*/
/* The tests above read every parameter by the name and the index it is known
 * to have, which says that the two tables agree with what is written here
 * and not that they agree with each other. What is checked now is the round
 * trip on every index there is, which is what breaks when a parameter is
 * added to one of the two tables alone, and what the two do with what is not
 * a parameter at all, which is the only way they can be asked something
 * wrong. */

static void test_parameter_tables( void )
{
 LinearFunction fun;

 // every index has a name, and that name is read back as the same index:
 // an entry added to one table alone shows up here
 for( auto i = 0 ; i < fun.C05Function::get_num_int_par() ; ++i )
  assert( fun.C05Function::int_par_str2idx(
	   fun.C05Function::int_par_idx2str( i ) ) == i );

 for( auto i = 0 ; i < fun.C05Function::get_num_dbl_par() ; ++i )
  assert( fun.C05Function::dbl_par_str2idx(
	   fun.C05Function::dbl_par_idx2str( i ) ) == i );

 for( auto i = 0 ; i < fun.Function::get_num_int_par() ; ++i )
  assert( fun.Function::int_par_str2idx(
	   fun.Function::int_par_idx2str( i ) ) == i );

 for( auto i = 0 ; i < fun.Function::get_num_dbl_par() ; ++i )
  assert( fun.Function::dbl_par_str2idx(
	   fun.Function::dbl_par_idx2str( i ) ) == i );

 // the parameters of the base class are reached through the derived one,
 // which is what a caller that knows a Function only as a C05Function does
 assert( fun.C05Function::int_par_str2idx( "intMaxIter" ) ==
	 Function::intMaxIter );
 assert( fun.C05Function::dbl_par_str2idx( "dblRelAcc" ) ==
	 Function::dblRelAcc );

 // a name that is no parameter is answered, not thrown at: the caller is
 // meant to compare the answer with Inf and say so itself
 using idx_type = Function::idx_type;
 assert( fun.Function::int_par_str2idx( "" ) == Inf< idx_type >() );
 assert( fun.Function::int_par_str2idx( "intMaxIte" ) == Inf< idx_type >() );
 assert( fun.Function::int_par_str2idx( "dblRelAcc" ) == Inf< idx_type >() );
 assert( fun.Function::dbl_par_str2idx( "intMaxIter" ) == Inf< idx_type >() );
 assert( fun.C05Function::int_par_str2idx( "nonesuch" ) ==
	 Inf< idx_type >() );

 // an index that is no parameter is thrown at, there being no name to
 // answer with
 auto throws = []( auto f ) {
  try { f(); } catch( const std::invalid_argument & ) { return( true ); }
  return( false );
  };

 assert( throws( [ & ] { fun.Function::int_par_idx2str(
			  fun.Function::get_num_int_par() ); } ) );
 assert( throws( [ & ] { fun.Function::dbl_par_idx2str(
			  fun.Function::get_num_dbl_par() ); } ) );
 assert( throws( [ & ] { fun.Function::get_dflt_int_par(
			  fun.Function::get_num_int_par() ); } ) );
 assert( throws( [ & ] { fun.Function::get_dflt_dbl_par(
			  fun.Function::get_num_dbl_par() ); } ) );
 assert( throws( [ & ] { fun.C05Function::int_par_idx2str(
			  fun.C05Function::get_num_int_par() ); } ) );
 assert( throws( [ & ] { fun.C05Function::get_dflt_dbl_par(
			  fun.C05Function::get_num_dbl_par() ); } ) );
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 runAllTests();
 test_parameter_tables();
 return( 0 );
}

/*--------------------------------------------------------------------------*/
/*----------------------- End File tests_Function.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
