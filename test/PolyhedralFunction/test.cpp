/*--------------------------------------------------------------------------*/
/*-------------------------- File test.cpp ---------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Main for testing PolyhedralFunction
 *
 * A "random" PolyhedralFunction is constructed and put as the only Objective
 * of an otherwise "empty" Block. The same PolyhedralFunction is represented
 * in terms of linear inequalities for another otherwise "empty" Block. The
 * two Block are solved by a BundleSolver and a MILPSolver, respectively,
 * and the results are compared.
 *
 * TODO:
 * The two Block are repeatedly randomly modified "in the same way", and
 * re-solved several times.
 *
 * \version 0.10
 *
 * \date 21 - 08 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ DEFINES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <fstream>
#include <sstream>
#include <iomanip>

#include "AbstractBlock.h"
#include "BundleSolver.h"
#include "CPXMILPSolver.h"

#include "PolyhedralFunction.h"

// #include "FakeSolver.h"

/*--------------------------------------------------------------------------*/
/*-------------------------------- MACROS ----------------------------------*/
/*--------------------------------------------------------------------------*/

#define LOG_LEVEL 1
// 0 = only pass/fail
// 1 = result of each test
// 2 = print data

#if( LOG_LEVEL >= 1 )
 #define LOG1( x ) cout << x
#else
 #define LOG1( x )
#endif

#define PANICMSG { \
                   cout << endl << "something very bad happened!" << endl; \
		   exit( 1 ); \
                   }

#define PANIC( x ) if( ! ( x ) ) PANICMSG

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define DYNAMIC_VARS 0
// if 1, half of the variables are dynamic

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace std;
using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

const double scale = 10;

/*--------------------------------------------------------------------------*/
/*------------------------------- GLOBALS ----------------------------------*/
/*--------------------------------------------------------------------------*/

AbstractBlock * LPBlock;      // the problem expressed as an LP

AbstractBlock * NDOBlock;     // the problem expressed via PolyhedralFunction

double lb = - 1000;           // a tentative LB to detect unbounded instances

Block::Index nvar = 10;       // number of variables
#if DYNAMIC_VARS > 0
 Block::Index nsvar;          // number of static variables
 Block::Index ndvar;          // number of dynamic variables
#else
 #define nsvar nvar           // all variables are static
#endif

ColVariable * vLP;            // pointer to v LP variable

std::vector< ColVariable > * xLP;  // pointer to (static) x LP variables
#if DYNAMIC_VARS > 0
 std::list< ColVariable > * xLPd;  // pointer to (dynamic) x LP variables
#endif

PolyhedralFunction::MultiVector A;

std::vector < Function::FunctionValue > b;

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

template<class T>
static inline void Str2Sthg( const char* const str , T &sthg )
{
 istringstream( str ) >> sthg;
 }

/*--------------------------------------------------------------------------*/

static inline double rndfctr( void )
{
 // return a random number between 0.5 and 2, with 50% probability of being
 // < 1
 double fctr = drand48() - 0.5;
 return( fctr < 0 ? - fctr : fctr * 4 );
 }

/*--------------------------------------------------------------------------*/

static void GenerateA( Block::Index nr , Block::Index nc )
{
 A.resize( nr );

 for( auto & Ai : A ) {
  Ai.resize( nc );
  for( auto & aij : Ai )
   aij = scale * ( 2 * drand48() - 1 );
  }
 }

/*--------------------------------------------------------------------------*/

static void Generateb( Block::Index nr )
{
 b.resize( nr );

 for( auto & bj : b )
  bj = scale * nvar * ( 2 * drand48() - 1 ) / 4;
 }

/*--------------------------------------------------------------------------*/

static void GenerateAb( Block::Index nr , Block::Index nc )
{
 // rationale: the solution x^* will be more or less the solution of some
 // square sub-system A_B x = b_B. We want x^* to be "well scaled", i.e.,
 // the entries to be ~= 1 (in absolute value). The average of each row A_i
 // is 0, the maximum (and minimum) expected value is something like
 // scale * nvars / 2. So we take each b_j in +- scale * nvars / 4

 GenerateA( nr , nc );
 Generateb( nr );
 }

