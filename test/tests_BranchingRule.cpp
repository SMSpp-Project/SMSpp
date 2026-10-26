/*--------------------------------------------------------------------------*/
/*----------------------- File tests_BranchingRule.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Unit tests for the components of BranchingRule.h, against values computed
 * by hand: the fractionality of the candidates, their stable sort and their
 * features (BranchingFeatures), the product score of strong branching, its
 * ties, its infeasible children and its CSV (StrongBranching), the
 * perceptron read from a file (LearnedBranching), the pseudocosts and the
 * estimates of the children that they give (PseudoCosts), their priors read
 * from a file (LearnedPseudoCosts), and the interface of the estimates
 * (BranchEstimator).
 *
 * The limit cases are the point: no candidate, a single candidate, all the
 * costs zero, a child found empty in either sense, a candidate whose child
 * does not improve the bound, a direction that is neither 0 nor 1, an
 * observation that is not finite or that moves nothing, a negative output
 * of a model, and the files that are not there, are not a model, have a
 * model only or are truncated.
 *
 * A check that fails because of a defect of the library prints what it
 * found and what it expected, and the test goes on with the next one; the
 * return value of main() says whether any failed.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "BranchingRule.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// last, so that the headers above are read as the library was compiled
#include "TestAssert.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*------------------------------ CONSTANTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

static constexpr double c_eps = 1e-12;  ///< tolerance of the comparisons

static const double INF = std::numeric_limits< double >::infinity();

static const double NaN = std::numeric_limits< double >::quiet_NaN();

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

static int n_failures = 0;  ///< number of failed checks of the library

/// records a failed check of the library, saying what was found

static void expect( bool ok , const std::string & what )
{
 if( ok )
  return;
 std::cout << "FAILED: " << what << std::endl;
 ++n_failures;
 }

/*--------------------------------------------------------------------------*/

static bool equal( double a , double b )
{
 if( std::isinf( b ) )
  return( a == b );
 return( std::abs( a - b ) <= c_eps * std::max( 1.0 , std::abs( b ) ) );
 }

/*--------------------------------------------------------------------------*/
/// whether f() throws an exception of type E

template< class E , class F >
static bool throws( F && f )
{
 try { f(); }
 catch( E & ) { return( true ); }
 catch( ... ) { return( false ); }
 return( false );
 }

/*--------------------------------------------------------------------------*/
/// the number of lines of a file

static std::size_t lines_of( const std::string & file )
{
 std::ifstream in( file );
 std::size_t n = 0;
 for( std::string line ; std::getline( in , line ) ; )
  ++n;
 return( n );
 }

/*--------------------------------------------------------------------------*/
/// a model of LearnedBranching with one hidden unit, whose weights are all
/// 0 but the one of feature 0, so that its score is
/// b2 + w2 * tanh( w0 * x[ 0 ] )

