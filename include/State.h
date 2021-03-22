/*--------------------------------------------------------------------------*/
/*----------------------------- File State.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the State class, which is intended to represent a state of
 * a ThinComputeInterface. In general, a state of a ThinComputeInterface can
 * be composed by any data that can influence the computation of this
 * ThinComputeInterface. It can be anything that largely determines the result
 * of the next call to ThinComputeInterface::compute().
 *
 * \version 0.01
 *
 * \date 22 - 03 - 2021
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __State
#define __State  /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "netcdf"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------- State-RELATED TYPES ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup State_TYPES State-related types
 *  @{ */


/** @}  end( group( State_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup State_CLASSES Classes in State.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS State ---------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a state of a ThinComputeInterface
/** The State class is intended to represent a state of a
 * ThinComputeInterface. In general, a state of a ThinComputeInterface can be
 * composed by any data that can influence the computation of this
 * ThinComputeInterface. It can be anything that largely determines the result
 * of the next call to ThinComputeInterface::compute(). The State of a
 * :ThinComputeInterface is entirely determined by each particular :State for
 * that :ThinComputeInterface and the general State class makes no provisions
 * on what a State can be or do, except that it can be serialized and
 * de-serialized.
 *
 * The main idea is that a State of a ThinComputeInterface should represent
 * anything that this ThinComputeInterface can use to better restart its
 * computation considering the current state of the system. Some of the
 * possible uses of State are the following:
 *
 * - To allow a controlled interruption a complex computation process
 *   (computing, changing, re-computing, re-changing, ...), by saving the
 *   current state on file, and recovering it later for restarting the
 *   process.
 *
 * - To allow checkpointing, where the State of a ThinComputeInterface is
 *   saved from time to time and can be used to restart the computation
 *   process in case the computation is abruptly interrupted for any reason.
 *
 * - To allow the improvement of performance in cases where some object (for
 *   instance, a :Block) is submitted to numerous modifications, but then
 *   these modifications are un-done and the computation starts with a new set
 *   of different modifications from the state that the :Block had "a long
 *   time ago". */

class State {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*@} -----------------------------------------------------------------------*/
/*------------------ CONSTRUCTING AND DESTRUCTING State --------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing State
 *  @{ */

 State( void ) { }  ///< constructor of State, it has nothing to do

/*--------------------------------------------------------------------------*/

 State( const State & ) = delete;  ///< inhibit copy constructor

/*--------------------------------------------------------------------------*/

 /// inhibit assignment operator
 State & operator=( const State & ) = delete;

/*--------------------------------------------------------------------------*/
 /// de-serialize a :State out of netCDF::NcGroup
 /** The method takes a netCDF::NcGroup supposedly containing all the
  * information required to de-serialize the :State, and produces a "full"
  * State object as a result. Most likely, the netCDF::NcGroup has been
  * produced by calling serialize() with a previously existing :State (of the
  * very same type as this one), but individual :State should openly declare
  * the format of their :State so that possibly a netCDF::NcGroup containing
  * some initial state can be constructed from scratch whenever this is
  * useful.
  *
  * This method is pure virtual, as it clearly has to be implemented by
  * derived classes. */

 virtual void deserialize( const netCDF::NcGroup & group ) = 0;

/*--------------------------------------------------------------------------*/

 virtual ~State() { }  ///< destructor: it is virtual, and empty

/*@} -----------------------------------------------------------------------*/
/*--------------- METHODS DESCRIBING THE BEHAVIOR OF A State ---------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a State
 *  @{ */

/*--------------------------------------------------------------------------*/
 /// serialize a :State into a netCDF::NcGroup
 /** The method takes a (supposedly, "full") State object and serializes
  * it into the provided netCDF::NcGroup, so that it can possibly be read by
  * deserialize() (of a :State of the very same type as this one).
  *
  * This method is pure virtual, as it clearly has to be implemented by
  * derived classes. */

 virtual void serialize( netCDF::NcGroup & group ) = 0;

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR LOADING, PRINTING & SAVING THE State ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for printing the State
 */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected virtual method print(). This way operator<<() is
  * defined for each State, but its behavior can be customized by derived
  * classes. */

 friend std::ostream& operator<<( std::ostream& out , const State &s ) {
  s.print( out );
  return( out );
  }

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

 /// print information about the State on an ostream
 /** Protected method intended to print information about the State; it is
  * virtual so that derived classes can print their specific information in
  * the format they choose. */

 virtual void print( std::ostream &output ) const {
  output << "State [" << this << "]";
  }

/*--------------------------------------------------------------------------*/

 };  // end( class( State ) )

/*@}  end( group( State_CLASSES ) ) ----------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* State.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File State.h -----------------------------*/
/*--------------------------------------------------------------------------*/
