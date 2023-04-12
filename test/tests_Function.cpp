/** @file
 * Unit tests for Function. Since it's an interface,
 * is it tested through its closest implementations.
 *
 * \author Niccolo' Iardella \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Niccolo' Iardella
 */

#include <gtest/gtest.h>

#include "LinearFunction.h"

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------- TEST CASES -------------------------------*/
/*--------------------------------------------------------------------------*/

// TODO: ThinComputeInterface::set/get_ComputeConfig()
// TODO: C05Function linearization stuff
// TODO: C15Function hessian approximation stuff

TEST( FunctionTest, GetsNumberOfParameters ) {
 LinearFunction fun;
 auto iinf = Inf< int >();
 auto dinf = Inf< double >();

 ASSERT_EQ( fun.ThinComputeInterface::get_num_int_par(), 0 );
 ASSERT_EQ( fun.ThinComputeInterface::get_num_dbl_par(), 0 );
 ASSERT_EQ( fun.ThinComputeInterface::get_num_str_par(), 0 );
 ASSERT_EQ( fun.Function::get_num_int_par(), Function::intLastParFun );
 ASSERT_EQ( fun.Function::get_num_dbl_par(), Function::dblLastParFun );
 ASSERT_EQ( fun.C05Function::get_num_int_par(), C05Function::intLastParC05F );
 ASSERT_EQ( fun.C05Function::get_num_dbl_par(), C05Function::dblLastParC05F );
}

TEST( FunctionTest, GetsParameterDefaultValues ) {
 LinearFunction fun;
 auto iinf = Inf< int >();
 auto dinf = Inf< double >();

 ASSERT_EQ( fun.Function::get_dflt_int_par( Function::intMaxIter ), iinf );
 ASSERT_EQ( fun.Function::get_dflt_int_par( Function::intMaxThread ), 0 );
 ASSERT_EQ( fun.Function::get_dflt_dbl_par( Function::dblMaxTime ), dinf );
 ASSERT_EQ( fun.Function::get_dflt_dbl_par( Function::dblRelAcc ), 1e-6 );
 ASSERT_EQ( fun.Function::get_dflt_dbl_par( Function::dblAbsAcc ), dinf );
 ASSERT_EQ( fun.Function::get_dflt_dbl_par( Function::dblUpCutOff ), dinf );
 ASSERT_EQ( fun.Function::get_dflt_dbl_par( Function::dblLwCutOff ), -dinf );

 ASSERT_EQ( fun.C05Function::get_dflt_int_par( C05Function::intLPMaxSz ), 1 );
 ASSERT_EQ( fun.C05Function::get_dflt_int_par( C05Function::intGPMaxSz ), 0 );
 ASSERT_EQ( fun.C05Function::get_dflt_dbl_par( C05Function::dblRAccLin ), 0 );
 ASSERT_EQ( fun.C05Function::get_dflt_dbl_par( C05Function::dblAAccLin ), 0 );
 ASSERT_EQ( fun.C05Function::get_dflt_dbl_par( C05Function::dblAAccMlt ),
            1e-10 );
}

TEST( FunctionTest, ChecksParameterValues ) {
 LinearFunction fun;

 for( int i = 0; i < Function::intLastParFun; ++i ) {
  ASSERT_EQ( fun.Function::get_int_par( i ),
             fun.Function::get_dflt_int_par( i ) );
 }
 for( int i = 0; i < Function::dblLastParFun; ++i ) {
  ASSERT_EQ( fun.Function::get_dbl_par( i ),
             fun.Function::get_dflt_dbl_par( i ) );
 }

 for( int i = 0; i < C05Function::intLastParC05F; ++i ) {
  ASSERT_EQ( fun.C05Function::get_int_par( i ),
             fun.C05Function::get_dflt_int_par( i ) );
 }
 for( int i = 0; i < C05Function::dblLastParC05F; ++i ) {
  ASSERT_EQ( fun.C05Function::get_dbl_par( i ),
             fun.C05Function::get_dflt_dbl_par( i ) );
 }
}

TEST( FunctionTest, ChecksParameterIndices ) {
 LinearFunction fun;

 ASSERT_EQ( fun.Function::int_par_str2idx( "intMaxIter" ),
            Function::intMaxIter );
 ASSERT_EQ( fun.Function::int_par_str2idx( "intMaxThread" ),
            Function::intMaxThread );
 ASSERT_EQ( fun.Function::dbl_par_str2idx( "dblMaxTime" ),
            Function::dblMaxTime );
 ASSERT_EQ( fun.Function::dbl_par_str2idx( "dblRelAcc" ),
            Function::dblRelAcc );
 ASSERT_EQ( fun.Function::dbl_par_str2idx( "dblAbsAcc" ),
            Function::dblAbsAcc );
 ASSERT_EQ( fun.Function::dbl_par_str2idx( "dblUpCutOff" ),
            Function::dblUpCutOff );
 ASSERT_EQ( fun.Function::dbl_par_str2idx( "dblLwCutOff" ),
            Function::dblLwCutOff );

 ASSERT_EQ( fun.C05Function::int_par_str2idx( "intLPMaxSz" ),
            C05Function::intLPMaxSz );
 ASSERT_EQ( fun.C05Function::int_par_str2idx( "intGPMaxSz" ),
            C05Function::intGPMaxSz );
 ASSERT_EQ( fun.C05Function::dbl_par_str2idx( "dblRAccLin" ),
            C05Function::dblRAccLin );
 ASSERT_EQ( fun.C05Function::dbl_par_str2idx( "dblAAccLin" ),
            C05Function::dblAAccLin );
 ASSERT_EQ( fun.C05Function::dbl_par_str2idx( "dblAAccMlt" ),
            C05Function::dblAAccMlt );

}

TEST( FunctionTest, ChecksParameterNames ) {
 LinearFunction fun;

 ASSERT_EQ( fun.Function::int_par_idx2str( Function::intMaxIter ),
            "intMaxIter" );
 ASSERT_EQ( fun.Function::int_par_idx2str( Function::intMaxThread ),
            "intMaxThread" );
 ASSERT_EQ( fun.Function::dbl_par_idx2str( Function::dblMaxTime ),
            "dblMaxTime" );
 ASSERT_EQ( fun.Function::dbl_par_idx2str( Function::dblRelAcc ),
            "dblRelAcc" );
 ASSERT_EQ( fun.Function::dbl_par_idx2str( Function::dblAbsAcc ),
            "dblAbsAcc" );
 ASSERT_EQ( fun.Function::dbl_par_idx2str( Function::dblUpCutOff ),
            "dblUpCutOff" );
 ASSERT_EQ( fun.Function::dbl_par_idx2str( Function::dblLwCutOff ),
            "dblLwCutOff" );

 ASSERT_EQ( fun.C05Function::int_par_idx2str( C05Function::intLPMaxSz ),
            "intLPMaxSz" );
 ASSERT_EQ( fun.C05Function::int_par_idx2str( C05Function::intGPMaxSz ),
            "intGPMaxSz" );
 ASSERT_EQ( fun.C05Function::dbl_par_idx2str( C05Function::dblRAccLin ),
            "dblRAccLin" );
 ASSERT_EQ( fun.C05Function::dbl_par_idx2str( C05Function::dblAAccLin ),
            "dblAAccLin" );
 ASSERT_EQ( fun.C05Function::dbl_par_idx2str( C05Function::dblAAccMlt ),
            "dblAAccMlt" );
}


/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/
int main( int argc, char ** argv ) {
 ::testing::InitGoogleTest( &argc, argv );
 return( RUN_ALL_TESTS() );
}
