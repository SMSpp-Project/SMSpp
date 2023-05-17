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
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
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

 Configuration() = default;  ///< constructor: does nothing

/*--------------------------------------------------------------------------*/

 virtual ~Configuration() = default;  ///< destructor: does nothing

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
  *       return( new( MyConfiguration( *this ) ) );
  *      }
  *
  *      MyConfiguration( MyConfiguration & ) { < copy constructor > }
  *     };
  *
  * There could be smart template-based ways to avoid having to do this
  * explicitly for each :Configuration, but in our case it seems that the
  * pain is higher than the gain. */

 [[nodiscard]] virtual Configuration * clone( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// construct a :Configuration of given type using the Configuration factory
 /** Use the Configuration factory to construct a :Configuration object of
  * type specified by classname (a std::string with the name of the class
  * inside). Note that the method is static because the factory is static,
  * hence it is to be called as
  *
  *     Configuration * myConfiguration =
  *                            Configuration::new_Configuration( some_class );
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
  *   to exactly *one* .cpp file, typically that :Configuration .cpp file. If
  *   the name of the class contains any parentheses, then one must enclose
  *   the name of the class in parentheses and instead add the line
  *
  *     SMSpp_insert_in_factory_cpp_0( ( name_of_the_class ) );
  *
  * Any whitespaces that the given \p classname may contain is ignored. So,
  * for example, to create an instance of the class MyConfiguration< int > one
  * could pass "MyConfiguration< int >" or "MyConfiguration< int >" (even " M y
  * C o n f i g u r a t i o n < int > " would work).
  *
  * @param classname The name of the :Configuration class that must be
  *        constructed. */

 static Configuration * new_Configuration( const std::string & classname ) {
  const std::string classname_( SMSpp_classname_normalise(
						std::string( classname ) ) );
  const auto it = Configuration::f_factory().find( classname_ );
  if( it == Configuration::f_factory().end() )
   throw( std::invalid_argument( classname +
				 " not present in Configuration factory" ) );
  return( ( it->second )() );
  }

/*--------------------------------------------------------------------------*/
 /// set the executable-wide prefix for all Configuration filenames
 /** Loading a Configuration from file (either text or netCDF) is likely to
  * be one of the most common way to create one, and the static method
  * deserialize( std::string ) is provided for this purpose. That method is
  * in turn used for "file redirection"; a Configuration file (either text or
  * netCDF) can contain one (or many) filenames in which a parts of the
  * description of that Configuration (typically a Configuration inside the
  * Configuration) can be found, see new_Configuration( netCDF::NcGroup ) and
  * deserialize( std::istream ). It could arguably be convenient to be able
  * to specify filenames relative to some given prefix, so as to be able to
  * freely move the description of a Configuration (which can be a rather
  * large object, and therefore require many files) across the filesystem.
  * Configuration provides a *static* member for this purpose, that can be
  * set with this (static) method. Note that
  *
  *     BEING THE MEMBER STATIC, THE PREFIX IS APPLIED TO ALL LOADING 
  *     OPERATIONS OF ANY Configuration IN THE EXECUTABLE
  *
  * Use of this feature therefore requires care. */

 static void set_filename_prefix( std::string && prefix ) {
  f_prefix = prefix;
  }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Configuration out of a file
 /** Top-level de-serialization method: takes the \p filename of a file
  * (possibly also encoding a position into it), and returns the complete
  * :Configuration object whose description is the one found (at the
  * specified position) in the file.
  *
  * The method supports two different kind of files:
  *
  * - text files,
  *
  * - SMS++ netCDF files.
  *
  * It distinguishes between the two by the suffix. In particular, the format
  * of \p filename can be:
  *
  * - either \p filename terminates by ".txt" (case sensitive) then a
  *   std::fstream is opened and deserialize( istream ) is called, with
  *   the Configuration being extracted is the first one found in it;
  *
  * - otherwise a netCDF::NcFile is opened and deserialize( netCDF::NcFile )
  *   is called; since netCDF::NcFile support the notion of having
  *   multiple Configuration inside, \p filename can be used to encode the
  *   position (Configuration) in the file:
  *
  *     * if the \p filename ends with ']', then is supposed to have the
  *       form "real filename[idx]": the "[idx] part is excised and used to
  *       compute the int parameter of deserialize() (the position), with the
  *       remaining part being used for the string parameter (the filename);
  *
  *     * otherwise, the whole string is used as the string parameter (the
  *       filename).
  *
  * If anything goes wrong with the entire operation, nullptr is returned.
  *
  * Note that if a filename prefix has been defined (for all Configuration)
  * by means of set_filename_prefix(), then \p filename has to be intended
  * as relative to that prefix (in the sense that the prefix is prefix to
  * \p filename).
  *
  * Note that the method is static, hence it is to be called as
  *
  *     auto myConfig = Configuration::deserialize( somefile );
  *
  * i.e., without any reference to any specific Configuration (and, therefore,
  * it can be used to construct the very first Configuration if needed). */

 static Configuration * deserialize( const std::string & filename );

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Configuration out of an open netCDF SMS++ file
 /** Second-level de-serialization method: takes an open netCDF file and the
  * index of a Configuration into the file, and returns the correspinding
  * complete :Configuration object.
  *
  * There are three types of SMS++ netCDF files, corresponding to three values
  * of the enum smspp_netCDF_file_type; see SMSTypedefs.h for details, the
  * two ones relevant here being
  *
  * - eProbFile: the file (which is also a group) has any number of child
  *   groups with names "Prob_0", "Prob_1", ... In turn, each child group
  *   has exactly three child groups with names "Block", "BlockConfig" and
  *   "BlockSolver", respectively.
  *
  * - eConfigFile: the file (which is also a group) has any number of child
  *   groups with names "Config_0", "Config_1", ...
  *
  * The :Configuration extracted from the file is specified by the parameter
  * idx. In the case of an eConfigFile, \p idx is just the index (0, 1, ...)
  * of the "Config_*" group. For the case of eProbFile, instead, one has to
  * distinguish between "BlockConfig" and "BlockSolver" groups, which is done
  * in the following way:
  *
  * - if \p idx >= 0, then the BlockConfig in the "Prob_<idx>" group is
  *   returned;
  *
  * - if \p idx z 0, then the BlockSolver in the "Prob_(- <idx> + 1)" group
  *   is returned.
  *
  * Once the appropriate group is selected, the :Configuration is loaded from
  * it with a call to new_Configuration( netCDF::NcGroup & ); see the
  * corresponding comments for the format options. Anything going wrong with
  * the entire operation (the file is not there, the "SMS++_file_type"
  * attribute is not there, there is no required "Config_<idx>" child group,
  * there is any fatal error during the process, ...) results in nullptr being
  * returned.
  *
  * Note that the method is static, hence it is to be called as
  *
  *     Configuration *myConfig = Configuration::deserialize( somefile );
  *
  * i.e., without any reference to any specific Configuration (and,
  * therefore, it can be used to construct the very first Configuration if
  * needed). */

 static Configuration * deserialize( const netCDF::NcFile & f , int idx = 0 );

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Configuration out of netCDF::NcGroup, returns it
 /** Third-level de-serialization method: takes a netCDF::NcGroup supposedly
  * containing (all the information describing) a :Configuration (either
  * "directly" or "indirectly") and returns a pointer to a newly minted
  * Configuration object corresponding to what is found in the file.
  *
  * The method works with two different kinds of netCDF::NcGroup:
  *
  * - A "direct" group that contains at least the string attribute "type";
  *   this is used it in the factory to construct an "empty" :Configuration
  *   of that type [see new_Configuration( string )], and then the method
  *   deserialize( netCDF::NcGroup ) of the newly minted :Configuration is
  *   invoked (with argument \p group) to finish the work.
  *
  * - An "indirect" group that just need to contain the single string
  *   attribute "filename"; in this case, the attribute is used as argument
  *   for a call to deserialize( const std::string & ) that will extract the
  *   :Configuration by the corresponding file (be it a text or netCDF one).
  *   Note that for netCDF files the filename string can also be used to
  *   encode the position in the file, see the comments in the method for
  *   details.
  *
  * In case \p group contains both "type" and "filename", the first takes the
  * precedence (direct groups have precedence over indirect ones).
  *
  * Note that this method is static (see the previous versions for comments
  * about it) and returns a pointer to Configuration, hence it has to have a
  * different name from deserialize( netCDF::NcGroup ) (since the signature
  * is the same but for the return type).
  *
  * If anything goes wrong with the process, nullptr is returned. */

 static Configuration * new_Configuration( const netCDF::NcGroup & group );

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

 virtual void deserialize( const netCDF::NcGroup & group )
 {
  #ifndef NDEBUG
   netCDF::NcGroupAtt gtype = group.getAtt( "type" );
   if( gtype.isNull() )
    throw( std::invalid_argument( "missing type attribute in netCDF group" )
	   );

   std::string cfgtype;
   gtype.getValues( cfgtype );
   if( cfgtype != classname() )
    throw( std::invalid_argument( "wrong Config type in netCDF group" ) );
 #endif
 }

/*--------------------------------------------------------------------------*/
 /// de-serialize a :Configuration out of std::istream, returns it
 /** Convenience static method that creates a :Configuration reading all its
  * data from an open std::istream. The format of the istream (from the
  * point the pointer is onwards) is assumed to be:
  *
  * - Either the character '*' is the first one that is found after any
  *   whitespace and comment, after which two cases arise:
  *
  *   = The characters immediately following '*' a nonempty string, which
  *     means that '*' is not immediately followed by a whitespace (note that
  *     comments are *not* skipped here): then, the string is used as the
  *     filenam of deserialize( string ), which opens it and reads the
  *     :Configuration from there without advancing the pointer in \p input
  *     (save for discarding '*' and the string). Check the comments of
  *     deserialize( string ) for the details of the possible formats of the
  *     string.
  *
  *   = The characters immediately following '*' form an empty string (which
  *     means that '*' is immediately followed by whitespaces or comments):
  *     then, nullptr is returned;
  *
  * - Or the first character that is found after any whitespace and comment
  *   is not '*', in which case it has to be the first character of a
  *   nonempty string that specifies the classname of the :Configuration, as
  *   required by the Configuration factory; this has to be followed by all
  *   the rest of the information describing the :Configuration, which of
  *   course depends on its type (and should be described in the comments
  *   to the load() method of that :Configuration, which is used by the
  *   >> operator to perform the task). */

 static Configuration * deserialize( std::istream & input );

/** @} ---------------------------------------------------------------------*/
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

 [[nodiscard]] const std::string & classname( void ) const {
  return( private_name() );
  }

/** @} ---------------------------------------------------------------------*/
/*----------- METHODS DESCRIBING THE BEHAVIOR OF A Configuration -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods describing the behavior of a Configuration
 *  @{ */

 /// clear this Configuration
 /** A Configuration is basically used to configure Block and its Solver. As a
  * Block has a complex tree structure, it is regularly the case in which a
  * Configuration also presents a similar tree structure, as it may commonly
  * be used to configure not only the Block (or its Solver) but all of its
  * sub-Block as well, recursively. If the structure of a Configuration object
  * for a Block is unknown, its construction can therefore be a
  * computationally expensive task, often requiring the inspection of the
  * whole Block (its Constraint, Objective, and sub-Block, recursively). It
  * turns out that it can be useful in some situations to have at hand a
  * "cleared" Configuration object: one that contains its complete structure
  * but without any other data (e.g., parameters upon which a Block or a
  * Solver may depend). This method must "clear" this Configuration,
  * preserving its complex structure and thus erasing any data that does not
  * affect its structure.
  *
  * This method has a default empty implementation as some Configuration may
  * not have any data to be "cleared". */

 virtual void clear( void ) {}

/** @} ---------------------------------------------------------------------*/
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

 friend std::ostream & operator<<( std::ostream & out ,
				   const Configuration & b ) {
  b.print( out );
  return( out );
  }

/*--------------------------------------------------------------------------*/
 /// friend operator>>(), dispatching to *pure* virtual protected load()
 /** Not really a method, but a friend operator>>() that just calls the
   * protected *pure* virtual method load(). This way the operator>>() is
   * defined for each Configuration, but it won't work for the base class,
   * which is abstract: it can only work for concrete derived classes which 
   * have actually implemented load() (because they have some actual data to
   * load). */

 friend std::istream & operator>>( std::istream & in , Configuration & c ) {
  c.load( in );
  return( in );
  }

/*--------------------------------------------------------------------------*/
 /// friend operator>>() for pointers
 /** Not really a method, but a friend operator>>() that loads a new
  * :Configuration and stores its pointer; this is basically calling
  * Configuration::deserialize( stream ) to load the string classname, use
  * the factory to build the object, and then use the standard operator>>()
  * to finish loading. It would not even really need to be a friend. */

 friend std::istream & operator>>( std::istream & in , Configuration * &c ) {
  c = Configuration::deserialize( in );
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

 virtual void serialize( const std::string & filename ,
			 int type = eProbFile ) const
 {
  if( ( type != eProbFile ) && ( type != eConfigFile ) )
   throw( std::invalid_argument( "invalid SMS++ netCDF file type" ) );

  netCDF::NcFile f( filename, netCDF::NcFile::replace );

  f.putAtt( "SMS++_file_type", netCDF::NcInt(), type );

  serialize( f , type );
  }

/*--------------------------------------------------------------------------*/
 /// serialize a Configuration to an open netCDF file
 /** Method to serialize a Configuration to an open netCDF file in
  * netCDF-based SMS++-format. The type of the file, provided as a parameter
  * (mainly to make the signature of the method not ambiguous with the
  * serialize( netCDF::NcGroup ) one), must be the same as that found in the
  * :SMS++_file_type" attribute, with the meaning set out by the enum
  * smspp_netCDF_file_type. See SMSTypedefs.h for details of the type
  * formats; in this setting, the only applicable type is
  *
  * - eConfigFile: the file (which is also a group) has any number of child
  *   groups with names "Config_0", "Config_1", ... Each child group contains
  *   the serialization of a :Configuration (the string attribute "type" and
  *   all the rest).
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

 virtual void serialize( netCDF::NcFile & f, int type ) const
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
  * starting with the "type" attribute that has to contain the classname() of
  * the Configuration. Although each :Configuration is completely free to
  * organize the netCDF NcGroup as it best sees fit, the idea is that is the
  * Configuration has any sub-Configuration these should be saved into child
  * groups of the current group.
  *
  * The method of the base class just creates and fills the "type" attribute
  * (with the right name, thanks to the classname() method). */

 virtual void serialize( netCDF::NcGroup & group ) const {
  group.putAtt( "type" , classname() );
  }

/** @} ---------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 protected:

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED TYPES ------------------------------*/
/*--------------------------------------------------------------------------*/

 typedef boost::function< Configuration *( void ) > ConfigurationFactory;
 // type of the factory of Configuration

 typedef std::map< std::string , ConfigurationFactory >
                                                 ConfigurationFactoryMap;
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

 virtual void print( std::ostream & output ) const {
  output << "Configuration [" << this << "]" << std::endl;
  }

/*--------------------------------------------------------------------------*/
 /// load the Configuration out of an istream
 /** *pure virtual* method intended to provide support for Configuration to
  * load themselves out of an istream. This is precisely what makes
  * Configuration an *abstract* base class: the actual content of the
  * Configuration depends on the specific derived class, which is why this
  * method cannot be implemented. */

 virtual void load( std::istream & input ) = 0;

/** @} --------------------------------------------------------------------*/
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

 static void static_initialization( void ) {}

/** @} ---------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

 static std::string f_prefix;  ///< the executable-wide filename prefix

/** @} ---------------------------------------------------------------------*/
/*--------------------- PRIVATE PART OF THE CLASS --------------------------*/
/*--------------------------------------------------------------------------*/

 private:

/*--------------------------------------------------------------------------*/
/*-------------------------- PRIVATE METHODS -------------------------------*/
/*--------------------------------------------------------------------------*/
 // Definition of Configuration::private_name() (pure virtual)

 [[nodiscard]] virtual const std::string & private_name( void ) const = 0;

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
 * Each time a SimpleConfiguration< something > is used, is has to be
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
 *  - SimpleConfiguration< std::pair< Configuration * , Configuration * > >
 *  - SimpleConfiguration< std::vector< Configuration * > >
 *  - SimpleConfiguration< std::vector< std::pair< int , Configuration * > > >
 *  - SimpleConfiguration< std::map< std::string , Configuration * > >
 *
 * but whomever is using a different SimpleConfiguration< something > for the
 * first time has the responsibility of doing it for their variant.
 *
 * Besides this, the main issue with this class are the serialize(), load()
 * and deserialize() methods, as the netCDF C++ interface is not particularly
 * nice with templates. Provided that the template type has working
 * serialize( type ), operator>>( type ) and deserialize( type ) functions
 * (some of which are provided in SMSTypedefs.h for double, int and some STL
 * containers thereof), the generic versions of SimpleConfiguration<> may work
 * out of the bat. However, this is not always true, which means that, besides
 * adding the specific template realization to the factory, one may also have
 * to implement these three methods for it. This is in particular needed for
 * SimpleConfiguration containing pointers (in this file this is always
 * Configuration *), mostly because template arguments deduction removes
 * references, thereby making it hard to work with pointer types in the exact
 * same ways in which you work with non-pointer types. Again, in
 * Configuration.cpp this is done for all the previous cases.
 *
 * IMPORTANT NOTE: adding a specific SimpleConfiguration< something > to the
 *                 factory requires a call to
 *
 *     SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration< something > );
 *
 * This is actually a macro, and therefore it has a problem if "something"
 * contains commas (",") as they are considered as separate macro arguments.
 * Hence, while
 *
 *     SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration<
 *                                                     std::vector< int > > );
 *
 * is legal,
 *
 *     SMSpp_insert_in_factory_cpp_0_t( SimpleConfiguration<
 *                                                 std::pair< int , int > > );
 *
 * is not because of the comma. This can be swiftly solved by just adding
 * parentheses around the type name, as in
 *
 *     SMSpp_insert_in_factory_cpp_0_t( ( SimpleConfiguration<
 *                                              std::pair< int , int > > ) );
 *
 * thanks to a specific feature of SMSpp_insert_in_factory_cpp. */

