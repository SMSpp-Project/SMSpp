/*--------------------------------------------------------------------------*/
/*----------------------- File ThinVarDepInterface.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* ThinVarDepInterface class, a very thin
 * base class for all objects in SMS++ (Constraint, Objective, Function, ...)
 * that depend on a set of "active" Variable.
 *
 * \version 0.10
 *
 * \date 14 - 08 - 2018
 *
 * \author Antonio Frangioni \n
 *         Operations Research Group \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * Copyright &copy by Antonio Frangioni
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

// #include "Variable.h"
#include <iterator>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{
 class Variable;  // forward definition of Variable

/*--------------------------------------------------------------------------*/
/*------------------ ThinVarDepInterface-RELATED TYPES ---------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup ThinVarDepInterface_TYPES ThinVarDepInterface-related types.
 *  @{ */

/*@}  end( group( ThinVarDepInterface_TYPES ) ) */
/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup ThinVarDepInterface_CLASSES Classes in ThinVarDepInterface.h
 *  @{ */
 
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
 
 typedef unsigned int Index;   ///< type for the indices of "active" Variable

 typedef const Index c_Index;           ///< a const Index

 typedef std::vector<Index> Vec_Index;  ///< a std::vector of Index

 typedef const Vec_Index c_Vec_Index;   ///< a const Vec_Index

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
  * then transformed into the ordinary two by the "normal" iterator. */

 class v_iterator
 {
  public:

  typedef Variable value_type;
  typedef Variable& reference;
  typedef Variable* pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;

  v_iterator( void ) { }                                     ///< constructor
  virtual ~v_iterator() { };                                  ///< destructor

  virtual void operator++( void ) = 0;                         ///< increment
  virtual reference operator*( void ) const = 0;               ///< operator*
  virtual pointer operator->( void ) const = 0;                ///< operator->
  virtual bool operator==( const v_iterator& rhs ) const = 0;  ///< operator==
  virtual bool operator!=( const v_iterator& rhs ) const = 0;  ///< operator!=
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

 class v_const_iterator
 {
  public:

  typedef const Variable value_type;
  typedef const Variable& reference;
  typedef const Variable* pointer;
  typedef int difference_type;
  typedef std::forward_iterator_tag iterator_category;

  v_const_iterator( void ) { }                              ///< constructor
  virtual ~v_const_iterator() { };                          ///< destructor
  
  virtual void operator++( void ) = 0;                        ///< increment
  virtual reference operator*( void ) const = 0;              ///< operator*
  virtual pointer operator->( void ) const = 0;               ///< Operator->
  virtual bool operator==( const v_const_iterator & rhs ) const = 0;
  ///< operator==
  virtual bool operator!=( const v_const_iterator& rhs ) const = 0;
  ///< operator!=
  };

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// standard iterator (redirecting a v_iterator)
 /** ThinVarDepInterface::iterator is a full-fledged forward_iterator with
  * the right traits to allow sifting through the "active" Variable. It gets
  * a pointer to a ThinVarDepInterface::v_iterator and redirects all its
  * methods to it. 
  *
  * Note that iterator automatically destroys the v_iterator it depends
  * onto; hence, copy constructor and assignment operators both have the
  * move semantic. */

 class iterator
 {
  public:

  typedef Variable value_type;
  typedef Variable& reference;
  typedef Variable* pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef int difference_type;
  
  iterator( v_iterator * itr ) : itr_( itr ) { }
  iterator( iterator && itr ) { itr_ = itr.itr_; itr.itr_ = nullptr; }
  iterator & operator=( iterator && itr ) {
   itr_ = itr.itr_; itr.itr_ = nullptr; return *this;
   }
  ~iterator( ) { delete itr_; }

  iterator operator++( void ) {
   iterator i( itr_ ); itr_->operator++(); return( i );
   }
  iterator & operator++( int ) { itr_->operator++(); return( *this ); }
  reference operator*( void ) const { return *(*itr_); }
  pointer operator->( void ) const { return itr_->operator->(); }
  bool operator==( const iterator & rhs ) const {
   return( *itr_ == *(rhs.itr_) );
   }
  bool operator!=( const iterator & rhs ) const {
   return( *itr_ != *(rhs.itr_) );
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
  * depends onto; hence, copy constructor and assignment operators both have
  * the move semantic. */

 class const_iterator
 {
  public:

  typedef const Variable value_type;
  typedef const Variable& reference;
  typedef const Variable* pointer;
  typedef int difference_type;
  typedef std::forward_iterator_tag iterator_category;

  const_iterator( v_const_iterator * itr ) : itr_( itr ) { }
  const_iterator( const_iterator && itr ) {
   itr_ = itr.itr_; itr.itr_ = nullptr;
   }
  const_iterator & operator=( const_iterator && itr ) {
   itr_ = itr.itr_; itr.itr_ = nullptr; return *this;
   }
  ~const_iterator( ) { delete itr_; }

  const_iterator operator++( void ) {
   const_iterator i( itr_ ); itr_->operator++(); return( i );
   }
  const_iterator & operator++( int ) { itr_->operator++(); return( *this ); }
  reference operator*( void ) const { return *(*itr_); }
  pointer operator->( void ) const { return itr_->operator->(); }
  bool operator==( const const_iterator & rhs ) const {
   return( *itr_ == *(rhs.itr_) );
   }
  bool operator!=( const const_iterator & rhs ) const {
   return( *itr_ != *(rhs.itr_) );
   }

  private:

  v_const_iterator * itr_;
  };

/*@}------------------------------------------------------------------------*/
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
 ThinVarDepInterface( void ) {}

/*--------------------------------------------------------------------------*/
 /// destructor: it is virtual, and empty
 /** The destructor of the base ThinVarDepInterface class has nothing to do.
  * However, it is important to remark that the destructor of any derived
  * class *is* (unlike that of Variable) assumed to scan through the list of
  * "active Variable" of this ThinVarDepInterface and remove the itself from
  * then. The idea is that
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

 virtual ~ThinVarDepInterface() {}

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
  * this mathod should only be called when the underlying assumption is
  * guaranteed to be satisfied; in doubt, do not call it. */
 
 virtual void clear( void ) { }

/*@} -----------------------------------------------------------------------*/
/*-------------------------- OTHER INITIALIZATIONS -------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Other initializations
 *  @{ */

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR READING THE SET OF "ACTIVE" Variable ------------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for reading the set of "active" Variable
 *  @{ */

 /// get the number of Variables that are "active"
 /** Pure virtual method to get the number of Variables that are "active".
  * Note that the base ThinVarDepInterface class makes no provisions about
  * how this set is stored in order to leave more freedom to derived classes
  * to implement it in specialized ways. */

 virtual Index get_num_active_var( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns the Index of a given "active" Variable
 /** Pure virtual method that returns:
  *
  * - if this ThinVarDepInterface depends on the given Variable, the index i
  *   in 0, ..., get_num_active_var() such that the given Variable is the
  *   i-th "active" variable;
  *
  * - otherwise, any number >= get_num_active_var() (say, Inf<Index>()).
  *
  * The base ThinVarDepInterface class makes no provisions about how this is
  * done in order to leave more freedom to derived classes to implement it in
  * specialized ways. */

 virtual Index is_active( const Variable * const var ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns the set of indices of a given set "active" Variable
 /** Given in vars a set of (pointers to) Variable that are "active" in the
  * ThinVarDepInterface, returns the set of their indices in map; that is,
  * map[ i ] will contain the index of var[ i ]. If map has size < vars then
  * it is resized, otherwise it is not changed (which means that only its
  * first vars.size() are changed, the others being left untouched). If any
  * of the Variable in vars is not "active" in the ThinVarDepInterface, an
  * exception is thrown.
  *
  * The parameter ordered tells if vars is ordered by increasing name =
  * pointer.
  *
  * This method is not pure virtual: the base ThinVarDepInterface provides a
  * trivial implementation using repeated calls to is_active(). However, the
  * method is virtual , so that derived classes may provide more efficient
  * implementations exploiting properties of their specific data structures.
  * Doing so might require ordered to be true (which is not true for the
  * base class implementation). */

 virtual void map_active( const std::vector<Variable *> & vars ,
			  Vec_Index & map , const bool ordered = false ) const
 {
  if( map.size() < vars.size() )
   map.resize( vars.size() );

  auto it = map.begin();
  for( auto var : vars ) {
   Index i = is_active( var );
   if( i >= get_num_active_var() )
    throw( std::invalid_argument( "not an active Variable" ) );
   *(it++) = i;
   }   
  }

/*--------------------------------------------------------------------------*/
 /// get a pointer to the i-th "active" Variable
 /** Pure virtual method to get a pointer to the i-th Variable that is
  * "active" for this ThinVarDepInterface, where i is between 0 and n =
  * get_num_active_var() - 1. For the i-th active Variable, i is said to be
  * the index of the Variable in the ThinVarDepInterface.
  *
  * The order in which indices are associated with Variables is determined by
  * the order of the addresses of the Variables. If two active Variables v1
  * and v2 have addresses a1 and a2, respectively, such that a1 < a2 then the
  * index of v1 in the ThinVarDepInterface is less than the index of v2.
  *
  * Note that the base ThinVarDepInterface class makes no provisions about
  * how the set of "active" Variables is stored in order to leave more
  * freedom to derived classes to implement it in specialized ways. */

 virtual Variable *get_active_var( const Index i ) const = 0;

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

 virtual v_const_iterator * v_begin( void ) const = 0;

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

 virtual v_const_iterator * v_end( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// get an iterator for scanning the "active" Variable
 /** This method only converts a pointer to a v_iterator to the iterator and
  * returns the latter. However it is virtual, so that derived classes may
  * redefine it if needed. */

 virtual iterator begin( void ) { return( iterator( v_begin() ) ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a const iterator for scanning the "active" Variable
 /** Const version of end(), see the comments there. */

 virtual const_iterator begin( void ) const {
  return( const_iterator( v_begin() ) );
  }

/*--------------------------------------------------------------------------*/
 /// get an iterator for the end of the "active" Variable
 /** This method only converts a pointer to a v_iterator to the iterator and
  * returns the latter. However it is virtual, so that derived classes may
  * redefine it if needed. */

 virtual iterator end( void ) { return( iterator( v_end() ) ); }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// get a const iterator for for the end of the "active" Variable
 /** Const version of end(), see the comments there. */

 virtual const_iterator end( void ) const {
  return( const_iterator( v_end() ) );
  }

/*@} -----------------------------------------------------------------------*/
/*------------ METHODS FOR CHANGING THE SET OF "ACTIVE" Variable -----------*/
/*--------------------------------------------------------------------------*/
/** @name Methods for changing the set of "active" Variable
 *  @{ */

 /// removes the given Variable from the "active" ones
 /** Pure virtual method that deletes the given Variable from the set of
  * "active" ones. The base ThinVarDepInterface class makes no provisions
  * about how this is done in order to leave more freedom to derived classes
  * to implement it in specialized ways.
  *
  * The parameter issueMod decides if and how the a Modification (whose
  * specific type depends on the :ThinVarDepInterface at hand) is issued, as
  * described in Observer::make_par().
  *
  * Usually, the Modification should be thrown. A relevant exception is the
  * case in which the method is called while destroying the (dynamic)
  * Variable. Indeed, in such a case the affected Variable don't really need
  * to know that it are no longer active on this ThinVarDepInterface (as it
  * is to be destroyed anyway). Note that this may leave the Variable in an
  * inconsistent state, whereby the Variable still counts this
  * ThinVarDepInterface among the ones it is active in, while the
  * ThinVarDepInterface does not. However, since the Variable is deleted (it
  * may remain "alive" only into a Modification object waiting to be
  * processed by some Solver), the overall Block is in a consistent state. */

 virtual void remove_variable( Variable * var ,
			       c_ModParam issueMod = eModBlck ) = 0;

/*--------------------------------------------------------------------------*/
 /// removes the given set of Variable from the set of "active" ones
 /** Pure virtual method that deletes the given set (vector) of Variable from
  * the set of active Variable in this ThinVarDepInterface. The base
  * ThinVarDepInterface class makes no provisions about how this is done in
  * order to leave more freedom to derived classes to implement it in
  * specialized ways. As the the && tells, vars is potentially "consumed" by
  * the constructor and its resources become property of the
  * ThinVarDepInterface object (which may e.g. immediately dispatch them to
  * the Modification object generated by this operation).
  *
  * The parameter ordered tells whether the vector is ordered by the name (the
  * memory address) of the Variable.
  *
  * This is basically equivalent to invoking remove_variable() on each
  * Variable in vars, except that only one Modification is issued for all
  * the removals, as opposed to one for each Variable (which is likely to
  * be more efficient). If a Modification is actually issued is controlled
  * by the parameter issueMod, with the usual format; see the comments to
  * remove_variable(). */

 virtual void remove_variables( std::vector<Variable *> && vars ,
				const bool ordered = false ,
				c_ModParam issueMod = eModBlck ) = 0;

/*@}------------------------------------------------------------------------*/
/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

//  protected:

/*--------------------------------------------------------------------------*/
/*-------------------------- PROTECTED METHODS -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @name Protected methods for printing and serializing
    @{ */

/*--------------------------------------------------------------------------*/
/*--------------------------- PROTECTED FIELDS  ----------------------------*/
/*--------------------------------------------------------------------------*/

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
