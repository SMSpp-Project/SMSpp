/*--------------------------------------------------------------------------*/
/*----------------------- File ThinVarDepInterface.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* ThinVarDepInterface class, a very thin
 * base class for all objects in SMS++ (Constraint, Objective, Function, ...)
 * that depend on a set of "active" Variable.
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

#ifndef __ThinVarDepInterface
 #define __ThinVarDepInterface
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <iterator>
#include "Modification.h"

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class Variable;  // forward definition of Variable

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ThinVarDepInterface -------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class of all objects depending on a set of "active" Variable
/** Several objects in SMS++ explicitly depend on a set of "active" Variable;
 * these are Constraint, Objective and Function (Solver may "indirectly"
 * depend on a set of Variable, but these are not defined through this
 * interface).
 *
 * The *abstract* ThinVarDepInterface class is meant to factor out many of
 * the methods required to deal with this aspect. Factoring them is primarily
 * meant to avoid un-necessary code duplication and to ensure consistency
 * between similar parts of the interface of different objects; in particular,
 * all :ThinVarDepInterface have to behave as standard STL containers (with
 * underlying type Variable), with iterator and const_iterator defined to
 * allow sifting through the  set of "active" Variable. This is done by
 * defining "virtual" iterators that can be specialized by derived classes
 * (using inheritance, not templates) and a iterators redirecting from those.
 * Furthermore, this allows Variable to deal with all these objects in an
 * unified way.
 *
 * Another reason for introducing the class at this point of the design cycle
 * is that SMS++ may evolve by allowing "more structured" access to the set
 * of "active" Variable; say, explicitly distinguishing between static and
 * dynamic ones and/or allowing to partition them in groups, like having as
 * input multiple whole vector/list/multi-arrays of Variable. However, how
 * this is to be accomplished (if ever) is not decided yet. By factoring all
 * aspects of the "active" Variable interface into a single class, the effort
 * to later refactor the interfaces to allow for this might be decreased. */

