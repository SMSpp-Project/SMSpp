/*--------------------------------------------------------------------------*/
/*------------------------ File SMSTypedefs.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file defining a bunch of data types that are useful in multiple
 * SMS++ classes, and therefore that it would be annoying to define as
 * public data types of some specific class. The file also provides:
 *
 * - some macros for easily using factories
 *
 * - some methods and macros for easily applying some operations to a
 *   boost::any in a way that is as much independent as possible to the shape
 *   of the content (individual/std::vector/boost::multi_array of [std::list]
 *   of [classes derived from] Variable/Constraint);
 *
 * - handles printing (in the sense of operator<<()) of boost::multi_array<>,
 *   std::list<> and std::vector<>;
 *
 * - some templates to simplify handling serialization and deserialization
 *   to/from a netCDF file;
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
#include <fstream>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

// boost libraries
#include <boost/any.hpp>
#include "boost/function.hpp"
#include "boost/functional/factory.hpp"
#include "boost/functional/forward_adapter.hpp"
#include <boost/multi_array.hpp>

// NetCDF
#include <netcdf>

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

typedef std::vector< boost::any > Vec_any;
///< a vector of boost::any, i.e., almost anything

typedef const std::vector< boost::any > c_Vec_any;
///< a const vector of boost::any, i.e., almost anything

typedef Vec_any::iterator Vec_any_it;
///< iterator for a Vec_any

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

typedef std::vector< std::string > Vec_string;
///< a vector of strings (std::string)

typedef Vec_string::iterator Vec_string_it;
///< iterator for a Vec_string

typedef const std::vector< std::string > c_Vec_string;
///< a const vector of strings (std::string)

typedef const c_Vec_string::const_iterator c_Vec_string_it;
///< iterator for a c_Vec_string

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
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
 * be read and produced by SMS++ objects (notably, Block and Configuration).
 *
 * There are three "basic" types of SMS++ netCDF files, corresponding to the
 * three values of this enum smspp_netCDF_file_type. Each file, when opened
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
 * The value eLastFileParam is provided if some :Block or :Configuration
 * needs to read/write files with a specific structure.
 */