/*--------------------------------------------------------------------------*/

static std::vector< Block::Index > && GenerateRand( Block::Index m ,
						    Block::Index k )
{
 // generate a sorted random k-vector of unique integers in 0 ... m - 1

 std::vector< Block::Index > rnd( m );
 std::iota( rnd.begin() , rnd.end() , 1 );

 for( Block::Index i = 0 ; i < k ; i++ )
  swap( rnd[ i ] , rnd[ i + drand48() * ( m - i ) ] );

 rnd.resize( k );
 sort( rnd.begin() , rnd.end() );

 return( std::move( rnd ) );
 }

/*--------------------------------------------------------------------------*/

static void ConstructLPConstraint( Block::c_Index i , FRowConstraint & ci ,
				   const bool setblock = true )
{
 // construct constraint ci out of A[ i ] and b[ i ]:
 // the constraint is b[ i ] <= vLP - \sum_j Ai[ j ] * xLP[ j ] <= INF
 ci.set_lhs( b[ i ] );
 ci.set_rhs( SMSpp_di_unipi_it::Inf< FRowConstraint::RHSValue >() );
 LinearFunction::v_coeff_pair vars;
 vars.push_back( std::make_pair( vLP , 1 ) );
 for( Block::Index j = 0 ; j < nsvar ; ++j ) {
  if( A[ i ][ j ] != 0 )
   vars.push_back( std::make_pair( &((*xLP)[ j ] ) , - A[ i ][ j ] ) );
  #if DYNAMIC_VARS > 0
   for( Block::Index j = 0 ; j < ndvar ; ++j )
    if( A[ i ][ nsvar + j ] != 0 )
     vars.push_back( std::make_pair( &((*xLPd)[ j ] ) ,
				     - A[ i ][ nsvar + j ] ) );
  #endif
  ci.set_function( new LinearFunction( std::move( vars ) ) );
  if( setblock )
   ci.set_Block( LPBlock );
  }
 }

/*--------------------------------------------------------------------------*/

static bool SolveBoth( void ) 
{
 try {
  // solve the LPBlock- - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Solver * slvrLP = (LPBlock->get_registered_solvers()).front();
  int rtrnLP = slvrLP->compute( false );

  // solve the NODBlock - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Solver * slvrNDO = (NDOBlock->get_registered_solvers()).front();
  int rtrnNDO = slvrNDO->compute( false );

  if( ( rtrnLP >= Solver::kOK ) && ( rtrnLP < Solver::kError ) &&
      ( rtrnNDO >= Solver::kOK ) && ( rtrnNDO < Solver::kError ) ) {
   auto foLP = slvrLP->get_ub();
   auto foNDO = slvrNDO->get_ub();
   if( abs( foLP - foNDO )
       <= 1e-9 *  max( double( 1 ) , abs( max( foLP , foNDO ) ) ) ) {
    LOG1( "OK(f)" << endl );
    return( true );
    }
   }

  if( ( rtrnLP >= Solver::kOK ) && ( rtrnLP < Solver::kError ) &&
      ( rtrnNDO == Solver::kUnbounded ) ) {
   /* Weird case: the LP found an optimal solution but the NDO declared the
    * problem unbounded below. This may be because the tentative lb is too
    * high, check it this actually is the case and if so declare the
    * run a success (but also decrease the lb). */
   if( slvrNDO->get_ub() <= lb * ( 1 + 1e-9 ) ) {
    LOG1( "OK(?lb?)" << endl );
    lb *= 2;
    return( false );
    }
   }

  if( ( rtrnLP == Solver::kInfeasible ) &&
      ( rtrnNDO == Solver::kInfeasible ) ) {
    LOG1( "OK(?e?)" << endl );
    return( false );
    }

  if( ( rtrnLP == Solver::kUnbounded ) &&
      ( rtrnNDO == Solver::kUnbounded ) ) {
    LOG1( "OK(u)" << endl );
    return( false );
    }

  #if( LOG_LEVEL >= 1 )
   cout << " ~ LPBlock = ";
   if( ( rtrnLP >= Solver::kOK ) && ( rtrnLP < Solver::kError ) )
    cout << slvrLP->get_ub() << endl;
   else
    if( rtrnLP == Solver::kInfeasible )
     cout << "    +INF(?)";
    else
     if( rtrnLP == Solver::kUnbounded )
      cout << "        -INF";
     else
      cout << "      Error!";

   cout << " ~ NDOBlock = ";
   if( ( rtrnNDO >= Solver::kOK ) && ( rtrnNDO < Solver::kError ) )
    cout << slvrNDO->get_ub() << endl;
   else
    if( rtrnNDO == Solver::kInfeasible )
     cout << "    +INF(?)";
    else
     if( rtrnNDO == Solver::kUnbounded )
      cout << "        -INF";
     else
      cout << "      Error!";
   cout << endl;
  #endif

  return( false );
  }
 catch( exception &e ) {
  cerr << e.what() << endl;
  exit( 1 );
  }
 catch(...) {
  cerr << "Error: unknown exception thrown" << endl;
  exit( 1 );
  }
 }

