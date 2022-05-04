/*--------------------------------------------------------------------------*/
/*--------------------------- File Change.h --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* class Change. 
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Federica Di Pasquale \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni, Federica Di Pasquale
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Change
 #define __Change
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "SMSTypedefs.h"

/*--------------------------------------------------------------------------*/
/*------------------------------ NAMESPACE ---------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Change_CLASSES Classes in Change.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Change --------------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// 
/** The class Change ...
 */

class Change {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------- PUBLIC METHODS OF THE CLASS -----------------------*/

 Change( void ) = default;     ///< constructor: does nothing

 virtual ~Change() = default;  ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected *pure* virtual method print(). This way the
  * operator<<() is defined for each Change, but its behavior must be
  * customized by derived classes (since the base class has nothing to
  * print). */

 friend std::ostream & operator<<( std::ostream & out , const Change & b ) {
  b.print( out );
  return ( out );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Change out of a file
 /** Top-level de-serialization method */

 static Change * deserialize( const std::string & filename );

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Change out of an open netCDF SMS++ file
 /** Second-level de-serialization method*/

 static Change * deserialize( const netCDF::NcFile & f , int idx = 0 );

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Change out of an open netCDF SMS++ file
 /** Third-level de-serialization method*/

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Change out of an open netCDF SMS++ file
 /** Fourth-level de-serialization method*/

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR LOADING, PRINTING & SAVING THE Change -----------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
 /// serialize a Change to a netCDF file given the filename
 /** Method to serialize a Change to a file in netCDF-based SMS++-format */

 virtual void serialize( const std::string & filename ,
       int type = eProbFile ) const;

/*--------------------------------------------------------------------------*/
 /// serialize a Change to an open netCDF file
 /** Method to serialize a Change to an open netCDF file in netCDF-based 
  * SMS++-format. */

 virtual void serialize( netCDF::NcFile & f, int type ) const;

/*--------------------------------------------------------------------------*/
 /// serialize a Change to a netCDF NcGroup
 /** Method to serialize a Change to a netCDF NcGroup.
  *
  *      THIS IS THE METHOD TO BE IMPLEMENTED BY DERIVED CLASSES
  *
  */

 virtual void serialize( netCDF::NcGroup & group ) const;

/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// *pure virtual* method for allowing any Change to print itself
 /** *pure virtual* method intended to provide support for Changes to
  * print themselves out in human-readable form. The base Change class
  * does not have anything to print, and this method is precisely what makes
  * it an abstract base class. */

 virtual void print( std::ostream & output ) const = 0;

/*--------------------------------------------------------------------------*/

 };  // end( class( Change ) )

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif /* Change.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Change.h ----------------------------*/
/*--------------------------------------------------------------------------*/
