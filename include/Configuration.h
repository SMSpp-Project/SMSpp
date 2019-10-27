/*--------------------------------------------------------------------------*/
/*------------------------ File Configuration.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the Configuration class, only the basically empty top of
 * a hierarchy of objects intended to provide possibly complex configuration
 * options for the various elements of SMS++ (basically, Block and Solver).
 * A template version SimpleConfiguration is immediately provided for simple
 * configurations boiling down to one single value of some type.
 *
 * \version 0.11
 *
 * \date 16 - 08 - 2019
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Configuration
 #define __Configuration
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "SMSTypedefs.h"
#include "netcdf"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Configuration_CLASSES Classes in Configuration.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS Configuration ----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class for possibly complex configurations of Block or Solver
/** The Configuration class is the basically empty top of a hierarchy of
 * objects intended to provide possibly complex configuration options for
 * the various elements of SMS++ (basically, Block and Solver). */

class Configuration
{

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
/*--------------------------------------------------------------------------*/
/*-------------- CONSTRUCTING AND DESTRUCTING Configuration ----------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructing and destructing Configuration
 *  @{ */

 Configuration( void ) { }  ///< constructor: does nothing

/*--------------------------------------------------------------------------*/

 virtual ~Configuration() { }  ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
 /// method for creating an exact clone of the current Configuration
 /** This method is supposed to do a "deep copy" of the current Configuration
  * object, which means that if the object contains pointers to other object
  * then the pointed objects will have to be copied, too, so that the cloned
  * version is completely independent from the original one. This means that
  * all pointers will have to be to clonable object, which is true by default
  * if they are pointer to (sub-)Configuration. This is basically a
  * virtualized version of the copy constructor (with explicit statement that
  * the copy is entirely independent from the original), and in fact its
  * standard implementation is
  *
  *     class MyConfiguration : Configuration {
  *      virtual MyConfiguration * clone( void ) {
  *      return( new( MyConfiguration( *this ) ) );
  *      }
  *
  *      MyConfiguration( MyConfiguration & ) { < copy constructor > }
  *      };
  *
  * There could be smart template-based ways to avoid having to do this
  * explicitly for each :Configuration, but in our case it seems that the
  * pain is higher than the gain. */