/*--------------------------------------------------------------------------*/

int main( int argc , char **argv )
{
 // reading command line parameters - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 long int seed = 1;
 Block::Index wchg = 63;
 double dens = 4;  
 double p_change = 0.5;
 Block::Index n_change = 10;
 Block::Index n_repeat = 0;

 switch( argc ) {
  case( 8 ): Str2Sthg( argv[ 7 ] , p_change );
  case( 7 ): Str2Sthg( argv[ 6 ] , n_change );
  case( 6 ): Str2Sthg( argv[ 5 ] , n_repeat );
  case( 5 ): Str2Sthg( argv[ 4 ] , dens );
  case( 4 ): Str2Sthg( argv[ 3 ] , nvar );
  case( 3 ): Str2Sthg( argv[ 2 ] , wchg );
  case( 2 ): Str2Sthg( argv[ 1 ] , seed );
             break;
  default: cerr << "Usage: " << argv[ 0 ] <<
  #if DYNAMIC_VARS > 0
	   " seed [wchg nvar dens #rounds #chng %chng]"
  #else
	   " seed [wchg nvar dens #rounds rchng]"
  #endif
 		<< endl <<
           "       wchg: what to change, coded bit-wise "
		<< endl <<
           "             0 = add rows, 1 = delete rows "
		<< endl <<
           "             2 = modify rows, 3 = modify constants"
  #if DYNAMIC_VARS > 0  
		<< endl <<
           "             4 = add variables rows, 5 = delete variables"
  #endif
	        << endl <<
           "       nvar: number of variables [10]"
	        << endl <<
           "       dens: rows / variables [4]"
	        << endl <<
           "       #rounds: how many iterations [80]"
	        << endl <<
           "       #chng: number changes [10]"
	        << endl <<
           "       %chng: probability of changing [50%]"
	        << endl;
	   return( 1 );
  }

 if( nvar < 1 ) {
  cout << "error: nvar too small";
  exit( 1 );
  }

 #if DYNAMIC_VARS > 0
  nsvar = nvar / 2;      // half of the variables are dynamic
  ndvar = nvar - nsvar;  // the other half are static
 #endif

 Block::Index m = nvar * dens;
 if( m < 1 ) {
  cout << "error: dens too small";
  exit( 1 );
  }

 srand48( seed );  // seed the pseudo-random number generator

 // constructing the data of the problem- - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // construct the matrix m x nvars matrix A and the m-vector b
 
 GenerateAb( m , nvar );

 #if( LOG_LEVEL >= 2 )
  cout << "n = " << nvar << ", m = " << m << endl;
  for( Block::Index i = 0 ; i < m ; ++i ) {
   cout << "A[ " << i << " ] = [ ";
   for( Block::Index j = 0 ; j < nvar ; ++j )
    cout << A[ i ][ j ] << " ";
   cout << " ], b[ " << i << " ] = " << b[ i ] << endl;
   }
 #endif

 // construction and loading of the objects - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // construct the LP- - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 {
  // ensure all original pointers go out of scope immediately after that
  // the construction has finished

  LPBlock = new AbstractBlock();

  // construct the Variable
  xLP = new std::vector< ColVariable >( nsvar );
  #if DYNAMIC_VARS > 0
   xLPd = new std::list< ColVariable >( ndvar );
   for( auto & xi : *xLPd )
    xi.set_Block( LPBlock );
  #endif
  for( auto & xi : *xLP )
   xi.set_Block( LPBlock );

  vLP = new ColVariable;
  vLP->set_Block( LPBlock );

  // construct the Constraint
  auto ALP = new std::list< FRowConstraint >( m );
  auto ALPit = ALP->begin();
  for( Block::Index i = 0 ; i < m ; )
   ConstructLPConstraint( i++ , *(ALPit++) );
  
  // construct the Objective
  auto objLP = new FRealObjective();
  objLP->set_function( new LinearFunction( { std::make_pair( vLP , 1 ) } ) );

  // now set the Variable, Constraint and Objective in the AbstractBlock
  LPBlock->add_static_variable( *vLP );
  #if DYNAMIC_VARS > 0
   LPBlock->add_dynamic_variable( *xLPd );
  #endif
  LPBlock->add_static_variable( *xLP );
  LPBlock->add_dynamic_constraint( *ALP );
  LPBlock->set_objective( objLP );
  }

 // construct the NDO problem - - - - - - - - - - - - - - - - - - - - - - - -
 {
  // ensure all original pointers go out of scope immediately after that
  // the construction has finished

  NDOBlock = new AbstractBlock();

  // construct the Variable
  auto xNDO = new std::vector< ColVariable >( nsvar );
  #if DYNAMIC_VARS > 0
   auto xNDOd = new std::list< ColVariable >( ndvar );
   for( auto & xi : *xNDOd )
    xi.set_Block( LPBlock );
  #endif
  PolyhedralFunction::VarVector vars( nvar );
  auto vit = vars.begin();
  for( auto & xi : *xNDO ) {
   *(vit++) = & xi;
   xi.set_Block( NDOBlock );
   }

  #if DYNAMIC_VARS > 0
   std::sort( vars.begin() , vars.end() );
  #endif

  // construct the Objective
  auto objNDO = new FRealObjective();
  objNDO->set_function( new PolyhedralFunction( std::move( vars ) ,
						std::move( A ) ,
						std::move( b ) ) );

  // now set the Variable and Objective in the AbstractBlock
  NDOBlock->add_static_variable( *xNDO );
  #if DYNAMIC_VARS > 0
   NDOBlock->add_dynamic_variable( *xNDOd );
  #endif
  NDOBlock->set_objective( objNDO );
  }

 // attach the Solver to the Block- - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 LPBlock->register_Solver( Solver::new_Solver( "CPXMILPSolver" ) );

 NDOBlock->register_Solver( Solver::new_Solver( "BundleSolver" ) );

 #if( LOG_LEVEL >= 2 )
  ((LPBlock->get_registered_solvers()).front())->set_par(
			      CPXMILPSolver::strOutputFile , "LPBlock.lp" );
 #endif

 // first solver call - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 cout << "First call: ";
 cout.setf( ios::scientific, ios::floatfield );
 cout << setprecision( 6 );

 bool AllPassed = SolveBoth();
 
 // main loop - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // now, for n_repeat times:
 // - up to n_change rows are added
 // - up to n_change rows are deleted
 // - up to n_change rows are modified
 // - up to n_change rows are modified
 //
 // then the two problems are re-solved

 while( n_repeat-- ) {

  LOG1( "Changes: ");

  // add rows - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 1 ) && ( drand48() <= p_change ) ) {
   Block::Index tochange = max( double( 1 ) , drand48() * n_change );
   if( tochange ) {
    LOG1( "added " << tochange << " rows - " );

    GenerateAb( tochange , nvar );

    // add them to the LP
    vLP = LPBlock->get_static_variable< ColVariable >( 0 );
    xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
    #if DYNAMIC_VARS > 0
     xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
    #endif

    std::list< FRowConstraint > nc( tochange );
    auto ncit = nc.begin();
    for( Block::Index i = 0 ; i < tochange ; )
     ConstructLPConstraint( i++ , *(ncit++) );
    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );
    LPBlock->add_dynamic_constraints( *cnst , nc );

    // add them to the NDO
    auto NDOobj = dynamic_cast< FRealObjective * >( NDOBlock->get_objective() );
    PANIC( NDOobj );
    auto PF = dynamic_cast< PolyhedralFunction * >( NDOobj );
    PANIC( PF );
     
    if( tochange == 1 )
     PF->add_row( std::move( A[ 0 ] ) , b[ 0 ] );
    else
     PF->add_rows( std::move( A ) , b );

    // update m
    m += tochange;

    // sanity checks
    PANIC( m == PF->get_A().size() );
    PANIC( m == PF->get_b().size() );
    PANIC( m == cnst->size() );
    }
   }

  // delete rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 2 ) && ( drand48() <= p_change ) ) {
   Block::Index tochange = max( double( 1 ) , drand48() * n_change );
   if( tochange ) {
    LOG1( "deleted " << tochange << " rows - " );

    std::vector<Function::Index> nms = GenerateRand( tochange , m );

    // remove them from the LP
    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );
    if( tochange == 1 )
     LPBlock->remove_dynamic_constraint( *cnst , std::next( cnst->begin() ,
							    nms[ 0 ] ) );
    else {
     std::vector< std::list< FRowConstraint >::iterator > itrs( tochange );
     Block::Index prev = 0;
     auto cit = cnst->begin();
     for( Block::Index i = 0 ; i < tochange ; ) {
      itrs[ i ] = cit = std::next( cit , nms[ i ] - prev );
      prev = nms[ i++ ];
      }

     LPBlock->remove_dynamic_constraints( *cnst , itrs );
     }
    
    // remove them from the NDO
    auto NDOobj = dynamic_cast< FRealObjective * >( NDOBlock->get_objective() );
    PANIC( NDOobj );
    auto PF = dynamic_cast< PolyhedralFunction * >( NDOobj );
    PANIC( PF );
     
    if( tochange == 1 )
     PF->delete_row( nms[ 0 ] );
    else
     PF->delete_rows( nms );

    // update m
    m -= tochange;

    // sanity checks
    PANIC( m == PF->get_A().size() );
    PANIC( m == PF->get_b().size() );
    PANIC( m == cnst->size() );
    }
   }

  // modify rows- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 4 ) && ( drand48() <= p_change ) ) {
   Block::Index tochange = max( double( 1 ) , drand48() * n_change );
   if( tochange ) {
    LOG1( "modified " << tochange << " rows - " );

    GenerateAb( tochange , nvar );

    std::vector<Function::Index> nms = GenerateRand( tochange , m );

    // modify them in the LP
    vLP = LPBlock->get_static_variable< ColVariable >( 0 );
    xLP = LPBlock->get_static_variable_v< ColVariable >( 1 );
    #if DYNAMIC_VARS > 0
     xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
    #endif
    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

    Block::Index prev = 0;
    auto cit = cnst->begin();
    for( Block::Index i = 0 ; i < tochange ; ) {
     cit = std::next( cit , nms[ i ] - prev );
     prev = nms[ i ];
     ConstructLPConstraint( i++ , *cit );
     }

    // modify them in the NDO
    auto NDOobj = dynamic_cast< FRealObjective * >( NDOBlock->get_objective() );
    PANIC( NDOobj );
    auto PF = dynamic_cast< PolyhedralFunction * >( NDOobj );
    PANIC( PF );
     
    if( tochange == 1 )
     PF->modify_row( nms[ 0 ] , std::move( A[ 0 ] ) , b[ 0 ] );
    else
     PF->modify_rows( nms , std::move( A ) , b );
    }
   }

  // modify constants - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 8 ) && ( drand48() <= p_change ) ) {
   Block::Index tochange = max( double( 1 ) , drand48() * n_change );
   if( tochange ) {
    LOG1( "modified " << tochange << " constants - " );

    Generateb( tochange );

    std::vector<Function::Index> nms = GenerateRand( tochange , m );

    // change them in the LP
    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

    Block::Index prev = 0;
    auto cit = cnst->begin();
    for( Block::Index i = 0 ; i < tochange ; ) {
     cit = std::next( cit , nms[ i ] - prev );
     prev = nms[ i ];
     (*cit).set_lhs( b[ i++ ] );
     }

    // modify them in the NDO
    auto NDOobj = dynamic_cast< FRealObjective * >( NDOBlock->get_objective() );
    PANIC( NDOobj );
    auto PF = dynamic_cast< PolyhedralFunction * >( NDOobj );
    PANIC( PF );
     
    if( tochange == 1 )
     PF->modify_constant( nms[ 0 ] , b[ 0 ] );
    else
     PF->modify_constants( nms , b );
    }
   }

  // add variables- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // before really going down to this, one issue should be clarified:
  // IT IS POSSIBLE TO KEEP THE LISTS OF DYNAMIC VARIABLES ORDERED??
  // CURRENTLY IT IS NOT, BUT IN THIS CASE IT WOULD MOST DEFINITELY BE
  // USEFUL - FOR Variable, BUT *NOT* FOR Constraint. SO, IF THE
  // MECHANISM IS ADDED, IT SHOULD BE OPTIONAL (AS WELL AS UNOBTRUSIVE)

  #if DYNAMIC_VARS > 0
  if( ( wchg & 16 ) && ( drand48() <= p_change ) ) {
   Block::Index tochange = max( double( 1 ) , drand48() * n_change );
   if( tochange ) {
    LOG1( "added " << tochange << " variables - " );

    GenerateA( m , tochange );

    std::list< ColVariable > newxLPd( tochange );

    std::vector<Function::Index> nms = GenerateRand( tochange , ndvar );

    // add them in the LP
    xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

    // TBD ...

    // modify them in the NDO
    std::list< ColVariable > newxNDOd( tochange );

    auto NDOobj = dynamic_cast< FRealObjective * >( NDOBlock->get_objective() );
    PANIC( NDOobj );
    auto PF = dynamic_cast< PolyhedralFunction * >( NDOobj );
    PANIC( PF );

    // add them to the NDOBlock: TBD

    if( tochange == 1 )
     PF->add_variable( & newxNDOd.front() , A[ 0 ] );
    else
     PF->add_variables( std::move( newxNDOd ) , std::move( A ) );
    }
   }

  // remove variables - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 32 ) && ( drand48() <= p_change ) ) {
   Block::Index tochange = max( double( 1 ) , drand48() * n_change );
   if( tochange ) {
    LOG1( "removed " << tochange << " variables - " );

    std::vector<Function::Index> nms = GenerateRand( tochange , ndvar );
    for( auto & nmsi : nms )
     nmsi += nsvar;

    // remove them from the LP
    xLPd = LPBlock->get_dynamic_variable< ColVariable >( 0 );
    auto cnst = LPBlock->get_dynamic_constraint< FRowConstraint >( 0 );

    // TBD ...

    // remove them from the NDO
    auto NDOobj = dynamic_cast< FRealObjective * >( NDOBlock->get_objective() );
    PANIC( NDOobj );
    auto PF = dynamic_cast< PolyhedralFunction * >( NDOobj );
    PANIC( PF );
     
    if( tochange == 1 )
     PF->remove_variable( nms[ 0 ] );
    else
     PF->remove_variables( nms , true );
    }
   }

  
  #endif  // DYNAMIC_VARS > 0

  
  // finally, re-solve the problems- - - - - - - - - - - - - - - - - - - - -

  AllPassed &= SolveBoth();

  }  // end( main loop )- - - - - - - - - - - - - - - - - - - - - - - - - - -
     // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 if( AllPassed )
  cout << "All test passed!!" << endl;
 else
  cout << "Shit happened!!" << endl;
 
 // destroy objects and vectors - - - - - - - - - - - - - - - - - - - - - - - 
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 delete NDOBlock;
 delete LPBlock;

 // terminate - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 return( 0 );

 }  // end( main )

/*--------------------------------------------------------------------------*/
/*------------------------ End File test.cpp -------------------------------*/
/*--------------------------------------------------------------------------*/
