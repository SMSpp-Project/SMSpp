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
 * \date 23 - 07 - 2019
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

#define LOG_LEVEL 2
// 0 = only pass/fail
// 1 = result of each test
// 2 = print data

#if( LOG_LEVEL >= 1 )
 #define LOG1( x ) cout << x
#else
 #define LOG1( x )
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------------- USING -----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace std;
using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*-------------------------------- TYPES -----------------------------------*/
/*--------------------------------------------------------------------------*/

typedef unsigned int Index;

/*--------------------------------------------------------------------------*/
/*------------------------------- CONSTANTS --------------------------------*/
/*--------------------------------------------------------------------------*/

const double scale = 10;

/*--------------------------------------------------------------------------*/
/*------------------------------- GLOBALS ----------------------------------*/
/*--------------------------------------------------------------------------*/

AbstractBlock * LPBlock;      // the problem expressed as an LP

AbstractBlock * NDOBlock;     // the problem expressed via PolyhedralFunction

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

static inline bool SolveBoth( void ) 
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
 Index wchg = 127;
 Index nvar = 10;
 double dens = 4;  
 double p_change = 0.5;
 Index n_change = 10;
 Index n_repeat = 0;

 switch( argc ) {
  case( 8 ): Str2Sthg( argv[ 7 ] , p_change );
  case( 7 ): Str2Sthg( argv[ 6 ] , n_change );
  case( 6 ): Str2Sthg( argv[ 5 ] , n_repeat );
  case( 5 ): Str2Sthg( argv[ 4 ] , dens );
  case( 4 ): Str2Sthg( argv[ 3 ] , nvar );
  case( 3 ): Str2Sthg( argv[ 2 ] , wchg );
  case( 2 ): Str2Sthg( argv[ 1 ] , seed );
  default: cerr << "Usage: " << argv[ 0 ] <<
	   " [seed wchg nvar dens #rounds #chng %chng]"
           "       wchg: what to change, coded bit-wise "
		<< endl <<
           "             0 = cost, 1 = cap, 2 = dfct, 3 = o.arc, 4 = c.arc"
		<< endl <<
           "             5 = add arc, 6 = delete arc"
	        << endl <<
           "       nvar: number of variables [10]"
	        << endl <<
           "       dens: rows / variables [4]"
	        << endl;
	   return( 1 );
  }

 if( nvar < 1 ) {
  cout << "error: nvar too small";
  exit( 1 );
  }

 Index m = nvar * dens;
 if( m < 1 ) {
  cout << "error: dens too small";
  exit( 1 );
  }

 srand48( seed );  // seed the pseudo-random number generator

 // constructing the data of the problem- - - - - - - - - - - - - - - - - - -
 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 // construct the matrix m x nvars matrix A and the m-vector b
 //
 // rationale: the solution x^* will be more or less the solution of some
 // square sub-system A_B x = b_B. We want x^* to be "well scaled", i.e.,
 // the entries to be ~= 1 (in absolute value). The average of each row A_i
 // is 0, the maximum (and minimum) expected value is something like
 // scale * nvars / 2. So we take each b_j in +- scale * nvars / 4
 
 PolyhedralFunction::MultiVector A( m );
 std::vector < Function::FunctionValue > b( m );

 for( auto & Ai : A ) {
  Ai.resize( nvar );
  for( auto & aij : Ai )
   aij = scale * ( 2 * drand48() - 1 );
  }

 for( auto & bj : b )
  bj = scale * nvar * ( 2 * drand48() - 1 ) / 4; 

 #if( LOG_LEVEL >= 2 )
  cout << "n = " << nvar << ", m = " << m << endl;
  for( Index i = 0 ; i < m ; ++i ) {
   cout << "A[ " << i << " ] = [ ";
   for( Index j = 0 ; j < nvar ; ++j )
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
  /*!!
  auto xLP = new std::list<ColVariable>( nvars );
  !!*/
  auto xLP = new std::vector<ColVariable>( nvar );
  for( auto & xi : *xLP )
   xi.set_Block( LPBlock );

  auto vLP = new ColVariable;
  vLP->set_Block( LPBlock );

  // construct the Constraint
  auto Ait = A.begin();
  auto ALP = new std::list<FRowConstraint>( m );
  for( auto & ci : *ALP ) {
   // the constraint is 0 <= vLP - \sum_j Ai[ j ] * xLP[ j ] <= INF
   ci.set_rhs( SMSpp_di_unipi_it::Inf<FRowConstraint::RHSValue>() );
   LinearFunction::v_coeff_pair vars;
   vars.push_back( std::make_pair( vLP , 1 ) );
   auto xit = xLP->begin();
   for( auto aij : *Ait ) {
    if( aij != 0 )
     vars.push_back( std::make_pair( &(*xit) , - aij ) );
    ++xit;
    }
   ci.set_function( new LinearFunction( std::move( vars ) ) );
   ci.set_Block( LPBlock );
   ++Ait;
   }

  // construct the Objective
  auto objLP = new FRealObjective();
  objLP->set_function( new LinearFunction( { std::make_pair( vLP , 1 ) } ) );

  // now set the Variable, Constraint and Objective in the AbstractBlock
  LPBlock->add_static_variable( *vLP );
  /*!!
  LPBlock->add_dynamic_variable( *xLP );
  !!*/
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
  /*!!
  auto xNDO = new std::list<ColVariable>( nvar );
    !!*/
  auto xNDO = new std::vector<ColVariable>( nvar );
  PolyhedralFunction::VarVector vars( nvar );
  auto vit = vars.begin();
  for( auto & xi : *xNDO ) {
   *(vit++) = & xi;
   xi.set_Block( NDOBlock );
   }

  /*!!
  std::sort( vars.begin() , vars.end() );
    !!*/

  // construct the Objective
  auto objNDO = new FRealObjective();
  objNDO->set_function( new PolyhedralFunction( std::move( vars ) ,
						std::move( A ) ,
						std::move( b ) ) );

  // now set the Variable and Objective in the AbstractBlock
  /*!!
  NDOBlock->add_dynamic_variable( *xNDO );
    !!*/
  NDOBlock->add_static_variable( *xNDO );
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
 // - up tp n_change rows are added
 // - ...
 //
 // then the two problems are re-solved

 while( n_repeat-- ) {

  cout << "Changing: ";

  // add rows - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /*!!
  if( ( wchg & 1 ) && ( drand48() <= p_change ) ) {
   }
   !!*/

  /*!!
  // change capacities- - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( ( wchg & 2 ) && ( drand48() <= p_change ) ) {
   MCFBlock::Index tochange = max( double( 1 ) , drand48() * n_change );
   cout << tochange << " capacit";

   if( tochange == 1 ) {
    MCFBlock::Index arc = MCFBlock::Index( drand48() * ( m - 1 ) );
    MCFBlock::CNumber newcap = mcf->MCFUCap( arc ) * rndfctr();
    mcf->ChgUCap( arc , newcap );

    if( ( mode & 16 ) && ( drand48() < 0.5 ) ) {
     // change via abstract representation
     cout << "y(a) - ";
     mMCFB->i2p_ub( arc )->set_rhs( newcap );
     }
    else {  // change via call to chg_* method
     mMCFB->chg_ucap( newcap , arc );
     cout << "y - ";
     }
    }
   else {
    MCFBlock::Vec_FNumber newcaps( tochange );

    // in 50% of the cases do a ranged change, in the others a sparse change
    if( drand48() <= 0.5 ) {
     MCFBlock::Index strt = drand48() * ( m - tochange );
     MCFBlock::Index stp = strt + tochange;
     for( MCFBlock::Index i = 0 ; i < tochange ; ++i )
      newcaps[ i ] = mcf->MCFUCap( i + strt ) * rndfctr();
     mcf->ChgUCaps( newcaps.data() , nullptr , strt , stp );

     if( ( mode & 16 ) && ( drand48() < 0.5 ) ) {
      // change via abstract representation
      cout << "ies(a,r) - ";
      for( MCFBlock::Index i = 0 ; i < tochange ; ++i )
       mMCFB->i2p_ub( i + strt )->set_rhs( newcaps[ i ] );
      }
     else {  // change via call to chg_* method
      mMCFB->chg_ucaps( newcaps.begin() , strt , stp );
      cout << "ies(r) - ";
      }
     }
    else {
     MCFBlock::Vec_Index nms( m + 1 );
     for( MCFBlock::Index i = 0 ; i < m ; i++ )
      nms[ i ] = i;

     for( MCFBlock::Index i = 0 ; i < tochange ; i++ ) {
      swap( nms[ i ] , nms[ i + drand48() * ( m - i ) ] );
      newcaps[ i ] = mcf->MCFUCap( nms[ i ]  ) * rndfctr();
      }

     auto end = nms.begin() + tochange;
     sort( nms.begin() , end );
     *end = OPTtypes_di_unipi_it::Inf<Index>();
     mcf->ChgUCaps( newcaps.data() , nms.data() );
     nms.resize( tochange );

     if( ( mode & 16 ) && ( drand48() < 0.5 ) ) {
      // change via abstract representation
      cout << "ies(a,s) - ";
      for( MCFBlock::Index i = 0 ; i < tochange ; ++i )
       mMCFB->i2p_ub( nms[ i ] )->set_rhs( newcaps[ i ] );
      }
     else {  // change via call to chg_* method
      mMCFB->chg_ucaps( newcaps.begin() , std::move( nms ) , true );
      cout << "ies(s) - ";
      }
     }
    }
   }  // end( if( change capacities ) )
   !!*/

  // change deficits- - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /*!!
  if( ( wchg & 4 ) && ( drand48() <= p_change ) ) {
   cout << "2 deficits";

   Index posn;
   Index negn;
   FNumber posd;
   FNumber negd;

   if( nzdfct ) {  // if there are nonzero deficits
    MCFBlock::Vec_FNumber dfcts( n );
    mcf->MCFDfcts( dfcts.data() );

    do
     posn = Index( drand48() * n );  // select node with positive
    while( dfcts[ posn ] <= 0 );               // deficit (one must exist)
    posd = dfcts[ posn ];

    do
     negn = Index( drand48() * n );  // select node with negative
    while( dfcts[ negn ] >= 0 );               // deficit (one must exist)
    negd = dfcts[ negn ];
    }
   else {
    posn = Index( drand48() * n );   // just select at random
    negn = Index( drand48() * n );
    posd = negd = 0;
    }

   FNumber Dlt = u_avg * 2 * drand48();
   if( drand48() <= 0.5 ) {  // in 50% of cases up, in 50% of cases down
    posd += Dlt;
    negd -= Dlt;
    }
   else {
    Dlt = min( Dlt , max( max( posd , - negd ) / 2 , double( 1 ) ) );
    posd -= Dlt;
    negd += Dlt;
    }

   mcf->ChgDfct( posn , posd );
   mcf->ChgDfct( negn , negd );

   if( ( mode & 16 ) && ( drand48() < 0.5 ) ) {
    // change via abstract representation
    cout << "(a)";
    mMCFB->i2p_e( posn )->set_both( posd );
    mMCFB->i2p_e( negn )->set_both( negd );
    }
   else {  // change via call to chg_* method
    mMCFB->chg_dfct( posd , posn );
    mMCFB->chg_dfct( negd , negn );
    }

   cout << " - ";

   }  // end( change deficits )
   !!*/

  // closing arcs- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /*!!
  if( ( wchg & 8 ) && ( drand48() <= p_change ) ) {
   MCFBlock::Index changed = 0;

   MCFBlock::Vec_Index nms( n_change );
   for( MCFBlock::Index i = mMCFB->get_NStaticArcs() ;
	i < mMCFB->get_NArcs() ; ++i ) {
    if( mcf->IsDeletedArc( i ) )
     continue;
    if( mcf->IsClosedArc( i ) )
     continue;
    if( drand48() <= 0.5 )
     continue;
    
    nms[ changed++ ] = i;
    mcf->CloseArc( i );

    if( changed >= n_change )
     break;
    }

   if( changed ) {
    nms.resize( changed );
    cout << changed << " close";

    if( ( mode & 16 ) && ( drand48() < 0.5 ) ) {
     // change via abstract representation
     cout << "(a)";
     for( auto i : nms ) {
      auto x = mMCFB->i2p_x( i );
      x->set_value( 0 );
      x->is_fixed( true );
      }
     }
    else  // change via call to chg_* method
     mMCFB->close_arcs( std::move( nms ) );

    cout << " - ";
    }
   }
   !!*/

  // re-opening arcs - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /*!!
  if( ( wchg & 16 ) && ( drand48() <= p_change ) ) {
   MCFBlock::Index changed = 0;

   MCFBlock::Vec_Index nms( n_change );
   for( MCFBlock::Index i = mMCFB->get_NStaticArcs() ;
	i < mMCFB->get_NArcs() ; ++i ) {
    if( mcf->IsDeletedArc( i ) )
     continue;
    if( ! mcf->IsClosedArc( i ) )
     continue;
    if( drand48() <= 0.5 )
     continue;
    
    nms[ changed++ ] = i;
    mcf->OpenArc( i );

    if( changed >= n_change )
     break;
    }

   if( changed ) {
    nms.resize( changed );
    cout << changed << " open";

    if( ( mode & 16 ) && ( drand48() < 0.5 ) ) {
     // change via abstract representation
     cout << "(a)";
     for( auto i : nms )
      mMCFB->i2p_x( i )->is_fixed( false );
     }
    else  // change via call to chg_* method
     mMCFB->open_arcs( std::move( nms ) );

    cout << " - ";
    }
   }
   !!*/

  // deleting arcs - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /*!!
  if( ( wchg & 32 ) && ( drand48() <= p_change ) ) {
   MCFBlock::Index changed = 0;

   if( drand48() < 0.5 ) {
    // delete somewhere in the middle

    for( MCFBlock::Index i = mMCFB->get_NStaticArcs() ;
	 i < mMCFB->get_NArcs() ; ++i ) {
     if( mcf->IsDeletedArc( i ) )
      continue;
     if( drand48() <= 0.75 )
      continue;

     mcf->DelArc( i );
     mMCFB->remove_arc( i );
     if( ++changed >= n_change )
      break;
     }

    if( changed )
     cout << changed << " delete(m) - ";
    }
   else {
    for( MCFBlock::Index i =  mMCFB->get_NArcs() ;
	 --i >= mMCFB->get_NStaticArcs() ; ) {
     if( mcf->IsDeletedArc( i ) )
      continue;
     if( drand48() <= 0.13 )
      break;

     mcf->DelArc( i );
     mMCFB->remove_arc( i );
     if( ++changed >= n_change )
      break;
     }

    if( changed )
     cout << changed << " delete(e) - ";
    }
   }
   !!*/

  // creating new arcs - - - - - - - - - - - - - - - - - - - - - - - - - - -

  /*!!
  if( ( wchg & 64 ) && ( drand48() <= p_change ) ) {

   MCFBlock::Index changed = 0;
   MCFBlock::Index afterend = 0;
   while( changed < n_change ) {
    if( drand48() <= 0.13 )
     break;

    ++changed;

    // random sn != en
    MCFBlock::Index sn , en;
    do {
     sn = drand48() * mMCFB->get_NNodes() + 1;
     en = drand48() * mMCFB->get_NNodes() + 1;
     } while( sn == en );

    // random cost in [ - c_max , c_max ]
    auto cst = c_max * ( 1 - 2 * drand48() );

    // random capacity <= 0.75 u_avg
    auto cap = 1.5 * ( u_avg - u_min ) * drand48() + u_min;

    auto arc = mMCFB->add_arc( sn , en , cst , cap );
    if( arc != mcf->AddArc( sn , en , cap , cst ) )
     diffarcs = false;

    if( arc >= m )
     ++afterend;

    if( mMCFB->get_NArcs() >= mMCFB->get_MaxNArcs() )
     break;
    }

   if( changed ) {
    cout << "create " << changed << "(" << afterend << ")";
    if( diffarcs )
     cout << "[d]";
    cout << " - ";
    }
   }
   !!*/

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