enum smspp_netCDF_file_type
{
 eProbFile = 0 ,      ///< a "complete" file of both Block and Configuration
 eBlockFile = 1 ,     ///< a file of Block
 eConfigFile = 2 ,    ///< a file of Configuration
 eLastFileParam = 3   ///< first value available to define new file types
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

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

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
/*--------------------------------------------------------------------------*/
/*------------------------- UTILITIES FOR FACTORIES ------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup macro_for_factories Macros to simplify insertion in factories
 *
 * The five macros
 *
 *   SMSpp_insert_in_factory_h
 *
 *   SMSpp_insert_in_factory_cpp_0( < class name > )
 *
 *   SMSpp_insert_in_factory_cpp_1( < class name > )
 *
 *   SMSpp_insert_in_factory_cpp_0_t( < class name > )
 *
 *   SMSpp_insert_in_factory_cpp_1_t( < class name > )
 *
 * can be used to quickly ensure that < class name > is inserted in the
 * corresponding factory. As the name says, they have to be put respectively
 * in the private part of < class name > declaration in the .h, and in the
 * .cpp of < class name > (if any, in *exactly one* .cpp otherwise). The "_k"
 * versions of the _cpp macro refer to the fact that the constructor of the
 * base class has k parameters. The further "_t" versions need be used if the
 * class is template, since then a template specialization is needed and this
 * requires adding "template<>" at proper places.
 *
 * IMPORTANT NOTE: while the macros properly define the code in the object,
 * it has to be actually included in the final executable. If a component
 * using this feature is not *explicitly* referenced anywhere in all the
 * linked objects, the linker may decide to exclude it from the final
 * executable. This is particularly common when the unit containing the object
 * is contained in a dynamic library. In this case you may end up seeing an
 * error of the kind
 *
 *     terminate called after throwing an instance of 'std::invalid_argument'
 *      what():  XXXX not present in YYYY factory
 *
 * If this happens, it is because the code inserting it in the factory has not
 * been included due to having been aggressively optimised out by the linker.
 * In this case, one must either forcibly include an object of the specific
 * type in the final main (which requires also #include-ing the corresponding
 * headers, and it is therefore best avoided), or use options of the linker
 * that disable this optimization; at the time of writing this is
 * "--no-as-needed" for g++ (to be passed as "-Wl,--no-as-needed" if the
 * linker is invoked via the compiler), and "-all_load" for clang++. Note that
 * the behaviour depends on the default settings of the linker and therefore
 * it may change even for the same compiler toolchain on different OS
 * distributions. Yet, activating said options may have unwanted side effects
 * that one may want to do without. This is why the further macro
 *
 *   SMSpp_ensure_load( < class name > )
 *
 * is provided. By adding it at the beginning of a .cpp, it does some dirty
 * but hopefully not terribly costly things that ensure that < class name >
 * is added to the corresponding factory. However, note that for this to
 * work the corresponding headed must be explicitly included; it is not
 * possible to automatically include files using a macro due to the fact that
 * the C preprocessor only does one pass, and while there are some very dirty
 * hacks around that we prefer to steer well clear of those. Anyway this
 * mechanism should only be used as last resort since it has a (hopefully,
 * little but) nonzero cost at runtime.
 * @{ */

/** The macro defines a very small, "fake" class _init. Its only meaning is to
 * define a static member _initializer that is initialized in whatever object
 * the other macro SMSpp_insert_in_factory_cpp() is put as soon as the program
 * starts; when the constructor is called, it will register the class in the
 * f_factory of the appropriate base class, as well as calling the
 * static_initialization() method of the class (useful for any other static
 * initialization, such as registering methods in the methods factory).
 *
 * It also defines the private_name() method and the _private_name() static
 * method which is the implementation of private_name() (but it is static,
 * and therefore can be called in initialization statements, whereas
 * private_name() is virtual and therefore it can not). */

#define SMSpp_insert_in_factory_h                                            \
 static class _init { public:  _init(); } _initializer;                      \
 const std::string & private_name() const override;                          \
 static const std::string & _private_name()

namespace SMSpp_type_traits
{
/* The name of template classes may have commas, which prevent them to be
 * directly passed as arguments to the SMSpp_insert_in_factory_cpp_*_t
 * macros, as the comma is then interpreted by the tokenizer as separating
 * different arguments of the macro. To allow passing such names as
 * arguments to these macros, some extra template shenanigans are needed.
 *
 * The get_type struct is used within those macros to obtain the type U of a
 * (template) class whose name may have been enclosed in parentheses. The
 * type T is not relevant, as it is used only to obtain a well-formed type
 * name which contains parentheses. To obtain the right type of any class
 * "ClassName" within these macros, one can do the following:
 *
 *     SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type
 *
 * where ClassName is the parameter of the macro. For instance, the class
 *
 *     SimpleConfiguration< std::pair< int , int > >
 *
 * can be inserted in the factory as:
 *
 *     SMSpp_insert_in_factory_cpp_0_t( ( SimpleConfiguration<
 *                                        std::pair< int , int > > ) );
 *
 * There, ClassName would be
 *
 *     ( SimpleConfiguration< std::pair< int , int > > )
 *
 * Note that *not* using the "(" and ")" would result in the macro
 * invocation to fail. */

template< typename T >
struct t;
template< typename T , class U >
struct t< T( U ) >
{
 using type = U;
};
}

/*--------------------------------------------------------------------------*/
/// reformats the string in the way required by the class factory
/** This functions takes a std::string && str and returns the string that has
 * been stripped of the whitespaces and of all enclosing pair of parentheses
 * so that it can be inserted in the factory. This can be useful to check if
 * an object out of the factory is of a particular class as in
 *
 *     if( object.classname() == SMSpp_classname_normalise( name_string ) )
 *
 * in the case name_string is not guaranteed to be free of whitespaces and
 * parentheses. */

inline std::string && SMSpp_classname_normalise( std::string && str ) {
 str.erase( std::remove_if( str.begin() , str.end() , ::isspace ) ,
            str.end() );
 while( str.front() == '(' ) {
  str.pop_back();
  str.erase( 0 , 1 );
 }
 return( std::move( str ) );
}

/*--------------------------------------------------------------------------*/

/** The macros SMSpp_insert_in_factory_cpp_* do five things for the class
 * \p ClassName for which they are invoked:
 *
 * 1) the actual implementation of the ClassType::_init::_init( void )
 *    constructor, within which:
 *
 *    1.1) the class is inserted into the corresponding factory;
 *
 *    1.2) the static_initialization() method of the class is invoked;
 *
 * 2) the actual declaration of the ClassType::_initializer static object;
 *
 * 3) the actual implementation of the static ClassType::_private_name();
 *
 * 4) the actual implementation of virtual ClassType::private_name(), itself
 *    using ClassType::_private_name().
 *
 * The approach can be applied to any class deriving from a base class that
 * has the following (possibly, pure virtual) methods (that should be visible
 * to each of its derived classes, and therefore typically protected):
 *
 * - private_name(), defined e.g. as
 *   
 *      virtual const std::string & private_name( void ) const = 0;
 *
 * - f_factory(), defined e.g. as
 *
 *      static XXXFactoryMap & f_factory( void );
 *
 *   where
 *
 *       using XXXFactory = boost::function< XXX *( XXX * ) >;
 *       using XXXFactoryMap = std::map< std::string , XXXFactory >;
 *
 *    and XXX is the base class
 *
 * - static_initialization(), defined e.g. as
 *
 *       static void static_initialization( void ) {}
 *
 *   (note that this need not be virtual because it will be called with the
 *   explicit classname::static_initialization() format and classname being
 *   that of the derived class, which requires care if XXX or some derived
 *   class from which classname further derives does something, as then the
 *   parent class static_initialization() will have to be explicitly called
 *   in the derived one)
 *
 * The \p ClassName parameter of the macros must be the type of a class
 * satisfying the above constraints, possibly enclosed in any number of
 * parentheses "( ... )"; that is, for ClassName == MyBlock, the following
 *
 *     SMSpp_insert_in_factory_cpp_0( MyBlock )
 *
 *     SMSpp_insert_in_factory_cpp_0( ( MyBlock ) )
 *
 *     SMSpp_insert_in_factory_cpp_0( ( ( MyBlock ) ) )
 *
 * are equivalent, save that
 *
 *     IN SOME CASES ONE PAIR OF PARENTHESES IS STRICTLY REQUIRED
 *
 * as detailed in the following.
 *
 *     IMPORTANT NOTE 1: THE STRING RETURNED BY [_]private_name(), WHICH IS
 *     THE ONE THAT IS USED TO INDEX THE FACTORY, IS STRIPPED OF ANY
 *     WHITESPACE AND ENCLOSING PARENTHESES. For instance, a template class
 *     like MyBlock< std::pair< int , int > > gets name
 *     "MyBlock< std::pair< int , int > >" (which is syntactically wrong due to
 *     the closing ">>" instead of "> >", but after all it is ony a string).
 *     This makes it possible to read it from a std::stream, where
 *     whitespaces are separators. If MyBlock derives from Block, it is then
 *     possible to create an object of class MyBlock with
 *
 *     new_Block( "MyBlock< std::pair< int , int > >" )
 *
 *     Note that the implementation of Block::my_Block() automatically strips
 *     all the whitespaces from the input string, which means that
 *
 *     new_Block( " MyBlock< std::pair< int , int > > " )
 *
 *     (and any similar version) works as well; however, this depends on
 *     my_Block() preprocessing its input, the string in the factory is
 *     the first one.
 *
 *     IMPORTANT NOTE 2: IF THE CLASS NAME CONTAINS ANY COMMAS, THE CLASS
 *     CLASS PASSED TO THE MACRO *MUST* BE ENCLOSED IN PARENTHESES. That
 *     is, class names whose "natural" form contain a comma (typically,
 *     some but not all of the template ones) will necessarily have to be
 *     enclosed in parentheses. For instance, the template class
 *
 *     MyBlock< std::pair< int , int > >
 *
 * must be registered in the factory by
 *
 *     SMSpp_insert_in_factory_cpp_1_t( ( MyBlock< std::pair< int , int > > ) );
 *
 * i.e., with at least one (possibly more, but there is no use in that) pair
 * of extra parentheses around the name of the class. For classes that do not
 * have commas in their names, the use of parentheses is optional. For
 * instance, the classes MyBlock and MyTBlock< int > could be registered in the
 * factory by
 *
 *    SMSpp_insert_in_factory_cpp_1_t( MyBlock );
 *
 *    SMSpp_insert_in_factory_cpp_1_t( MyTBlock< int > );
 *
 * or, equivalently,
 *
 *     SMSpp_insert_in_factory_cpp_1_t( ( MyBlock ) );
 *
 *     SMSpp_insert_in_factory_cpp_1_t( ( MyTBlock< int > ) );
 *
 * Note the use of the "stringification" operator "#" when converting the
 * macro parameter ClassName to its string representation.
 *
 * Note that a previous version of these macros required an ugly two-step
 * mechanism to define the object inserted in the factory in the "_1" version.
 * This was due to the fact that boost (up to and incl. 1.67) did not do
 * "perfect forwarding": factory<>() required lvalue arguments, but bind
 * provided rvalues. This flaw has apparently been fixed in later versions of
 * boost, and this implementation works with boost 1.74.0.
 *
 * The alert reader may similarly wonder why the funny "{}" after
 * "_initializer" in the _t versions, but not in the standard ones. This is
 * because for a template class (say, A, say template over the int) with a
 * static member (say, int a), something like
 *
 *   template<>
 *   int A<5>::a;
 *
 * is *not* a definition of the object for the specialization A<5>, as it
 * would be for a normal int, but rather a "specialization declaration" that
 * says "don't instantiate a from the primary template, because there is a
 * specialized definition somewhere else". In order to force it to be a
 * definition the object has to be explicitly initialized, as in
 *
 *   template<>
 *   int A<5>::a{};
 *
 * cue the funny "{}".
 *
 * As the ClassName may not be a type (as it may contain parentheses), we use
 * the trait get_type<> in order to extract the type of the class (ClassType
 * above) from ClassName.
 *
 * Finally, we remark that stripping the class name from whitespaces and
 * parentheses is done during the initialization of the static const
 * std::string my_name variable of _private_name() by using a lambda that is
 * defined and immediately called on #ClassName. This operation hence happens
 * *at runtime*, although only once the first time that _private_name() is
 * called (which is immediately as the class is registered in the factory).
 * This is slightly inefficient since the stripping should rather happen at
 * compile time; as C++-20 arrives most of std::algorithms will be
 * constexpr-able and therefore this will hopefully be possible. */

// using ellipsis to take all preprocessor-tokens
#define SMSpp_insert_in_factory_cpp_0( ... )                                 \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name( void ) {  \
  static const std::string _name( SMSpp_classname_normalise(                 \
                                            std::string( #__VA_ARGS__ ) ) ); \
  return( _name );                                                           \
  }                                                                          \
                                                                             \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::private_name( void )     \
 const {                                                                     \
  return(                                                                    \
       SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ); \
  }                                                                          \
                                                                             \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init::_init( void ) {   \
  f_factory()[                                                               \
   SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ] =    \
    boost::factory< SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type * >(); \
  SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::static_initialization();\
  }                                                                          \
                                                                             \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init                    \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_initializer

// using ellipsis to take all preprocessor-tokens
#define SMSpp_insert_in_factory_cpp_1( ... )                                 \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name( void ) {  \
  static const std::string _name( SMSpp_classname_normalise(                 \
                                            std::string( #__VA_ARGS__ ) ) ); \
  return( _name );                                                           \
  }                                                                          \
                                                                             \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::private_name( void )     \
 const {                                                                     \
  return(                                                                    \
       SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ); \
  }                                                                          \
                                                                             \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init::_init( void ) {   \
  f_factory()[                                                               \
   SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ] =    \
    boost::factory< SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type * >(); \
  SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::static_initialization();\
  }                                                                          \
                                                                             \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init                    \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_initializer

// using ellipsis to take all preprocessor-tokens
#define SMSpp_insert_in_factory_cpp_0_t( ... )                               \
 template<>                                                                  \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name( void ) {  \
  static const std::string _name( SMSpp_classname_normalise(                 \
                                            std::string( #__VA_ARGS__ ) ) ); \
  return( _name );                                                           \
  }                                                                          \
                                                                             \
 template<>                                                                  \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::private_name( void )     \
 const {                                                                     \
  return(                                                                    \
       SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ); \
  }                                                                          \
                                                                             \
 template<>                                                                  \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init::_init( void ) {   \
  f_factory()[                                                               \
   SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ] =    \
    boost::factory< SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type * >(); \
  SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::static_initialization();\
  }                                                                          \
                                                                             \
 template<>                                                                  \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init                    \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_initializer{}

// using ellipsis to take all preprocessor-tokens
#define SMSpp_insert_in_factory_cpp_1_t( ... )                               \
 template<>                                                                  \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name( void ) {  \
  static const std::string _name( SMSpp_classname_normalise(                 \
                                            std::string( #__VA_ARGS__ ) ) ); \
  return( _name );                                                           \
  }                                                                          \
                                                                             \
 template<>                                                                  \
 const std::string &                                                         \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::private_name( void )     \
 const {                                                                     \
  return(                                                                    \
       SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ); \
  }                                                                          \
                                                                             \
 template<>                                                                  \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init::_init( void ) {   \
  f_factory()[                                                               \
   SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_private_name() ] =    \
    boost::factory< SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type * >(); \
  SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::static_initialization();\
  }                                                                          \
                                                                             \
 template<>                                                                  \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_init                    \
 SMSpp_type_traits::t< void( __VA_ARGS__ ) >::type::_initializer{}

/*--------------------------------------------------------------------------*/
// definition of auxiliary template variable (don't you just love C++?), which
// serves to avoid having duplicated names in case SMSpp_ensure_load() is
// called for multiple classes in the same .cpp

template< class ClassName >
bool SMSpp_ensure_load_var;

// the SMSpp_ensure_load() creates an empty object of the given class and
// immediately destroys it; in all our cases, the empty constructor exists
// and should be relatively cheap. we tried to avoid it by taking the
// address of some method of the class, but this is not enough in all
// case to force the linker to include the relevant object, while creating
// an object of the class damn sure is

#define SMSpp_ensure_load( ClassName )                                      \
 template<>                                                                 \
 bool SMSpp_ensure_load_var<                                                \
   SMSpp_type_traits::t< void( ClassName ) >::type > =                      \
  []( void ) -> bool {                                                      \
   if( auto p = new SMSpp_type_traits::t< void( ClassName ) >::type() ) {   \
    delete p; return( true ); }                                             \
   else        return( false );                                             \
   }( )

/** @} ---------------------------------------------------------------------*/
/*------------------- HANDLE boost::any SPECIALIZATIONS --------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup boost_any_stuff Handling boost::any specializations for SMS++
 *
 *  Two separate approaches are provided to automate the task of applying some
 *  fixed operations to a boost::any in a way that is as much independent as
 *  possible to the shape of the content, which can typically be:
 *
 *  - a single pointer to an object of some type (Constraint, Variable or
 *    some of their derived classes);
 *
 *  - a pointer to a std::vector of objects of some type (Constraint,
 *    Variable or some of their derived classes);
 *
 *  - a pointer to a boost::multi_array< K > of objects of some type (...);
 *
 *  - a pointer to a single std::list of objects of some type (...);
 *
 *  - a pointer to a std::vector of std::list of objects of some type (...);
 *
 *  - a pointer to a boost::multi_array< K > of std::list of objects of some
 *    type (...).
 *
 * This is provided through the eight template functions
 *
 *   bool un_any_static( boost::any & any , F f , un_any_type< T > )
 *
 *   bool un_any_static_2( boost::any & any1 , boost::any & any2 ,
 *                         F f , un_any_type< T > , un_any_type< U > )
 *
 *   bool un_any_static_2_create( const boost::any & any1 ,
 *                                boost::any & any2 , un_any_type< T > ,
 *                                un_any_type< U > , F f , bool apply_f )
 *
 *   bool un_any_const_static( const boost::any & any , F f , un_any_type< T > )
 *
 *   bool un_any_dynamic( boost::any & any , F f , un_any_type< T > )
 *
 *   bool un_any_dynamic_2( boost::any & any1 , boost::any & any2 ,
 *                          F f , un_any_type< T > , un_any_type< U > )
 *
 *   bool un_any_dynamic_2_create( const boost::any & any1 ,
 *                                 boost::any & any2 , un_any_type< T > ,
 *                                 un_any_type< U > , F f , bool apply_f )
 *
 *   bool un_any_const_dynamic( const boost::any & any , F f ,
 *                              un_any_type< T > )
 *
 * and the four macros (which, however, behave as a bool-returning function)
 *
 *   #define un_any_thing( thing_type , my_thing , f )
 *
 *   #define un_any_thing_0( thing_type , my_thing , f )
 *
 *   #define un_any_thing_1( thing_type , my_thing , f )
 *
 *   #define un_any_thing_K( thing_type , my_thing , f )
 *
 * The difference between the two is that the functions take a "f" that is
 * a ( T & ) --> void function and it is applied to all *elements* in the
 * boost any (it could also be a ( T ) --> void function but this would
 * mean copying the object and no one wants that, right?), whereas the macros
 * take a "f" that is a *piece of code* that is applied to the *container*
 * of the elements, i.e., either a thing_type, or a std::vector< thing_type >,
 * or a boost::multi_array< thing_type >. This requires the piece of code to
 * be "type polymorphic" (it has to work in all three cases), which is
 * nontrivial and (to the best of our knowledge) cannot be obtained with
 * templates at all, whence the not-very-C++ approach of using macros.
 *  @{
 */

/*--------------------------------------------------------------------------*/
///< empty type, template over ints, for recursive template shenanigans

template< unsigned short K >
struct un_any_int {};

///< empty type, template over a type, for template functions shenanigans
/**< empty type for allowing to declare the expected inner type in
 * un_any_*(). */

template< class T >
struct un_any_type {};

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_static( boost::any & any , F f , un_any_type< T > )
 *
 * is intended to take a boost::any that contains either:
 *
 * - a pointer (reference) to a T;
 *
 * - a pointer (reference) to a std::vector< T >;
 *
 * - a pointer (reference) to a  boost::multi_array< T , K > for "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. "f"
 * must be a ( T & ) --> void function (it could also be a ( T ) --> void
 * function but this would mean copying the object and no one wants that,
 * right?); a lambda would work perfectly there.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template< typename T , class F >
bool un_any_static( boost::any & any , F f , un_any_type< T > ) {
 if( any.type() == typeid( T * ) ) {
  auto & el = *boost::any_cast< T * >( any );
  f( el );
  return( true );
  }
 else
  if( any.type() == typeid( std::vector< T > * ) ) {
   auto & var = *boost::any_cast< std::vector< T > * >( any );
   for( auto & el : var )
    f( el );
   return( true );
   }
  else
   return( un_any_static( any , f , un_any_type< T >() ,
                          un_any_int< 2 >() ) );
 }

template< typename T , class F >
bool un_any_static( boost::any & , F , un_any_type< T > , un_any_int< 9 > ) {
 return( false );
 }

template< typename T , class F , unsigned short K >
bool un_any_static( boost::any & any , F f , un_any_type< T > ,
                    un_any_int< K > ) {
 if( any.type() == typeid( boost::multi_array< T , K > * ) ) {
  auto & var = *boost::any_cast< boost::multi_array< T , K > * >( any );
  T * p = var.data();
  for( auto i = var.num_elements() ; i-- ; )
   f( *(p++) );
  return( true );
  }
 else
  return( un_any_static( any , f , un_any_type< T >() ,
                         un_any_int< K + 1 >() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_static_2( boost::any & any1 , boost::any & any2 ,
 *                         F f , un_any_type< T > , un_any_type< U > )
 *
 * is intended to take two boost::any "any1" and "any2" so that they
 * contain respectively:
 *
 * - a pointer (reference) to a T and a pointer (reference) to a U;
 *
 * - a pointer (reference) to a std::vector< T > and a pointer (reference) to a
 *   std::vector< U >;
 *
 * - a pointer (reference) to a boost::multi_array< T , K > and a
 *   pointer (reference) to a boost::multi_array< U , K >, for "all" K;
 *
 * and apply the function "f" to all corresponding pairs of objects of type T
 * and U they contain. "f" must be a ( T & , U & ) --> void function (it could
 * also be a ( T , U ) --> void function but this would mean copying the
 * object and no one wants that, right?); a lambda would work perfectly there.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any1" and "any2" did indeed contain one of the sought-for
 * pairs of types, in which case "f" have been applied to all its elements,
 * and false if "any1" or "any2" contained something else, and therefore "f"
 * has not been applied to anything.
 *
 * If "any1" and "any2" are std::vectors, then "f" will be applied to the i-th
 * elements of "any1" and "any2" for every position i that is present in both
 * vectors. If "any1" and "any2" are boost::multi_arrays, then the data of
 * each one is extracted as an array and then "f" is applied to the elements
 * in the i-th position of these arrays if and only if position i is present
 * in both arrays.
 *
 * Notice that in debug mode, the std::vectors are required to have the same
 * size and the boost:multi_arrays are required to have the same number of
 * dimensions and shape. */

template< typename T , typename U , class F >
bool un_any_static_2( const boost::any & any1 , const boost::any & any2 ,
                      F f , un_any_type< T > , un_any_type< U > ) {
 if( any1.type() == typeid( T * ) ) {
  auto & el1 = *boost::any_cast< T * >( any1 );
  #ifndef NDEBUG
   if( any2.type() != typeid( U * ) )
    throw( std::invalid_argument(
             "un_any_static_2: second argument not U *" ) );
  #endif
  auto & el2 = *boost::any_cast< U * >( any2 );
  f( el1 , el2 );
  return( true );
  }
 else {
  if( any1.type() == typeid( std::vector< T > * ) ) {
   auto & var1 = *boost::any_cast< std::vector< T > * >( any1 );
   #ifndef NDEBUG
    if( any2.type() != typeid( std::vector< U > * ) )
     throw( std::invalid_argument(
             "un_any_static_2: second argument not not std::vector< U > *" ) );
   #endif
   auto & var2 = *boost::any_cast< std::vector< U > * >( any2 );
   #ifndef NDEBUG
    if( var1.size() != var2.size() )
     throw( std::logic_error(
              "un_any_static_2: vectors must have the same size" ) );
   #endif
   auto i2 = var2.begin();
   for( auto i1 = var1.begin() ;
        ( i1 != var1.end() ) && ( i2 != var2.end() ) ; ++i1 , ++i2 )
    f( *i1 , *i2 );

   return( true );
   }
  else
   return( un_any_static_2( any1 , any2 , f , un_any_type< T >() ,
                            un_any_type< U >() , un_any_int< 2 >() ) );
  }
 }

template< typename T , typename U , class F >
bool un_any_static_2( const boost::any & , const boost::any & , F ,
                      un_any_type< T > , un_any_type< U > ,
                      un_any_int< 9 > ) {
 return( false );
 }

template< typename T , typename U , class F , unsigned short K >
bool un_any_static_2( const boost::any & any1 , const boost::any & any2 ,
                      F f , un_any_type< T > , un_any_type< U > ,
                      un_any_int< K > ) {
 if( any1.type() == typeid( boost::multi_array< T , K > * ) ) {
  auto & var1 = *boost::any_cast< boost::multi_array< T , K > * >( any1 );
  #ifndef NDEBUG
   if( any2.type() != typeid( boost::multi_array< U , K > * ) )
    throw( std::invalid_argument(
      "un_any_static_2: second argument not boost::multi_array< U , K > *" ) );
  #endif
  auto & var2 = *boost::any_cast< boost::multi_array< U , K > * >( any2 );
  #ifndef NDEBUG
   if( ( var1.num_dimensions() != var2.num_dimensions() ) ||
       ( ! std::equal( var1.shape() , var1.shape() + var1.num_dimensions() ,
                       var2.shape() ) ) )
    throw( std::logic_error(
              "un_any_static_2:  multi_arrays must have the same shape" ) );
  #endif
  T * p1 = var1.data();
  U * p2 = var2.data();
  for( auto i = std::min( var1.num_elements() , var2.num_elements() ) ;
       i-- ; )
   f( *(p1++) , *(p2++) );
  return( true );
  }
 else
  return( un_any_static_2( any1 , any2 , f , un_any_type< T >() ,
                           un_any_type< U >() , un_any_int< K + 1 >() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_static_2_create( const boost::any & any1 ,
 *                                boost::any & any2 , un_any_type< T > ,
 *                                un_any_type< U > , F f , bool apply_f )
 *
 * is intended to take two boost::any "any1" and "any2" so that if "any1"
 * contains
 *
 * - a pointer (reference) to a T, then a U is created and a pointer to this
 *   newly created object is stored in "any2";
 *
 * - a pointer (reference) to a std::vector< T >, then a std::vector< U > is
 *   created having the same size as the vector pointed by "any1" and the
 *   pointer to this just created object is stored in "any2";
 *
 * - a pointer (reference) to a boost::multi_array< T , K >, then a
 *   boost::multi_array< U , K > is created having the same shape as the
 *   boost::multi_array pointed by "any1" and the pointer to this newly
 *   created object is stored in "any2", for "all" K.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * If the function "f" is present and "apply_f" is true, then the function
 * "f" is applied to all corresponding pairs of objects of types T and U that
 * any1 and any2 contain. "f" must be a ( T & , U & ) --> void function (it
 * could also be a ( T , U ) --> void function but this would mean copying
 * the object and no one wants that, right?); a lambda would work perfectly
 * there.
 *
 * Returns true if "any1" did indeed contain one of the sought-for types. */

template< typename T , typename U , class F >
bool un_any_static_2_create( const boost::any & any1 , boost::any & any2 ,
                             un_any_type< T > , un_any_type< U > , F f ,
                             bool apply_f = true ) {
 if( any1.type() == typeid( T * ) ) {
  any2 = new U();

  if( apply_f ) {
   auto & var1 = *boost::any_cast< T * >( any1 );
   auto & var2 = *boost::any_cast< U * >( any2 );

   f( var1 , var2 );
   }

  return( true );
  }
 else {
  if( any1.type() == typeid( std::vector< T > * ) ) {
   auto & var1 = *boost::any_cast< std::vector< T > * >( any1 );
   any2 = new std::vector< U >( var1.size() );
   if( apply_f ) {
    auto & var2 = *boost::any_cast< std::vector< U > * >( any2 );
    auto i2 = var2.begin();
    for( auto i1 = var1.begin() ; i1 != var1.end() ; ++i1 , ++i2 )
     f( *i1 , *i2 );
    }

   return( true );
   }
  else
   return( un_any_static_2_create( any1 , any2 , un_any_type< T >() ,
                                   un_any_type< U >() , un_any_int< 2 >() ,
                                   f , apply_f ) );
  }
 }

template< typename T , typename U , class F >
bool un_any_static_2_create( const boost::any & , boost::any & ,
                             un_any_type< T > , un_any_type< U > ,
                             un_any_int< 9 > , F f , bool apply_f = true ) {
 return( false );
 }

template< typename T , typename U , class F , unsigned short K >
bool un_any_static_2_create( const boost::any & any1 , boost::any & any2 ,
                             un_any_type< T > , un_any_type< U > ,
                             un_any_int< K > , F f , bool apply_f = true ) {
 if( any1.type() == typeid( boost::multi_array< T , K > * ) ) {
  auto & var1 = *boost::any_cast< boost::multi_array< T , K > * >( any1 );
  auto first = var1.shape();
  std::vector< int > shape( first , first + var1.num_dimensions() );
  any2 = new boost::multi_array< U , K >( shape );
  if( apply_f ) {
   auto & var2 = *boost::any_cast< boost::multi_array< U , K > * >( any2 );
   T * p1 = var1.data();
   U * p2 = var2.data();
   for( auto i = std::min( var1.num_elements() , var2.num_elements() ) ;
        i-- ; )
    f( *(p1++) , *(p2++) );
   }

  return( true );
  }
 else
  return( un_any_static_2_create( any1 , any2 , un_any_type< T >() ,
                                  un_any_type< U >() ,
                                  un_any_int< K + 1 >() , f , apply_f ) );
 }

template< typename T , typename U >
bool un_any_static_2_create( const boost::any & any1 , boost::any & any2 ,
                             un_any_type< T > , un_any_type< U > ) {
 return( ( un_any_static_2_create( any1 , any2 ,
                                   un_any_type< T >() , un_any_type< U >() ,
                                   []( T & t , U & u ) {} , false ) ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_const_static( const boost::any & any , F f , un_any_type< T > )
 *
 * is intended to take a const boost::any that contains either:
 *
 * - a pointer (reference) to a T;
 *
 * - a pointer (reference) to a std::vector< T >;
 *
 * - a pointer (reference) to a  boost::multi_array< T , K > for "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. "f"
 * must be a ( T & ) --> void function (it could also be a ( T ) --> void
 * function but this would mean copying the object and no one wants that,
 * right?); a lambda would work perfectly there.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template< typename T , class F >
bool un_any_const_static( const boost::any & any , F f , un_any_type< T > ) {
 if( any.type() == typeid( T * ) ) {
  auto & el = *boost::any_cast< T * >( any );
  f( el );
  return( true );
  }
 else
  if( any.type() == typeid( std::vector< T > * ) ) {
   auto & var = *boost::any_cast< std::vector< T > * >( any );
   for( auto & el : var )
    f( el );
   return( true );
   }
  else
   return( un_any_const_static( any , f , un_any_type< T >() ,
                                un_any_int< 2 >() ) );

 }

template< typename T , class F >
bool un_any_const_static( const boost::any & , F ,
                          un_any_type< T > , un_any_int< 9 > ) {
 return( false );
 }

template< typename T , class F , unsigned short K >
bool un_any_const_static( const boost::any & any , F f ,
                          un_any_type< T > , un_any_int< K > ) {
 if( any.type() == typeid( boost::multi_array< T , K > * ) ) {
  auto & var = *boost::any_cast< boost::multi_array< T , K > * >( any );
  T * p = var.data();
  for( auto i = var.num_elements() ; i-- ; )
   f( *(p++) );
  return( true );
  }
 else
  return( un_any_const_static( any , f , un_any_type< T >() ,
                               un_any_int< K + 1 >() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_dynamic( boost::any & any , F f , un_any_type< T > )
 *
 * is intended to take a boost::any that contains either:
 *
 * - a pointer (reference) to a std::list< T >;
 *
 * - a pointer (reference) to a std::vector< std::list< T > >;
 *
 * - a pointer (reference) to a  boost::multi_array< std::list< T > , K > for
 *   "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. Note
 * that "f" is applied to the *individual objects*, *not* to the *lists* of
 * object: in fact, "f" must be a ( T & ) --> void function (it could also
 * be a ( T ) --> void function but this would mean copying the object and
 * no one wants that, right?)
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything.
 */

template< typename T , class F >
bool un_any_dynamic( boost::any & any , F f , un_any_type< T > ) {
 if( any.type() == typeid( std::list< T > * ) ) {
  auto & el = *boost::any_cast< std::list< T > * >( any );
  for( auto & ell : el )
   f( ell );
  return( true );
  }
 else
  if( any.type() == typeid( std::vector< std::list< T > > * ) ) {
   auto & var = *boost::any_cast< std::vector< std::list< T > > * >( any );
   for( auto & el : var )
    for( auto & ell : el )
     f( ell );
   return( true );
   }
  else
   return( un_any_dynamic( any , f , un_any_type< T >() ,
                           un_any_int< 2 >() ) );
 }

template< typename T , class F >
bool un_any_dynamic( boost::any & , F , un_any_type< T > , un_any_int< 9 > ) {
 return( false );
 }

template< typename T , class F , unsigned short K >
bool un_any_dynamic( boost::any & any , F f ,
                     un_any_type< T > , un_any_int< K > ) {
 if( any.type() == typeid( boost::multi_array< std::list< T > , K > * ) ) {
  auto & var =
   *boost::any_cast< boost::multi_array< std::list< T > , K > * >( any );
  std::list< T > * p = var.data();
  for( auto i = var.num_elements() ; i-- ; ++p )
   for( auto & ell : *p )
    f( ell );
  return( true );
  }
 else
  return( un_any_dynamic( any , f , un_any_type< T >() ,
                          un_any_int< K + 1 >() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_dynamic_2( boost::any & any1 , boost::any & any2 ,
 *                          F f , un_any_type< T > , un_any_type< U > )
 *
 * is intended to take two boost::any "any1" and "any2" so that they
 * contain respectively:
 *
 * - a pointer (reference) to a std::list< T > and a pointer (reference) to a U;
 *
 * - a pointer (reference) to a std::vector< std::list< T > > and a pointer
 *   (reference) to a std::vector< U >;
 *
 * - a pointer (reference) to a boost::multi_array< std::list< T > , K > and a
 *   pointer (reference) to a boost::multi_array< U , K >, for "all" K;
 *
 * and apply the function "f" to the objects they point to. "f" must be a
 * ( std::list< T > & , U & ) --> void function (it could also be a
 * ( std::list< T > , U ) --> void function but this would mean copying the
 * object and no one wants that, right?); a lambda would work perfectly there.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any1" and "any2" did indeed contain one of the sought-for
 * pair of types, in which case "f" have been applied to all its elements, and
 * false if "any1" or "any2" contained something else, and therefore "f" has
 * not been applied to anything.
 *
 * Notice that in debug mode, the std::vectors are required to have the same
 * size and the boost:multi_arrays are required to have the same number of
 * dimensions and shape. */

template< typename T , typename U , class F >
bool un_any_dynamic_2( const boost::any & any1 , const boost::any & any2 ,
                       F f , un_any_type< T > c , un_any_type< U > ) {
 if( any1.type() == typeid( std::list< T > * ) ) {
  auto & el1 = *boost::any_cast< std::list< T > * >( any1 );
  #ifndef NDEBUG
   if( any2.type() != typeid( U * ) )
    throw( std::invalid_argument(
                         "un_any_dynamic_2: second argument not U *" ) );
  #endif
  auto & el2 = *boost::any_cast< U * >( any2 );
  f( el1 , el2 );
  return( true );
  }
 else
  if( any1.type() == typeid( std::vector< std::list< T > > * ) ) {
   auto & var1 = *boost::any_cast< std::vector< std::list< T > > * >( any1 );
   #ifndef NDEBUG
    if( any2.type() != typeid( std::vector< U > * ) )
     throw( std::invalid_argument(
                          "un_any_dynamic_2: second argument not U *" ) );
   #endif
   auto & var2 = *boost::any_cast< std::vector< U > * >( any2 );
   #ifndef NDEBUG
    if( var1.size() != var2.size() )
     throw( std::invalid_argument(
                     "un_any_dynamic_2: vectors have different sizes" ) );
   #endif
   auto i2 = var2.begin();
   for( auto i1 = var1.begin() ;
        i1 != var1.end() && i2 != var2.end() ; ++i1 , ++i2 )
    f( *i1 , *i2 );
   return( true );
   }
  else
   return( un_any_dynamic_2( any1 , any2 , f , un_any_type< T >() ,
                             un_any_type< U >() , un_any_int< 2 >() ) );
 }

template< typename T , typename U , class F >
bool un_any_dynamic_2( const boost::any & , const boost::any & , F ,
                       un_any_type< T > , un_any_type< U > , un_any_int< 9 > ) {
 return( false );
 }

template< typename T , typename U , class F , unsigned short K >
bool un_any_dynamic_2( const boost::any & any1 , const boost::any & any2 ,
                       F f , un_any_type< T > , un_any_type< U > ,
                       un_any_int< K > ) {
 if( any1.type() == typeid( boost::multi_array< std::list< T > , K > * ) ) {
  auto & var1 =
   *boost::any_cast< boost::multi_array< std::list< T > , K > * >( any1 );
  #ifndef NDEBUG
   if( any2.type() != typeid( boost::multi_array< U , K > * ) )
    throw( std::invalid_argument(
                          "un_any_dynamic_2: second argument not U *" ) );
  #endif
  auto & var2 = *boost::any_cast< boost::multi_array< U , K > * >( any2 );
  #ifndef NDEBUG
   if( ( var1.num_dimensions() != var2.num_dimensions() ) ||
       ( ! std::equal( var1.shape() , var1.shape() + var1.num_dimensions() ,
                       var2.shape() ) ) )
    throw( std::logic_error(
            "un_any_dynamic_2: multi_arrays must have the same shape" ) );
  #endif
  std::list< T > * p1 = var1.data();
  U * p2 = var2.data();
  for( auto i = std::min( var1.num_elements() , var2.num_elements() ) ;
       --i ; )
   f( *(p1++) , *(p2++) );
  return( true );
  }
 else
  return( un_any_dynamic_2( any1 , any2 , f , un_any_type< T >() ,
                            un_any_type< U >() , un_any_int< K + 1 >() ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_dynamic_2_create( const boost::any & any1 ,
 *                                 boost::any & any2 , un_any_type< T > ,
 *                                 un_any_type< U > , F f , bool apply_f )
 *
 * is intended to take two boost::any "any1" and "any2" so that if "any1"
 * contains
 *
 * - a pointer (reference) to a std::list< T >, then a U is created and a
 *   pointer to this newly created object is stored in "any2";
 *
 * - a pointer (reference) to a std::vector< std::list< T > >, then a
 *   std::vector< U > is created having the same size as the vector pointed by
 *   "any1" and the pointer to this just created object is stored in "any2";
 *
 * - a pointer (reference) to a boost::multi_array< std::list< T > , K >, then a
 *   boost::multi_array< U , K > is created having the same shape as the
 *   boost::multi_array pointed by "any1" and the pointer to this newly
 *   created object is stored in "any2", for "all" K.
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * If the function "f" is present and "apply_f" is true, then the
 * function "f" is applied to all corresponding pairs of objects of
 * types std::list< T > and U that any1 and any2 contain. "f" must be a
 * ( std::list< T > & , U & ) --> void function (it could also be a
 * ( std::list< T > , U ) --> void function but this would mean copying
 * the object and no one wants that, right?); a lambda would work
 * perfectly there.
 *
 * Returns true if "any1" did indeed contain one of the sought-for types. */

template< typename T , typename U , class F >
bool un_any_dynamic_2_create( const boost::any & any1 , boost::any & any2 ,
                              un_any_type< T > , un_any_type< U > ,
                              F f , bool apply_f ) {
 if( any1.type() == typeid( std::list< T > * ) ) {
  any2 = new U();
  if( apply_f ) {
   auto & var1 = *boost::any_cast< std::list< T > * >( any1 );
   auto & var2 = *boost::any_cast< U * >( any2 );
   f( var1 , var2 );
   }
  return( true );
  }
 else
  if( any1.type() == typeid( std::vector< std::list< T > > * ) ) {
   auto & var1 = *boost::any_cast< std::vector< std::list< T > > * >( any1 );
   any2 = new std::vector< U >( var1.size() );
   if( apply_f ) {
    auto & var2 = *boost::any_cast< std::vector< U > * >( any2 );
    auto i2 = var2.begin();
    for( auto i1 = var1.begin() ;
         i1 != var1.end() && i2 != var2.end() ; ++i1 , ++i2 )
     f( *i1 , *i2 );
    }
   return( true );
   }
  else
   return( un_any_dynamic_2_create( any1 , any2 , un_any_type< T >() ,
                                    un_any_type< U >() , un_any_int< 2 >() ,
                                    f , apply_f ) );
 }

template< typename T , typename U , class F >
bool un_any_dynamic_2_create( const boost::any & , boost::any & ,
                              un_any_type< T > , un_any_type< U > ,
                              un_any_int< 9 > , F f , bool apply_f ) {
 return( false );
 }

template< typename T , typename U , class F , unsigned short K >
bool un_any_dynamic_2_create( const boost::any & any1 , boost::any & any2 ,
                              un_any_type< T > , un_any_type< U > ,
                              un_any_int< K > , F f , bool apply_f ) {
 if( any1.type() == typeid( boost::multi_array< std::list< T > , K > * ) ) {
  auto & var1 =
   *boost::any_cast< boost::multi_array< std::list< T > , K > * >( any1 );
  auto first = var1.shape();
  std::vector< int > shape( first , first + var1.num_dimensions() );
  any2 = new boost::multi_array< U , K >( shape );
  if( apply_f ) {
   auto & var2 = *boost::any_cast< boost::multi_array< U , K > * >( any2 );
   std::list< T > * p1 = var1.data();
   U * p2 = var2.data();
   for( auto i = std::min( var1.num_elements() , var2.num_elements() ) ;
        --i ; )
    f( *(p1++) , *(p2++) );
   }
  return( true );
  }
 else
  return( un_any_dynamic_2_create( any1 , any2 , un_any_type< T >() ,
                                   un_any_type< U >() ,
                                   un_any_int< K + 1 >() , f , apply_f ) );
 }

template< typename T , typename U >
bool un_any_dynamic_2_create( const boost::any & any1 , boost::any & any2 ,
                              un_any_type< T > , un_any_type< U > ) {
 return( un_any_dynamic_2_create( any1 , any2 , un_any_type< T >() ,
                                  un_any_type< U >() ,
                                  []( T & t , U & u ) {} , false ) );
 }

/*--------------------------------------------------------------------------*/
/** The template function
 *
 *   bool un_any_const_dynamic( const boost::any & any , F f ,
 *                              un_any_type< T > )
 *
 * is intended to take a const boost::any that contains either:
 *
 * - a pointer (reference) to a std::list< T >;
 *
 * - a pointer (reference) to a std::vector< std::list< T > >;
 *
 * - a pointer (reference) to a  boost::multi_array< std::list< T > , K > for
 *   "all" K;
 *
 * and apply the function "f" to all the objects of type T it contains. Note
 * that "f" is applied to the *individual objects*, *not* to the *lists* of
 * object: in fact, "f" must be a ( T & ) --> void function (it could also
 * be a ( T ) --> void function but this would mean copying the object and
 * no one wants that, right?)
 *
 * The function can work with any K, but a maximum K has to be fixed at
 * compile time; currently the maximum K is 8, but it may be easily extended
 * to go higher if needed.
 *
 * Returns true if "any" did indeed contain one of the sought-for types, in
 * which case "f" have been applied to all its elements, and false if "any"
 * contained something else, and therefore "f" has not been applied to
 * anything. */

template< typename T , class F >
bool un_any_const_dynamic( const boost::any & any , F f , un_any_type< T > ) {
 if( any.type() == typeid( std::list< T > * ) ) {
  auto & el = *boost::any_cast< std::list< T > * >( any );
  for( auto & ell : el )
   f( ell );
  return( true );
  }
 else
  if( any.type() == typeid( std::vector< std::list< T > > * ) ) {
   auto & var = *boost::any_cast< std::vector< std::list< T > > * >( any );
   for( auto & el : var )
    for( auto & ell : el )
     f( ell );
   return( true );
   }
  else
   return( un_any_const_dynamic( any , f , un_any_type< T >() ,
                                 un_any_int< 2 >() ) );
 }

template< typename T , class F >
bool un_any_const_dynamic( const boost::any & , F , un_any_type< T > ,
                           un_any_int< 9 > ) {
 return( false );
 }

template< typename T , class F , unsigned short K >
bool un_any_const_dynamic( const boost::any & any , F f ,
                           un_any_type< T > , un_any_int< K > ) {
 if( any.type() == typeid( boost::multi_array< std::list< T > , K > * ) ) {
  auto & var =
   *boost::any_cast< boost::multi_array< std::list< T > , K > * >( any );
  std::list< T > * p = var.data();
  for( auto i = var.num_elements() ; i-- ; ++p )
   for( auto & ell : *p )
    f( ell );
  return( true );
  }
 else
  return( un_any_const_dynamic( any , f , un_any_type< T >() ,
                                un_any_int< K + 1 >() ) );
 }

/*--------------------------------------------------------------------------*/
/** The four macro
 *
 *   #define un_any_thing( thing_type , my_thing , f )
 *
 *   #define un_any_thing_0( thing_type , my_thing , f )
 *
 *   #define un_any_thing_1( thing_type , my_thing , f )
 *
 *   #define un_any_thing_K( thing_type , my_thing , f )
 *
 * takes the boost::any "my_thing", that is assumed to only take values in
 * the correct types for a "thing" described by "thing_type". This means
 * that "thing_type" is expected to be:
 *
 * - an object of class Variable or of any class derived from Variable;
 *
 * - an object of class Constraint or of any class derived from Constraint;
 *
 * - a pointer to an object of class Variable or of any class derived from
 *   Variable;
 *
 * - a pointer to object of class Constraint or of any class derived from
 *   Constraint;
 *
 * and that "my_thing" has to be:
 *
 * - a pointer to a "thing_type";
 *
 * - a pointer to a std::vector of "thing_type";
 *
 * - a pointer to a boost::multi_array< K > of "thing_type";
 *
 * - a pointer to a std::list of "thing_type";
 *
 * - a pointer to a std::vector of std::list of "thing_type";
 *
 * - a pointer to a boost::multi_array< K > of std::list of "thing_type";
 *
 * for "all" K, and apply the type-independent block of code "f" to the
 * corresponding variable "var" of the type (among the above) that the
 * boost::any turns out to be. Note that "var" is, therefore *not always
 * of the same type*: it can be an object, an array, a multi-array, a
 * list, an array of lists, or a multi-array of lists (technically, "var"
 * is a reference to any of these). Hence, "f" is not a function with a
 * well-specified input type that is applied to all *elements of the
 * container*, but rather a *piece of code* that is applied to the
 * *container itself*. This requires the piece of code to be "type
 * polymorphic" (it has to work in all three cases), which is nontrivial
 * and (to the best of our knowledge) cannot be obtained with templates at
 * all, whence the not-very-C++ approach of using macros.
 *
 * Because this may be impossible to do, there are four macros:
 *
 *  - un_any_thing() applies the same "f" to all types of containers;
 *
 *  - un_any_thing_0() only applies "f" if "my_thing" is a single
 *    "thing_type";
 *
 *  - un_any_thing_1() only applies "f" if "my_thing" is a
 *    std::vector<"thing_type">;
 *
 *  - un_any_thing_K() only applies "f" if "my_thing" is a
 *    boost::multi_array<"thing_type" , K>.
 *
 * This is why, although these are macros, they have been structured to
 * "behave like functions", in the sense that they are an expression
 * returning a bool (this is obtained by the magic of defining a lambda
 * returning a bool and immediately evaluating it on "my_thing"). The
 * "function" returns true if "my_thing" did indeed contain one of the
 * sought-for types, in which case "f" has been executed with the
 * corresponding "var" of the right type, and false if "my_thing" contained
 * something else, and therefore "f" has not been executed at all anything.
 *
 * Note that, unlike in the un_any_*_*() functions, there is no distinction
 * between the static (single "thing_type" elements) and dynamic (lists of
 * "thing_type" elements), because one can (and perhaps must) separately
 * call
 *
 *   un_any_thing( basic_type , ... );
 *
 * and
 *
 *   un_any_thing( std:list< basic_type > , ... );
 *
 * The pesky part in these macros (in particular, in un_any_thing_K() and
 * therefore in un_any_thing()) is that they have to work with "all" K, but
 * a maximum K has to be fixed at compile time; currently the maximum K is
 * 8, but it may be easily extended to go higher if needed.
 */

#define un_any_thing( thing_type , my_thing , f )                            \
 [&]( const boost::any & _any ) -> bool {                                    \
  if( un_any_thing_0( thing_type , _any , f ) )                              \
   return( true );                                                           \
  else                                                                       \
   if( un_any_thing_1( thing_type , _any , f ) )                             \
    return( true );                                                          \
  return( un_any_thing_K( thing_type , _any , f ) );                         \
  }( my_thing )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// TODO: Remove this when it's not needed anymore
#ifdef CLANG_1200_0_32_27_PATCH
#define un_any_thing_0( thing_type , my_thing , f )                          \
 [&]( const boost::any & _any ) -> bool {                                    \
  if( _any.type() == typeid( thing_type ) ) {}                               \
  if( _any.type() == typeid( thing_type * ) ) {                              \
   auto & var = * boost::any_cast< thing_type * >( _any );                   \
   f; return( true );                                                        \
   }                                                                         \
  return( false );                                                           \
  }( my_thing )
#else
#define un_any_thing_0( thing_type , my_thing , f )                          \
 [&]( const boost::any & _any ) -> bool {                                    \
  if( _any.type() == typeid( thing_type * ) ) {                              \
   auto & var = * boost::any_cast< thing_type * >( _any );                   \
   f; return( true );                                                        \
   }                                                                         \
  return( false );                                                           \
  }( my_thing )
#endif

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define un_any_thing_1( thing_type , my_thing , f )                          \
 [&]( const boost::any & _any ) -> bool {                                    \
  if( _any.type() == typeid( std::vector< thing_type > * ) ) {               \
   auto & var = * boost::any_cast< std::vector< thing_type > * >( _any );    \
   f; return( true );                                                        \
   }                                                                         \
  return( false );                                                           \
  }( my_thing )

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define un_any_thing_K( thing_type , my_thing , f )                          \
 [&]( const boost::any & _any ) -> bool {                                    \
  if( _any.type() == typeid( boost::multi_array< thing_type , 2 > * ) ) {    \
   auto & var =                                                              \
    * boost::any_cast< boost::multi_array< thing_type , 2 > * >( _any );     \
   f; return( true );                                                        \
   }                                                                         \
  if( _any.type() == typeid( boost::multi_array< thing_type , 3 > * ) ) {    \
   auto & var =                                                              \
    * boost::any_cast< boost::multi_array< thing_type , 3 > * >( _any );     \
   f; return( true );                                                        \
   }                                                                         \
  if( _any.type() == typeid( boost::multi_array< thing_type , 4 > * ) ) {    \
   auto & var =                                                              \
    * boost::any_cast< boost::multi_array< thing_type , 4 > * >( _any );     \
   f; return( true );                                                        \
   }                                                                         \
  if( _any.type() == typeid( boost::multi_array< thing_type , 5 > * ) ) {    \
   auto & var =                                                              \
    * boost::any_cast< boost::multi_array< thing_type , 5 > * >( _any );     \
   f; return( true );                                                        \
   }                                                                         \
  if( _any.type() == typeid( boost::multi_array< thing_type , 6 > * ) ) {    \
   auto & var =                                                              \
    * boost::any_cast< boost::multi_array< thing_type , 6 > * >( _any );     \
   f; return( true );                                                        \
   }                                                                         \
  if( _any.type() == typeid( boost::multi_array< thing_type , 7 > * ) ) {    \
   auto & var =                                                              \
    * boost::any_cast< boost::multi_array< thing_type , 7 > * >( _any );     \
   f; return( true );                                                        \
   }                                                                         \
  if( _any.type() == typeid( boost::multi_array< thing_type , 8 > * ) ) {    \
   auto & var =                                                              \
    * boost::any_cast< boost::multi_array< thing_type , 8 > * >( _any );     \
   f; return( true );                                                        \
   }                                                                         \
  return( false );                                                           \
  }( my_thing )

/** @} ---------------------------------------------------------------------*/
/*----------------- PRINTING list, array and multi_array -------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup print_multi_arrays Printing list, pair, array and multi_array
 *
 * A few versions of operator<< for printing std::list, std::pair,
 * std::array and boost::multi_array.
 *  @{ */

template< class T1 , class T2 >
std::ostream &
operator<<( std::ostream & os , const std::pair< T1 , T2 > & p ) {
 os << "( " << p.first << ", " << p.second << " )";
 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template< typename T , std::size_t K >
std::ostream & operator<<( std::ostream & os ,
                           const boost::multi_array< T , K > & A ) {
 const T * p = A.data();
 for( boost::multi_array_types::size_type i = A.num_elements() ; i-- ; ++p ) {
  os << "[ ";
  for( boost::multi_array_types::size_type k = 0 ; k < K ; ) {
   os << ( p - A.origin() ) / A.strides()[ k ] % A.shape()[ k ]
         + A.index_bases()[ k ];
   if( ++k < K )
    os << ", ";
   }
  os << " ] = " << *p << std::endl;
 }

 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template< typename T , std::size_t K >
std::ostream & operator<<( std::ostream & os ,
                           const boost::multi_array< T * , K > & A ) {
 typedef T * TP;
 const TP * p = A.data();
 for( boost::multi_array_types::size_type i = A.num_elements() ; i-- ; ++p ) {
  os << "[ ";
  for( boost::multi_array_types::size_type k = 0 ; k < K ; ) {
   os << ( p - A.origin() ) / A.strides()[ k ] % A.shape()[ k ]
         + A.index_bases()[ k ];
   if( ++k < K )
    os << ", ";
   }
  os << " ] = " << **p << std::endl;
  }

 return( os );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template< typename T >
std::ostream & operator<<( std::ostream & os , const std::vector< T > & l ) {
 for( unsigned int i = 0 ; i < l.size() ; ++i )
  os << "[ " << i << " ] = " << l[ i ] << std::endl;

 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template< typename T >
std::ostream & operator<<( std::ostream & os , const std::vector< T * > & l ) {
 for( unsigned int i = 0 ; i < l.size() ; ++i )
  os << "[ " << i << " ] = " << *l[ i ] << std::endl;

 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template< typename T >
std::ostream & operator<<( std::ostream & os , const std::list< T > & l ) {
 auto it = l.begin();
 for( unsigned int i = 0 ; i < l.size() ; ++i , ++it )
  os << i << " ) = " << *it;

 return( os );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

template< typename T >
std::ostream & operator<<( std::ostream & os , const std::list< T * > & l ) {
 auto it = l.begin();
 for( unsigned int i = 0 ; i < l.size() ; ++i , ++it )
  os << i << " ) = " << **it;

 return( os );
 }

/** @} ---------------------------------------------------------------------*/
/*----------------- LOADING things while skipping comments -----------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup eatcomments simple operator which eats up comments in a istream
 *  @{ */

inline std::istream & eatcomments( std::istream & is ) {
 for( ;; ) {
  if( is.eof() )  // never do any other reading if eof
   break;
  is >> std::ws;  // skip whitespaces
  if( is.eof() )  // never do any other reading if eof
   break;
  if( is.peek() == is.widen( '#' ) )
   // a comment: skip the rest of line and move to next
   is.ignore( std::numeric_limits< std::streamsize >::max() ,
              is.widen( '\n' ) );
  else
   break;
  }

 return( is );
 }

/** @} ---------------------------------------------------------------------*/
/*------------------ LOADING list, array and multi_array -------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup load_composite_data structures Loading lists and vectors
 *
 * A few versions of operator>> for loading pairs, lists and vectors out of
 * a std::istream. For pairs, the format is just
 *
 *     first element of the pair
 *     second element of the pair
 *
 * List and vectors can instead be either in dense or sparse format. That is,
 * the format always starts with a signed integer
 *
 *     +/- number of elements k
 *
 * If k == 0, the empty vector is returned. If k > 0, the vector has k
 * elements, and the format is the obvious "dense" one, i.e.,
 *
 *     for i = 1 to k
 *     - i-th element of the array/list
 *
 * If k < 0, instead, the vector/list will have abs( k ) elements. The format
 * is instead the "sparse" one, i.e., (after k)
 *
 *     number of non-default elements h (<= k)
 *     for i = 1 to h
 *     - index 0 <= j < k of the i-th non-default element
 *     - value of the i-th non-default element
 *
 *     IMPORTANT NOTE: THE VALUES OF THE INDICES MUST BE IN THE RIGHT RANGE,
 *     ORDERED IN INCREASING SENSE AND NOT REPLICATED
 *
 * It is assumed that each element can be read with >> itself, which rules
 * out pointers. Elements are separated by whitespaces and comments (see
 * eatcomments above). This creates a problem with dense vectors/lists of
 * std::strings, because there is no way in which one can have any of their
 * elements to be empty; if this is the case, use the sparse input instead.
 */

template< class T1 , class T2 >
std::istream & operator>>( std::istream & is , std::pair< T1 , T2 > & p ) {
 is >> eatcomments >> p.first;
 is >> eatcomments >> p.second;
 return( is );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// note the template wizardry in the third parameter which prevents the
// templated operator>> for "any" std container to clash with the predefined
// one for std:: string

template< template< class ... > class C , typename T ,
 typename std::enable_if< ! std::is_same< C< T > , std::string >::value
                                         >::type * = nullptr >
std::istream & operator>>( std::istream & is , C< T > & l ) {
 int k = 0;
 is >> eatcomments;
 if( ! is.eof() )
  is >> k;

 if( ! k ) {
  l.clear();
  return( is );
  }

 if( k > 0 ) {
  l.resize( k );
  for( auto & li : l )
   is >> eatcomments >> li;
  }
 else {
  l.resize( -k );
  unsigned int h;
  is >> eatcomments >> h;
  auto lit = l.begin();
  for( unsigned int p = 0 ; h-- ; ++p ) {
   unsigned int j;
   is >> eatcomments >> j;
   for( ; p < j ; ++p )
    *(lit++) = T();
   is >> eatcomments >> *(lit++);
   }
  while( lit != l.end() )
   *(lit++) = T();
  }

 return( is );
 }

/** @} ---------------------------------------------------------------------*/
/*------------------ DE/SERIALIZING FROM/TO netCDF FILES -------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup helper functions for serializing/deserializing to/from netCDF
 *  files.
 *
 * The following functions are intended to help in the execution of typical
 * tasks that must be performed when serializing and deserializing objects.
 *  @{ */

template< typename T >
netCDF::NcType typ2nCDF( void ) { return( netCDF::NcOpaqueType() ); }

template<>
inline netCDF::NcType typ2nCDF< char >( void ) {
 return( netCDF::NcChar() );
 }

template<>
inline netCDF::NcType typ2nCDF< int >( void ) { return( netCDF::NcInt() ); }

template<>
inline netCDF::NcType typ2nCDF< double >( void ) {
 return( netCDF::NcDouble() );
 }

template<>
inline netCDF::NcType typ2nCDF< std::string >( void ) {
 return( netCDF::NcString() );
 }

template< class T >
inline constexpr bool is_netCDF_type_v =
 std::is_arithmetic_v< T > || std::is_enum_v< T > ||
 std::is_same_v< T , std::string >;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - - - SERIALIZING AND DESERIALIZING BASIC TYPES - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a simple value out of a given group
/** Deserialize a "simple" value, one for which NcGroup::getVar() is defined,
 * out of the given \p group and into \p data. This is supposed to live in
 * the netCDF variable with name \p name. If a variable with the given name
 * is not present within the given \p group, an exception is thrown unless
 * the variable is declared as optional.
 *
 * If \p group has more than one variable with the same name, then the
 * variable that is deserialized follows the rule defined by the netCDF method
 * NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens, then the
 * variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the value will be obtained
 *                  from.
 *
 * @param[out] data A reference to the object that will store the desired
 *                  value.
 *
 * @param[in] name The name of the variable within the given \p group that
 *                 contains the desired value, default "value".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is
 *                     false, then an exception is thrown. Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise.
 */

template< typename T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , T & data ,
             const std::string & name = "value" , bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( ! optional )
   throw( std::invalid_argument( "deserialize(): " + name +
                                 " not present in group " + group.getName()
                                 ) );
  return( false );
  }

 ncVar.getVar( &data );
 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::string out of a netCDF NcGroup
/** Deserialize a std::string value for which deserialize() is defined as a
 * double pointer to char out of the given \p group and into \p data. This is
 * supposed to live in the netCDF variable with name \p name.
 *
 * If the variable with the given name is not present within the given \p
 * group, an exception is thrown unless the variable is declared as optional.
 *
 * If \p group has more than one variable with the same name, then the
 * variable that is deserialized follows the rule defined by the netCDF method
 * NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens, then the
 * variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the value will be obtained
 *                  from.
 *
 * @param[out] data A reference to the std::string object that will store the
 *                  desired value.
 *
 * @param[in] name The name of the variable within the given \p group that
 *                 contains the desired value, default "value".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is
 *                     false, then an exception is thrown. Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise.
 */

inline bool deserialize( const netCDF::NcGroup & group , std::string & data ,
                         const std::string & name = "value" ,
                         bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( ! optional )
   throw( std::invalid_argument( "deserialize(): " + name +
                                 " not present in group " + group.getName()
                                 ) );
  return( false );
  }

 char * data_ptr;
 ncVar.getVar( &data_ptr );
 data = std::string( data_ptr );
 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a single (scalar) variable into a netCDF NcGroup
/** Serialize a "simple" value, one for which NcGroup::putVar() is defined,
 * out of \p data and into of the variable with name \p name of the given
 * \p group. Note that this can be a std::string, so it's not necessarily
 * a "small" value (which is why it is passed by reference).
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name       The name of the variable that will be added.
 *
 * @param[in] ncType     The netCDF type of the variable to be added.
 *
 * @param[in] data       A reference to the value to be written into the
 *                       variable. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType , const T & data ) {
 ( group.addVar( name , ncType ) ).putVar( &data );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a single (scalar) variable into a netCDF NcGroup
/** Serialize a "simple" value, one for which NcGroup::putVar() is defined,
 * out of \p data and into of the variable with name \p name of the given
 * \p group. Note that this can be a std::string, so it's not necessarily
 * a "small" value (which is why it is passed by reference).
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] data       A reference to the value to be written into the
 *                       variable
 *
 * @param[in] name       The name of the variable that will be added,
 *                       default "value". */

template< typename T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const T & data ,
           const std::string & name = "value" ) {
 serialize( group , name , typ2nCDF< T >() , data );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::pair value out of a netCDF NcGroup
/** Deserialize a std::pair of values for which deserialize() is well-defined
 * (e.g., "simple" values) out of the given \p group and into \p data. This
 * is done by calling deserialize() for data.first on "<name>_f" and for
 * data.second on"<name>_s".
 *
 * @param[in] group The netCDF NcGroup from which the value will be obtained
 *                  from.
 *
 * @param[out] data A reference to the std::pair that will store the desired
 *                  value.
 *
 * @param[in] name The initial part of the name of the two variables within
 *                 the given \p group that contains the desired values,
 *                 default "value".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the any of the two
 *                     variables is not present in the given NcGroup and
 *                     \p optional == false, then an exception is thrown.
 *                     Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise.
 */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , bool >
deserialize( const netCDF::NcGroup & group , std::pair< T1 , T2 > & data ,
             const std::string & name = "value" , bool optional = true ) {
 bool did_it = deserialize( group , data.first , name + "_f" , optional );
 return( deserialize( group , data.second , name + "_s" , optional ) &&
         did_it );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a std::pair value into a given group
/** Serialize a std::pair of values for which deserialize() is well-defined
 * (e.g., "simple" values) out of \p data and into of the two variables with
 * name "<name>_f" and "<name>_s" of the given \p group. */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , void >
serialize( netCDF::NcGroup & group , const std::pair< T1 , T2 > & data ,
           const std::string & name = "value" ) {
 serialize( group , data.first , name + "_f" );
 serialize( group , data.second , name + "_s" );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - - - - - - - - - -DESERIALIZING DIMENSIONS - - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a dimension out of a netCDF NcGroup
/** If the given netCDF NcGroup contains a dimension whose name is \p
 * dim_name, then its size is stored in \p data. If the group does not contain
 * a dimension with the \p dim_name name and the value of the parameter \p
 * optional is false, then an std::invalid_argument exception is thrown.
 *
 * If the given \p group has more than one dimension with the same name, then
 * the dimension that is considered follows the rule defined by the netCDF
 * method NcGroup::getDim(). As of version 4.3.1 of netCDF, if this happens,
 * then the dimension closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the dimension will be
 *                  obtained from.
 *
 * @param[in]  name A string with the name of the dimension.
 *
 * @param[out] data A reference to the object that will store the size of the
 *                  desired dimension; it's template so as to allow the user to
 *                  read it with different types of integers than std::size_t
 *
 * @param[in] optional This parameter informs whether the dimension is
 *                     optional. This means that if the dimension is not
 *                     present in the given NcGroup and \p optional ==
 *                     false, then an exception is thrown. Default is true.
 *
 * @return true if the desired dimension was deserialized; false, otherwise.
 */

template< typename T >
inline bool deserialize_dim( const netCDF::NcGroup & group ,
                             const std::string & name , T & data ,
                             bool optional = true ) {
 netCDF::NcDim ncDim = group.getDim( name );
 if( ncDim.isNull() ) {
  if( optional )
   return( false );
  throw( std::invalid_argument( "deserialize_dim(): " + name +
                                " not present in group '" + group.getName()
                                ) );
  }

 data = ncDim.getSize();
 return( true );
 }

/*--------------------------------------------------------------------------*/
/// return the sizes of the dimensions of a netCDF variable
/** This function receives a netCDF variable and returns a vector whose size
 * is the number of dimensions of the given variable. The i-th position of
 * this vector stores the size of the i-th dimension of the given netCDF
 * variable.
 *
 * @param[in] var The netCDF variable from which the sizes of the dimensions
 *                will be extracted.
 *
 * @return A vector with the sizes of the dimensions of the variable var. */

inline std::vector< std::size_t > get_sizes_dimensions(
 const netCDF::NcVar & var ) {
 std::vector< std::size_t > sizes_dimensions( var.getDimCount() );
 std::vector< std::size_t >::size_type i = 0;
 for( const auto & dim : var.getDims() )
  sizes_dimensions[ i++ ] = dim.getSize();
 return( sizes_dimensions );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - -SERIALIZING AND DESERIALIZING std::vector OF BASIC TYPES - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::vector out of a netCDF NcGroup
/** This function reads a std::vector of "simple" values of type \p T, for
 * which NcGroup::getVar() is defined, from a netCDF variable with name
 * \p name within the given netCDF NcGroup \p group. The values read are
 * stored in the given std::vector \p data.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if \p optional == true, while an
 * std::invalid_argument exception is thrown if \p optional == false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[in] name  The name of the variable within the given group.
 *
 * @param[in] size  The size of the array to be read; note that this is
 *                  *not* checked against the actual size of the variable,
 *                  so it's the caller's responsibility to ensure they match.
 *                  If size == 0, data is clear()-ed and false is returned
 *                  with no other check (not even that the variable exists).
 *
 * @param[out] data A reference to the vector that will store the values of
 *                  the array.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is false,
 *                     an exception is thrown. Default is true.
 *
 * @param[in] allow_scalar_var This parameter indicates whether the desired
 *                             variable (whose name is \p name) can have
 *                             a zero number of dimensions, i.e., it can be
 *                             a scalar instead of an array. Its default
 *                             value is false, and this means it is a scalar
 *                             then an exception is thrown. If, instead, \p
 *                             allow_scalar_var == true, then if the netCDF
 *                             variable is a scalar, then \p data is resized
 *                             to 1 and the value of the netCDF variable is
 *                             stored in the first (and only) position of \p
 *                             data (in this case, \p size is ignored).
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             const std::size_t & size , std::vector< T > & data ,
             bool optional = true , bool allow_scalar_var = false ) {
 if( ! size ) {
  data.clear();
  return( false );
  }

 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   data.clear();
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name + " is not present"
                                ) );
  }

 auto dc = ncVar.getDimCount();
 if( ( ( dc == 0 ) && ( ! allow_scalar_var ) ) || ( dc > 1 ) )
  throw( std::invalid_argument( "deserialize(): netCDF variable " +
                                 name + " of group " + group.getName() +
                                 " has " +
                                 std::to_string( dc ) + " != 1 dimensions" ) );

 if( dc == 0 ) {
  data.resize( 1 );
  ncVar.getVar( &data[ 0 ] );
  return( true );
  }

 data.resize( size );
  ncVar.getVar( { 0 } , { size } , data.data() );

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::vector out of a netCDF NcGroup
/** This function reads a std::vector of "simple" values of type \p T, for
 * which NcGroup::getVar() is defined, from a netCDF variable with name
 * \p name within the given netCDF NcGroup \p group. The values read are
 * stored in the given std::vector \p data.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if \p optional == true, while an
 * std::invalid_argument exception is thrown if \p optional == false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[out] data A reference to the vector that will store the values of
 *                  the array.
 *
 * @param[in] name  The name of the variable within the given group. The
 *                  default is "value".
 *
 * @param[in] size  The name of the dimension of the variable within the
 *                  given group. If there is no dimension with that name
 *                  and \p optional == false then an exception is thrown,
 *                  while if \p optional == true then data is clear()-ed and
 *                  false is returned with no other check (not even that the
 *                  variable exists). The default is "size".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional == false
 *                     then an exception is thrown. The default is true.
 *
 * @param[in] allow_scalar_var This parameter indicates whether the desired
 *                             variable (whose name is \p name) can have
 *                             a zero number of dimensions, i.e., it can be
 *                             a scalar instead of an array. Its default
 *                             value is false, and this means it is a scalar
 *                             then an exception is thrown. If, instead, \p
 *                             allow_scalar_var == true, then if the netCDF
 *                             variable is a scalar, then \p data is resized
 *                             to 1 and the value of the netCDF variable is
 *                             stored in the first (and only) position of \p
 *                             data (in this case, \p size is ignored).
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< typename T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , std::vector< T > & data ,
             const std::string & name = "value" ,
             const std::string & size = "size" ,
             bool optional = true , bool allow_scalar_var = false ) {
 auto dim = group.getDim( size );
 if( dim.isNull() ) {
  if( optional ) {
   data.clear();
   return( false );
  }

  throw( std::invalid_argument( "deserialize(): dimension " + size +
                                 " is not present" ) );
  }

 return( deserialize( group , name , dim.getSize() , data , optional ,
                      allow_scalar_var ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a std::vector into a given group
/** serialize a std::vector of "simple" values, for which NcGroup::putVar()
 * is defined out of \p data and into the variable with name \p name of
 * netCDF type \p ncType with dimension \p ncDim. If \p data is empty,
 * \p group is left unchanged (no netCDF variable is created).
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name       The name of the variable that will be added.
 *
 * @param[in] ncType     The type of the elements of the array.
 *
 * @param[in] ncDim      The netCDF dimension of the array.
 *
 * @param[in] data       A vector containing the data to be stored in the
 *                       variable.
 *
 * @param[in] allow_scalar_var Although this function is supposed to serialize
 *                             an array, it can also be used to serialize a
 *                             scalar. If the \p data has size 1 and \p
 *                             allow_scalar_var == true, then a netCDF scalar
 *                             variable is created instead of a
 *                             multi-dimensional one (notice that, in this
 *                             case, the argument \p ncDim is completely
 *                             ignored). The default is false. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType , const netCDF::NcDim & ncDim ,
           const std::vector< T > & data , bool allow_scalar_var = false ) {
 if( data.empty() )
  return; // Nothing to be serialized.

 if( allow_scalar_var && ( data.size() == 1 ) ) {
  serialize( group , name , ncType , data[ 0 ] );
  return;
  }

 group.addVar( name , ncType , ncDim ).putVar( { 0 } , { data.size() } ,
                                               data.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/** serialize a std::vector of "simple" values, for which NcGroup::putVar()
 * is defined out of \p data and into the variable with name \p name of
 * netCDF type \p ncType with dimension with name \p size. If a dimension
 * with name \p size is already present in \p group then it is used,
 * otherwise it is added. If \p data is empty, \p group is left unchanged
 * (no netCDF variable is created) except possibly for adding \p size.
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] data       A vector containing the data to be stored in the
 *
 * @param[in] name       The name of the variable that will be added,
 *                       default "value".
 *
 * @param[in] size       The name of the dimension of the array, default
 *                       "size".
 *
 * @param[in] allow_scalar_var Although this function is supposed to serialize
 *                             an array, it can also be used to serialize a
 *                             scalar. If the \p data has size 1 and
 *                             \p allow_scalar_var == true, then a netCDF
 *                             scalar variable is created instead of a
 *                             multi-dimensional one (notice that, in this
 *                             case, the argument \p ncDim is completely
 *                             ignored). The default is false. */

template< typename T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::vector< T > & data ,
           const std::string & name = "value" ,
           const std::string & size = "size" ,
           bool allow_scalar_var = false ) {
 auto sz = group.getDim( size );
 if( sz.isNull() )
  sz = group.addDim( size , data.size() );

 serialize( group , name , typ2nCDF< T >() , sz , data , allow_scalar_var );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - -SERIALIZING AND DESERIALIZING std::vector OF std::pair - - - - - -*/
/*- - - - OF BASIC TYPES- - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::vector< std::pair > > out of a netCDF NcGroup
/** This function reads a std::vector< std::pair< T1 , T2 > > where both T1
 * and T2 are "simple" types (for which NcGroup::getVar() is defined) from
 * a netCDF variable with name \p name within the given netCDF NcGroup
 * \p group. The values read are stored in the given
 * std::vector< std::pair >\p data.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if \p optional == true, while an
 * std::invalid_argument exception is thrown if \p optional == false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[in] name  For efficiency, the data is assumed to be stored in
 *                  \p group under the form of *two* one-dimensional
 *                  variables of name \p name + "_f" and \p name + "_s",
 *                  of type T1 and T2 and indexed on the same dimension, with
 *                  the obvious format. If any of the two variables is not
 *                  present and \p optional == true then data is clear()-ed
 *                  and false returned, otherwise exception is throw.
 *
 * @param[in] size  The size of the array to be read; note that this is
 *                  *not* checked against the actual size of the variables,
 *                  so it's the caller's responsibility to ensure they match.
 *                  If size == 0, data is clear()-ed and false is returned
 *                  with no other check (not even that the variable exists).
 *
 * @param[out] data A reference to the vector that will store the values of
 *                  the array.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if any of the two required
 *                     variables is not present in the given NcGroup and
 *                     \p optional is false, an exception is thrown.
 *                     Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             const std::size_t & size ,
             std::vector< std::pair< T1 , T2 > > & data ,
             bool optional = true ) {
 if( ! size ) {
  data.clear();
  return( false );
  }

 std::vector< T1 > f;
 if( ! deserialize( group , name + "_f" , size , f , optional ) ) {
  data.clear();
  return( false );
  }

 std::vector< T2 > s;
 if( ! deserialize( group , name + "_s" , size , s , optional ) ) {
  data.clear();
  return( false );
  }

 data.resize( size );

 auto fit = f.begin();
 auto sit = s.begin();
 for( auto & el : data )
  el = std::pair( *(fit++) , *(sit++) );

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a std::vector< std::pair > > out of a netCDF NcGroup
/** This function reads a std::vector< std::pair< T1 , T2 > > where both T1
 * and T2 are "simple" types (for which NcGroup::getVar() is defined) from
 * a netCDF variable with name \p name within the given netCDF NcGroup
 * \p group. The values read are stored in the given
 * std::vector< std::pair >\p data.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if \p optional == true, while an
 * std::invalid_argument exception is thrown if \p optional == false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[out] data A reference to the std::vector< std::pair< T1 , T2 > >
 *                  that will store the values
 *
 * @param[in] name  For efficiency, the data is assumed to be stored in
 *                  \p group under the form of *two* one-dimensional
 *                  variables of name \p name + "_f" and \p name + "_s",
 *                  of type T1 and T2 and indexed on the same dimension, with
 *                  the obvious format. If any of the two variables is not
 *                  present and \p optional == true then data is clear()-ed
 *                  and false returned, otherwise exception is throw.
 *                  The default is "value".
 *
 * @param[in] size  The name of the dimension of the variable within the
 *                  given group. If there is no dimension with that name
 *                  and \p optional == false then an exception is thrown,
 *                  while if \p optional == true then data is clear()-ed and
 *                  false is returned with no other check (not even that the
 *                  variable exists). The default is "size".
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional == false
 *                     then an exception is thrown. The default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , bool >
deserialize( const netCDF::NcGroup & group ,
             std::vector< std::pair< T1 , T2 > > & data ,
             const std::string & name = "value" ,
             const std::string & size = "size" , bool optional = true ) {
 auto dim = group.getDim( size );
 if( dim.isNull() ) {
  if( optional ) {
   data.clear();
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): dimension " + size +
                                 " is not present" ) );
  }

 return( deserialize( group , name , dim.getSize() , data , optional ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a std::vector< std::pair > > into a given group
/** serialize a std::vector< std::pair< T1 , T2 > > where both T1 and T2 are
 * "simple" types (for which NcGroup::getVar() is defined) out of \p data and
 * into two variable with name \p name + "_f" and \p name + "_s", both of the
 * same dimension \p ncDim. If \p data is empty, \p group is left unchanged
 * (no netCDF variable is created).
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name       The common part of the names of the two variables
 *                       that will be added.
 *
 * @param[in] ncDim      The netCDF dimension of the array.
 *
 * @param[in] data       A vector containing the data to be stored in the
 *                       variables. */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcDim & ncDim ,
           const std::vector< std::pair< T1 , T2 > > & data ) {
 if( data.empty() )
  return;  // nothing to be serialized

 auto size = ncDim.getSize();
 std::vector< T1 > f( size );
 std::vector< T2 > s( size );

 auto fit = f.begin();
 auto sit = s.begin();
 for( auto & el : data ) {
  *(fit++) = el.first;
  *(sit++) = el.second;
  }

 serialize( group , name + "_f" , typ2nCDF< T1 >() , ncDim , f );
 serialize( group , name + "_s" , typ2nCDF< T2 >() , ncDim , s );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a std::vector< std::pair > > into a given group
/** serialize a std::vector< std::pair< T1 , T2 > > where both T1 and T2 are
 * "simple" types (for which NcGroup::getVar() is defined) out of \p data and
 * into two variable with name \p name + "_f" and \p name + "_s", both of the
 * same dimension whose name is \p size. If \p data is empty, \p group is
 * left unchanged (no netCDF variable is created).
 *
 * @param[in, out] group The netCDF NcGroup in which the variables will be
 *                       added.
 *
 * @param[in] data       A std::vector< std::pair< T1 , T2 > > containing the
 *                       data to be stored
 *
 * @param[in] name       The common part of the names of the two variables
 *                       that will be added, default "value".
 *
 * @param[in] size       The name of the dimension of the array, default
 *                       "size". */

template< typename T1 , typename T2 >
std::enable_if_t< is_netCDF_type_v< T1 > && is_netCDF_type_v< T2 > , void >
serialize( netCDF::NcGroup & group ,
           const std::vector< std::pair< T1 , T2 > > & data ,
           const std::string & name = "value" ,
           const std::string & size = "size" ) {
 auto sz = group.getDim( size );
 if( sz.isNull() )
  sz = group.addDim( size , data.size() );

 serialize( group , name , sz , data );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - DESERIALIZING MULTI-DIMENSIONAL VARIABLES OF BASIC TYPES- - - - -*/
/*- - - - STORED ROW-MAJOR INTO A SINGLE std::vector- - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a multi-dimensional variable out of a netCDF NcGroup
/** This function reads a multi-dimensional array of values of type \p T from
 * a netCDF variable with name \p name within the given netCDF NcGroup \p
 * group. The number of dimensions of the multi-dimensional array is given by
 * the size of the vector \p sizes and the i-th entry of \p sizes provides the
 * size of the i-th dimension. The values read are stored in the given vector
 * \p data in row-major layout.
 *
 * If the size of any dimension is zero, then \p data is resized to zero. If
 * the variable is not present in the given \p group, then the vector \p data
 * is resized to zero if the value of the parameter \p optional is true or an
 * std::invalid_argument exception is thrown if \p optional is false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 * from.
 *
 * @param[in] name  The name of the variable within the given \p group.
 *
 * @param[in] sizes A vector containing the sizes of each dimension of the
 *                  multi-dimensional array.
 *
 * @param[out] data A reference to the vector that will store the
 *                  multi-dimensional array in row-major layout.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is
 *                     false, an exception is thrown.
 *
 * @param[in] allow_scalar_var This parameter indicates whether the desired
 *                             variable (whose name is \p name) can have
 *                             dimension zero (i.e., it could be a scalar
 *                             instead of an array). Its default value is
 *                             false and this means that, if the size of the
 *                             given \p sizes vector is not the same as the
 *                             number of dimensions of the netCDF variable or
 *                             the sizes of the dimensions specified by \p
 *                             sizes do not match that of the netCDF
 *                             variable, an exception is thrown. If \p
 *                             allow_scalar_var is true, this means that if
 *                             the netCDF variable has dimension zero (i.e.,
 *                             it is a scalar), then the given vector \p data
 *                             is resized to 1 and the value of the netCDF
 *                             variable is stored in the first position of \p
 *                             data (notice that, in this case, the given
 *                             vector \p sizes is completely ignored).
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             const std::vector< std::size_t > & sizes ,
             std::vector< T > & data , bool optional = true ,
             bool allow_scalar_var = false ) {
 if( sizes.empty() ) {
  data.resize( 0 );
  return( false );
  }

 auto total_size = std::accumulate( begin( sizes ) , end( sizes ) , 1 ,
                                    std::multiplies< std::size_t >() );
 if( total_size == 0 ) {
  data.resize( 0 );
  return( false );
  }

 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   data.resize( 0 );
   return( false );
  }

  throw( std::invalid_argument( "deserialize(): " + name +
                                 " is not present" ) );
  }

 if( ( ( ncVar.getDimCount() < 0 ) ||
       ( sizes.size() != static_cast< decltype( sizes.size() ) >(
        ncVar.getDimCount() ) ) ) &&
     ( ( ncVar.getDimCount() != 0 ) || ( ! allow_scalar_var ) ) )
  throw( std::invalid_argument( "deserialize(): netCDF variable " +
                                 name + " of group " + group.getName() +
                                 " has " +
                                 std::to_string( ncVar.getDimCount() ) +
                                 " dimension(s), but provided argument has " +
                                 std::to_string( sizes.size() ) +
                                 " dimension(s)." ) );

 if( ncVar.getDimCount() == 0 ) {
  data.resize( 1 );
  ncVar.getVar( &data[ 0 ] );
  return( true );
  }

 auto var_sizes = get_sizes_dimensions( ncVar );

 if( sizes != var_sizes )
  throw( std::invalid_argument(
   "deserialize(): given sizes of dimensions and the sizes of "
   "dimensions of netCDF variable " + name + " of group " +
   group.getName() + " do not match" ) );

 data.resize( total_size );

 std::vector< std::size_t > start( sizes.size() , 0 );

 ncVar.getVar( start , sizes , data.data() );

 return( true );
 }

/*--------------------------------------------------------------------------*/
/// deserialize a multi-dimensional variable out of a netCDF NcGroup
/** This function reads a multi-dimensional array of values of type \p T from
 * a netCDF variable with name \p name within the given netCDF NcGroup \p
 * group. The values that are read are stored in the given vector \p data in
 * row-major layout.
 *
 * If the netCDF variable has no dimension (i.e., it is a scalar variable),
 * then \p data is resized to 1 and its only element receives the value of the
 * netCDF variable.
 *
 * If the netCDF variable has at least one dimension and the size of any of
 * its dimension is zero, then \p data is resized to zero.
 *
 * If the variable is not present in the given \p group, then the vector \p
 * data is resized to zero if the value of the parameter \p optional is true
 * or an std::invalid_argument exception is thrown if \p optional is false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[in] name  The name of the variable within the given \p group.
 *
 * @param[out] data A reference to the vector that will store the
 *                  multi-dimensional array in row-major layout.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup and \p optional is false,
 *                     an exception is thrown.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             std::vector< T > & data , bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   data.resize( 0 );
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " not present in group '" + group.getName()
                                ) );
  }

 auto sizes_dimensions = get_sizes_dimensions( ncVar );
 if( sizes_dimensions.empty() ) {
  // The variable is a scalar one.
  data.resize( 1 );
  ncVar.getVar( data.data() );
  return( true );
  }

 auto total_size = std::accumulate( begin( sizes_dimensions ) ,
                                    end( sizes_dimensions ) , 1 ,
                                    std::multiplies< std::size_t >() );
 if( total_size == 0 ) {
  data.resize( 0 );
  return( false );
  }

 data.resize( total_size );

 std::vector< std::size_t > start( sizes_dimensions.size() , 0 );

 ncVar.getVar( start , sizes_dimensions , data.data() );

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - SERIALIZING AND DESERIALIZING BI-DIMENSIONAL VARIABLES OF - - - -*/
/*- - - - BASIC TYPES WITH VARIABLE-LENGTH ROWS, I.E.,- - - - - - - - - - -*/
/*- - - - std::vector< std::vector< T > > - - - - - - - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a matrix with variable-length rows
/** This function reads a matrix with variable-length rows, i.e., a
 * std::vector< std::vector< T > >. This is obtained in the netCDF by having
 * two distinct netCDF::NcVars: a one-dimensional T array val[] (whose name is
 * \p name), and a one-dimensional int array start[] (whose name is \p
 * start_name). The length of start[] gives the number of rows in \p array;
 * the elements of start[] are supposed to be non-negative, ordered in
 * non-decreasing sense, and smaller than the number of elements of val[].
 * Then, the i-th row of \p array will contain the elements found in val[]
 * with indices in the closed-open interval [ start[ i ] , start[ i + 1 ] ),
 * except for the last row for which start[ i + 1 ] is undefined and the total
 * number of elements in val[] is used instead. Note that one typically expect
 * start[ 0 ] == 0, but this is not enforced for the slight chance that this
 * may be useful to the user; clearly, the elements of val[] before start[ 0 ]
 * are ignored.
 *
 * @param[in] group The netCDF::NcGroup from which the matrix with
 *                  variable-length rows will be obtained.
 *
 * @param[in] name  The name of the one-dimensional netCDF::NcVar of type
 *                  T within the given \p group in which the values to be stored
 *                  in a 1D array will be found;
 *
 * @param[in] start_name The name of the one-dimensional netCDF::NcVar (whose
 *                       type is compatible with T) within the given \p group
 *                       which tells how the elements of name are subdivided
 *                       between the rows of \p array; start_name is optional
 *                       if name is not found in \p group;
 *
 * @param[out] array A reference to the std::vector< std::vector< T > > to
 *                   be deserialized;
 *
 * @param[in] optional A bool stating if it is allowed for \p name not to be
 *                     a netCDF::NcVar in \p group, in which case an empty \p
 *                     array is returned as opposed to throwing exception.
 *                     Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             const std::string & start_name ,
             std::vector< std::vector< T > > & array , bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   array.clear();
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " is not present in group " +
                                group.getName() ) );
  }

 if( ncVar.getDimCount() != 1 )
  throw( std::invalid_argument( "deserialize(): " + name +
                                " has wrong number of dimensions" ) );

 auto ncVar_S = group.getVar( start_name );
 if( ncVar_S.isNull() )
  throw( std::invalid_argument( "deserialize(): " + start_name +
                                " is not present in group " +
                                group.getName() ) );

 if( ncVar_S.getDimCount() != 1 )
  throw( std::invalid_argument( "deserialize(): " + start_name +
                                " has wrong number of dimensions" ) );

 auto nrows = ncVar_S.getDim( 0 ).getSize();
 std::vector< int > strt( nrows + 1 );
 ncVar_S.getVar( strt.data() );

 strt[ nrows ] = ncVar.getDim( 0 ).getSize();

 for( decltype( nrows ) i = 0 ; i < nrows ; ++i ) {
  strt[ i ] = strt[ i + 1 ] - strt[ i ];
  if( strt[ i ] < 0 )
   throw( std::invalid_argument( "deserialize(): wrong indices in " +
                                 start_name ) );
  }

 std::vector< T > tmp( ncVar.getDim( 0 ).getSize() );
 ncVar.getVar( tmp.data() );

 array.resize( nrows );

 auto tit = tmp.begin();
 for( decltype( nrows ) i = 0 ; i < nrows ; ++i ) {
  array[ i ].resize( strt[ i ] );
  auto aiit = array[ i ].begin();
  for( const auto aiend = array[ i ].end() ; aiit != aiend ; )
   *(aiit++) = *(tit++);
  }

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a matrix with variable-length rows
/** This function serializes a matrix with variable-length rows, i.e., a
 * std::vector< std::vector< T > >. This matrix is serialized by adding two
 * distinct netCDF::NcVars: a one-dimensional array val[] (with name \p name,
 * type \p ncType, and size equal to the total number of \p T elements in
 * \p array) and a one-dimensional array start[] (with name \p start_name,
 * type netCDF::NcUint, and size equal to the number of rows of \p array).
 * The array val[] will contain the elements of \p matrix in row-major layout.
 * The i-th element of start[] will contain the index in val[] of the first
 * element of the i-th row of \p array. That is,
 *
 *   start[ i ] = sum_{j = 0}^{i - 1} array[ i ].size().
 *
 * Thus, the i-th row of \p array will be saved into the elements of val[]
 * with indices in the closed-open interval [ start[ i ] , start[ i + 1 ] ),
 * except for the last row for which start[ i + 1 ] is undefined and whose
 * elements are saved into the elements of val[] with indices in the
 * closed-open interval [ start[ i ] , | val | ), where | val | is the size
 * of val[].
 *
 * @param[in] group The netCDF::NcGroup into which the matrix with
 *                   variable-length rows will be serialized.
 *
 * @param[in] name  The name of the one-dimensional netCDF::NcVar of type
 *                  \p ncType to be added to the given \p group and into
 *                  which the values of \p array will be saved;
 *
 * @param[in] ncType The type of the netCDF variable whose name is \p name.
 *
 * @param[in] start_name The name of the one-dimensional netCDF::NcVar to be
 *                       added to the given \p group that represents the
 *                       start[] array;
 *
 * @param[in] array A reference to the std::vector< std::vector< T > > to
 *                  be serialised;
 *
 * @param[in] array_dim The netCDF dimension of the netCDF variable whose name
 *                      is \p name. This dimension must already be part of \p
 *                      group. If it is not provided, then a dimension with
 *                      name given by the concatenation of \p name and "_dim"
 *                      will be added to \p group. For instance, if it is not
 *                      provided and \p name is the string "matrix", then a
 *                      dimension with name "matrix_dim" will be added to \p
 *                      group.
 *
 * @param[in] start_dim The netCDF dimension of the netCDF variable whose name
 *                      is \p start_name. This dimension must already be part
 *                      of \p group. If it is not provided, then a dimension
 *                      with name given by the concatenation of \p start_name
 *                      and "_dim" will be added to \p group. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType , const std::string & start_name ,
           const std::vector< std::vector< T > > & array ,
           const netCDF::NcDim & array_dim = netCDF::NcDim() ,
           const netCDF::NcDim & start_dim = netCDF::NcDim() ) {
 if( array.empty() )
  return; // Nothing to be serialized.

 // Compute the total number of elements in the given array
 decltype( array.size() ) num_elements = 0;
 for( decltype( array.size() ) i = 0 ; i < array.size() ; ++i ) {
  num_elements += array[ i ].size();
  }

 // Add the netCDF variable that will store the elements of the given array
 netCDF::NcVar array_var;

 if( ! array_dim.isNull() ) {
  if( num_elements != array_dim.getSize() )
   throw( std::invalid_argument(
    "serialize(): variable " + name + " of group " + group.getName() +
    ", the given dimension is not compatible with the given array." ) );
  array_var = group.addVar( name , ncType , { array_dim } );
  }
 else {
  // Add a default dimension for variable "name"
  auto dim = group.addDim( name + "_dim" , num_elements );
  array_var = group.addVar( name , ncType , { dim } );
  }

 // Save the elements of the given array into the netCDF variable "name"
 decltype( array.size() ) offset = 0;
 for( decltype( array.size() ) i = 0 ; i < array.size() ; ++i ) {
  array_var.putVar( { offset } , { array[ i ].size() } , array[ i ].data() );
  offset += array[ i ].size();
  }

 // Create the "start" vector
 std::vector< unsigned int > start( array.size() );
 start.front() = 0;
 for( decltype( array.size() ) i = 1 ; i < array.size() ; ++i )
  start[ i ] = start[ i - 1 ] + array[ i - 1 ].size();

 // Add the netCDF variable "start"
 netCDF::NcVar start_var;

 if( ! start_dim.isNull() ) {
  if( start_dim.getSize() != array.size() )
   throw( std::invalid_argument(
    "serialize(): variable " + name + " of group " + group.getName() +
    ", the size of the given dimension to the 'start' netCDF variable is "
    "different from the number of rows of the given array" ) );
  start_var = group.addVar( start_name , netCDF::NcUint() , { start_dim } );
  }
 else {
  // Add a default dimension for variable "start_name".
  auto dim = group.addDim( start_name + "_dim" , array.size() );
  start_var = group.addVar( start_name , netCDF::NcUint() , { dim } );
  }

 // Save the start vector into the netCDF variable "start_name"
 start_var.putVar( start.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - SERIALIZING AND DESERIALIZING BI-DIMENSIONAL VARIABLES OF - - - -*/
/*- - - - BASIC TYPES WITH FIXED-LENGTH ROWS STORED INTO A- - - - - - - - -*/
/*- - - - std::vector< std::vector< T > > - - - - - - - - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a matrix (with fixed-length rows)
/** This function reads a matrix with fixed-length rows stored into a
 * std::vector< std::vector< T > > where each "inner" vector has the same
 * size. This is obtained from the two-dimensional netCDF variable whose
 * name is \p name in the given netCDF \p group.
 *
 * @param[in] group The netCDF::NcGroup from which the matrix will be
 *                  obtained.
 *
 * @param[in] name The name of the two-dimensional netCDF::NcVar within the
 *                 given \p group which contains the data to be stored in \p
 *                 matrix;
 *
 * @param[out] matrix A reference to the std::vector< std::vector< T > > to be
 *                    deserialized;
 *
 * @param[in] optional A bool stating if it is allowed for \p name not to be
 *                     be present in the netCDF::NcVar in \p group, in which
 *                     case an empty \p matrix is returned as opposed to
 *                     throwing an exception. Default is true.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             std::vector< std::vector< T > > & matrix , bool optional = true ) {
 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   matrix.clear();
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " is not present in group " +
                                group.getName() ) );
  }

 if( ncVar.getDimCount() != 2 )
  throw( std::invalid_argument( "deserialize(): " + name +
                                " has wrong number of dimensions" ) );

 auto num_rows = ncVar.getDim( 0 ).getSize();
 auto num_cols = ncVar.getDim( 1 ).getSize();

 matrix.resize( num_rows );
 for( decltype( num_rows ) i = 0 ; i < num_rows ; ++i ) {
  matrix[ i ].resize( num_cols );
  ncVar.getVar( { i , 0 } , { 1 , num_cols } , matrix[ i ].data() );
  }

 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a matrix (with fixed-length rows)
/** This function serializes a matrix with fixed-length rows stored into a
 * std::vector< std::vector< T > > where each "inner" vector has the same
 * size. A netCDF variable with name \p name and type \p ncType is created
 * in the given netCDF \p group and the given \p matrix is serialized into
 * this variable. The pair \p dimensions must contain the dimensions of this
 * matrix: the first dimension is the number of rows and the second one is the
 * number of columns of the matrix. Note that these dimensions must either
 * already be present in the given \p group or be null, in which case two
 * new dimensions with "default" names are added to \p group.
 *
 * @param[in] group The netCDF::NcGroup into which the matrix will be
 *                  serialized.
 *
 * @param[in] name  The name of the two-dimensional netCDF::NcVar which
 *                  will be added to the given \p group and store the data of
 *                  the given \p matrix.
 *
 * @param[in] ncType The type of the netCDF variable whose name is \p name.
 *
 * @param[out] matrix A reference to the std::vector< std::vector< T > > to be
 *                    serialised;
 *
 * @param[in] dimensions The netCDF dimensions of the netCDF variable whose
 *                       name is \p name. If the first dimension is null, then
 *                       a dimension with name given by the concatenation of
 *                       \p name and "_rows" will be added to the given \p
 *                       group. If the second dimension is null, then a
 *                       dimension with name given by the concatenation of \p
 *                       name and "_cols" will be added to the given \p group.
 */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType ,
           const std::vector< std::vector< T > > & matrix ,
           std::pair< netCDF::NcDim , netCDF::NcDim > dimensions = {} ) {
 if( matrix.empty() )
  return;  // nothing to be serialized

 auto num_rows = matrix.size();
 auto num_cols = matrix.front().size();

 if( ! num_cols )
  return;  // nothing to be serialized

 if( std::any_of( matrix.begin() , matrix.end() ,
                  [ num_cols ]( auto & ci ) {
                   return( ci.size() != num_cols );
                   } ) )
  throw( std::invalid_argument(
   "serialize(): matrix rows are not all of the same length" ) );

 if( dimensions.first.isNull() )
  dimensions.first = group.addDim( name + "_rows" , num_rows );
 else
  if( num_rows != dimensions.first.getSize() )
   throw( std::invalid_argument(
     "serialize(): variable " + name + " of group " + group.getName() +
     ", matrix rows do not match with first dimension" ) );

 if( dimensions.second.isNull() )
  dimensions.second = group.addDim( name + "_cols" , matrix.front().size() );
 else
  if( num_cols != dimensions.second.getSize() )
   throw( std::invalid_argument(
     "serialize(): variable " + name + " of group " + group.getName() +
     ", matrix columns do not match with second dimension" ) );

 // add the netCDF variable
 auto ncVar = group.addVar( name , ncType ,
                            { dimensions.first , dimensions.second } );

 // set the data of the netCDF variable
 for( decltype( num_rows ) i = 0 ; i < num_rows ; ++i )
  ncVar.putVar( { i , 0 } , { 1 , num_cols } , matrix[ i ].data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - SERIALIZING AND DESERIALIZING MULTI-DIMENSIONAL VARIABLES - - - -*/
/*- - - - OF BASIC TYPES STORED INTO A boost::multi_array< T > >- - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize a multi-dimensional variable out of a netCDF NcGroup
/** This function reads a multi-dimensional array of values of type \p T, from
 * a netCDF variable with name \p name within the given netCDF NcGroup \p
 * group. The number of dimensions of the multi-dimensional array is given by
 * the template parameter \p N. The values read are stored in the given \p
 * array in row-major layout.
 *
 * If the variable is not present in the given \p group, then the array is
 * emptied (every dimension becomes zero-sized) if the value of the parameter
 * \p optional is true or an std::invalid_argument exception is thrown if \p
 * optional is false.
 *
 * If the given \p group has more than one variable with the same name, then
 * the variable that is considered follows the rule defined by the netCDF
 * method NcGroup::getVar(). As of version 4.3.1 of netCDF, if this happens,
 * then the variable closest to the given group is considered.
 *
 * @param[in] group The netCDF NcGroup from which the array will be obtained
 *                  from.
 *
 * @param[in] name  The name of the variable within the given \p group.
 *
 * @param[out] array A reference to the boost::multi_array that will store
 *                   the multi-dimensional array in row-major layout.
 *
 * @param[in] optional This parameter informs whether the variable is
 *                     optional. This means that if the variable is not
 *                     present in the given NcGroup then: (i) an exception is
 *                     thrown if \p optional == false; (ii) every dimension of
 *                     \p array will have size 0 if \p optional == true.
 *                     Default is true.
 *
 * @param[in] allow_scalar_var This parameter indicates whether the desired
 *                             variable (whose name is \p var_name) can have
 *                             dimension zero (i.e., it could be a scalar
 *                             instead of an array). Its default value is
 *                             false and this means that, if the variable has
 *                             dimension zero (i.e., it is a scalar), an
 *                             exception is thrown. If, instead, \p
 *                             allow_scalar_var == true, then if the netCDF
 *                             variable is a scalar, then the given
 *                             boost::multi_array \p array will have a single
 *                             element (the origin) whose value will be that
 *                             of the netCDF variable.
 *
 * @return true if the desired variable was deserialized; false, otherwise. */

template< class T , std::size_t N >
std::enable_if_t< is_netCDF_type_v< T > , bool >
deserialize( const netCDF::NcGroup & group , const std::string & name ,
             boost::multi_array< T , N > & array , bool optional = true ,
             bool allow_scalar_var = false ) {
 using index = typename boost::multi_array< T , N >::index;

 auto ncVar = group.getVar( name );
 if( ncVar.isNull() ) {
  if( optional ) {
   std::vector< index > new_sizes( N , 0 );
   array.resize( new_sizes );
   return( false );
   }

  throw( std::invalid_argument( "deserialize(): " + name +
                                " not present in group " + group.getName() ) );
  }

 if( ncVar.getDimCount() == 0 ) {
  if( allow_scalar_var ) {
   std::vector< index > new_sizes( N , 1 );
   array.resize( new_sizes );
   ncVar.getVar( array.origin() );
   return( true );
   }

  throw( std::invalid_argument(
   "deserialize(): netCDF variable " + name + " is a scalar," +
   " but a multi-dimensional array with " + std::to_string( N ) +
   " dimensions was expected in group " + group.getName() ) );
  }

 auto sizes_dimensions = get_sizes_dimensions( ncVar );
 if( sizes_dimensions.size() < array.num_dimensions() )
  sizes_dimensions.resize( array.num_dimensions() , 1 );

 array.resize( sizes_dimensions );

 std::vector< std::size_t > start( sizes_dimensions.size() , 0 );

 ncVar.getVar( start , sizes_dimensions , array.data() );
 return( true );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a multi-dimensional variable into a netCDF NcGroup
/** Add a new multi-dimensional netCDF variable with the given \p name in
 * the given netCDF NcGroup \p group. Moreover, it stores the given data into
 * that variable in row-major layout.
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name       The name of the variable that will be added.
 *
 * @param[in] ncType     The type of the elements of the array.
 *
 * @param[in] ncDim      A vector with the netCDF dimensions of the array.
 *
 * @param[in] data       A vector containing the data to be stored in the
 *                       variable in row-major layout.
 *
 * @param[in] sizes This is an optional parameter that indicates the sizes of
 *                  each dimension of the multi-dimensional array to be
 *                  serialized. If the number of dimensions is greater than 1,
 *                  then (a) if \p sizes is not provided then unlimited
 *                  dimensions are not supported (an exception is thrown in
 *                  this case); (b) if \p sizes is provided, it must have the
 *                  same number of elements as \p ncDim (otherwise, an
 *                  exception is thrown).
 *
 * @param[in] allow_scalar_var Although this function is supposed to serialize
 *                             a multi-dimensional array, it can also be used
 *                             to serialize a scalar. If the given vector \p
 *                             data has size 1 and \p allow_scalar_var is
 *                             true, then a netCDF scalar variable is created
 *                             instead of a multi-dimensional one (notice
 *                             that, in this case, the argument \p ncDim is
 *                             completely ignored). */

template< class T >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType ,
           const std::vector< netCDF::NcDim > & ncDim ,
           const std::vector< T > & data ,
           const std::vector< std::size_t > & sizes = {} ,
           bool allow_scalar_var = false ) {
 if( data.empty() )
  return; // Nothing to be serialized.

 if( ( ncDim.size() == 1 ) && ( sizes.size() <= 1 ) )
  serialize( group , name , ncType , ncDim[ 0 ] , data , allow_scalar_var );

 if( allow_scalar_var && ( data.size() == 1 ) ) {
  // Serializes the only element of the given vector as a scalar variable.
  serialize( group , name , ncType , data[ 0 ] );
  return;
 }

 if( sizes.empty() ) {
  // The sizes of the dimensions of the multidimensional array to be
  // serialized were not provided. We thus consider the sizes of the given
  // netCDF dimensions. In this case, unlimited dimensions are not allowed.

  std::vector< std::size_t > sz( ncDim.size() );
  for( std::size_t i = 0 ; i < ncDim.size() ; ++i ) {
   sz[ i ] = ncDim[ i ].getSize();

   if( ncDim[ i ].isUnlimited() )
    throw( std::invalid_argument(
     "serialize(): error when serializing variable " + name +
     " of group " + group.getName() + ". The given netCDF dimension " +
     std::to_string( i ) +
     " is unlimited, but unlimited dimension is not supported when"
     " the sizes of each dimension of the multi-dimensional array"
     " represented by the vector parameter 'data' are not provided." ) );
   }

  std::vector< std::size_t > start( ncDim.size() , 0 );
  group.addVar( name , ncType , ncDim ).putVar( start , sz , data.data() );
  }
 else
  if( ncDim.size() != sizes.size() )
   throw( std::invalid_argument(
   "serialize(): error when serializing variable " + name +
   " of group " + group.getName() + ". The vector 'sizes' has size " +
   std::to_string( sizes.size() ) + ", while the vector 'ncDim' has size "
   + std::to_string( ncDim.size() ) +
   ". When the optional vector parameter 'sizes' is present,"
   " it must have the same size as the vector 'ncDim'" ) );
  else {
   std::vector< std::size_t > start( ncDim.size() , 0 );
   group.addVar( name , ncType , ncDim ).putVar( start , sizes ,
                                                 data.data() );
   }
 }

/*--------------------------------------------------------------------------*/
/// serialize a multi-dimensional variable into a netCDF NcGroup
/** Add a new multi-dimensional netCDF variable with the given name in the
 * given netCDF NcGroup. Moreover, it stores the given data into that variable
 * in row-major layout. The data is given by the boost::multi_array \p
 * multi_array. The type of the elements in \p multi_array is determined by
 * the template parameter \p T, while the number of dimensions of the \p
 * multi_array is given by the template parameter \p N.
 *
 * The netCDF dimensions of the array are given by the \p expected_ncDim
 * parameter. In general, the size of each of these dimensions should be the
 * same as the size of the corresponding dimension of the given \p multi_array
 * (i.e., size of dimension i of the given netCDF dimensions should be equal
 * to the size of dimension i of \p multi_array). There are three cases in
 * which those sizes do not need agree:
 *
 * 1) If the given \p multi_array has a single element and \p
 *    allow_scalar_var is true, then this element is serialized into a
 *    netCDF scalar variable. In this case, the given \p expected_ncDim is
 *    ignored.
 *
 * 2) If a netCDF dimension given in \p expected_ncDim has size greater than
 *    1 but the size of the corresponding dimension in the given \p
 *    multi_array is 1 and \p allow_singleton_dim is true, then that
 *    dimension will have size 1 in the netCDF variable. This artificial
 *    dimension with size 1 is referred to as the singleton dimension. The
 *    name of the singleton dimension is specified by the \p
 *    singleton_dim_name parameter. If the singleton dimension is not
 *    present in the given \p group, one is created with the name given by
 *    \p singleton_dim_name.
 *
 * 3) If a netCDF dimension is unlimited, then the corresponding dimension of
 *    \p multi_array can have any size.
 *
 * Notice that the given \p multi_array will not be serialized in the given \p
 * group if some of its dimensions has size 0 (i.e., empty arrays are not
 * serialized).
 *
 * If the given \p multi_array is not serialized into a netCDF *scalar*
 * variable, then an exception is thrown in each of the following cases:
 *
 * - The size of the \p expected_ncDim vector is different from the size of
 *   the template parameter \p N.
 *
 * - The size of some dimension i in the \p expected_ncDim vector is positive,
 *   it is different from the size s_i of dimension i of \p multi_array and
 *   either \p allow_singleton_dim is false or s_i != 1.
 *
 * @param[in, out] group The netCDF NcGroup in which the variable will be
 *                       added.
 *
 * @param[in] name The name of the variable that will be added.
 *
 * @param[in] ncType The type of the elements of the \p multi_array.
 *
 * @param[in] expected_ncDim A vector with the (expected) netCDF dimensions of
 *                           the array. If the given \p multi_array is not
 *                           serialized into a netCDF *scalar* variable, then
 *                           the size of this vector must necessarily be
 *                           equal to the value of the template parameter \p
 *                           N (otherwise, an exception is thrown).
 *
 * @param[in] multi_array A boost::multi_array containing the data to be
 *                        stored in the netCDF variable in row-major layout.
 *
 * @param[in] allow_scalar_var Although this function is supposed to serialize
 *                             an array, it can also be used to serialize a
 *                             scalar. If the given \p multi_array has size 1
 *                             and \p allow_scalar_var == true, then a netCDF
 *                             *scalar* variable is created instead of a
 *                             multi-dimensional one (notice that, in this
 *                             case, the argument \p expected_ncDim is
 *                             completely ignored).
 *
 * @param[in] allow_singleton_dim This parameter indicates whether singleton
 *                                dimensions are allowed. If
 *                                \p allow_singleton_dim == true, then a
 *                                dimension given in \p expected_ncDim may be
 *                                replaced by a singleton dimension as
 *                                explained above. If \p allow_singleton_dim
 *                                == false, then, for each i in {0, ...,
 *                                N-1}, the size of the i-th dimension in \p
 *                                expected_ncDim must be equal to the size of
 *                                the i-th dimension of \p multi_array,
 *                                except when (i) the \p multi_array is
 *                                serialized into a netCDF *scalar* variable
 *                                or (ii) the i-th dimension in \p
 *                                expected_ncDim is unlimited. If \p
 *                                allow_singleton_dim == false and neither
 *                                condition (i) nor condition (ii) is met,
 *                                then an exception is thrown when the i-th
 *                                dimension has different sizes in \p
 *                                expected_ncDim and \p multi_array.
 *
 * @param[in] singleton_dim_name The name of the singleton dimension. If the
 *                                singleton dimension is used (see
 *                                \p allow_singleton_dim parameter), then the
 *                                dimension whose name is given by \p
 *                                singleton_dim_name is considered. If this
 *                                dimension is present in the given \p group,
 *                                it must have size 1 (otherwise, an
 *                                exception is thrown). If a dimension with
 *                                this name is not found in \p group, a
 *                                dimension with this name is added to the \p
 *                                group. */

template< class T , std::size_t N >
std::enable_if_t< is_netCDF_type_v< T > , void >
serialize( netCDF::NcGroup & group , const std::string & name ,
           const netCDF::NcType & ncType ,
           const std::vector< netCDF::NcDim > & expected_ncDim ,
           const boost::multi_array< T , N > & multi_array ,
           bool allow_scalar_var = false , bool allow_singleton_dim = false ,
           const std::string & singleton_dim_name = "__Singleton__" ) {
 if( multi_array.num_elements() == 0 )
  return; // Nothing to be serialized.

 if( allow_scalar_var && multi_array.num_elements() == 1 ) {
  // Serializes the only value of the given multi_array into a netCDF scalar
  // variable.
  serialize( group , name , ncType , *multi_array.origin() );
  return;
  }

 if( expected_ncDim.size() != N )
  throw( std::invalid_argument(
   "serialize(): variable " + name + " of group " + group.getName() +
   ", the given boost::multi_array has " + std::to_string( N ) +
   " dimensions, but " + std::to_string( expected_ncDim.size() ) +
   " netCDF::NcDim were provided" ) );

 auto ncDim = expected_ncDim;

 for( std::vector< netCDF::NcDim >::size_type i = 0 ; i < N ; ++i ) {
  if( ncDim[ i ].isUnlimited() )
   continue;

  auto ncdim_size = ncDim[ i ].getSize();
  auto multi_array_dim_size = multi_array.shape()[ i ];

  if( ncdim_size != multi_array_dim_size ) {
   if( allow_singleton_dim && multi_array_dim_size == 1 ) {
    // Ignore the given netCDF::NcDim for the i-th dimension and consider
    // the singleton dimension instead.

    auto singleton_dim = group.getDim( singleton_dim_name );

    if( singleton_dim.isNull() )
     // The singleton dimension is currently not present in the given
     // group. So, we add the singleton dimension to the given group.
     singleton_dim = group.addDim( singleton_dim_name , 1 );
    else
     if( singleton_dim.getSize() != 1 ) {
      std::string error;
      error += "serialize(): error when serializing variable ";
      error += name + " of group " + group.getName();
      error += "'. The singleton dimension must have size 1,"
               "but the dimension ";
      error += singleton_dim_name + " has size ";
      error += std::to_string( singleton_dim.getSize() );
      throw( std::invalid_argument( error ) );
      }

    ncDim[ i ] = singleton_dim;
    }
   else {
    throw( std::invalid_argument(
     "serialize(): variable " + name + " of group " + group.getName() +
     ", the size of dimension " + std::to_string( i ) +
     " of the given boost::multi_array is " +
     std::to_string( multi_array_dim_size ) +
     " but the size of the provided netCDF dimension is " +
     std::to_string( ncdim_size ) +
     ". At least one of the following requirements must be met:"
     " (1) these sizes are equal;"
     " (2) the given boost::multi_array has a single element and"
     " 'allow_scalar_var' is true;"
     " (3) the size of the dimension of the given boost::"
     "multi_array is 1 and 'allow_singleton_dim' is true;"
     " (4) provided netCDF dimension is unlimited." ) );
    }
   }
  }

 std::vector< std::size_t > start( N , 0 );
 std::vector< std::size_t > dim_sizes( N );
 for( std::vector< std::size_t >::size_type i = 0 ; i < N ; ++i )
  dim_sizes[ i ] = multi_array.shape()[ i ];

 group.addVar( name , ncType , ncDim ).putVar( start , dim_sizes ,
                                               multi_array.data() );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/*- - - - CHECKING netCDF FILES FOR (UN)EXPECTED STUFF  - - - - - - - - - -*/
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// check if all the expected netCDF variables are present in a group
/** Check if all the expected netCDF variables are present in a group
 *
 * @param group    The group to be checked
 * @param expected The names of the expected variables
 * @param stream   The output stream for putting out the unexpected variables
 */

inline void check_variables( const netCDF::NcGroup & group ,
                             const std::vector< std::string > & expected ,
                             std::ostream & stream ) {
 auto vars = group.getVars();
 for( const auto & e : expected ) {
  auto search = vars.find( e );
  if( search != vars.end() )
   vars.erase( search );
  }

 if( ! vars.empty() ) {
  stream << "Unexpected netCDF Vars found in group: ";
  for( const auto & v : vars )
   stream << v.first << "; ";
  stream << std::endl;
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// check if all the expected netCDF dimensions are present in a group
/** Check if all the expected netCDF dimensions are present in a group
 *
 * @param group    The group to be checked
 * @param expected The names of the expected dimensions
 * @param stream   The output stream for putting out the unexpected dimensions
 */

inline void check_dimensions( const netCDF::NcGroup & group ,
                              const std::vector< std::string > & expected ,
                              std::ostream & stream ) {
 auto dims = group.getDims();
 for( const auto & e : expected ) {
  auto search = dims.find( e );
  if( search != dims.end() )
   dims.erase( search );
  }

 if( ! dims.empty() ) {
  stream << "Unexpected netCDF Dims found in group: ";
  for( const auto & d : dims )
   stream << d.first << "; ";
  stream << std::endl;
  }
 }

/** @} ---------------------------------------------------------------------*/

} // end( namespace SMS_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* SMSTypedefs.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SMSTypedefs.h ---------------------------*/
/*--------------------------------------------------------------------------*/
