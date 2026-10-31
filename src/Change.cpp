/*--------------------------------------------------------------------------*/
/*--------------------------- File Change.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Implementation of the classes defined in Change.h: everything is defined
 * in the header, only the factory registration of the concrete GroupChange
 * lives here.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Filippo Magi \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni, Filippo Magi, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*---------------------------- IMPLEMENTATION ------------------------------*/
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Change.h"

/*--------------------------------------------------------------------------*/
/*------------------------- NAMESPACE AND USING ----------------------------*/
/*--------------------------------------------------------------------------*/

using namespace SMSpp_di_unipi_it;

/*--------------------------------------------------------------------------*/
/*----------------------------- STATIC MEMBERS -----------------------------*/
/*--------------------------------------------------------------------------*/

// register GroupChange to the Change factory

SMSpp_insert_in_factory_cpp_0( GroupChange );

/*--------------------------------------------------------------------------*/
// the factory is defined here, and not in the header, so that there is one
// of it also when the libraries are DLLs of Windows, each of which would
// otherwise have a copy of its own

Change::ChangeFactoryMap & Change::f_factory( void )
{
 static ChangeFactoryMap c_factory;
 return( c_factory );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Change.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
