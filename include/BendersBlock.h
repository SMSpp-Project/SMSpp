/*--------------------------------------------------------------------------*/
/*------------------------ File BendersBlock.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the BendersBlock class, which derives from Block and has
 * the following characteristics. It has a vector of ColVariable and an
 * FRealObjective whose Function is a BendersBFunction whose active Variable
 * are the ones defined in this BendersBlock.
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __BendersBlock
#define __BendersBlock
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "BendersBFunction.h"
#include "Block.h"
#include "ColVariable.h"
#include "FRealObjective.h"

/*--------------------------------------------------------------------------*/
/*----------------------------- NAMESPACE ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
/*------------------------- CLASS BendersBlock -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a Block whose FRealObjective has a BendersBFunction
/** A BendersBlock is a Block whose Objective is an FRealObjective whose
 * Function is a BendersBFunction. Moreover, it has a vector of ColVariable
 * which are the active Variable of that BendersBFunction.
 */

class BendersBlock : public Block {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

public:

/*--------------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING BendersBlock ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing BendersBlock
 *  @{ */

 /// constructor
 /** Constructs a BendersBlock whose father Block is \p father and that has \p
  * num_variables ColVariable.
  *
  * @param father A pointer to the father of this BendersBlock.
  *
  * @param num_variables The number of Variable of this BendersBlock. */

 BendersBlock( Block * father = nullptr , Index num_variables = 0 ) :
  Block( father ) {
  v_variables.resize( num_variables );
  set_objective( & objective , eNoMod );
  }

/*--------------------------------------------------------------------------*/
 /// destructor

 virtual ~BendersBlock() { objective.clear(); }

/*--------------------------------------------------------------------------*/
 /// loads the BendersBlock out of an istream: it is not implemented yet

 void load( std::istream & input , char frmt = 0  ) override {}

/*--------------------------------------------------------------------------*/
 /// deserialize a BendersBBlock out of netCDF::NcGroup
 /** The method takes a netCDF::NcGroup supposedly containing all the
  * information required to deserialize the BendersBlock. Besides the 'type'
  * attribute common to all :Block, it should contain:
  *
  * - The number of ColVariable of this BendersBlock into a dimension named
  *   "NumVar".
  *
  * - The sense of the Objective of this BendersBlock enconded into a
  *   dimension called "ObjectiveSense". If the size of this dimension is
  *   zero, then the sense of the Objective is maximization. Otherwise, if it
  *   is nonzero or this dimension is not provided, the sense of the Objective
  *   is minimization.
  *
  * - A description of the BendersBFunction into a sub-group named
  *   "BendersBFunction".
  *
  * @param group A netCDF::NcGroup holding the required data. */

 void deserialize( const netCDF::NcGroup & group ) override;

/** @} ---------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 /// sets the BendersBFunction of the FRealObjective of this BendersBlock
 /** This function sets the given \p function as the Function of the
  * FRealObjective of this BendersBlocks.
  *
  * @param function A pointer to the BendersBFunction which will be the
  *        function of the FRealObjective of this BendersBlock.
  *
  * @param issueMod This parameter indicates if and how the FRealObjectiveMod
  *        is issued, as described in Observer::make_par().
  *
  * @param deleteold This parameter indicates whether the previous Function of
  *        the FRealObjective of this BendersBlock must be deleted.
  */

 void set_function( BendersBFunction * function ,
                    c_ModParam issueMod = eModBlck ,
                    bool deleteold = true ) {
  objective.set_function( function , issueMod , deleteold );
 }

/** @} ---------------------------------------------------------------------*/
/*-------------- METHODS FOR Saving THE DATA OF THE BendersBlock -----------*/
/*--------------------------------------------------------------------------*/
/** @name Saving the data of the BendersBlock
 *  @{ */

 /// serialize a BendersBlock into a netCDF::NcGroup
 /** Serialize a BendersBlock into a netCDF::NcGroup, with the following
  * format:
  *
  * - The dimension "NumVar" containing the number of ColVariable of this
  *   BendersBlock.
  *
  * - The group "BendersBFunction" containing the description of the
  *   BendersBFunction that is the Function of the FRealObjective is this
  *   BendersBlock.
  *
  * @param group The netCDF group into which this BendersBlock will be
  *        serialized.
  */