 virtual Configuration * clone( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// construct a :Configuration of given type using the Configuration factory
 /** Use the Configuration factory to construct a :Configuration object of
  * type specified by classname (a std::string with the name of the class
  * inside). Note that the method is static because the factory is static,
  * hence it is to be called as
  *
  *     Configuration *myConfiguration =
  *                             Configuration::new_Configuration( someclass );
  *
  * i.e., without any reference to any specific Configuration (and, therefore,
  * it can be used to construct the very first Configuration if needed).
  * 
  * For this to work, each :Configuration has to:
  *
  * - add the line
  *
  *     SMSpp_insert_in_factory_h;
  *
  *   to its definition (typically, in the private part in its .h file);
  *
  * - add the line
  *
  *     SMSpp_insert_in_factory_cpp_0( name_of_the_class );
  *
  *   to exactly *one* .cpp file, typically that :Configuration .cpp file. */

 static Configuration *new_Configuration( const std::string &classname )
 {
  const auto it = Configuration::f_factory().find( classname );
  if( it == Configuration::f_factory().end() )
   throw( std::invalid_argument( classname +
		   std::string( " not present in Configuration factory" ) ) );

  return( (it->second)() );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Configuration out of a netCDF file
 /** Top-level de-serialization method: takes the filename of a SMS++ netCDF
  * file, opens it in a netCDF::NcFile object, and returns the complete
  * :Configuration object whose description is *the first one* found in the
  * file. It does so by just forwarding to deserialize( netCDF::NcFile ). If
  * something goes wrong with the entire operation, nullptr is returned. See
  * deserialize( netCDF::NcFile ) for details of the SMS++ netCDF file format.
  *
  * Note that the method is static, hence it is to be called as
  *
  *       Configuration *myConfig = Configuration::deserialize( somefile );
  *
  * i.e., without any reference to any specific Configuration (and,
  * therefore, it can be used to construct the very first Configuration if
  * needed). */

 static Configuration * deserialize( const char * filename )
 {
  try {
   netCDF::NcFile f( filename , netCDF::NcFile::read );
   return( Configuration::deserialize( f ) );
   }
  catch( netCDF::exceptions::NcException & e ) {
   std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
   }
  catch( std::exception & e ) {
   std::cerr << "error " << e.what() << " in deserialize" << std::endl;
   }
  catch( ... ) {
   std::cerr << "unknown error in deserialize" << std::endl;
   }

  return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Configuration out of an open netCDF SMS++ file
 /** Second-level de-serialization method: takes an open netCDF file and the
  * index of a Configuration into the file, and returns the correspinding
  * complete :Configuration object.
  *
  * There are three types of SMS++ netCDF files, corresponding to three values
  * of the enum smspp_netCDF_file_type [see SMSTypedefs.h]. Each file, when
  * opened in a netCDF::NcFile (which is also a netCDF::NcGroup), must have an
  * int netCDF attribute "SMS++_file_type" with one of the three values of the
  * enum. The structure of the corresponding files is:
  *
  * - eProbFile: the file (which is also a group) has any number of child
  *   groups with names "Prob_0", "Prob_1", ... In turn, each child group
  *   has exactly three child groups with names "Block", "BlockConfig" and
  *   "SolverConfig", respectively. The first is intended to contain the
  *   serialization of a :Block, the second the serialization of a
  *   :BlockConfig of the same :Block, and the third the serialization of a
  *   :BlockSolverConfig of the same :Block, although any of the three can
  *   in principle be empty. If any of the child is not empty, it must
  *   necessarily contain a string attribute "type" contaiming the name() of
  *   the corresponding :Block / :Configuration class, plus od course all the
  *   information necessary to reconstruct the specific instance. Note that
  *   inner Block of the Block and inner Configuration of the Configuration
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
  * The :Configuration extracted from the file is specified by the parameter
  * idx. For obvious reasons, the base Configuration class can only handle
  * the eConfigFile case (it is not a Block, and it does not know if it is a
  * BlockConfig or a BlockSolverConfig or none of the two); therefore, the
  * :Configuration is extracted out of the netCDF::NcGroup "Config_<idx>".
  * BlockConfig and BlockSolverConfig can provide versions handling their
  * specific case.
  *
  * Note that the method is static, hence it is to be called as
  *
  *       Configuration *myConfig = Configuration::deserialize( somefile );
  *
  * i.e., without any reference to any specific Configuration (and,
  * therefore, it can be used to construct the very first Configuration if
  * needed).
  *
  * What this method does is finding the right child group, and then 
  * dispatching to new_Configuration( netCDF::NcGroup && ). */

 static Configuration * deserialize( netCDF::NcFile & f ,
				     const unsigned int idx = 0 )
 {
  try {
   auto gtype = f.getAtt( "SMS++_file_type" );
   if( gtype.isNull() )
    return( nullptr );

   int type;
   gtype.getValues( & type );

   if( type != eConfigFile )
    return( nullptr );

   auto cg = f.getGroup( "Config_" + std::to_string( idx ) );
   return( new_Configuration( cg ) );
   }
  catch( netCDF::exceptions::NcException & e ) {
   std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
   }
  catch( std::exception & e ) {
   std::cerr << "error " << e.what() << " in deserialize" << std::endl;
   }
  catch( ... ) {
   std::cerr << "unknown error in deserialize" << std::endl;
   }

  return( nullptr );
  }
 
/*--------------------------------------------------------------------------*/
 /// de-serialize a :Configuration out of netCDF::NcGroup, returns it
 /** Third-level de-serialization method: takes a netCDF::NcGroup supposedly
  * containing a Configuration, extracts the string attribute "type" out of
  * the netCDF::NcGroup, uses it in the factory to construct the "empty"
  * :Configuration [see new_Configuration( string )], and then finally
  * dispatches to deserialize( netCDF::NcGroup ), which is where the
  * :Configuration-dependent de-serialization happens. This method is
  * static (see the previous versions for comments about it) and returns a
  * pointer to the newly minted Configuration, hence it has to have a
  * different name from deserialize( netCDF::NcGroup ) (since the signature
  * is the same but for the return type). */

 static Configuration * new_Configuration( netCDF::NcGroup & group )
 {
  try {
   if( group.isNull() )
    return( nullptr );

   netCDF::NcGroupAtt gtype = group.getAtt( "type" );
   if( gtype.isNull() )
    return( nullptr );

   std::string cfgtype;
   gtype.getValues( cfgtype );
   Configuration * result = new_Configuration( cfgtype );
   result->deserialize( group );
   return( result );
   }
  catch( netCDF::exceptions::NcException & e ) {
   std::cerr << "netCDF error " << e.what() << " in deserialize" << std::endl;
   }
  catch( std::exception & e ) {
   std::cerr << "error " << e.what() << " in deserialize" << std::endl;
   }
  catch( ... ) {
   std::cerr << "unknown error in deserialize" << std::endl;
   }

  return( nullptr );
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize the current :Configuration out of netCDF::NcGroup
 /** Fourth and final level de-serialization method: takes a netCDF::NcGroup
  * supposedly containing all the information required to de-serialize the
  * Configuration must, starting with the "type" attribute that has to
  * contain the name() of the current Configuration (and exception should
  * clearly be thrown is this does not happen), and initialize the current
  * Configuration out of it.
  *
  *      THIS IS THE METHOD TO BE IMPLEMENTED BY DERIVED CLASSES
  *
  * and in fact it is virtual. The format of the information is clearly that
  * set by the serialize( netCDF::NcGroup ) method of the specific
  * :Configuration class, and exception should be thrown if anything goes
  * wrong in the process. */

 virtual void deserialize( netCDF::NcGroup & group )
 {
  #ifndef NDEBUG
   netCDF::NcGroupAtt gtype = group.getAtt( "type" );
   if( gtype.isNull() )
    throw( std::invalid_argument( "missing type attribute in netCDF group" )
	   );

   std::string cfgtype;
   gtype.getValues( cfgtype );
   if( cfgtype != name() )
    throw( std::invalid_argument( "wrong Config type in netCDF group" ) );
  #endif
  }

/**@} ----------------------------------------------------------------------*/
/*------------- Methods for reading the data of the Configuration ----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the data of the Configuration
 *  @{ */

 /// getting the classname of this Configuration
 /** Given a Configuration, this method returns a string with its class name;
  * unlike std::type_info.name(), there *are* guarantees, i.e., the name will
  * always be the same.
  *
  * The method works by dispatching the private virtual method private_name().
  * The latter is automatically implemented by the 
  * SMSpp_insert_in_factory_cpp_* macros [see SMSTypedefs.h], hence this
  * comes at no cost since these have to be called somewhere to ensure that
  * any :Configuration will be added to the factory. Actually, since
  * Configuration::private_name() is pure virtual, this ensures that it is not
  * possible to forget to call the appropriate SMSpp_insert_in_factory_cpp_*
  * for any :Configuration because otherwise it is a pure virtual class
  * (unless the programmer purposely defines private_name() without calling
  * the macro, which seems rather pointless). */

 inline const std::string & name( void ) const {
  return( private_name() );
  }

/**@} ----------------------------------------------------------------------*/
/*-------- METHODS FOR LOADING, PRINTING & SAVING THE Configuration --------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for loading, printing & saving the Configuration
 *
 * The base Configuration class provides two friend operator<<() and
 * operator>>() dispatching to protected virtual methods
 * print( std::ostream& ) and load( std::istream & ); the idea is that
 * derived classes will implement the latter two in order to provide input
 * and output on std::stream.
 *
 * The base Configuration class also defines the interface for serializing and
 * de-serializing a :Configuration onto netCDF files. This is done via the two
 * versions of serialize() taking a file name (char *) and a netCDF file.
 * The first dispatches on the second, and the latter ultimately to the
 * protected method taking a netCDF group, like their deserialize()
 * counterparts.
 *  @{ */

 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected *pure* virtual method print(). This way the
  * operator<<() is defined for each Configuration, but its behavior must be
  * customized by derived classes (since the base class has nothing to
  * print). */

 friend std::ostream& operator<< ( std::ostream &out ,
				   const Configuration &b ) {
  b.print( out );
  return( out );
  }

/*--------------------------------------------------------------------------*/
 /// friend operator>>(), dispatching to *pure* virtual protected load()
 /** Not really a method, but a friend operator>>() that just calls the
   * protected *pure* virtual method load(). This way the operator>>() is
   * defined for each Configuration, but it won't work for the case class,
   * which is abstract: it can only work for concrete derived classes which 
   * have actually implemented load() (because they have some actual data to
   * load). */
 friend std::istream& operator>> ( std::istream& in , Configuration& c ) {
  c.load( in );
  return( in );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Configuration to a netCDF file given the filename
 /** Method to serialize a Configuration to a file in netCDF-based
  * SMS++-format, given the filename and its type. See deserialize( char * )
  * for details of the different file types. Note that any existing contect
  * of the file is overwritten, and that the Configuration is saved as *the
  * first one* in the newly created file.
  *
  * The base class implementation opens the netCDF file, creates the required
  * attribute "SMS++_file_type" and assigns it the type, and dispatches to
  * the netCDF file version of the method. If anything goes wrong with any
  * step of the process, exception is thrown.  Although the method is
  * virtual, it is not expected that derived classes will have a need to
  * re-define it. */

 virtual void serialize( const char *filename , const int type = eProbFile )
  const
 {
  if( ( type != eProbFile ) && ( type != eConfigFile ) )
   throw( std::invalid_argument( "invalid SMS++ netCDF file type" ) );

  netCDF::NcFile f( filename , netCDF::NcFile::replace );

  f.putAtt( "SMS++_file_type" , netCDF::NcInt() , type );

  serialize( f , type );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Configuration to an open netCDF file
 /** Method to serialize a Configuration to an open netCDF file in
  * netCDF-based SMS++-format. The type of the file, provided as a parameter
  * (mainly to make the signature of the method not ambiguous with the
  * serialize( netCDF::NcGroup ) one), must be the same as that found in the
  * :SMS++_file_type" attribute, with the meaning set out by the enum
  * smspp_netCDF_file_type [see SMSTypedefs.h].
  *
  * The current Configuration is *appended* after any existing Configuration
  * in the file.
  *
  * The base class implementation creates the new group and dispatches to
  * serialize( netCDF::NcGroup ), which is where the :Configuration-dependent
  * serialization happens. For obvious reasons, the base Configuration class
  * can only handle the eConfigFile case (it is not a Block, and it does not
  * know if it is a BlockConfig or a BlockSolverConfig or none of the two).
  * BlockConfig and BlockSolverConfig can provide versions handling their
  * specific case. If anything goes wrong with any step of the process,
  * exception is thrown. Although the method is virtual, it is not expected
  * that derived classes will have a need to re-define it. */

 virtual void serialize( netCDF::NcFile & f , const int type ) const
 {
  if( type != eConfigFile )
   throw( std::invalid_argument( "invalid SMS++ netCDF file type" ) );

  auto cg = f.addGroup( "Config_" + std::to_string( f.getGroupCount() ) );
  serialize( cg );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Configuration to a netCDF NcGroup
 /** Method to serialize a Configuration to a netCDF NcGroup.
  *
  *      THIS IS THE METHOD TO BE IMPLEMENTED BY DERIVED CLASSES
  *
  * All the information required to de-serialize the Configuration need be
  * saved in the provided netCDF NcGroup, which is assumed to be "empty",
  * starting with the "type" attribute that has to contain the name() of the
  * Configuration. Although each :Configuration is completely free to
  * organize the netCDF NcGroup as it best sees fit, the idea is that is the
  * Configuration has any sub-Configuration these should be saved into child
  * groups of the current group.
  *
  * The method of the base class just creates and fills the "name" attribute
  * (with the right name, thanks to the name() method). */

 virtual void serialize( netCDF::NcGroup & group ) const
 {
  group.putAtt( "type" , name() );
  }

/**@} ----------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/

 typedef boost::function<Configuration*(void)> ConfigurationFactory;
 // type of the factory of Configuration

 typedef std::map<std::string,ConfigurationFactory> ConfigurationFactoryMap;
 // Type of the map between strings and the factory of Configuration

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for inserting and extracting
 *
 * The Configuration class provides two pairs of vaguely symmetric print() /
 * load() and serialize() / deserialize() methods to save information about
 * it on a std::stream / netCDF::NcGroup and retrieve it. For print() the
 * save information is *not* supposed to be enough to fully reconstruct the
 * original Configuration via load(), while this must be true for serialize().
 *   @{ */

 /// method for allowing any Configuration to print itself
 /** Method intended to provide support for Configuration to print themselves
  * out in human-readable form. The base Configuration class has preciously
  * little to print, but it still does a bit. */

 virtual void print( std::ostream &output ) const  {
  output << "Configuration [" << this << "]" << std::endl;
  }

/*--------------------------------------------------------------------------*/
 /// load the Configuration out of an istream
 /** *pure virtual* method intended to provide support for Configuration to
  * load themselves out of an istream. This is precisely what makes
  * Configuration an *abstract* base class: the actual content of the
  * Configuration depends on the specific derived class, which is why this
  * method cannot be implemented. */

 virtual void load( std::istream &input ) = 0;

/**@} ---------------------------------------------------------------------*/
/** @name Protected methods for handling static fields
 *
 * These methods allow derived classes to partake into static initialization
 * procedures performed once and for all at the start of the program. These
 * are typically related with factories.
 * @{ */

 /// method incapsulating the Configuration factory
 /** This method returns the Configuration factory, which is a static object.
  * The rationale for using a method is that this is the "Construct On First
  * Use Idiom" that solves the "static initialization order problem". */

 static ConfigurationFactoryMap & f_factory( void );

/*--------------------------------------------------------------------------*/
 /// empty placeholder for class-specific static initialization
 /** The method static_initialization() is an empty placeholder which is made
  * available to derived classes that need to perform some class-specific
  * static initialization besides these of any :Configuration class, i.e., the
  * management of the factory. This method is invoked by the
  * SMSpp_insert_in_factory_cpp_* macros [see SMSTypedefs.h] during the
  * standard initialization procedures. If a derived class needs to perform
  * any static initialization it just have to do this into its version of
  * this method; if not it just has nothing to do, as the (empty) method of
  * the base class will be called.
  *
  * This mechanism has a potential drawback in that a redefined
  * static_initialization() may be called multiple times. Assume that a
  * derived class X redefines the method to perform something, and that a
  * further class Y is derived from X that has to do nothing, and that
  * therefore will not define Y::static_initialization(): them, within the
  * SMSpp_insert_in_factory_cpp_* of Y, X::static_initialization() will be
  * called again.
  *
  * If this is undesirable, X will have to explicitly instruct derived classes
  * to redefine their (empty) static_initialization(). Alternatively,
  * X::static_initialization() may contain mechanisms to ensure that it will
  * actually do things only the very first time it is called. One standard
  * trick is to do everything within the initialisation of a static local
  * variable of X::static_initialization(): this is guaranteed by the
  * compiler to happen only once, regardless of how many times the function
  * is called. Alternatively, an explicit static boolean could be used (this
  * may just be the same as what the compiler does during the initialization
  * of static variables without telling you). */

 static void static_initialization( void ) { }

/**@} ----------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
 // Definition of Configuration::private_name() (pure virtual)

 virtual const std::string & private_name( void ) const = 0;

/*--------------------------------------------------------------------------*/

 };  // end( class( Configuration ) )

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS SimpleConfiguration ------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// template class for simple configurations (one single value)
/** The SimpleConfiguration class provides a simple implementation for a
 * Configuration object that boils down to one single value of some type.
 * The type of the value is generic, but it is somehow assumed it is a basic
 * type in that it must have a working assignment operator and << operator
 * and be automatically destructible (hence, no pointer).
 *
 * Important note: the class is template, hence infinitely many classes.
 * Each time a SimpleConfiguration<something> is used, is has to be
 * inserted in the Configuration factory; see
 *
 *      SMSpp_insert_in_factory_cpp_0_t()
 *
 * in SMSTypedefs.h. For convenience, in Configuration.cpp this is done for
 *
 *  - SimpleConfiguration< int >
 *  - SimpleConfiguration< double >
 *  - SimpleConfiguration< std::pair< int , int > >
 *  - SimpleConfiguration< std::pair< double , double > >
 *  - SimpleConfiguration< std::pair< int , double > >
 *  - SimpleConfiguration< std::pair< double , int > >
 *  - SimpleConfiguration< std::vector< int > >
 *  - SimpleConfiguration< std::vector< double > >
 *  - SimpleConfiguration< std::list< int > >
 *  - SimpleConfiguration< std::list< double > >
 *  - SimpleConfiguration< std::pair< Configuration * , Configuration * > >
 *  - SimpleConfiguration< std::vector< Configuration * > >
 *
 * but whomever is using a different SimpleConfiguration<something> for the
 * first time has the responsibility of doing it for their variant.
 *
 * Besides this, the main issue with this class are the serialize() and
 * deserialize() methods, as the netCDF C++ interface is not particularly
 * nice with templates. So, besides adding the specific template realization
 * to the factory, one also has to implement these two methods for it. Again,
 * in Configuration.cpp this is done for all the previous cases. */

template<class SimpleConfiguration_value_type>
class SimpleConfiguration : public Configuration
{

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// void constructor (the value is not initialized)
 SimpleConfiguration( void ) : Configuration() { }

 /// constructor taking the value (&) as input
 explicit SimpleConfiguration( const SimpleConfiguration_value_type & initval )
  : Configuration() { f_value = initval; }

 /// move constructor taking the value (&&) as input
 explicit SimpleConfiguration( SimpleConfiguration_value_type && initval )
  : Configuration() { f_value = initval; }

 /// constructor taking the value as input
 explicit SimpleConfiguration( const SimpleConfiguration_value_type && initval ) :
  Configuration() { f_value = initval; }

 /// copy constructor: does what it says on the tin
 SimpleConfiguration( const SimpleConfiguration & old ) : Configuration() {
  f_value = old.f_value;
  }
  
 virtual void deserialize( netCDF::NcGroup & group ) override;

 virtual ~SimpleConfiguration() { }  ///< destructor: does nothing

 /// clone method
 virtual SimpleConfiguration * clone( void ) const override
 {
  return( new SimpleConfiguration( *this ) );
  }

/*--------------------------------------------------------------------------*/

 virtual void serialize( netCDF::NcGroup & group ) const override;

/*---------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 SimpleConfiguration_value_type f_value;  // the value
  
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// printing out the value of this SimpleConfiguration

 void print( std::ostream &output ) const override { output << f_value; }

/*--------------------------------------------------------------------------*/
 /// load this SimpleConfiguration out of an istream
 /** Load this SimpleConfiguration out of an istream.
  * The format of the istream can only be rather simple:
  * - skip any whitespace
  * - skip any comment lines (starting with '#')
  * - load an object of type SimpleConfiguration_value_type
  */

 void load( std::istream &input ) override
 {
  input >> eatcomments >> f_value;
  }

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 /* manual expansion of SMSpp_insert_in_factory_h to avoid including the
  * whole of SMSTypedefs.h. */

 static class _init {
 public:
  _init( void );
  } _initializer;

 const std::string & private_name( void ) const override;

 static const std::string & _private_name( void );

/*--------------------------------------------------------------------------*/

 };  // end( class( SimpleConfiguration ) )

/** @} end( group( Configuration_CLASSES ) ) */ 
/*--------------------------------------------------------------------------*/
/*-------------------- Configuration-RELATED TYPES -------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Configuration_TYPES Configuration-related types.
 *  @{ */

 typedef Configuration *p_Conf;
 ///< a pointer to Configuration

 typedef std::vector<p_Conf> Vec_p_Conf;
 ///< a vector of pointer to Configuration

/** @} end( group( Configuration_TYPES ) ) */
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif /* Configuration.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Configuration.h -------------------------*/
/*--------------------------------------------------------------------------*/