static std::string model( double w0 , double w2 , double b2 )
{
 std::ostringstream s;
 s << "LBRModel " << BranchingFeatures::NFeatures << " 1\n" << w0;
 for( std::size_t i = 1 ; i < BranchingFeatures::NFeatures ; ++i )
  s << " 0";
 s << "\n0\n" << w2 << "\n" << b2 << "\n";
 return( s.str() );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/
/// the fractionality, the stable sort and the features of the candidates

static void test_BranchingFeatures( void )
{
 expect( equal( BranchingFeatures::fractionality( 2.25 ) , 0.25 ) &&
	 equal( BranchingFeatures::fractionality( 2.75 ) , 0.25 ) &&
	 equal( BranchingFeatures::fractionality( -1.75 ) , 0.25 ) &&
	 equal( BranchingFeatures::fractionality( 0.5 ) , 0.5 ) &&
	 ( BranchingFeatures::fractionality( 3 ) == 0 ) ,
	 "fractionality: the distance from the nearest integer" );

 // ids 0 .. 3 at 0.9, 0.5, 1.5 and 2: the two at 0.5 keep their order
 std::vector< BranchCandidate > cands = {
  { 0 , 0.9 , 1.0 , 0 , 0 , 0 } , { 1 , 0.5 , 0.2 , -4 , 0.25 , 1 } ,
  { 2 , 1.5 , 1.5 , 2 , 1 , 0 } , { 3 , 2.0 , 2.0 , 1 , 0.5 , 2 } };
 BranchingFeatures::sort_by_fractionality( cands );
 expect( ( cands[ 0 ].id == 1 ) && ( cands[ 1 ].id == 2 ) &&
	 ( cands[ 2 ].id == 0 ) && ( cands[ 3 ].id == 3 ) ,
	 "sort_by_fractionality: nonincreasing and stable" );

 const auto x = BranchingFeatures::features( cands , 0.3 );
 expect( x.size() == 4 , "features: one per candidate" );
 const BranchingFeatures::Features x0 = { 0.5 , 0.5 , 0.2 , 0.3 , -1 ,
					  0.25 , 0.3 , 0 };
 bool same = true;
 for( std::size_t i = 0 ; i < BranchingFeatures::NFeatures ; ++i )
  same &= equal( x[ 0 ][ i ] , x0[ i ] );
 expect( same , "features: those of the first candidate" );
 expect( equal( x[ 1 ][ 4 ] , 0.5 ) && equal( x[ 2 ][ 4 ] , 0 ) &&
	 equal( x[ 1 ][ 7 ] , 1.0 / 3 ) && equal( x[ 3 ][ 7 ] , 1 ) ,
	 "features: the cost over the largest one and the rank" );

 // all the costs 0, a single candidate, no candidate
 std::vector< BranchCandidate > one = { { 5 , 0.4 , 0.4 , 0 , 0 , 0 } };
 const auto x1 = BranchingFeatures::features( one , 0 );
 expect( ( x1.size() == 1 ) && ( x1[ 0 ][ 4 ] == 0 ) &&
	 ( x1[ 0 ][ 7 ] == 0 ) ,
	 "features: a single candidate of cost 0 has cost and rank 0" );
 expect( BranchingFeatures::features( {} , 0 ).empty() ,
	 "features: none without candidates" );
 }

/*--------------------------------------------------------------------------*/
/// the product score of strong branching, in both senses

static void test_StrongBranching( void )
{
 std::vector< BranchCandidate > cands = {
  { 0 , 0.5 , 0.5 , 1 , 0 , 0 } , { 1 , 0.5 , 0.5 , 1 , 0 , 0 } ,
  { 2 , 0.5 , 0.5 , 1 , 0 , 0 } };

 // a minimization at a node of bound 10, hence delta = 1e-5: the first
 // candidate improves only down, the third has its "down" child empty
 const std::vector< std::array< double , 2 > > kids = {
  { 12 , 10 } , { 11 , 13 } , { INF , 10.5 } };
 int calls = 0;
 const StrongBranching::Evaluator children =
  [ & ]( const BranchCandidate & c ) {
   ++calls;
   return( kids[ c.id ] );
   };
 std::ostringstream log;
 const auto evals = StrongBranching::evaluate( cands , children , 10 ,
					       false , & log );
 expect( calls == 3 , "evaluate: the children of each candidate once" );
 expect( equal( evals[ 0 ][ 0 ] , 12 ) && equal( evals[ 0 ][ 1 ] , 10 ) &&
	 equal( evals[ 0 ][ 2 ] , 2 * 1e-5 ) ,
	 "evaluate: a child that does not improve counts as delta" );
 expect( equal( evals[ 1 ][ 2 ] , 3 ) , "evaluate: the product score" );
 expect( evals[ 2 ][ 2 ] == INF , "evaluate: an empty child is infinite" );
 expect( StrongBranching::best( evals ) == 2 ,
	 "best: the candidate with an empty child" );
 std::size_t n = 0;
 for( char ch : log.str() )
  n += ( ch == '\n' );
 expect( n == 3 , "evaluate: one line of log per candidate" );

 // a maximization at a node of bound 10: the children improve downwards,
 // and an empty child has bound -infinity
 const std::vector< std::array< double , 2 > > mkids = {
  { 8 , 9 } , { 11 , 7 } , { - INF , - INF } };
 const auto mevals = StrongBranching::evaluate(
  cands , [ & ]( const BranchCandidate & c ) { return( mkids[ c.id ] ); } ,
  10 , true );
 expect( equal( mevals[ 0 ][ 2 ] , 2 ) &&
	 equal( mevals[ 1 ][ 2 ] , 1e-5 * 3 ) && ( mevals[ 2 ][ 2 ] == INF ) ,
	 "evaluate: the scores of a maximization" );

 // ties go to the first one
 const std::vector< StrongBranching::Eval > tied = {
  { 0 , 0 , 1 } , { 0 , 0 , 3 } , { 0 , 0 , 3 } };
 expect( StrongBranching::best( tied ) == 1 , "best: ties to the first" );

 // the CSV: the header once, then one line per candidate at each call
 const std::string file = "BranchingRule-test-strong.csv";
 std::remove( file.c_str() );
 const auto x = BranchingFeatures::features( cands , 0 );
 StrongBranching::write_log( file , 1 , 10 , cands , x , evals , 2 );
 StrongBranching::write_log( file , 2 , 10 , cands , x , evals , 0 );
 expect( lines_of( file ) == 1 + 2 * cands.size() ,
	 "write_log: the header once and one line per candidate" );
 std::ifstream in( file );
 std::string header , first;
 std::getline( in , header );
 std::getline( in , first );
 expect( ( header.rfind( "call,node," , 0 ) == 0 ) &&
	 ( first.rfind( "1,10," , 0 ) == 0 ) && ( first.back() == '0' ) ,
	 "write_log: the header, the call, the node and the chosen flag" );
 in.close();
 std::remove( file.c_str() );
 expect( throws< std::runtime_error >( [ & ] {
	  StrongBranching::write_log( "no-such-directory/strong.csv" , 1 , 10 ,
				      cands , x , evals , 0 ); } ) ,
	 "write_log: a file that cannot be opened throws" );
 }

/*--------------------------------------------------------------------------*/
/// the perceptron read from a file, and the files that are not a model

static void test_LearnedBranching( void )
{
 const std::string file = "BranchingRule-test-model.txt";

 LearnedBranching lb;
 expect( lb.empty() , "LearnedBranching: empty before any load()" );
 expect( throws< std::invalid_argument >( [ & ] { lb.load( "" ); } ) ,
	 "load: no file given throws" );
 expect( throws< std::runtime_error >( [ & ] {
	  lb.load( "no-such-directory/model.txt" ); } ) ,
	 "load: a file that is not there throws" );

 // score = 0.5 + 2 tanh( x[ 0 ] )
 std::ofstream( file ) << model( 1 , 2 , 0.5 );
 lb.load( file );
 expect( ! lb.empty() , "load: the model is read" );
 BranchingFeatures::Features x{};
 x[ 0 ] = 0.5;
 expect( equal( lb.score( x ) , 0.5 + 2 * std::tanh( 0.5 ) ) ,
	 "score: the perceptron" );
 std::vector< BranchingFeatures::Features > xs( 3 );
 xs[ 0 ][ 0 ] = 0.1;
 xs[ 1 ][ 0 ] = 0.5;
 xs[ 2 ][ 0 ] = 0.5;
 expect( lb.best( xs ) == 1 , "best: the largest score, ties to the first" );

 // a negative weight of the output reverses the order
 std::ofstream( file ) << model( 1 , -2 , 0 );
 lb.load( file );
 expect( lb.best( xs ) == 0 , "best: the largest score of another model" );

 // files that are not a model: the tag, the number of the features, no
 // hidden unit, too few numbers; a failed load() leaves no model
 for( const std::string bad : {
       std::string( "LBRModelX 8 1\n" ) , std::string( "LBRModel 7 1\n" ) ,
       std::string( "LBRModel 8 0\n" ) ,
       std::string( "LBRModel 8 2\n1 2" ) } ) {
  std::ofstream( file ) << bad;
  expect( throws< std::invalid_argument >( [ & ] { lb.load( file ); } ) ,
	  "load: \"" + bad.substr( 0 , bad.find( '\n' ) ) +
	  "\" is refused" );
  expect( lb.empty() , "load: no model after a failed one" );
  }
 std::remove( file.c_str() );
 }

/*--------------------------------------------------------------------------*/
/// the pseudocosts, and the estimates of the children that they give

static void test_PseudoCosts( void )
{
 PseudoCosts pc;
 expect( ( pc.get( 3 , 0 , NaN ) == 0 ) && ( pc.observations() == 0 ) ,
	 "get: 0 with no observation at all" );
 expect( pc.get( 3 , 1 , 2.5 ) == 2.5 , "get: the prior with none" );

 pc.record( 3 , 0 , 0.5 , 2 );        // 4 per unit
 pc.record( 3 , 0 , 0.25 , 2 );       // 8 per unit
 pc.record( 7 , 1 , 0.5 , 1 );        // 2 per unit
 pc.record( 7 , 1 , 0.5 , -1 );       // worse < 0 counts as 0
 pc.record( 9 , 0 , 0 , 5 );          // moves nothing: ignored
 pc.record( 9 , 0 , 0.5 , INF );      // empty child: ignored
 pc.record( 9 , 1 , 0.5 , NaN );      // not a number: ignored
 expect( equal( pc.get( 3 , 0 , NaN ) , 6 ) && pc.known( 3 , 0 ) &&
	 ( ! pc.known( 3 , 1 ) ) , "get: the mean of the observations" );
 expect( equal( pc.get( 3 , 0 , 100 ) , 6 ) ,
	 "get: the prior is not taken where there are observations" );
 expect( equal( pc.get( 7 , 1 , NaN ) , 1 ) ,
	 "record: a negative worsening counts as 0" );
 expect( ( ! pc.known( 9 , 0 ) ) && ( ! pc.known( 9 , 1 ) ) &&
	 ( pc.observations() == 4 ) ,
	 "record: what moves nothing or is not finite is ignored" );
 expect( equal( pc.get( 9 , 0 , NaN ) , 6 ) &&
	 equal( pc.get( 9 , 1 , NaN ) , 1 ) ,
	 "get: the mean of all the candidates with none" );
 expect( throws< std::invalid_argument >( [ & ] {
	  pc.record( 3 , 2 , 1 , 1 ); } ) &&
	 throws< std::invalid_argument >( [ & ] {
	  pc.record( 3 , -1 , 1 , 1 ); } ) ,
	 "record: a direction neither 0 nor 1 throws" );

 // 3 at 0.25 and 7 at 0.5, branching on 3 at a node of bound 10: its
 // children worsen it by 6 * 0.25 down and by the mean "up" 1 * 0.75 up,
 // plus the cheapest rounding of 7, min( 6 * 0.5 , 1 * 0.5 )
 const std::vector< BranchCandidate > cands = {
  { 3 , 0.25 , 0.25 , 1 , 0 , 0 } , { 7 , 0.5 , 0.5 , 1 , 0 , 0 } };
 const auto e = pc.estimates( cands , 0 , 10 , false );
 expect( equal( e[ 0 ] , 10 + 1.5 + 0.5 ) &&
	 equal( e[ 1 ] , 10 + 0.75 + 0.5 ) ,
	 "estimates: those of a minimization" );
 const auto m = pc.estimates( cands , 0 , 10 , true );
 expect( equal( m[ 0 ] , 10 - 2 ) && equal( m[ 1 ] , 10 - 1.25 ) ,
	 "estimates: those of a maximization" );
 const auto p = pc.estimates( cands , 0 , 10 , false ,
			      []( std::size_t ) {
			       return( std::array< double , 2 >{ 100 , 100 } );
			       } );
 expect( equal( p[ 0 ] , e[ 0 ] ) && equal( p[ 1 ] , 10 + 75 + 0.5 ) ,
	 "estimates: the prior only where there is no observation" );
 const std::vector< BranchCandidate > single = {
  { 3 , 0.25 , 0.25 , 1 , 0 , 0 } };
 const auto s = pc.estimates( single , 0 , 10 , false );
 expect( equal( s[ 0 ] , 11.5 ) && equal( s[ 1 ] , 10.75 ) ,
	 "estimates: a single candidate" );
 expect( throws< std::invalid_argument >( [ & ] {
	  ( void ) pc.estimates( cands , 2 , 10 , false ); } ) ,
	 "estimates: no such candidate throws" );
 }

/*--------------------------------------------------------------------------*/
/// the priors of the pseudocosts read from a file

static void test_LearnedPseudoCosts( void )
{
 const std::string file = "BranchingRule-test-priors.txt";

 LearnedPseudoCosts lp;
 expect( lp.empty() , "LearnedPseudoCosts: empty before any load()" );
 expect( throws< std::invalid_argument >( [ & ] { lp.load( "" ); } ) &&
	 throws< std::runtime_error >( [ & ] {
	  lp.load( "no-such-directory/priors.txt" ); } ) ,
	 "load: no file and a file that is not there throw" );

 // two models whose output is the bias: 0.5 down, -1 (hence 0) up
 std::ofstream( file ) << model( 0 , 0 , 0.5 ) << model( 0 , 0 , -1 );
 lp.load( file );
 expect( ! lp.empty() , "load: the two models are read" );
 const auto pr = lp.prior( BranchingFeatures::Features{} , -20 );
 expect( equal( pr[ 0 ] , 0.5 * 20 ) && ( pr[ 1 ] == 0 ) ,
	 "prior: scaled by |L|, a negative output counting as 0" );
 const auto small = lp.prior( BranchingFeatures::Features{} , 0.5 );
 expect( equal( small[ 0 ] , 0.5 ) ,
	 "prior: scaled by 1 when |L| < 1" );

 std::ofstream( file ) << model( 0 , 0 , 0.5 );
 expect( throws< std::invalid_argument >( [ & ] {
	  LearnedPseudoCosts().load( file ); } ) ,
	 "load: a file with one model only throws" );
 std::ofstream( file ) << "LBRModel 8 2\n1 2 3\n";
 expect( throws< std::invalid_argument >( [ & ] {
	  LearnedPseudoCosts().load( file ); } ) ,
	 "load: a truncated file throws" );
 std::remove( file.c_str() );
 }

/*--------------------------------------------------------------------------*/
/// the estimates through the interface

static void test_BranchEstimator( void )
{
 struct Fixed : public BranchEstimator {
  std::vector< double > branch_estimates( void ) override {
   return( std::vector< double >{ 1 , NaN } );
   }
  };

 Fixed f;
 BranchEstimator & be = f;
 const auto e = be.branch_estimates();
 expect( ( e.size() == 2 ) && ( e[ 0 ] == 1 ) && std::isnan( e[ 1 ] ) ,
	 "BranchEstimator: the estimates of the derived class, NaN for none" );
 }

/*--------------------------------------------------------------------------*/

int main( void )
{
 test_BranchingFeatures();
 test_StrongBranching();
 test_LearnedBranching();
 test_PseudoCosts();
 test_LearnedPseudoCosts();
 test_BranchEstimator();

 if( n_failures ) {
  std::cout << "BranchingRule_unit_test: " << n_failures
	    << " check(s) failed" << std::endl;
  return( 1 );
  }

 std::cout << "All tests passed!!" << std::endl;
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File tests_BranchingRule.cpp --------------------*/
/*--------------------------------------------------------------------------*/