template< class SimpleConfiguration_value_type >
class SimpleConfiguration : public Configuration
{
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// void constructor (the value is not initialized)

 SimpleConfiguration( void ) : Configuration() {}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// constructor taking the value (&) as input

 explicit SimpleConfiguration( const SimpleConfiguration_value_type &
			       initval )
  : Configuration() { f_value = initval; }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// move constructor taking the value (&&) as input

 explicit SimpleConfiguration( SimpleConfiguration_value_type && initval )
  : Configuration() { f_value = std::move( initval ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// copy constructor: does what it says on the tin

 SimpleConfiguration( const SimpleConfiguration & old ) : Configuration() {
  f_value = old.f_value;
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

 void deserialize( const netCDF::NcGroup & group ) override {
  Configuration::deserialize( group );
  SMSpp_di_unipi_it::deserialize( group , f_value );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// default destructor, apparently does nothing
/** The destructor must not be defined "default" since it is redefined by the
 * classes handling Configuration * as they muat delete them. However it must
 * be explicitly defined so that template specializations for those classes
 * are allowed. The default implementation does nothing, which means it
 * deletes f_value, whatever that is. */

 ~SimpleConfiguration() override {};

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// clone method

 [[nodiscard]] SimpleConfiguration * clone( void ) const override {
  return( new SimpleConfiguration( *this ) );
  }

/*--------------------------------------------------------------------------*/

 void serialize( netCDF::NcGroup & group ) const override {
  Configuration::serialize( group );
  SMSpp_di_unipi_it::serialize( group , f_value );
  }

/*--------------------------------------------------------------------------*/

 void clear( void ) override {}

/*--------------------------------------------------------------------------*/
 /// accessor to the "simple" value contained in the SimpleConfiguration

 SimpleConfiguration_value_type & value( void ) { return( f_value ); }

/*---------------------- PUBLIC FIELDS OF THE CLASS ------------------------*/

 SimpleConfiguration_value_type f_value;  // the value

/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// printing out the value of this SimpleConfiguration

 void print( std::ostream & output ) const override { output << f_value; }

/*--------------------------------------------------------------------------*/
 /// load this SimpleConfiguration out of an istream
 /** Load this SimpleConfiguration out of an istream. The format of the
  * istream can only be rather simple: it "just" has to contain an object of
  * type SimpleConfiguration_value_type.
  *
  * For SimpleConfiguration_value_type any type that has a working operator>>,
  * this is done by the default method (skipping any whitespace and comments).
  * In particular, some of these are defined in SMSTypedefs.h for:
  *
  * - std::pair< T1 , T2 > that just read .first first and .second second
  *   using T1::operator>> and T2::operator>
  *
  * - std::vector< T > and std::list< T > that first read the number of
  *   elements (using [unsigned int]::operator>>) and then read the elements
  *   one by one (using T::operator>>) 
  *
  * However, for SimpleConfiguration_value_type being anything that
  * contains other Configuration [*], specialised versions are implemented
  * that use Configuration::deserialize( std::istream ) to load the
  * :Configuration object; this means that all the corresponding input
  * options, like '*' for  nullptr and "*<filename>" for loading it out of a
  * different file, can be used. */

 void load( std::istream & input ) override {
  input >> eatcomments >> f_value;
  if( input.fail() )
   throw( std::invalid_argument(
			  "SimpleConfiguration::load: stream read error" ) );
  }

/*---------------------- PRIVATE PART OF THE CLASS -------------------------*/

 private:

/*---------------------------- PRIVATE FIELDS ------------------------------*/

 /* manual expansion of SMSpp_insert_in_factory_h to avoid including the
  * whole of SMSTypedefs.h. */

 static class _init {
  public:
  _init();
  } _initializer;

 [[nodiscard]] const std::string & private_name( void ) const override;

 static const std::string & _private_name();

/*--------------------------------------------------------------------------*/

 };  // end( class( SimpleConfiguration ) )

/** @} end( group( Configuration_CLASSES ) ) */
/*--------------------------------------------------------------------------*/
/*-------------------- Configuration-RELATED TYPES -------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Configuration_TYPES Configuration-related types.
 *  @{ */

typedef Configuration * p_Conf;   ///< a pointer to Configuration

typedef std::vector< p_Conf > Vec_p_Conf;
///< a vector of pointer to Configuration

/** @} end( group( Configuration_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------ Configuration-RELATED FUNCTIONS -----------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Configuration_FUNCTIONS Configuration-related functions.
 *  @{ */

/// deserialize a Configuration (*) out of a given group
/** Deserialize a Configuration (*) , out of the given \p group and into
 * \p data. This is done by calling Configuration::new_Configuration() on
 * the given \p group if \p name is empty, and otherwise on the sub.group of
 * \p group with the given \p name. */

inline void deserialize( const netCDF::NcGroup & group ,
			 const std::string & name ,
			 Configuration * & data )
{
 if( name.empty() )
  data = Configuration::new_Configuration( group );
 else
  data = Configuration::new_Configuration( group.getGroup( name ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize a Configuration (*) into a given group
/** Serialize a Configuration (*) out of \p data. This is done by serializing
 * the Configuration in \p group if \p name is empty, and otherwise by
 * creating the sub-group of \p group with the given \p name and serializing
 * the Configuration there. */

inline void serialize( netCDF::NcGroup & group , const std::string & name ,
		       const Configuration * data )
{
 if( name.empty() )
  data->serialize( group );
 else {
  auto gr = group.addGroup( name );
  data->serialize( gr );
  }
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// deserialize basically any STL container of Configuration (*)
/** Deserialize basically any STL container of Configuration (*) out of the
 * given \p group and into \p data. This is supposed to be represented by
 * the dimension with name \p size giving the size of the container, plus
 * by as many sub-groups of \p group with name <name>0, <name>1, ..., each
 * one containing one of the Configuration. */

template< template< class ... > class C >
void deserialize( const netCDF::NcGroup & group ,
		  C< Configuration * > & data ,
		  const std::string & size = "size" ,
		  const std::string & name = "Config_" )
{
 for( auto el : data )
  delete el;
 auto dim = group.getDim( size );
 if( dim.isNull() ) {
  data.clear();
  return;
  }
 data.resize( dim.getSize() );
 size_t i = 0;
 for( auto & el : data )
  el = Configuration::new_Configuration(
			   group.getGroup( name + std::to_string( i++ ) ) );
 }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/// serialize basically any STL container of Configuration (*)
/** Serialize basically any STL container of Configuration (*) into the
 * given \p group and into \p data. This is supposed to be represented by
 * the dimension with name \p size giving the size of the container, plus
 * by as many sub-groups of \p group with name <name>0, <name>1, ..., each
 * one containing one of the Configuration. */

template< template< class ... > class C >
void serialize( netCDF::NcGroup & group , const C< Configuration * > & data ,
		const std::string & size = "size" ,
		const std::string & name = "Config_" )
{
 group.addDim( size , data.size() );
 size_t i = 0;
 for( auto el : data ) {
  auto gr = group.addGroup( name + std::to_string( i++ ) );
  el->serialize( gr );
  }
 }

/** @} end( group( Configuration_FUNCTIONS ) ) */
/*--------------------------------------------------------------------------*/
/*---------------------- TEMPLATE SPECIALIZATIONS --------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup SimpleConfiguration_SPECIALIZATIONS SimpleConfiguration
 *  template specializations.
 *
 *  Template specializations are mostly needed for SimpleConfiguration
 *  containing Configuration *, mostly because template arguments deduction
 *  removes references, and making it hard to work with pointer types in the
 *  exact same ways in which you work with non-pointer types.
 *  @{ */

// std::pair< Configuration * , Configuration * >

template<>
SimpleConfiguration< std::pair< Configuration * , Configuration * > > *
 SimpleConfiguration< std::pair< Configuration * , Configuration * >
 >::clone( void ) const;

template<>
inline SimpleConfiguration< std::pair< Configuration * , Configuration * >
 >::~SimpleConfiguration< std::pair< Configuration * , Configuration * > >() {
 delete f_value.second;
 delete f_value.first;
 }

template<>
void SimpleConfiguration< std::pair< Configuration * , Configuration * >
                                     >::serialize( netCDF::NcGroup & group )
 const;

template<>
void SimpleConfiguration< std::pair< Configuration * , Configuration * >
                          >::deserialize( const netCDF::NcGroup & group );

template<>
void SimpleConfiguration< std::pair< Configuration * , Configuration * >
                          >::clear( void );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// std::vector< Configuration * >

template<>
SimpleConfiguration< std::vector< Configuration * > > *
SimpleConfiguration< std::vector< Configuration * > >::clone( void ) const;

template<>
inline SimpleConfiguration< std::vector< Configuration * >
 >::~SimpleConfiguration< std::vector< Configuration * > >() {
 for( auto rit = f_value.rbegin() ; rit != f_value.rend() ; ++rit )
  delete *rit;
 }

template<>
inline void SimpleConfiguration< std::vector< Configuration * >
 >::serialize( netCDF::NcGroup & group ) const {
 SMSpp_di_unipi_it::serialize< std::vector >( group , f_value );
 }

template<>
inline void SimpleConfiguration< std::vector< Configuration * >
 >::deserialize( const netCDF::NcGroup & group ) {
 SMSpp_di_unipi_it::deserialize< std::vector >( group , f_value );
 }

template<>
void SimpleConfiguration< std::vector< Configuration * > >::clear( void );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// std::vector< std::pair< int , Configuration * > >

template<>
SimpleConfiguration< std::vector< std::pair< int , Configuration * > > > *
 SimpleConfiguration< std::vector< std::pair< int , Configuration * > >
 >::clone( void ) const;

template<>
inline SimpleConfiguration< std::vector< std::pair< int , Configuration * > >
 >::~SimpleConfiguration< std::vector< std::pair< int , Configuration * > >
 >() {
 for( auto rit = f_value.rbegin() ; rit != f_value.rend() ; ++rit )
  delete rit->second;
 }

template<>
void SimpleConfiguration< std::vector< std::pair< int , Configuration * > >
                          >::serialize( netCDF::NcGroup & group )
 const;

template<>
void SimpleConfiguration< std::vector< std::pair< int , Configuration * > >
                          >::deserialize( const netCDF::NcGroup & group );

template<>
void SimpleConfiguration< std::vector< std::pair< int , Configuration * > >
                          >::clear( void );

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
// std::map< std::string , Configuration * >

template<>
SimpleConfiguration< std::map< std::string , Configuration * > > *
 SimpleConfiguration< std::map< std::string , Configuration * >
                      >::clone( void ) const;

template<>
inline SimpleConfiguration< std::map< std::string , Configuration * >
 >::~SimpleConfiguration< std::map< std::string , Configuration * > >() {
 for( auto & [ key , val ] : f_value )
  delete val;
 }

template<>
void SimpleConfiguration< std::map< std::string , Configuration * >
                          >::serialize( netCDF::NcGroup & group )
 const;

template<>
void SimpleConfiguration< std::map< std::string , Configuration * >
                          >::deserialize( const netCDF::NcGroup & group );

template<>
void SimpleConfiguration< std::map< std::string , Configuration * >
                          >::clear( void );

template<>
void SimpleConfiguration< std::map< std::string , Configuration * >
                          >::load( std::istream & input );

template<>
void SimpleConfiguration< std::map< std::string , Configuration * >
                          >::print( std::ostream & output ) const;

/** @} end( group( SimpleConfiguration_SPECIALIZATIONS ) ) */
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif /* Configuration.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File Configuration.h -------------------------*/
/*--------------------------------------------------------------------------*/
