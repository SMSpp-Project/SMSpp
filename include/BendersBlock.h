/*--------------------------------------------------------------------------*/
/*------------------------ File BendersBlock.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the BendersBlock class, which derives from Block and has
 * the following characteristics. It has a vector of ColVariable and an
 * FRealObjective whose Function is a BendersBFunction whose active Variable
 * are the ones defined in this BendersBlock.
 *
 * \version 0.1
 *
 * \date 02 - 12 - 2019
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
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
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup BendersBlock_CLASSES Classes in BendersBlock.h
 *  @{ */

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
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
 *  @{ */


/**@} ----------------------------------------------------------------------*/
/*--------------- CONSTRUCTING AND DESTRUCTING BendersBlock ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing BendersBlock
 *  @{ */

 /// constructor
 /** Constructs a BendersBlock whose father Block is \p father and that has \p
  * num_variables ColVariable.
  *
  * @param
  */
 BendersBlock( Block * father = nullptr , Index num_variables = 0 ) :
  Block( father ) {
  variables.resize( num_variables );
 }

/*--------------------------------------------------------------------------*/
 /// destructor
 virtual ~BendersBlock() { }

/**@} ----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

 virtual void load( std::istream &input ) override {}

/*--------------------------------------------------------------------------*/

 /// deserialize a BendersBBlock out of netCDF::NcGroup
 /** The method takes a netCDF::NcGroup supposedly containing all the
  * information required to deserialize the BendersBlock, i.e., the number of
  * ColVariable of this BendersBlock into a dimension named "NumVar" and a
  * description of the BendersBFunction into a sub-group named
  * "BendersBFunction".
  *
  * @param group A netCDF::NcGroup holding the data in the format described
  *        in the comments of deserialize().
  */

 void deserialize( netCDF::NcGroup & group ) override;

/*--------------------------------------------------------------------------*/

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

/**@} ----------------------------------------------------------------------*/
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
  */

 virtual void serialize( netCDF::NcGroup & group ) const override;

/**@} ----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE DATA OF THE BendersBlock ------------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the BendersBlock
    @{ */

 /// Returns the number of Variable of this BendersBlock
 /** This function returns the number of Variable of this BendersBlock.
  */

 Index get_number_variables() const {
  return variables.size();
 }

/*--------------------------------------------------------------------------*/

 /// returns the vector of ColVariable of this BendersBlock
 /** Returns a const reference to the vector of ColVariable of this
  * BendersBlock.
  */

 const std::vector< ColVariable > & get_variables() const {
  return variables;
 }

/**@} ----------------------------------------------------------------------*/
/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/

protected:

/*--------------------------------------------------------------------------*/
/*---------------------------- PROTECTED FIELDS  ---------------------------*/
/*--------------------------------------------------------------------------*/

 /// The Objective of this BendersBlock
 FRealObjective objective;

 /// The Variable that are the active ones in the BendersBFunction
 std::vector< ColVariable > variables;

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;

/*--------------------------------------------------------------------------*/

};   // end( class BendersBlock )

/** @} end( group( BendersBlock_CLASSES ) ) */

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* BendersBlock.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File BendersBlock.h ---------------------------*/
/*--------------------------------------------------------------------------*/
