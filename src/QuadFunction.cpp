/*--------------------------------------------------------------------------*/
/*------------------------ File DQuadFunction.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the class QuadFunction, which implements
 * C15Function with a non-diagonal quadratic function.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Wim van Ackooij \n
 *         EDF Lab Paris-Saclay \n
 *
 *
 * \copyright &copy; by Antonio Frangioni, Wim van Ackooij
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "QuadFunction.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------- METHODS DESCRIBING THE BEHAVIOR OF THE QuadFunction -----------*/
/*--------------------------------------------------------------------------*/

int QuadFunction::compute( bool changedvars )
{
  DQuadFunction::compute( changedvars );
  f_value = DQuadFunction::f_value;
  if( changedvars ) {
    // Compute the off-diagonal
    for (int k=0; k < mat_nd.outerSize(); ++k){
      for (Qmat::InnerIterator it(mat_nd,k); it; ++it){
        auto variable_value1 = std::get< 0 >( DQuadFunction::v_triples[it.row() ] )->get_value();
        auto variable_value2 = std::get< 0 >( DQuadFunction::v_triples[it.col() ] )->get_value();
      
        f_value += variable_value1 * variable_value2 * it.value();
      }
    }
  }
  return( kOK );
 }

/*--------------------------------------------------------------------------*/

bool QuadFunction::is_convex( void ) 
{
  identify_convexity();
  switch(my_convexity){
    case Convex:
      return( true );
      break;
    case Concave:
      return( false );
      break;
    case NotConvex:
      return( false );
      break;
    case Unknown:
      throw( std::logic_error( "At this stage we should know what function this is" ) );
      break;
    default:
      throw( std::logic_error( "We can not be in this state" ) );
      break;
  }
 }

/*--------------------------------------------------------------------------*/

bool QuadFunction::is_concave( void ) 
{
  identify_convexity();
  switch(my_convexity){
    case Convex:
      return( false );
      break;
    case Concave:
      return( true );
      break;
    case NotConvex:
      return( false );
      break;
    case Unknown:
      throw( std::logic_error( "At this stage we should know what function this is" ) );
      break;
    default:
      throw( std::logic_error( "We can not be in this state" ) );
      break;
  }
 }

/*--------------------------------------------------------------------------*/

bool QuadFunction::is_linear( void ) 
{
  return( DQuadFunction::is_linear() && ( mat_nd.nonZeros()==0 ) );
}

/*--------------------------------------------------------------------------*/

void QuadFunction::get_hessian_approximation( SparseHessian & hessian ) const {
  int num_active_var = DQuadFunction::get_num_active_var();

  std::vector< Eigen::Triplet< FunctionValue > > tripletList;
  tripletList.reserve( num_active_var + 2*mat_nd.nonZeros() );
  int index = 0;
  for( const auto & triple : v_triples )
    tripletList.push_back(
    Eigen::Triplet< FunctionValue >( index , index, 2 * std::get< 2 >( triple )
            ) );
  
  // now shove in the off_diagonal terms
  for (int k=0; k < mat_nd.outerSize(); ++k){
      for (Qmat::InnerIterator it(mat_nd,k); it; ++it){
        tripletList.push_back( Eigen::Triplet< FunctionValue >( it.row(), it.col(), it.value() ) );
            
        tripletList.push_back( Eigen::Triplet< FunctionValue >( it.col(), it.row(), it.value() ) );
      }
  }
  hessian.setZero();
  hessian.reserve( Eigen::VectorXi::Constant( num_active_var + 2*mat_nd.nonZeros(), 1 ) );
  hessian.setFromTriplets( tripletList.begin() , tripletList.end() );
 }

/*--------------------------------------------------------------------------*/

