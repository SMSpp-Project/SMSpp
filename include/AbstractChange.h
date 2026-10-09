/*--------------------------------------------------------------------------*/
/*------------------------- File AbstractChange.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *concrete* class AbstractChange, a Change of the
 * abstract representation of a Block: the objective coefficient, the
 * integrality, the fixing or a bound of a ColVariable, or the sense of an
 * Objective, each identified by an AbstractPath.
 *
 * \author Filippo Magi \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Filippo Magi, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __AbstractChange
 #define __AbstractChange   /* self-identification: #endif at the end of the
                               file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Change.h"

#include "AbstractPath.h"

/*--------------------------------------------------------------------------*/
/*------------------------------ NAMESPACE ---------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup AbstractChange_CLASSES Classes in AbstractChange.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS AbstractChange ---------------------------*/
/*--------------------------------------------------------------------------*/
/// a Change of the abstract representation of a Block
/** The class AbstractChange derives from Change and represents a change of
 * the abstract representation of a Block: its type [see
 * AbstractChangeType], the data of the change and the AbstractPath of the
 * elements (ColVariable, Objective) it acts upon, relative to the Block the
 * Change is applied to. apply() makes the change through the methods of the
 * elements, issuing the corresponding abstract Modification, and with
 * doUndo returns the AbstractChange that undoes it.
 *
 * The data and the paths of each type are:
 *
 * - eChgObj: the paths of the ColVariable and of the (FRealObjective whose
 *   Function is the) objective, the new linear coefficient of the variable
 *   and, if the Function is a DQuadFunction, the new quadratic one; a
 *   variable not yet in the Function is added to it;
 *
 * - eChgSense: the path of the Objective and its new sense;
 *
 * - eChgIntegrality: the path of the ColVariable and 1 (integer) or 0;
 *
 * - eFixX: the path of the ColVariable and the value it is fixed to;
 *
 * - eUnfixX: the path of the ColVariable, and no data;
 *
 * - eChgLB, eChgUB: the path of the ColVariable and its new lower (upper)
 *   bound, which is written in the left (right) hand side of the first
 *   BoxConstraint or LBConstraint (UBConstraint) active on the variable;
 *   a variable with none of them is not supported (apply() throws).
 *
 * A derived class may handle some types by itself, or define new ones from
 * eLastACTtype onwards. */

class AbstractChange : public Change {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the types of AbstractChange
 enum AbstractChangeType {
  eEmpty = 0 ,      ///< empty change, used for initialization
  eChgObj ,         ///< change the objective coefficient of a variable
  eChgSense ,       ///< change the sense of the objective
  eChgIntegrality , ///< change the integrality of a variable
  eFixX ,           ///< fix a variable to a value
  eUnfixX ,         ///< unfix a variable
  eChgLB ,          ///< change the lower bound of a variable
  eChgUB ,          ///< change the upper bound of a variable
  eLastACTtype      ///< first allowed new type for derived classes
  };

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 /// constructor: an empty change

 AbstractChange( void ) : Change() , f_type( eEmpty ) {}

 /// constructor: the type, the data and the paths of the change

 AbstractChange( int type , std::vector< double > data ,
		 std::vector< AbstractPath > paths )
  : Change() , f_type( type ) , v_data( std::move( data ) ) ,
    v_paths( std::move( paths ) ) {}

 /// destructor: does nothing

 ~AbstractChange() override = default;

/*--------------------------------------------------------------------------*/
/*-------------- METHODS FOR LOADING, PRINTING & SAVING --------------------*/
/*--------------------------------------------------------------------------*/

 /// read the AbstractChange from the given netCDF NcGroup
 /** The format is that of serialize(): the attribute "AbstractChange_type"
  * with the type, the (optional) variable "Data" over the dimension "dim"
  * with the data, and the (optional) group "VariablesPath" with the paths
  * [see AbstractPath::vector_deserialize()]. */

 void deserialize( const netCDF::NcGroup & group ) override;

/*--------------------------------------------------------------------------*/
 /// write the AbstractChange into the given netCDF NcGroup [see deserialize()]

 void serialize( netCDF::NcGroup & group ) const override;

/*--------------------------------------------------------------------------*/
/*------------- METHODS FOR ADDING / REMOVING / CHANGING DATA --------------*/
/*--------------------------------------------------------------------------*/

 /// apply the AbstractChange to the given Block [see the class notes]

 Change * apply( Block * block , bool doUndo = false ,
		 ModParam issueMod = eNoBlck ,
		 ModParam issueAMod = eNoBlck ) override;

/*--------------------------------------------------------------------------*/
/*---------------------- METHODS FOR READING THE DATA ----------------------*/
/*--------------------------------------------------------------------------*/

 /// the type of the AbstractChange [see AbstractChangeType]

 [[nodiscard]] int get_type( void ) const { return( f_type ); }

 /// the data of the AbstractChange

 [[nodiscard]] const std::vector< double > & get_data( void ) const {
  return( v_data );
  }

 /// the paths of the elements the AbstractChange acts upon

 [[nodiscard]] const std::vector< AbstractPath > & get_paths( void ) const {
  return( v_paths );
  }

/*--------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS -----------------------------*/
/*--------------------------------------------------------------------------*/

 int f_type;                         ///< the type of the AbstractChange
 std::vector< double > v_data;       ///< the data of the AbstractChange
 std::vector< AbstractPath > v_paths;  ///< the paths of the elements

/*--------------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/

 SMSpp_insert_in_factory_h;  // insert AbstractChange in the Change factory

/*--------------------------------------------------------------------------*/

 };  // end( class( AbstractChange ) )

/** @} end( group( AbstractChange_CLASSES ) ) */

/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* AbstractChange.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File AbstractChange.h -------------------------*/
/*--------------------------------------------------------------------------*/
