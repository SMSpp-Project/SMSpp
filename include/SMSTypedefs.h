/*--------------------------------------------------------------------------*/
/*------------------------ File SMSTypedefs.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file defining a bunch of data types that are useful in multiple
 * SMS++ classes, and therefore that it would be annoying to define as
 * public data types of some specific class. The file also includes the
 * headers with the other utilities used throughout SMS++, so that including
 * it gives all of them:
 *
 * - SMSFactory.h, the macros for easily using factories;
 *
 * - SMSStreams.h, printing (in the sense of operator<<()) and loading (in
 *   the sense of operator>>()) of boost::multi_array<>, std::list<>,
 *   std::vector<> and std::pair<>;
 *
 * - SMSnetCDF.h, the templates that simplify handling serialization and
 *   deserialization to/from a netCDF file, and the class NcMemFile, a
 *   netCDF file held in memory.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Rafael Durbano Lobato \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Rafael Durbano Lobato
 */
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __SMSTypedefs
 #define __SMSTypedefs
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

// standard C++ libraries (alphabetical order)
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <future>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <vector>

// boost libraries
#include <boost/multi_array.hpp>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/
/// namespace for the Structured Modeling System++ (SMS++)

namespace SMSpp_di_unipi_it
{
/*--------------------------------------------------------------------------*/
/*---------------------------- GENERAL TYPES -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup SMS_types General types useful in SMS++
 *
 * A few useful typedefs for types not directly tied to any of the major
 * classes of SMS++.
 * @{ */

typedef std::vector< std::string > Vec_string;
///< a vector of strings (std::string)

typedef Vec_string::iterator Vec_string_it;
///< iterator for a Vec_string

typedef const std::vector< std::string > c_Vec_string;
///< a const vector of strings (std::string)

typedef const c_Vec_string::const_iterator c_Vec_string_it;
///< iterator for a c_Vec_string

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// Inf< T >() = infinity value for T

template< typename T >
static constexpr T Inf( void ) noexcept {
 return( std::numeric_limits< T >::has_infinity ?
          std::numeric_limits< T >::infinity() :
          std::numeric_limits< T >::max() );
}

/*--------------------------------------------------------------------------*/
/// public enum for types of SMS++ netCDF files
/** Public enum for describing the different kinds of netCDF files that can
 * be read and produced by SMS++ objects (notably, Block, Configuration, and
 * Solution.
 *
 * There are four "basic" types of SMS++ netCDF files, corresponding to the
 * four values of this enum smspp_netCDF_file_type. Each file, when opened
 * in a netCDF::NcFile (which is also a netCDF::NcGroup), must have an int
 * netCDF attribute "SMS++_file_type" with one of the three values of the
 * enum. The structure of the corresponding files is:
 *
 * - eProbFile: the file (which is also a group) has any number of child
 *   groups with names "Prob_0", "Prob_1", ... In turn, each child group
 *   has exactly three child groups with names "Block", "BlockConfig" and
 *   "BlockSolver", respectively. The first is intended to contain the
 *   serialization of a :Block [see Block.h], the second the serialization
 *   of a :BlockConfig of the same :Block [see Block.h or RBlockConfig.h],
 *   and the third the serialization of a :BlockSolverConfig of the same
 *   :Block [see BlockSolverConfig.h], although any of the three can in
 *   principle be empty. If any of the child is not empty, it must
 *   necessarily contain a string attribute "type" containing the classname()
 *   of the corresponding :Block / :Configuration class, plus of course all
 *   the information necessary to reconstruct the specific instance. Note
 *   that sub-Block of the Block and sub-Configuration of the Configuration
 *   (if any) are assumed each to be contained into a child of the group
 *   containing the original :Block / :Configuration, recursively.
 *
 * - eBlockFile: the file (which is also a group) has any number of child
 *   groups with names "Block_0", "Block_1", ... Each child group contains
 *   the serialization of a :Block (the string attribute "type" and all the
 *   rest).
 *
 * - eConfigFile: the file (which is also a group) has any number of child
 *   groups with names "Config_0", "Config_1", ... Each child group contains
 *   the serialization of a :Configuration (the string attribute "type" and
 *   all the rest).
 *
 * - eSolutionFile: the file (which is also a group) has any number of child
 *   groups with names "Solution_0", "Solution_1", ... Each child group
 *   contains the serialization of a :Solution (the string attribute "type"
 *   and all the rest).
 *
 * - eStateFile: the file (which is also a group) has any number of child
 *   groups with names "State_0", "State_1", ... Each child group contains
 *   the serialization of a :State (the string attribute "type" and all the
 *   rest).
 *
 * The value eLastFileParam is provided if some :Block, :Configuration or
 * :Solution needs to read/write files with a specific structure. */

enum smspp_netCDF_file_type {
 eProbFile = 0 ,      ///< a "complete" file of both Block and Configuration
 eBlockFile = 1 ,     ///< a file of Block
 eConfigFile = 2 ,    ///< a file of Configuration
 eSolutionFile = 3 ,  ///< a file of Solution
 eStateFile = 4 ,     ///< a file of State
 eLastFileParam = 4   ///< first value available to define new file types
 };

/** @} end( group( SMS_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*----------------------- Variable-RELATED TYPES ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Variable_TYPES Variable-related types.
 *  @{ */

// forward definition of Variable
class Variable;

typedef Variable * p_Var;
///< a pointer to Variable (Variable *)

typedef std::vector< p_Var > Vec_p_Var;
///< a (1-D) vector of pointer to Variable

typedef const Vec_p_Var c_Vec_p_Var;
///< a (1-D) const vector of pointer to Variable

template< std::size_t K >
using KD_Vec_p_Var = boost::multi_array< p_Var , K >;
///< Vec_p_Var< K > is a K-D vector of pointer to Variable

template< std::size_t K >
using KD_c_Vec_p_Var = const boost::multi_array< p_Var , K >;
///< c_Vec_p_Var< K > is a const K-D vector of pointer to Variable

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

typedef std::list< p_Var > List_p_Var;
///< a list of pointer to Variable (Variable *)

typedef List_p_Var::iterator List_p_Var_it;
///< iterator for a List_p_Var

typedef const List_p_Var c_List_p_Var;
///< a const list of pointers to Variable

typedef c_List_p_Var::const_iterator c_List_p_Var_it;
///< iterator for a c_List_p_Var

typedef std::vector< List_p_Var > Vec_List_p_Var;
///< a 1-D array of lists of pointer to Variable

typedef const Vec_List_p_Var c_Vec_List_p_Var;
///< a const 1-D array of lists of Variable *

template< std::size_t K >
using KD_Vec_List_p_Var = boost::multi_array< List_p_Var , K >;
///< Vec_List_p_Var< K > is a K-D vector of lists of Variable *

template< std::size_t K >
using KD_c_Vec_List_p_Var = const boost::multi_array< List_p_Var , K >;
///< c_Vec_List_p_Var< K > is a const K-D vector of lists of Variable *


/** @} end( group( Variable_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*--------------------- Constraint-RELATED TYPES ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Constraint_TYPES Constraint-related types.
 *  @{ */

// forward definition
class Constraint;

typedef Constraint * p_Const;
///< a pointer to Constraint (Constraint *)

typedef const p_Const c_p_Const;
///< a const pointer to Constraint

typedef std::vector< p_Const > Vec_p_Const;
///< a (1-D) vector of pointer to Constraint

typedef const Vec_p_Const c_Vec_p_Const;
///< a (1-D) const vector of pointer to Constraint

template< std::size_t K >
using KD_Vec_p_Const = boost::multi_array< p_Const , K >;
///< Vec_p_Const< K > is a K-D vector of pointer to Constraint

template< std::size_t K >
using KD_c_Vec_p_Const = const boost::multi_array< p_Const , K >;
///< c_Vec_p_Const< K > is a const K-D vector of pointer to Constraint

typedef std::list< p_Const > List_p_Const;
///< a list of pointers to Constraint (Constraint *)

typedef List_p_Const::iterator List_p_Const_it;
///< iterator for a List_p_Const

typedef const List_p_Const c_List_p_Const;
///< a const list of Constraint *

typedef c_List_p_Const::const_iterator c_List_p_Const_it;
///< iterator for a c_List_p_Const

typedef std::vector< List_p_Const > Vec_List_p_Const;
///< a 1-D array of lists of Constraint *

typedef const Vec_List_p_Const c_Vec_List_p_Const;
///< a const 1-D array of lists of Constraint *

template< std::size_t K >
using KD_Vec_List_p_Const = boost::multi_array< List_p_Const , K >;
///< Vec_List_p_Const< K > is a K-D vector of lists of Constraint *

template< std::size_t K >
using KD_c_Vec_List_p_Const = const boost::multi_array< List_p_Const , K >;
///< c_Vec_List_p_Const< K > is a const K-D vector of lists of Constraint *

/** @} end( group( Constraint_TYPES ) ) */

} // end( namespace SMS_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*------------------------- INCLUDES OF THE PIECES -------------------------*/
/*--------------------------------------------------------------------------*/

#include "SMSFactory.h"
#include "SMSStreams.h"
#include "SMSnetCDF.h"

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* SMSTypedefs.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SMSTypedefs.h ---------------------------*/
/*--------------------------------------------------------------------------*/