 void serialize( netCDF::NcGroup & group ) const override;

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE BendersBlock ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the BendersBlock
    @{ */

 /// returns the sense of the Objective of this BendersBlock
 /** This function returns the sense of the Objective of this BendersBlock.
  *
  * @return the sense of the Objective of this BendersBlock. */

 int get_objective_sense() const override {
  return( objective.get_sense() );
 }

/*--------------------------------------------------------------------------*/

 /// Return the number of Variable of this BendersBlock
 /** This function returns the number of Variable of this BendersBlock.
  *
  * @return The number of Variable of this BendersBlock. */

 Index get_number_variables() const {
  return( v_variables.size() );
 }

/*--------------------------------------------------------------------------*/

 /// returns the vector of ColVariable of this BendersBlock
 /** This function returns a const reference to the vector of ColVariable of
  * this BendersBlock.
  *
  * @return A const reference to the vector of ColVariable of
  *         this BendersBlock. */

 const std::vector< ColVariable > & get_variables() const {
  return( v_variables );
 }

/*--------------------------------------------------------------------------*/

 /// returns a vector with the values of the ColVariable of this BendersBlock
 /** This function return a vector containing the values of the of ColVariable
  * of this BendersBlock.
  *
  * @return A vector containing the values of the of ColVariable
  *         of this BendersBlock. */

 std::vector< double > get_variable_values() const {
  std::vector< double > variable_values;
  variable_values.reserve( v_variables.size() );
  for( const auto & variable : v_variables )
   variable_values.push_back( variable.get_value() );
  return( variable_values );
 }

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS DESCRIBING THE BEHAVIOR OF A BendersBlock -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a BendersBlock
 *  @{ */

 /// sets the values of the Variable of this BendersBlock
 /** This function sets the values of the Variable defined in this
  * BendersBlock according to the given \p values. The size of the \p values
  * vector parameter must be at least the number of Variable defined in this
  * BendersBlock, so that the value of the i-th Variable will be values[ i ],
  * for each i in {0, ..., get_number_variables() - 1}.
  *
  * @param values The vector containing the values of the Variable.
  */
 template< class T >
 void set_variable_values( const std::vector< T > & values ) {
  assert( ( values.size() >= 0 ) &&
          ( static_cast< decltype( v_variables.size() ) >( values.size() ) ==
            v_variables.size() ) );
  for( Index i = 0 ; i < v_variables.size() ; ++i )
   v_variables[ i ].set_value( values[ i ] );
 }

/*--------------------------------------------------------------------------*/
 /// sets the values of the Variable of this BendersBlock
 /** This function sets the values of the Variable defined in this
  * BendersBlock according to the given \p values. The size of the \p values
  * array parameter must be at least the number of Variable defined in this
  * BendersBlock, so that the value of the i-th Variable will be values( i ),
  * for each i in {0, ..., get_number_variables() - 1}.
  *
  * @param values The Eigen::ArrayXd containing the values of the Variable. */

 void set_variable_values( const Eigen::ArrayXd & values ) {
  assert( ( values.size() >= 0 ) &&
          ( static_cast< decltype( v_variables.size() ) >( values.size() ) ==
            v_variables.size() ) );
  for( Index i = 0 ; i < v_variables.size() ; ++i )
   v_variables[ i ].set_value( values( i ) );
  }

/*--------------------------------------------------------------------------*/
 /// sets the values of the Variable of this BendersBlock
 /** This function sets the values of the Variable defined in this
  * BendersBlock according to the values given by the iterator \p it. The
  * number of successors (before the past-the-end iterator) of the given
  * iterator must be at least get_number_variables() - 1, so that the value of
  * the i-th Variable will be given by std::next( it , i ), for each i in {0,
  * ..., get_number_variables() - 1}.
  *
  * @param values The vector containing the values of the Variable. */

 template< class Iterator >
 void set_variable_values( Iterator it ) {
  for( Index i = 0 ; i < v_variables.size() ; ++i , std::advance( it , 1 ) )
   v_variables[ i ].set_value( * it );
  }

/** @} ---------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

protected:

/*--------------------------------------------------------------------------*/
/*---------------------------- PROTECTED METHODS ---------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*---------------------------- PROTECTED FIELDS ----------------------------*/
/*--------------------------------------------------------------------------*/

 /// The Objective of this BendersBlock
 FRealObjective objective;

 /// The Variable that are the active ones in the BendersBFunction
 std::vector< ColVariable > v_variables;

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE FIELDS --------------------------------*/
/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};   // end( class BendersBlock )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* BendersBlock.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File BendersBlock.h ---------------------------*/
/*--------------------------------------------------------------------------*/
