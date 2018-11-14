/*--------------------------------------------------------------------------*/
/*------------------------ File DQuadFunction.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the DQuadFunction class.
 *
 * \version 0.10
 *
 * \date 29 - 11 - 2017
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Department of Applied Mathematics \n
 *         State University of Campinas, Brazil \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato, Kostas
 * Tavlaridis-Gyparakis
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "DQuadFunction.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*--------------------------------- METHODS --------------------------------*/
/*--------------------------------------------------------------------------*/

void DQuadFunction::evaluate( void )
{
  reset();

  f_value = constant_term;  // value of the function
  for( const auto &triple : *v_variables ) {
    FunctionValue variable_value = std::get<0>(triple)->get_value();
    f_value += variable_value *
      (std::get<1>(triple) + std::get<2>(triple) * variable_value);
  }
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::throw_add_variables_modification
( v_var_coeff_coeff_triple *v_var ) {

  // create the modification
  std::shared_ptr<FunctionModificationVariables> modification =
    std::make_shared<FunctionModificationVariables>
    (this, FunctionModificationVariables::AddVar);

  for(const auto &triple : *v_var)
    modification->add_variable(std::get<0>(triple));

  delete v_var;

  // add the Modification
  f_Block->add_Modification(modification);
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::add_variables( v_var_coeff_coeff_triple *v_var ,
                                   const bool ordered )
{

 if( ! v_var ) // actually nothing to add
   return; // cowardly (and silently) return

 if( v_var->empty() ) { // actually nothing to add
   delete v_var;  // delete v_var, which is now our property, and return
   return;
 }

 Lipschitz_constant = Inf<double>(); // reset the Lipschitz constant

 if( ! ordered )
   std::sort( v_var->begin() , v_var->end() ,
              [](auto const &t1, auto const &t2) {
                return std::get<0>(t1) < std::get<0>(t2); } );

 if( ( ! v_variables ) || v_variables->empty() ) {      // adding to nothing

  if( v_variables )
    delete v_variables;

  if( ( ! f_Block ) || ( ! f_Block->anyone_there() ) )  // and noone is there
   v_variables = v_var;         // just use what is provided
  else {                        // some Solver is listening
   v_variables = new v_var_coeff_coeff_triple( *v_var );  // copy the variables


   this->throw_add_variables_modification( v_var );
  }
 }
 else {                         // adding to a nonempty set
  v_var_coeff_coeff_triple *v_union =
    new v_var_coeff_coeff_triple( v_var->size() + v_variables->size() );
  auto newit = v_var->begin();
  auto oldit = v_variables->begin();
  auto unionit = v_union->begin();
  for( ; ( newit != v_var->end() ) && ( oldit != v_variables->end() ) ; ) {
    if( std::get<0>(*newit) == std::get<0>(*oldit) )
    throw( std::invalid_argument(
            "add_variables: some variable is already in the Function" ) );

   if( std::get<0>(*newit) < std::get<0>(*oldit) )
    *(unionit++) = *(newit++);
   else
    *(unionit++) = *(oldit++);
   }

  for( ; newit != v_var->end() ; )
   *(unionit++) = *(newit++);

  for( ; oldit != v_variables->end() ; )
   *(unionit++) = *(oldit++);

  delete v_variables;

  v_variables = v_union;

  if( ( ! f_Block ) || ( ! f_Block->anyone_there() ) ) {
   delete v_var;  // no Solver is listening anyway: delete v_var now
  }
  else {
    this->throw_add_variables_modification( v_var );
  }
 }
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::add_variable( ColVariable * const variable ,
                                  const Coefficient linear_coefficient ,
                                  const Coefficient quadratic_coefficient ) {
  v_var_coeff_coeff_triple *v_var = new v_var_coeff_coeff_triple;
  v_var->push_back(std::make_tuple(variable, linear_coefficient,
                                   quadratic_coefficient));
  this->add_variables(v_var, true);
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::throw_delete_variable_modification
( std::vector<Variable *> *v_var ) {

  // create the modification
  std::shared_ptr<FunctionModificationVariables> modification =
    std::make_shared<FunctionModificationVariables>
    (this, FunctionModificationVariables::RemoveVar);

  for(const auto & var : * v_var)
    modification->add_variable(var);

  delete v_var;

  // add the Modification
  f_Block->add_Modification(modification);
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::delete_variables( std::vector<Variable *> *v_var ,
                                      const bool ordered ,
                                      const bool throwModification )
{

 if( ! v_var ) // actually nothing to delete
   return; // cowardly (and silently) return

 if( v_var->empty() ) { // actually nothing to delete
   delete v_var;  // delete v_var, which is now our property, and return
   return;
 }

 if( ( ! v_variables ) || v_variables->empty() )  // deleting from nothing
  throw( std::logic_error( "delete_variables: deleting from an empty set" ) );

 Lipschitz_constant = Inf<double>(); // reset the Lipschitz constant

 if( ! ordered )
   std::sort( v_var->begin() , v_var->end() );

 auto it = v_var->begin();
 auto itv = v_variables->begin();

 // search the first variable to be eliminated
 itv = std::find_if( itv , v_variables->end() ,
                     [ &it ]( const auto &p )
                     { return( std::get<0>(p) == *it ); } );

 if( itv == v_variables->end() )  // if the variable is not there
  throw( std::invalid_argument(
         "delete_variables: some variable is not part of the function" ) );

 // TODO It should take advantage of the fact that the vectors are
 // sorted.

 auto curr = itv;  // record position where to move stuff
 ++it;             // skip the first elements
 ++itv;            // as they have been processed already
 for( ; it != v_var->end() ; ++itv ) {
  if( *it < std::get<0>(*itv) )
   throw( std::invalid_argument(
      "modify_coefficients: some variable is not part of the function" ) );

  if( *it == std::get<0>(*itv) ) // one element to be eliminated
   ++it;                         // skip it
  else
   *(curr++) = *itv;              // move in the current position
  }

 for( ; itv != v_variables->end() ; )  // copy the last part
  *(curr++) = *(itv++);                // after the last of v_var

 v_variables->erase( curr , itv );     // erase the last part

 if( ( ! f_Block ) || ( ! f_Block->anyone_there() ) ) {
  delete v_var;  // no Solver is listening anyway: delete v_var now
  return;        // all done
 }

 if(throwModification)
   throw_delete_variable_modification( v_var );
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::delete_variable( Variable * variable ,
                                     const bool throwModification ) {
  auto v_var = new std::vector<Variable *>;
  v_var->push_back(variable);
  this->delete_variables(v_var, true, throwModification);
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::throw_modify_coefficients_modification
( v_var_coeff_coeff_triple * v_var ) {

  // create the modification
  std::shared_ptr<C05FunctionModificationVariables> modification =
    std::make_shared<C05FunctionModificationVariables>
    (this, C05FunctionModificationVariables::SubgradientEntriesChange);

  for(const auto & triple : * v_var) {
    modification->add_variable(std::get<0>(triple));
  }

  delete v_var;

  // add the Modification
  f_Block->add_Modification(modification);
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::modify_coefficients( v_var_coeff_coeff_triple *v_var ,
                                         const bool ordered )
{

 if( ! v_var ) // actually nothing to add
   return; // cowardly (and silently) return

 if( v_var->empty() ) { // actually nothing to add
   delete v_var;  // delete v_var, which is now our property, and return
   return;
 }

 if( ( ! v_variables ) || v_variables->empty() )  // modifying nothing
  throw( std::logic_error( "modify_coefficients: modifying an empty set" ) );

 Lipschitz_constant = Inf<double>(); // reset the Lipschitz constant

 if( ! ordered )
   std::sort( v_var->begin() , v_var->end() ,
              [](auto const &t1, auto const &t2) {
                return std::get<0>(t1) < std::get<0>(t2); } );

 // TODO It should take advantage of the fact that the vectors are
 // sorted.

 auto itv = v_variables->begin();
 for( auto it = v_var->begin() ; it != v_var->end() ; ++it , ++itv ) {
  // look for position of next variable to be modified

  itv = std::find_if( itv , v_variables->end() ,
                     [ &it ]( const auto &p )
                     { return( std::get<0>(p) == std::get<0>(*it) ); } );

  if( itv == v_variables->end() ) // if the variable is not there
    throw( std::invalid_argument( // TODO Maybe the variable *is*
                                  // there, but the variable appears
                                  // more than once in the (input)
                                  // v_var vector
      "modify_coefficients: some variable is not part of the function" ) );

  // modify the coefficients
  std::get<1>(*itv) = std::get<1>(*it);
  std::get<2>(*itv) = std::get<2>(*it);
 }

 if( ( ! f_Block ) || ( ! f_Block->anyone_there() ) ) {
  delete v_var;  // no Solver is listening anyway: delete v_var now
  return;        // all done
 }

 this->throw_modify_coefficients_modification( v_var );
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::throw_constant_term_modification
( FunctionValue old_constant_term, FunctionValue new_constant_term ) {

  // create the modification
  std::shared_ptr<FunctionModification> modification =
    std::make_shared<FunctionModification>
    (this, new_constant_term - old_constant_term);

  // add the Modification
  f_Block->add_Modification(modification);
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::set_constant_term( const FunctionValue constant_term ) {
  if(this->constant_term == constant_term) // actually nothing to change
    return; // cowardly (and silently) return

  FunctionValue old_constant_term = this->constant_term;
  this->constant_term = constant_term;

  if( f_Block && f_Block->anyone_there() ) { // some Solver is listening
    throw_constant_term_modification( old_constant_term, constant_term );
  }
}

/*--------------------------------------------------------------------------*/

Function::FunctionValue DQuadFunction::get_constant_term( void ) const {
  return this->constant_term;
}

/*--------------------------------------------------------------------------*/

double DQuadFunction::get_Lipschitz_constant() {

  if(Lipschitz_constant == Inf<double>()) {
    Lipschitz_constant = 0;
    // TODO It should receive an epsilon and compute a Lipschitz
    // constant over the ball centered at the current point with
    // radius epsilon.
  }

  return Lipschitz_constant;
}

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_convex( void ) const {
  for(const auto &triple : *v_variables) {
    if(std::get<2>(triple) < 0) return false;
  }
  return true;
}

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_concave( void ) const {
  for(const auto &triple : *v_variables) {
    if(std::get<2>(triple) > 0) return false;
  }
  return true;
}

/*--------------------------------------------------------------------------*/

bool DQuadFunction::is_linear( void ) const {
  for(const auto &triple : *v_variables)
    if(std::get<2>(triple) != 0)
      return false;
  return true;
}

/*--------------------------------------------------------------------------*/

bool DQuadFunction::compute_new_subgradient( void ) {
  if(!subgradient_computed) {
    subgradient_computed = true;
    return true;
  }
  return false;
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_subgradient( DenseSubgradient &subgradient ) const {

  // Resize the subgradient if necessary.
  subgradient.resize(get_num_active_var());

  // Fill the subgradient.
  int index = 0;
  for(const auto &triple : *v_variables) {
    subgradient[index++] = std::get<1>(triple) +
      2 * std::get<2>(triple) * std::get<0>(triple)->get_value();
  }
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_hessian_approximation( SparseHessian &hessian ) const {

  int num_active_var = this->get_num_active_var();

  std::vector< Eigen::Triplet<FunctionValue> > tripletList;
  tripletList.reserve(num_active_var);

  int index = 0;
  for(const auto &triple : *v_variables)
    tripletList.push_back
      (Eigen::Triplet<FunctionValue>(index, index, 2 * std::get<2>(triple)));

  hessian.setZero();
  hessian.reserve(Eigen::VectorXi::Constant(num_active_var, 1));
  hessian.setFromTriplets(tripletList.begin(), tripletList.end());
}

/*--------------------------------------------------------------------------*/

void DQuadFunction::get_hessian_approximation( DenseHessian &hessian ) const {
  int num_active_var = get_num_active_var();
  hessian.setZero(num_active_var, num_active_var);
  int index = 0;
  for(const auto &triple : *v_variables) {
    hessian(index, index) = 2 * std::get<2>(triple);
    index++;
  }
}

/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*----------------------- End File DQuadFunction.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