class ThinVarDepInterface {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Public Types
    @{ */

 /// type for the indices of "active" Variable
 /** Type for the indices of "active" Variable. This is supposed to be
  * identical to the same-name type defined in Block; it is not "imported
  * from Block" because ThinVarDepInterface.h does not include Block.h. */
 using Index = unsigned int;

 using c_Index = const Index;    ///< a const Index

 /// define the Range type, i.e., a pair of indices
 /** Note: the Range type is supposed to be identical to the same-name type
  * defined in Block; it is not "imported from Block" because
  * ThinVarDepInterface.h does not include Block.h. */
 using Range = std::pair< Index , Index >;

 using c_Range = const Range;    ///< a const Range

 /// an "infinite Range", i.e., [ 0 , INF ), i.e., "everything"
 /** Note: and identical constexpr with the same-name is defined in Block;
  * it is not "imported from Block" because ThinVarDepInterface.h does not
  * include Block.h. */
 static constexpr auto INFRange = Range( 0 , Inf< Index >() );

 /// a std::vector< Index >
 /** Type a "generic set if Indices", i.e., a std::vector< Index >. This is
  * supposed to be identical to Block::Subset, but it is not "imported from 
  * Block" because ThinVarDepInterface.h does not include Block.h. */
 using Subset = std::vector< Index >;

 using c_Subset = const Subset;  ///< a const Subset

/*--------------------------------------------------------------------------*/
 /// virtualized standard iterator
 /** ThinVarDepInterface::v_iterator, the *definition* of an iterator with 
  * (almost) the right traits to allow sifting through the "active" Variable.
  *
  * Unless ordinary iterators, the class is abstract, which means that
  * classes derived from ThinVarDepInterface will have to actually implement
  * this for their own version of the set of "active" Variable. This is why,
  * rather than the two standard versions of operator++ (prefix and postfix)
  * returning an iterator (which cannot be done, since the class is abstract)
  * there is only one version doing the increment. This single version is
  * then transformed into the ordinary two by the "normal" iterator.
  *
  * Also, because the virtual iterator is accessed via pointers, its copy
  * cannot be done via standard copy constructors/assignments, and a clone()
  * method is required. */

 class v_iterator {
  public:

  typedef Variable value_type;
  typedef Variable & reference;
  typedef Variable * pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  v_iterator() = default;                                      ///< constructor
  virtual ~v_iterator() = default;                             ///< destructor
  virtual v_iterator * clone() = 0;                            ///< cloner
  virtual void operator++() = 0;                               ///< increment
  virtual reference operator*() const = 0;                     ///< operator*
  virtual pointer operator->() const = 0;                      ///< operator->
  virtual bool operator==( const v_iterator & rhs ) const = 0; ///< operator==
  virtual bool operator!=( const v_iterator & rhs ) const = 0; ///< operator!=
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// virtualized standard const iterator
 /** ThinVarDepInterface::v_const_iterator, the *definition* of a const
  * iterator with (almost) the right traits to allow sifting through the
  * "active" Variable.
  *
  * Unless ordinary iterators, the class is abstract, which means that
  * classes derived from ThinVarDepInterface will have to actually implement
  * this for their own version of the set of "active" Variable. This is why,
  * rather than the two standard versions of operator++ (prefix and postfix)
  * returning an iterator (which cannot be done, since the class is abstract)
  * there is only one version doing the increment. This single version is
  * then transformed into the ordinary two by the "normal" const_iterator. */

 class v_const_iterator {
  public:

  typedef const Variable value_type;
  typedef const Variable & reference;
  typedef const Variable * pointer;
  typedef int difference_type;
  typedef std::forward_iterator_tag iterator_category;

  v_const_iterator() = default;                                ///< constructor
  virtual ~v_const_iterator() = default;                       ///< destructor
  virtual v_const_iterator * clone() = 0;                      ///< cloner
  virtual void operator++() = 0;                               ///< increment
  virtual reference operator*() const = 0;                     ///< operator*
  virtual pointer operator->() const = 0;                      ///< Operator->
  virtual bool
  operator==( const v_const_iterator & rhs ) const = 0;        ///< operator==
  virtual bool
  operator!=( const v_const_iterator & rhs ) const = 0;        ///< operator!=
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// standard iterator (redirecting a v_iterator)
 /** ThinVarDepInterface::iterator is a full-fledged forward_iterator with
  * the right traits to allow sifting through the "active" Variable. It gets
  * a pointer to a ThinVarDepInterface::v_iterator and redirects all its
  * methods to it. 
  *
  * Note that iterator automatically destroys the v_iterator it depends
  * onto; hence, copy constructor, copy assignment operator and post-increment
  * operators all both use clone() to make a copy. */

 class iterator {
  public:

  typedef Variable value_type;
  typedef Variable & reference;
  typedef Variable * pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  // constructor taking a v_iterator, which becomes property
  explicit iterator( v_iterator * itr ) : itr_( itr ) {}

  // standard copy constructor, note the clone()
  iterator( iterator & itr ) { itr_ = itr.itr_->clone(); }

  // standard move constructor
  iterator( iterator && itr ) noexcept {
   itr_ = itr.itr_;
   itr.itr_ = nullptr;
  }

  // destructor, deletes the virtual iterator
  ~iterator() { delete itr_; }

  // standard assignment, note the clone()
  iterator & operator=( iterator & itr ) {
   itr_ = itr.itr_->clone();
   return( *this );
   }

  // standard move assignment
  iterator & operator=( iterator && itr ) {
   itr_ = itr.itr_;
   itr.itr_ = nullptr;
   return( *this );
   }

  // pre-increment operator ++it, use this preferably
  iterator & operator++() {
   itr_->operator++();
   return( *this );
   }

  // post-increment operator it++, avoid this if possible
  iterator operator++( int ) {
   // note that the copy need be disjoint, hence the clone()
   iterator i( itr_->clone() );
   itr_->operator++();
   return( i );
   }

  reference operator*() const { return( *( *itr_ ) ); }

  pointer operator->() const { return( itr_->operator->() ); }

  bool operator==( const iterator & rhs ) const {
   return( *( rhs.itr_ ) == *itr_ );
   }

  bool operator!=( const iterator & rhs ) const {
   return( *itr_ != *( rhs.itr_ ) );
   }

  private:

  v_iterator * itr_;
 };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// standard const iterator (redirecting a v_const_iterator)
 /** ThinVarDepInterface::iterator is a full-fledged const forward_iterator
  * with the right traits to allow sifting through the "active" Variable. It
  * gets a pointer to a ThinVarDepInterface::v_const_iterator and redirects
  * all its methods to it.
  *
  * Note that const_iterator automatically destroys the v_const_iterator it
  * depends onto; hence, copy constructor and assignment operators both use
  * clone() to make a copy. */

 class const_iterator {
  public:

  typedef const Variable value_type;
  typedef const Variable & reference;
  typedef const Variable * pointer;
  typedef int difference_type;
  typedef std::forward_iterator_tag iterator_category;

  // constructor taking a v_iterator, which becomes property
  explicit const_iterator( v_const_iterator * itr ) : itr_( itr ) {}

  // standard copy constructor, note the clone()
  const_iterator( const_iterator & itr ) { itr_ = itr.itr_->clone(); }

  // standard move constructor
  const_iterator( const_iterator && itr ) noexcept {
   itr_ = itr.itr_;
   itr.itr_ = nullptr;
   }

  // destructor, deletes the virtual iterator
  ~const_iterator() { delete itr_; }

  // standard assignment, note the clone()
  const_iterator & operator=( const_iterator & itr ) {
   itr_ = itr.itr_->clone();
   return( *this );
   }

  // standard move assignment
  const_iterator & operator=( const_iterator && itr ) {
   itr_ = itr.itr_;
   itr.itr_ = nullptr;
   return( *this );
   }

  // pre-increment operator ++it, use this preferably
  const_iterator & operator++() {
   itr_->operator++();
   return( *this );
   }

  // post-increment operator it++, avoid this if possible
  const_iterator operator++( int ) {
   // note that the copy need be disjoint, hence the clone()
   const_iterator i( itr_->clone() );
   itr_->operator++();
   return( i );
   }

  reference operator*() const { return( *( *itr_ ) ); }

  pointer operator->() const { return( itr_->operator->() ); }

  bool operator==( const const_iterator & rhs ) const {
   return( *itr_ == *( rhs.itr_ ) );
   }

  bool operator!=( const const_iterator & rhs ) const {
   return( *itr_ != *( rhs.itr_ ) );
   }

  private:

  v_const_iterator * itr_;
 };

/** @} ---------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Constructor and destructor
 *
 * Since ThinVarDepInterface and their "active" Variable are "doubly" linked,
 * destroying them has the problem of ensuring that the pointers in each
 * object's list are still "live" when the object is destroyed. To simplify
 * this, the underlying assumption is that
 *
 *     *Variable are constructed before the "stuff" they are active
 *      in and destructed after them*
 *
 * which makes it possible for Variable to ignore their list when they are
 * destroyed, possibly saving some pointless work, whereby the list are to
 * be updated by "stuff" destroying themselves right before than the Variable
 * itself is destroyed. However, (a)symmetrically this implies that when a
 * ThinVarDepInterface is destroyed, it has to remove itself from the list
 * of "stuff" the Variable is active in, to avoid leaving the Variable (that
 * is assumed to "live longer") in an inconsistent state. This may still
 * result in pointless work being done, for avoiding which the clear()
 * method is provided.
 *
 * Avoiding pointless work, however, is only one of the rationale for clear().
 * The other case that needs to be considered is that of ThinVarDepInterface
 * and their Variable (possibly std::vector<> etc. of them) being fields of a
 * :Block, so that they are automatically destroyed in the corresponding
 * destructor. As the order in which this happens is usually not very clear,
 * one should strive to have the fields explicitly cleared in the destructor,
 * so that all the ThinVarDepInterface are destroyed before all the Variable.
 * This is simple if everything lives in STL containers (std::vector<> etc.)
 * since then one can call the clear() method of the vector to have them
 * destroyed; however, this does not work for individual objects. Thus,
 * besides avoiding pointless work, clear() it allows for explicitly 
 * clearing objects, to ensure that they are in fact "destroyed" before their
 * active Variable.
 *  @{ */

 /// constructor of ThinVarDepInterface; does nothing
 ThinVarDepInterface( void ) = default;

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty
 /** The destructor of the base ThinVarDepInterface class has nothing to do.
  * However, it is important to remark that the destructor of any derived
  * class *is* (unlike that of Variable) assumed to scan through the list of
  * "active Variable" of this ThinVarDepInterface and remove itself from
  * them. The idea is that
  *
  *     *Variable are constructed before the "stuff" they are active
  *      in and destructed after them*
  *
  * and hence, before that a ThinVarDepInterface can be destructed, it should
  * in principle clear up the data structure in its "active" Variable, to
  * allow them to safely destruct themselves without making any reference to
  * no longer existing object.
  *
  * However, this means that a nontrivial amount of work may be done
  * pointlessly when destructing a Block, in that the ThinVarDepInterface will
  * have to update the data structures linking Variable to them right before
  * the Variable themselves are destructed. This is why the clean() method is
  * provided to do the "guts of destructor" *without* the removing from the
  * Variable, which helps in avoiding useless work. */

 virtual ~ThinVarDepInterface() = default;

/*--------------------------------------------------------------------------*/
 /// "rough destructor" that does not warn the "active" Variable
 /** The clear() method is intended to be a "guts of destructor": it should
  * leave the ThinVarDepInterface object "empty" and ready to be destructed
  * with zero effort. However, it does so *without first removing the
  * ThinVarDepInterface object from the list of "active stuff" in the
  * corresponding Variable*, unlike what the standard destructor is assumed
  * to do. This leaves any such Variable in an inconsistent state, so care
  * has to be exercised to only use clear() when the only thing that can
  * happen to these Variable after that is that they be destroyed. Note that
  * the destructor of Variable is assumed not to access its list of "active
  * stuff" and remove itself from it, hence it is safe to use clear() and
  * leave these Variable in an inconsistent state if the destructor is the
  * only method of theirs that is going to be called next.
  *
  * However, this method is "optional": not calling it before destroying a
  * ThinVarDepInterface means that the object will be removed from the
  * list of "active stuff" of the corresponding Variable, which may be
  * useless but at least never leaves anything in an inconsistent state.
  * Hence, this method is given an empty implementation for derived classes
  * that just do not have to care (for instance because they do not directly
  * register themselves as "active stuff" into Variable, see Function). Also,
  * this method should only be called when the underlying assumption is
  * guaranteed to be satisfied; in doubt, do not call it. */

 virtual void clear( void ) {}

/** @} ---------------------------------------------------------------------*/
/*-------- METHODS FOR READING THE DATA OF THE ThinVarDepInterface ---------*/
/*--------------------------------------------------------------------------*/
/** @name Reading the data of the ThinVarDepInterface
    @{ */

 /// returns a pointer to the Block to which the ThinVarDepInterface belongs
 /** Every ThinVarDepInterface belongs to a Block; return a pointer to it.
  * The base class implementation is pure virtual so as to allow derived
  * classes full flexibility. */

 [[nodiscard]] virtual Block * get_Block( void ) const = 0;

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE SET OF "ACTIVE" Variable ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the set of "active" Variable
 *
 * These methods provide a general interface that allows to sift through the
 * set of the "active" Variable of a ThinVarDepInterface. A particular issue
 * to be discussed is the fact that n variable are naturally addressed by an
 * Index in 0, 1, ... n - 1; hence, these methods (at least in part) rely on
 * the concept of "i-th active Variable". Since the implementation of the set 
 * of "active" Variable is entirely left to derived classes, to allow them
 * complete flexibility, in principle little can be said about how the
 * order is established. That is, in general how the Index is chosen for
 * each "active" Variable is left to the specific derived class. However,
 * a minimum set of general principles must be established that set the rules
 * that all implementation must satisfy. These are the following:
 *
 * 1) The ordering does not change unless the set of "active" Variable itself
 *    changes. That is, given an existing "active" Variable, the value
 *    returned by is_active() must remain the same unless "active" Variable
 *    are either added to or removed from the set.
 *
 * 2) The ordering is "maximally stable under addition": when a new "active"
 *    Variable is added to the set, all the existing "active" Variable keep
 *    the index they previously have, and the new one takes the only possible
 *    new index (that is, the value of get_num_active_var() before the
 *    addition). Note that, therefore, if a *set* of new "active" Variable is
 *    added, the indices of the new Variable *do* depend on the ordering in
 *    which the Variable are added. Since no method is explicitly provided
 *    for adding a set of Variable (actually, not even a single one), how the
 *    order is specified in the set of new Variable will depend on the
 *    individual methods of the derived classes.
 *
 * 3) The ordering is "minimally disrupted under deletion": when an existing
 *    "active" Variable whose current Index is "i" is deleted, all the
 *    remaining "active" Variable whose index is < i keep the same Index,
 *    while all those whose index is j > i get as new Index j - 1. That is,
 *    all indices greater than i are shifted left by one. Note that, 
 *    therefore, if a *set* of "active" Variable is deleted, the indices of
 *    remaining Variable *do not* depend on the ordering in which the
 *    Variable are removed (not that methods are explicitly provided in the
 *    base ThinVarDepInterface for deleting a set of Variable, only a single
 *    one).
 *
 * As a consequence of these principles, 
 *
 *     THE Index OF AN EXISTING "ACTIVE" Variable CAN ONLY DIMINISH AS THE
 *     SET OF "ACTIVE" Variable CHANGES, UNLESS THE Variable IS DELETED AND
 *     RE-ADDED.
 *
 * This may be useful for implementations, e.g. for Modification related to
 * adding and removing "active" Variable from a ThinVarDepInterface. The
 * issue there is that the state of the ThinVarDepInterface when the
 * Modification is processed can be rather different from when it was issued,
 * with many "active" Variable having been added/removed in arbitrary order.
 * Hence, to identify the involved "active" Variable its Index cannot be
 * used, and a pointer must be. This requires finding the Index given the
 * pointer [see is_active()], which may have a cost. However, assume for
 * instance that:
 *
 * - the Modification also stores the original Index of the Variable, which
 *   may no longer be the correct one (but may still be);
 *
 * - the Modification only has to be processed if the Variable has not been
 *   removed, even if it has been re-added afterwards (which makes sense,
 *   as removing ad adding typically entail their own Modification which
 *   will be proceeded in due time).
 *
 * Then, the current Index of the Variable need only be searched for among
 * these smaller than (or equal to) the original one, and if the Variable is
 * not found there then it means that is has been removed, and therefore the
 * Modification can safely be ignored (irrespectively to if the Variable has
 * been re-added in the meantime).
 *
 * Any ThinVarDepInterface must allows access to the set of "active" Variable
 * via their Index [see get_active_var()]. However, another basically
 * independent means of access is provided via a (virtualized) forward
 * iterator, with standard begin() and end() accessors, so that any standard
 * STL algorithm only relying on forward iterators can be applied.
 *
 * An important note is that
 *
 *     REPEATED "ACTIVE" Variable ARE NOT PERMITTED; THAT IS, IT IS NOT 
 *     ALLOWED THAT get_active_var( i ) == get_active_var( j ) FOR i != j
 *
 * This is only natural, and ties in with the fact that there is a one-to-one
 * map between "active" Variable pointers and their indices, see map_active().
 *  @{ */

 /// get the number of Variables that are "active"
 /** Pure virtual method to get the number of Variables that are "active".
  * The base ThinVarDepInterface class makes no provisions about how this set
  * is stored in order to leave complete freedom to derived classes to
  * implement as they best see fit. */

 [[nodiscard]] virtual Index get_num_active_var( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns the Index of a given "active" Variable
 /** Pure virtual method that returns:
  *
  * - if this ThinVarDepInterface depends on the given Variable, the index i
  *   in 0, ..., get_num_active_var() - 1 such that the given Variable is the
  *   i-th "active" variable;
  *
  * - otherwise, any number >= get_num_active_var() (say, Inf< Index >()).
  *
  * The base ThinVarDepInterface class makes no provisions about how this set
  * is stored in order to leave complete freedom to derived classes to
  * implement as they best see fit. */

 virtual Index is_active( const Variable * var ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns the set of indices of a given set "active" Variable
 /** Given in vars a set of (pointers to) Variable that are "active" in the
  * ThinVarDepInterface, returns the set of their indices in map; that is,
  * map[ i ] will contain the index of var[ i ]. If map.size() < vars.size()
  * then map is resized, otherwise it is not changed (which means that only
  * its first vars.size() are changed, the others being left untouched). If
  * any of the Variable in vars is not "active" in the ThinVarDepInterface,
  * an exception is thrown.
  *
  * The parameter ordered tells if vars is ordered by increasing name =
  * pointer.
  *
  * This method is not pure virtual: the base ThinVarDepInterface provides 
  * *two* implementations, selected by the value of ordered:
  *
  * - if ordered == false, the map is constructed by repeated calls to
  *   is_active(); this is at worst O( vars.size() * get_num_active_var() ),
  *   if is_active() has a trivial  O( get_num_active_var() ) implementation;
  *
  * - if ordered == true, the map is rather constructed by sifting through the
  *   list of "active" Variable and looking each up in vars(), which has
  *   O( log( vars.size() ) * get_num_active_var() ) complexity if
  *   get_active_var() is O( 1 ).
  *
  * However, the method is virtual, so that derived classes may provide more
  * efficient implementations exploiting properties of their specific data
  * structures (say, the set of "active" Variable has an ordered map, and
  * ordered == true). Furthermore, note that nothing is preventing a user to
  * set ordered == false even if vars is actually ordered to choose the
  * implementation (if the one of the base class is used) if it is for any
  * reason preferable to do so. */

 virtual void map_active( const std::vector< Variable * > & vars ,
                          Subset & map , bool ordered = false ) const {
  if( vars.empty() )
   return;

  if( map.size() < vars.size() )
   map.resize( vars.size() );

  if( ordered ) {
   Index found = 0;
   for( Index i = 0 ; i < get_num_active_var() ; ++i ) {
    auto vi = get_active_var( i );
    auto itvi = std::lower_bound( vars.begin() , vars.end() , vi );
    if( itvi != vars.end() ) {
     map[ std::distance( vars.begin(), itvi ) ] = i;
     ++found;
     }
    }

   if( found < vars.size() )
    throw( std::invalid_argument( "map_active: some Variable is not active"
				  ) );
   }
  else {
   auto it = map.begin();
   for( auto var : vars ) {
    Index i = is_active( var );
    if( i >= get_num_active_var() )
     throw( std::invalid_argument( "map_active: some Variable is not active"
				   ) );
    *(it++) = i;
    }
   }
  }

/*--------------------------------------------------------------------------*/
 /// returns the indices of a set of Variable given the original Subset 
 /** The method inputs a vector of (pointers to) Variable that *used to be*
  * "active" in the ThinVarDepInterface, as well as the Subset (of the same
  * size(), and ordered in increasing sense) that contains their original
  * indices. It returns a Subset (of again the same size()) that contains, in
  * the position corresponding to each Variable, the *current* index of the
  * Variable in the ThinVarDepInterface, or any number >=
  * get_num_active_var() (say, Inf< Index >()) if the Variable is not
  * currently "active" in the ThinVarDepInterface.
  *
  * The assumption for the method to work efficiently is that the input
  * Subset used to be correct at some point, but it may no longer be because
  * some "active" Variable may have been added / deleted in the meantime. This
  * is tyipically the case of a Subset that has been stored in a Modification,
  * which is processed after that, possibly, a bunch of changes have occurred
  * in the ThinVarDepInterface. Handling these changes may require to access
  * information in the ThinVarDepInterface corresponding to the Variable,
  * which is done via their index. This method tries to obtain the set of
  * current indices faster than map_active(), that just blindly seeks the
  * index of each Variable, by betting that the current index will be
  * "close to the original one to the left (because some Variable with
  * smaller index has been deleted), and otherwise close to the end of the
  * set of active Variable (because the Variable has been deleted and then
  * re-added)". This should be O( #vars ) if the set of indices has not
  * changed, but it can be O( #vars * get_num_active_var() ) (as map_active()
  * is) in the worst case where everything changed. 
  *
  * This method is not pure virtual: the base ThinVarDepInterface provides 
  * one implementation using the virtual methods of the class. However, the
  * method is virtual, so that derived classes may provide more efficient
  * implementations exploiting properties of their specific data structures
  * (if only by avoiding virtual calls). */

 virtual Subset map_index( const std::vector< Variable * > & vars ,
			   c_Subset & nms ) const {
  if( vars.size() != nms.size() )
   throw( std::invalid_argument( "vars and nms sizes do not match" ) );

  Subset map( vars.size() );
  if( map.empty() )
   return( map );

  auto nmsit = nms.begin();
  auto varsit = vars.begin();
  auto mapit = map.begin();
  c_Index nav = get_num_active_var();

  // for all Variable in the set
  while( varsit != vars.end() ) {
   auto var = *(varsit++);  // next variable
   auto oi = *(nmsit++);    // its original index
   // if var has not been deleted (and, possibly, re-added), its index
   // must be <= oi: search backward from oi to find it
   auto avoi = get_active_var( oi );
   Index i = oi;
   while( var != avoi ) {
    if( ! i )
     break;
    avoi = get_active_var( --i );
    }
   if( var == avoi )  // the Variable was found
    *(mapit++) = i;   // this is its index
   else {             // the Variable was not found
    // restart the search from the last variable to oi (excluded), for the
    // case where var has been deleted and re-added, and therefore its
    // index can now be arbitrary (but it is more likely to be "close to
    // the end" than "at the beginning")
    for( i = nav , avoi = get_active_var( --i ) ;
	 ( var != avoi ) && ( i > oi ) ; )
     avoi = get_active_var( --i );
    *(mapit++) = ( var == avoi ) ? i : Inf< Index >();
    }
   }  // end( for all Variable )

  return( map );

  }  // end( map_index( Subset )

/*--------------------------------------------------------------------------*/
 /// returns the indices of a set of Variable given the original Range 
 /** The method inputs a vector of (pointers to) Variable that *used to be*
  * "active" in the ThinVarDepInterface, as well as the Range (of the same
  * size) of their original indices. It returns a Subset (of again the same
  * size()) that contains, in the position corresponding to each Variable,
  * the *current* index of the Variable in the ThinVarDepInterface, or any
  * number >= get_num_active_var() (say, Inf< Index >()) if the Variable is
  * not currently "active" in the ThinVarDepInterface.
  *
  * The assumption for the method to work efficiently is that the input
  * Range used to be correct at some point, but it may no longer be because
  * some "active" Variable may have been added / deleted in the meantime. This
  * is tyipically the case of a Range that has been stored in a Modification,
  * which is processed after that, possibly, a bunch of changes have occurred
  * in the ThinVarDepInterface. Handling these changes may require to access
  * information in the ThinVarDepInterface corresponding to the Variable,
  * which is done via their index. This method tries to obtain the set of
  * current indices faster than map_active(), that just blindly seeks the
  * index of each Variable, by betting that the current index will be
  * "close to the original one to the left (because some Variable with
  * smaller index has been deleted), and otherwise close to the end of the
  * set of active Variable (because the Variable has been deleted and then
  * re-added)". This should be O( #vars ) if the set of indices has not
  * changed, but it can be O( #vars * get_num_active_var() ) (as map_active()
  * is) in the worst case where everything changed. 
  *
  * This method is not pure virtual: the base ThinVarDepInterface provides 
  * one implementation using the virtual methods of the class. However, the
  * method is virtual, so that derived classes may provide more efficient
  * implementations exploiting properties of their specific data structures
  * (if only by avoiding virtual calls). */

 virtual Subset map_index( const std::vector< Variable * > & vars ,
			   Range rng ) const {
  if( vars.size() != rng.second - rng.first )
   throw( std::invalid_argument( "vars and rng sizes do not match" ) );

  Subset map( vars.size() );
  if( map.empty() )
   return( map );

  auto varsit = vars.begin();
  auto mapit = map.begin();
  c_Index nav = get_num_active_var();

  // for all Variable in the set
  for( auto oi = rng.first ; oi < rng.second ; ++oi ) {
   auto var = *(varsit++);  // next variable
   // if var has not been deleted (and, possibly, re-added), its index
   // must be <= oi: search backward from oi to find it
   auto avoi = get_active_var( oi );
   Index i = oi;
   while( var != avoi ) {
    if( ! i )
     break;
    avoi = get_active_var( --i );
    }
   if( var == avoi )  // the Variable was found
    *(mapit++) = i;   // this is its index
   else {             // the Variable was not found
    // restart the search from the last variable to oi (excluded), for the
    // case where var has been deleted and re-added, and therefore its
    // index can now be arbitrary (but it is more likely to be "close to
    // the end" than "at the beginning")
    for( i = nav , avoi = get_active_var( --i ) ;
	 ( var != avoi ) && ( i > oi ) ; )
     avoi = get_active_var( --i );
    *(mapit++) = ( var == avoi ) ? i : Inf< Index >();
    }
   }  // end( for all Variable )

  return( map );

  }  // end( map_index( Range )

/*--------------------------------------------------------------------------*/
 /// get a pointer to the i-th "active" Variable
 /** Pure virtual method to get a pointer to the i-th Variable that is
  * "active" for this ThinVarDepInterface, where i is between 0 and n =
  * get_num_active_var() - 1. For the i-th active Variable, i is said to be
  * the index of the Variable in the ThinVarDepInterface.
  *
  * The base ThinVarDepInterface class makes no provisions about how the set
  * of "active" Variables is stored in order to leave more freedom to derived
  * classes to implement it in specialized ways. Accordingly, the way in
  * which indices are associated with Variables is determined by the derived
  * class, subject to the assumptions stated in the general comments to this
  * section. */

 [[nodiscard]] virtual Variable * get_active_var( Index i ) const = 0;

/*--------------------------------------------------------------------------*/
 /// get (a pointer to) a v_iterator for scanning the "active" Variable
 /** Pure virtual method to get an iterator which "points" at the beginning
  * of the set of "active" Variable, so that it can be used to iterate
  * through all them. However, because v_iterator is only a virtual base
  * class, the method cannot return the object but a reference (pointer) to
  * it, hence this cannot be used in the same way as ordinary STL containers
  * can. This is why one has begin() returning an "ordinary" iterator. This
  * method is still provided if the user wants to directly deal with the
  * v_iterator, and also because begin() can be implemented in terms of this
  * version. */

 virtual v_iterator * v_begin( void ) = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get (a pointer to) a v_const_iterator for scanning the "active" Variable
 /** Const version of v_begin(), see the comments there. */

 [[nodiscard]] virtual v_const_iterator * v_begin( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// get (a pointer to) a v_iterator for the end of the "active" Variable
 /** Pure virtual method to get an iterator which "points" at the end of the
  * set of "active" Variable, so that it can be used to iterate through all
  * them. However, because v_iterator is only a virtual base class, the
  * method cannot return the object but a reference (pointer) to it, hence
  * this cannot be used in the same way as ordinary STL containers can. This
  * is why one has end() returning an "ordinary" iterator. This method is
  * still provided if the user wants to directly deal with the v_iterator,
  * and also because end() can be implemented in terms of this version. */

 virtual v_iterator * v_end( void ) = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get (a pointer to) a v_const_iterator for the end the "active" Variable
 /** Const version of v_end(), see the comments there. */

 [[nodiscard]] virtual v_const_iterator * v_end( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// get an iterator for scanning the "active" Variable
 /** This method only converts a pointer to a v_iterator to the iterator and
  * returns the latter. However it is virtual, so that derived classes may
  * redefine it if needed. */

 virtual iterator begin( void ) { return( iterator( v_begin() ) ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a const iterator for scanning the "active" Variable
 /** Const version of end(), see the comments there. */

 [[nodiscard]] virtual const_iterator begin( void ) const {
  return( const_iterator( v_begin() ) );
  }

/*--------------------------------------------------------------------------*/
 /// get an iterator for the end of the "active" Variable
 /** This method only converts a pointer to a v_iterator to the iterator and
  * returns the latter. However it is virtual, so that derived classes may
  * redefine it if needed. */

 virtual iterator end( void ) { return( iterator( v_end() ) ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a const iterator for the end of the "active" Variable
 /** Const version of end(), see the comments there. */

 [[nodiscard]] virtual const_iterator end( void ) const {
  return( const_iterator( v_end() ) );
  }

/** @} ---------------------------------------------------------------------*/
/*------------ METHODS FOR CHANGING THE SET OF "ACTIVE" Variable -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for changing the set of "active" Variable
 *
 * The pure virtual method remove_variable() is the single entry point that
 * each ThinVarDepInterface must have to allow removal of one "active"
 * Variable. This is necessary for Block to be able to remove the (dynamic)
 * Variable that are deleted from all the existing Constraint.
 *  @{ */

 /// removes the given Variable from the "active" ones
 /** Pure virtual method that deletes the i-th Variable from the set of
  * "active" ones. The base ThinVarDepInterface class makes no provisions
  * about how this is done in order to leave more freedom to derived classes
  * to implement it in specialized ways.
  *
  * The parameter issueMod decides if and how a Modification (whose specific
  * type depends on the :ThinVarDepInterface at hand) is issued, as described
  * in Observer::make_par().
  *
  * Usually, the Modification should be thrown. A relevant exception is the
  * case in which the method is called while destroying the (dynamic)
  * Variable. Indeed, in such a case the affected Variable don't really need
  * to know that it is no longer active on this ThinVarDepInterface (as it
  * is to be destroyed anyway). Note that this may leave the Variable in an
  * inconsistent state, whereby the Variable still counts this
  * ThinVarDepInterface among the ones it is active in, while the
  * ThinVarDepInterface does not. Yet, since the Variable is deleted, the
  * not-deleted part of the overall Block is in a consistent state. Note,
  * however, that a deleted Variable may remain "undead" into a Modification
  * object waiting to be processed by some Solver; one should therefore make
  * as few assumptions as possible on the status of the internal data
  * structures of such "undead" Variable, in particular on their list of
  * "active stuff". */

 virtual void remove_variable( Index i , ModParam issueMod = eModBlck ) = 0;

/*--------------------------------------------------------------------------*/
 /// remove a range of Variable
 /** Remove a range of "active" Variable.
  *
  * @param range contains the indices of the Variable to be deleted
  *        (hence, range.second <= get_num_active_var());
  *
  * @param issueMod which decides if and how a Modification (whose specific
  *        type depends on the :ThinVarDepInterface at hand) is issued, as
  *        described in Observer::make_par().
  *
  * The method is not pure virtual in that the base class provides the obvious
  * implementation calling remove_variable( i ) for all i in nms. However, this
  * results in many Modification being issued instead of one, and it is
  * generally less efficient. Yet, more efficient implementations require
  * knowledge of the actual implementation of the set of active Variable of
  * the ThinVarDepInterface, and therefore necessarily need be demanded to
  * the derived classes. */

 virtual void remove_variables( Range range , ModParam issueMod = eModBlck ) {
  if( range.second <= range.first )  // empty range
   return;                           // silently (and cowardly) return

  if( range.second >= get_num_active_var() )
   throw( std::invalid_argument( "remove_variables: invalid range" ) );

  // note: the removal loop goes backward, since eliminating a variable
  //       changes the "names" of all the variable with larger name
  for( Index i = range.second; i > range.first; )
   remove_variable( --i, issueMod );
  }

/*--------------------------------------------------------------------------*/
 /// remove a set of variables
 /** Remove all the Variable in the given set of indices.
  *
  * @param nms is Subset & containing the indices of the Variable to be
  *        removed, i.e., integers between 0 and get_num_active_var() - 1;
  *        note that Subset is non-const because if it is not ordered (see
  *        next parameter) the method may decide to order it, and/or to
  *        move it into the Modification that is possibly issued (see third
  *        parameter), which means that it is in general not safe to assume
  *        that the parameter is still available after the call. hence, the
  *        parameter may as well be && (as it is). a special setting is if
  *
  *     nms.empty() == true, IN WHICH CASE ALL Variable ARE ELIMINATED
  *
  * @param ordered is a bool indicating if nms[] is already ordered in
  *        increasing sense);
  *
  * @param issueMod which decides if and how a Modification (whose specific
  *        type depends on the :ThinVarDepInterface at hand) is issued, as
  *        described in Observer::make_par().
  *
  * The method is not pure virtual in that the base class provides the obvious
  * implementation calling remove_variable( i ) for all i in nms. However,
  * this results in many Modification being issued instead of one, and it is
  * generally less efficient. Yet, more efficient implementations require
  * knowledge of the actual implementation of the set of active Variable of
  * the ThinVarDepInterface, and therefore necessarily need be demanded to
  * the derived classes. */

 virtual void remove_variables( Subset && nms , bool ordered = false ,
                                ModParam issueMod = eModBlck ) {
  // note: the removal loop goes backward, since eliminating a variable
  //       changes the "names" of all the variable with larger name;
  //       this is why nms need be ordered (if nonempty)

  if( nms.empty() )
   for( Index i = get_num_active_var() ; i > 0 ; )
    remove_variable( --i , issueMod );
  else {
   if( ! ordered )
    std::sort( nms.begin() , nms.end() );

   for( auto it = nms.rbegin() ; it != nms.rend() ; ++it )
    remove_variable( *it , issueMod );
   }
  }

/** @} ---------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

 };  // end( class( ThinVarDepInterface ) )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif  /* ThinVarDepInterface.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File ThinVarDepInterface.h ----------------------*/
/*--------------------------------------------------------------------------*/