void QuadFunction::get_hessian_approximation( DenseHessian & hessian ) const
{
  int num_active_var = DQuadFunction::get_num_active_var();
  hessian.setZero( num_active_var , num_active_var );
  int index = 0;
  for( const auto & triple : v_triples ) {
    hessian( index , index ) = 2 * std::get< 2 >( triple );
    index++;
  }
  // 
  for (int k=0; k < mat_nd.outerSize(); ++k){
    for (Qmat::InnerIterator it(mat_nd,k); it; ++it){
      hessian( it.row(), it.col() ) = it.value();
      hessian( it.col(), it.row() ) = it.value();
    }
  }
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*----- METHODS FOR HANDLING "ACTIVE" Variable IN THE QuadFunction --------*/
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR MODIFYING THE QuadFunction -------------------*/
/*--------------------------------------------------------------------------*/


void QuadFunction::add_variables( v_coeff_triple && vars , 
                    v_off_diag_term && v_nd_var , 
                     ModParam issueMod ){
  
  // It is probably best to manage adding of variable all at this level and 
  // not rely too much on parent functionalities
  //
  if( vars.empty() && v_nd_var.empty() )  // actually nothing to add
    return;            // cowardly (and silently) return

  // Variables that have to be added
  auto added = & vars;
  Index k = DQuadFunction::get_num_active_var();
  if( k )    // adding to a nonempty set
    DQuadFunction::v_triples.insert( v_triples.end() , vars.begin() , 
                                      vars.end() );
  else {     // adding to nothing
    DQuadFunction::v_triples = std::move( vars );
    added = & v_triples;
  }

  // Upgrade the dimensions of the matrix
  int new_sz = k + added->size();
  mat_nd.conservativeResize(new_sz, new_sz);
  if ( !v_nd_var.empty() ){
    mat_nd.makeCompressed();
    mat_nd.reserve( mat_nd.nonZeros() + v_nd_var.size() );
  }

  // Now working with the non-diagonal terms
  for (int i=0; i < v_nd_var.size(); ++i ){
    Index ir = std::get< 0 >( v_nd_var[i] );
    Index ic = std::get< 1 >( v_nd_var[i] );

    if ( std::max(ir,ic) >= new_sz ){
      throw( std::logic_error( "Cross terms must belong to the set of variables" ) );
    }
    if ( ic == ir ){
      throw( std::logic_error( "Diagonal terms are not to be specified in this way" ) );
    }

    // Insert in the lower diagonal only ; 
    // Double specification will lead to an error, likely thrown by Eigen
    mat_nd.insert( std::max(ir,ic), std::min(ir,ic) ) = std::get< 2 >( v_nd_var[i] );
  }

  // if noone is there or not listening
  if( ( ! DQuadFunction::f_Observer ) || 
        ( ! DQuadFunction::f_Observer->issue_mod( issueMod ) ) )
    return;

  // Firstly prepare the diagonal terms for modification
  Vec_p_Var vptr( added->size() );
  v_coeff_pair vcoef( added->size() );
  for( Index i = 0 ; i < added->size() ; ++i ) {
   vptr[ i ] = std::get< 0 >( (*added)[ i ] );
   vcoef[ i ].first = std::get< 1 >( (*added)[ i ] );
   vcoef[ i ].second = std::get< 2 >( (*added)[ i ] );
  }

  // a diagonal quadratic function is additive ==> strongly quasi-additive
  DQuadFunction::f_Observer->add_Modification( 
              std::make_shared< QuadFunctionModVarsAddd >(
                this , std::move( v_nd_var ) , std::move( vcoef ) , 
                std::move( vptr ) , k , 0 , Observer::par2concern( issueMod ) ) ,
              Observer::par2chnl( issueMod ) );

  my_convexity = Unknown;
}  // end( QuadFunction::add_variables )

/*--------------------------------------------------------------------------*/

void QuadFunction::add_nd_term ( ColVariable * var1 , ColVariable * var2 ,
                    Coefficient quad_coeff , ModParam issueMod ){
  // We will check if both variables exists in which case the coefficient gets 
  // added. (Maybe we want a numeric zero check...)
  if ( quad_coeff != 0.0 ){
    Index i = DQuadFunction::is_active( var1 );
    Index j = DQuadFunction::is_active( var2 );

    if ( ( std::min(i,j) < 0 ) || 
            ( std::max(i,j) >= DQuadFunction::get_num_active_var() ) ){
      throw( std::logic_error( "Only non-diagonal coefficients of existing "
        "variables can be specified" ) );
    }

    // Observe that this insertion is highly inefficient if done one coeff at 
    // a time
    mat_nd.insert( std::max(i,j), std::min(i,j) ) = quad_coeff;

    if ( ( ! DQuadFunction::f_Observer ) || 
            ( ! DQuadFunction::f_Observer->issue_mod( issueMod ) ) )
      return;  // noone is there: all done

    Coefficient od_term = quad_coeff;
    Subset var_idxs = { std::max(i,j), std::min(i,j) };
    Vec_p_Var vars = { var1 , var2 };

    DQuadFunction::f_Observer->add_Modification( 
                            std::make_shared< QuadFunctionModSbst >(
                              this , C05FunctionMod::AllLinearizationChanged ,
                              std::move( od_term ) , true ,
                              std::move( vars ) , std::move( var_idxs ) ,
                              true , Subset( {} ) , FunctionMod::NaNshift ,
                              Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );

    my_convexity = Unknown;
  }
}

/*--------------------------------------------------------------------------*/

void QuadFunction::modify_term( Index i , Index j , 
                                Coefficient quad_nd_coeff , 
                                ModParam issueMod ){
  if ( ( std::min(i,j) < 0 ) || 
          ( std::max(i,j) >= DQuadFunction::get_num_active_var() ) ){
      throw( std::invalid_argument( "QuadFunction::modify_term: invalid "
                                "index: " + std::to_string( i ) + " , " + 
                                std::to_string( j ) ) );
  }
  // Observe that this insertion is highly inefficient if done one coeff at a time
  mat_nd.coeffRef( std::max(i,j), std::min(i,j) ) = quad_nd_coeff;

  my_convexity = Unknown;

  if( ( ! DQuadFunction::f_Observer ) || 
        ( ! DQuadFunction::f_Observer->issue_mod( issueMod ) ) )
    return;  // noone is there: all done

  Coefficient od_term = quad_nd_coeff;
  Subset var_idxs = { std::max(i,j), std::min(i,j) };
  Vec_p_Var vars = { std::get< 0 >( DQuadFunction::v_triples[ std::max(i,j) ] ) , 
                    std::get< 0 >( DQuadFunction::v_triples[ std::min(i,j) ] ) };

  DQuadFunction::f_Observer->add_Modification( 
                            std::make_shared< QuadFunctionModSbst >(
                              this , C05FunctionMod::AllLinearizationChanged ,
                              std::move( od_term ) , false ,
                              std::move( vars ) , std::move( var_idxs ) ,
                              true , Subset( {} ) , FunctionMod::NaNshift ,
                              Observer::par2concern( issueMod ) ) ,
                            Observer::par2chnl( issueMod ) );

 }  // end( QuadFunction::modify_term )

/*--------------------------------------------------------------------------*/

void QuadFunction::remove_variable( Index i , ModParam issueMod ){

  if( DQuadFunction::get_num_active_var() <= i )
    throw( std::logic_error( "less than i Variable are active" ) );

  Qmat mat_nd_tmp( DQuadFunction::get_num_active_var() - 1, 
        DQuadFunction::get_num_active_var() - 1 );
  mat_nd_tmp.reserve( mat_nd.nonZeros() ); 

  for (int k=0; k < mat_nd.outerSize(); ++k){
    for (Qmat::InnerIterator it(mat_nd,k); it; ++it){
      if ( it.row() < i ){
        // We are storing the lower diagonal only so always it.col() < it.row() !      
        mat_nd_tmp.insert( it.row(), it.col() ) = it.value();
      }
      else if (it.row() > i ){
        if ( it.col() < i ){
          mat_nd_tmp.insert( it.row()-1, it.col() ) = it.value();
        }
        else if (it.col() > i ){
          mat_nd_tmp.insert( it.row()-1, it.col() - 1 ) = it.value();
        }
      }
    }
  }
  // We Squeez every bit of memory out of the beast before reallocating the whole deal
  mat_nd.setZero();
  mat_nd.data().squeeze();

  mat_nd.resize( DQuadFunction::get_num_active_var() - 1, 
                  DQuadFunction::get_num_active_var() - 1 );
  mat_nd.reserve( mat_nd_tmp.nonZeros() );
  
  // Copy the beast around:
  mat_nd = mat_nd_tmp;

  my_convexity = Unknown;

  // Now gun down the diagonal term
  DQuadFunction::remove_variable(i, issueMod);

}  // end( QuadFunction::remove_variable( index ) )

/*--------------------------------------------------------------------------*/

void QuadFunction::remove_variables( Range range, ModParam issueMod ){
  
  range.second = std::min( range.second , Index( v_triples.size() ) );
  if( range.second <= range.first )
    return;

  my_convexity = Unknown;

  if( ( range.first == 0 ) && ( range.second >= v_triples.size() ) ) {
    // removing *all* variable
    mat_nd.setZero();
    mat_nd.data().squeeze();

    if( DQuadFunction::f_Observer && DQuadFunction::f_Observer->issue_mod( issueMod ) ) {
    // an Observer is there: copy the names of deleted Variable (all of them)
    Vec_p_Var vars( DQuadFunction::v_triples.size() );

    for( Index i = 0 ; i < DQuadFunction::v_triples.size() ; ++i )
      vars[ i ] = std::get< 0 >( DQuadFunction::v_triples[ i ] );

    DQuadFunction::clear();  // then clear the DQuadFunction

    // now issue the Modification: note that the subset is empty
    // a diagonal quadratic function is additive ==> strongly quasi-additive
    if( DQuadFunction::f_Observer && DQuadFunction::f_Observer->issue_mod( issueMod ) )
      DQuadFunction::f_Observer->add_Modification( std::make_shared< C05FunctionModVarsSbst >(
                                    this , std::move( vars ) , Subset() , true ,
                                    0 , Observer::par2concern( issueMod ) ) ,
                                    Observer::par2chnl( issueMod ) );
    }
    else       // no-one is listening
      DQuadFunction::clear();  // just do it
      return;    // all done
    }

    // this is not a complete reset
    const auto strtit = v_triples.begin() + range.first;
    const auto stopit = v_triples.begin() + range.second;

    // How many are gunned down:
    int remIdx = range.second - range.first + 1;

    // As before, but more efficient to some degree, since we avoid too many re-allocs
    Qmat mat_nd_tmp( DQuadFunction::get_num_active_var() - remIdx, DQuadFunction::get_num_active_var() - remIdx );
    mat_nd_tmp.reserve( mat_nd.nonZeros() ); 

    for (int k=0; k < mat_nd.outerSize(); ++k){
      for (Qmat::InnerIterator it(mat_nd,k); it; ++it){
        if ( it.row() < range.first ){
          // We are storing the lower diagonal only so always it.col() < it.row() !      
          mat_nd_tmp.insert( it.row(), it.col() ) = it.value();
        }
        else if (it.row() > range.second ){
          if ( it.col() < range.first ){
            mat_nd_tmp.insert( it.row()-remIdx, it.col() ) = it.value();
          }
          else if (it.col() > range.second ){
            mat_nd_tmp.insert( it.row()-remIdx, it.col() - remIdx ) = it.value();
          }
        }
      }
    }
    // We Squeez every bit of memory out of the beast before reallocating the whole deal
    mat_nd.setZero();
    mat_nd.data().squeeze();

    //
    mat_nd.resize( DQuadFunction::get_num_active_var() - remIdx, DQuadFunction::get_num_active_var() - remIdx );
    mat_nd.reserve( mat_nd_tmp.nonZeros() );
    
    // Copy the beast around:
    mat_nd = mat_nd_tmp;

    // Now deal with the native variables

    if( DQuadFunction::f_Observer && DQuadFunction::f_Observer->issue_mod( issueMod ) ) {
      // somebody is there: meanwhile, prepare data for the Modification

      Vec_p_Var vars( range.second - range.first );
      auto vpit = vars.begin();
      for( auto tmpit = strtit ; strtit < stopit ; )
      *(vpit++) = std::get< 0 >( *(tmpit++) );

      DQuadFunction::v_triples.erase( strtit , stopit );

      // now issue the Modification
      // a diagonal quadratic function is additive ==> strongly quasi-additive
      DQuadFunction::f_Observer->add_Modification( std::make_shared< C05FunctionModVarsRngd >(
                                    this , std::move( vars ) , range , 0 ,
                                    Observer::par2concern( issueMod ) ) ,
                                    Observer::par2chnl( issueMod ) );
      }
    else  // noone is there: just do it
      DQuadFunction::v_triples.erase( strtit , stopit );

 }  // end( QuadFunction::remove_variables( range ) )

/*--------------------------------------------------------------------------*/

void QuadFunction::identify_convexity(){
    /**
       * 
       *  We need to perform the costly computation and figure out what kind of function this is
       * 
      */
      if ( my_convexity == Unknown ){      
        int sze = DQuadFunction::get_num_active_var();
        Eigen::MatrixXd Q(sze, sze);
        Q.setZero();
        for (int k=0; k < mat_nd.outerSize(); ++k){
          for (Qmat::InnerIterator it(mat_nd,k); it; ++it){
            Q( it.row(), it.col() ) = it.value();
            Q( it.col(), it.row() ) = it.value();
          }
        }
        for (int i=0; i < sze; ++i){
          Q(i,i) = DQuadFunction::get_quadratic_coefficient(i) * 2.0;
        }
        const auto ldlt = Q.ldlt();
        if ( ldlt.info() != Eigen::NumericalIssue && ldlt.isPositive() ) {
          my_convexity = Convex;
        }
        else if ( ldlt.info() != Eigen::NumericalIssue && ldlt.isNegative() ){
          my_convexity = Concave;
        }
        else if ( ldlt.info() == Eigen::NumericalIssue || ( !ldlt.isPositive() && !ldlt.isNegative() ) ){
          my_convexity = NotConvex;
        }
        else{
          throw( std::logic_error( "Symmetric matrices can not be something other than PSD, NSD or indefinite ... yet ..." ) );
        }
    }
  }


/*--------------------------------------------------------------------------*/
/*----------------------- End File QuadFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
