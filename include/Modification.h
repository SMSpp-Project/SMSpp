/*--------------------------------------------------------------------------*/
/*------------------------ File Modification.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the *abstract* class Modification, basically only the top
 * of the hierarchy of (small) objects describing the different changes that
 * can occur to the various pieces of a SMS++ model, in the classes [derived
 * from] Block, Variable, Constraint, Objective. Modification primarily
 * serve to ensure any "interested" Solver is made aware of the changes and
 * can efficiently react to them.
 *
 * However, Modification also have another crucial role: those that are
 * issued when the "abstract representation" of a :Block is modified are
 * used by the :Block to ensure that the "physical representation" of the 
 * :Block (if any) is kept in synch with the abstract one. This is why the
 * derived class AModification is also defined, which has ways to "tell" a
 * :Block whether the changes have still to be applied to its "physical
 * representation", or this has already be done.
 *
 * Some changes in a Block may trigger more than one Modification. For
 * instance, when a (dynamic) Variable is removed from a Block [see
 * Block::remove_dynamic_variables()], it is (unless otherwise stated) also
 * removed from every "stuff" (Constraint, Objective, Function, ...) it is
 * active in. It may be helpful for a Solver (or a Block, if this is an
 * "abstract Modification" that still has to be used to ensure that its
 * "physical representation" of the is kept in synch with the abstract one)
 * to know beforehand that some Modification are "logically related". This
 * is why the GroupModification class is also defined, which is simply a
 * container that holds a list of std::shared_ptr< Modification >. This cannot
 * complicate (too much) the handling of Modification by a Solver (or Block),
 * because if the Solver (Block) is not "interested" in the "logical relation"
 * between the individual Modification in the GroupModification it can simply
 * process them sequentially as if they were individual ones. Conversely,
 * having a GroupModification may in principle considerably simplify the
 * handling.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Kostas Tavlaridis-Gyparakis \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __Modification
 #define __Modification
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <iostream>
#include <list>
#include <memory>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

///< namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it {

 class Block;            // forward definition of Block

 class Variable;         // forward definition of Variable

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Modification_CLASSES Classes in Modification.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS Modification -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class for any possible modification that can happen to a Block
/** The class Modification is the top of the hierarchy of (small) objects
 * describing the different changes that can occur to the various pieces of
 * a SMS++ model, in the classes [derived from] Block, Variable, Constraint,
 * Objective.
 *
 * Modification objects primarily serve to ensure any "interested" Solver is
 * made aware of the changes and can efficiently react to them. In fact,
 * :Modification objects are passed "up" in the Block hierarchy, i.e., from
 * each Block to its ancestors, to ensure that all Solver attached to any
 * Block that can be affected by the Modification get the information. Since
 * the Modification has to reach an unknown number of Solver, smart pointers
 * to the Modification will be passed, so that each solver can individually
 * (implicitly) delete the smart pointer, so that the :Modification object
 * will ultimately be deleted when the last Solver is done with that. Thus,
 * a Modification would *in principle* not be needed if there are no Solver
 * attached to any of the relevant Blocks; this is why Block provides the
 * method anyone_there() to check if any Solver is "listening" to the
 * Modification, and potentially avoid to even generate it if none is.
 *
 * However, the fact that no Solver is "listening" does not necessarily imply
 * that a Modification must not be issued; the reason is that there are,
 * conceptually, *two* different kinds of Modification.
 *
 * The first kind directly acts on the "physical representation" (PR) of a
 * Block. Such a "physical Modification" (PMod) is typically issued when a
 * method of the specialized interface of a specific derived class from Block
 * is called. The PMod indicates how the PR changes, and this is all that a
 * *specialized* Solver for that *specific kind of Block* has to know to
 * update itself. However, any non-specialized solver, which rater relies on
 * the "abstract representation" (AR) of the Block, has no way of properly
 * reacting to such a Modification. Yet, when the PR is modified, the AR must
 * change accordingly; this means that, besides the PMod, one or more
 * Modification pertaining to the AR (which we will therefore denote by AMod)
 * will also be issued. For instance, consider an hypothetical KnapsackBlock
 * that represents a knapsack problem and has an array that stores the
 * weights of the items. In the AR, these weights may appear as coefficients
 * of a linear function in a constraint (or in different ways, if for
 * instance a dynamic programming formulation is used). When the user changes
 * the weight of an item in the KnapsackBlock, this produces a specific PMod.
 * To keep the AR and the PR synchronized, the KnapsackBlock would have to
 * update (say) that linear constraint where this weight appears as a
 * coefficient, which would then produce an AMod. This is, of course, if the
 * AR has already been generated, otherwise there is simply nothing to modify.
 *
 * All this means that there can be more than one Modification "in flight" at
 * any point in time that is actually related to a single change in a Block.
 * This requires that any Solver be capable of understanding if a Modification
 * is a "copy" of some other Modification that has already been handled, and
 * either discard it outright, or at least be sure that the handling does not
 * leave the Block in an inconsistent state (say, the same Variable is removed
 * twice ...) and, hopefully, that no excessive cost is incurred for
 * re-handling what is basically the same Modification. This should not be a
 * problem because, basically, a specialized Solver will use the PR, and
 * therefore only consider PMod, while a general-purpose Solver will use the
 * AR, and therefore only consider AMod.
 *
 * A relevant consequence of this, however, is that if a Solver receives a
 * Modification that it does not "understand", it may be allowed to ignore
 * it. This is in particular true for a general-purpose Solver using the AR,
 * which can assume that any "unknown" Modification is a PMod, and that one
 * or more AMod is still in (or has already been extracted from) the queue
 * that describe the same changes.
 *
 * However, a general-purpose Solver using the AR may want to directly modify
 * the AR of a Block. When the Block receives an AMod (associated with the
 * AR), it must update its PR; hence, it also has to issue one (or more) PMod
 * representing the same change. In the previous example of the KnapsackBlock,
 * a general-purpose Solver may change the coefficient in the linear
 * constraint; the KnapsackBlock should then modify its PR accordingly, and
 * issue the corresponding PMod.
 *
 * As a consequence, in general an AMod must be constructed and passed to the
 * Block whenever the AR changes, even if there is no Solver "listening" to
 * it (either directly or to any ancestor), because the Block has anyway to
 * update its PR; of course, if there is no Solver listening then no PMod
 * will be issued (but the PR will be changed), and the original AMod will
 * not be passed up.
 *
 * However, this gives rise to an issue. Assume a :Block, say our fictional
 * KnapsackBlock, has a method in its specific interface to change the weight
 * of an item. This method must modify both the PR and the AR of the :Block
 * (if the latter is constructed), which means that both a PMod and an AMod
 * will be issued. However, when the AMod will reaches the :Block, the latter
 * should in principle change its PR accordingly; this is wasted work, as the
 * PR is already in synch with the AR, and may give rise to issues in cases
 * when multiple AMod/PMod are issued as a consequence of a single change.
 * This is why Modification has the method concerns_Block(). This method is
 * intended to return true if the Modification "has not been seen by the
 * Block already", i.e., it has not been issued by the :Block itself while
 * modifying its PR. When a Modification is issued by "someone other than the
 * :Block" (say, a Solver), and therefore the :Block is not already aware of
 * the corresponding changes, concerns_Block() should return true. If,
 * instead, a Modification is issued by the :Block, then concerns_Block()
 * should return false. Of course, the object actually issuing the
 * Modification (Variable, Constraint, Objective, Function, ...) may not be
 * aware of which of the two cases is, which is why all methods that result
 * in changes that issue an AMod must have a parameter that controls that
 * return value. Note that the base Modification class always returns false:
 * this is typically the right behavior for PMod, which are directly issued
 * by the :Block and that therefore can be of no interest for it. The derived
 * class AModification is defined which just allows to set the value returned
 * by the method: typically, AMods should derive from AModification.
 *
 * Note that a Block may be unwilling to accept some AMod, e.g. because the
 * corresponding PMod may be too complex to handle. All :Block should clearly
 * define the set of AMod that they handle, and the user should be careful to
 * avoid any change that results in an unsupported AMod, for that would
 * typically result in an exception being thrown.
 *
 * Complex cases may occur in which a Solver uses partly the AR and partly the
 * PR of a block; in this case, the Solver will have to handle both AMod and
 * PMod. However, such a Solver will by necessity be specific for a given
 * Block, and therefore it will know exactly how the Block reacts to changes;
 * this should make it possible to handle both types of Modification avoiding
 * problems with "duplicates". This could actually happen frequently enough,
 * because some kinds of Modification may be seen as being both a AMod and a
 * PMod. These are in particular those concerning a Variable, since the
 * Variable are a necessary part of the Block, and hence in some sense
 * belonging both to the AR and to the PR.
 *
 * All this having been said, a number of general remarks on how Modification
 * should be organized, issued and reacted to are in order:
 *
 * 1. A Modification typically says *what* has changed but not (necessarily)
 *    *how*. For instance, a Modification changing, say, the RHS of a
 *    :Constraint, would in general indicate "the RHS has changed" but *not*
 *    report the new value of the RHS. When a Solver (and/or a Block managing
 *    this AMod to keep its PR in synch) handles this Modification, it will
 *    have to access the :Constraint to retrieve the *current* value of the
 *    RHS from there. Note that likely the Solver / Block is keeping some
 *    information "equivalent" to the value that the (say) RHS had before
 *    the Modification happened, and can therefore "see the difference"
 *    (e.g., check if the RHS has in fact been set to exactly the same value
 *    that is had before, so that the Modification is actually "void").
 *
 * 2. As a consequence of the previous point,
 *
 *       WHEN THE MODIFICATION IS ISSUED, THE CHANGE MUST BE DONE ALREADY
 *
 *    In fact, at the moment in which add_Modification() is called, control
 *    is passed to the Observer / Block (and ultimately, possibly, Solver)
 *    that receives it. This object may *immediately* access the changed
 *    object to retrieve its new state, and therefore the state at the
 *    moment in which the Modification is "physically issued" (i.e.,
 *    add_Modification() is called) should be "ready".
 *
 * 3. However,
 *
 *        MODIFICATION ARE "ASYNCHRONOUS" OBJECTS
 *
 *    Indeed, the rationale of Modification is that they be processed in a
 *    "lazy" way, possibly "a long time after they have been issued". The
 *    result is that
 *
 *        THE STATE OF THE Block WHEN THE Modification IS PROCESSED CAN
 *        BE VERY DIFFERENT FROM THAT WHEN THE Modification WAS ISSUED
 *
 *    In the same example as before, it may well happen that the RHS of the
 *    same :Constraint is changed several time before a Solver is asked to
 *    solve the Block again. This means that in the list of Modification of
 *    the Solver there might be several Modification; yet, at the moment in
 *    which the *first* one is processed, the RHS is set to the *current*
 *    value, i.e., that which in fact correspond to the *last* Modification.
 *    The net result is that all the Modification apart from the first "do
 *    nothing". It is the Solver's responsibility to ensure that this does
 *    not create problems, and hopefully to recognise this and basically
 *    "ignoring" all the "void" Modification with low computational cost.
 *
 * 4. Ensuring that a Modification issued when the state of the Block was
 *    different can still be meaningfully processed by using its current
 *    state can be delicate. This is in particular true when dealing with
 *    *dynamic* Variable / Constraint of a Block. The issue is that, say, a
 *    dynamic Variable may be created, then several dynamic Constraint may
 *    be created where the Variable is active, which issue Modification.
 *    However, before the first of these Modification is processed by a
 *    Solver, the Variable and/or Constraint may be deleted. It must still
 *    be possible for the Solver to process the first Modification, which
 *    implies that the Variable / Constraint are *not* actually destroyed
 *    until the last of the Modification concerning them (the one that
 *    "says" they are destroyed) is processed. This is actually made possible
 *    by the fact that remove_dynamic_* stores the to-be-destroyed objects
 *    within the Modification that are issued (if any), which means that
 *    the Variable / Constraint are only destroyed when the Modification
 *    indicating that they are destroyed if no longer "in flight" in any
 *    Solver. However, for this to work it is necessary to be careful about
 *    all other objects that may reference that Variable / Constrain. That
 *    is, before the Variable is destroyed it must be un-registered from
 *    all ThinVarDepInterface (Constraint, Objective, Function, ...) in
 *    which it had been active; thus, the Modification related to these
 *    un-registering must happen *before* that related to the Variable
 *    deletion Modification. Similarly, if a dynamic ThinVarDepInterface
 *    (Constraint, Objective, Function, ...) is deleted, it has to be
 *    un-registered from all its "active" Variable before this happens. This
 *    means that the "zombie" objects living in the Modification waiting to
 *    be destroyed are not referenced in any "living" object in the Block;
 *    hence, when accessing the Block (and themselves) to process the
 *    Modification, nothing that is accessed is in an inconsistent state.
 *
 * 5. A related case in which handling Modification requires care is that
 *    of the set of "active" Variable in a ThinVarDepInterface (Constraint,
 *    Objective, Function, ...), which is in principle a "dynamic set" even
 *    if the Variable and the ThinVarDepInterface possibly are themselves
 *    "static". One case that has to be taken into account is when one [or
 *    more] Variable[s] is[are] removed from the active set, see
 *    ThinVarDepInterface::remove_variable[s](). When the corresponding
 *    Modification is processed, the current state of the ThinVarDepInterface
 *    may accessed: in that moment, however, *the Variable may be active in
 *    the ThinVarDepInterface*, which is not what one would expect in view
 *    of the principle stated in point 2. Yet, what it may have happened is
 *    that, after having been removed, the Variable has been added back.
 *    The corresponding Modification will then be in the list of the Solver,
 *    but *after* that corresponding to it being removed. This should not
 *    be a problem, but it has to be explicitly handled. That is, the
 *    Solver may not "get confused" (say, throw exception) if the Variable
 *    that have been purportedly eliminated is still (again) there. The
 *    same goes for a Variable that is added and later eliminated.
 *
 * 6. Note, however, that the "asynchronicity" of Modification *only applies
 *    to Solver*. An AMod triggered by a change in an element of the AR
 *    *immediately* reaches the :Block. Thus, the processing of AMod from
 *    :Block to keep the PR in synch with the AR can assume that the
 *    Modification has just been issued, and therefore that the state of the
 *    :Block is the one implied by the Modification (say, if a Variable has
 *    just been removed from the set of those active in a
 *    ThinVarDepInterface, it should not be found there). Note that there
 *    could be an exception to this rule: a Modification could be contained
 *    in a GroupModification, that only "reaches" the :Block once the channel
 *    is closed. This would provide scope for some delay between the moment
 *    the Modification is issued and the one it reaches the :Block. However,
 *    a proper implementation of Block::add_Modification() [see] implies that
 *    the very :Block to which the elements being modified belong gets the
 *    Modification *before* it can be packed into a GroupModification. This is
 *    precisely the :Block that has to react to AMod, and therefore precisely
 *    the :Block that may contain complex logic to do that; the assumption
 *    that AMod are received *immediately* may considerably simplify that
 *    logic.
 *
 * 7. All the above conceptually requires that
 *
 *        Modification MUST BE PROCESSED IN THE ORDER THEY ARE ISSUED
 *
 *    The lists of Modification in Solver should be strictly accessed FIFO.
 *    For GroupModification, which is actually a tree, this translates to
 *    depth-first left-to-right transversal: 1) the Modification in the
 *    STL container are accessed in the natural order, and 2) once a
 *    sub-GroupModification of a GroupModification is entered, no other
 *    Modification of the original GroupModification is processed until all
 *    those of the sub-GroupModification have been. Actually, there is
 *    nothing preventing a Solver to access the Modification in any order it
 *    chooses, and it is reasonable than not doing FIFO could lead to
 *    performance improvements in some cases (say, bunch together all
 *    Modification pertaining the same object and only perform the last of
 *    them if it "overrides" all the previous ones, cf. the example in
 *    point 1.). However
 *
 *        A Solver NOT PROCEEDING FIFO DOES SO AT ITS OWN RISK
 *
 *    Changing the "natural order" may make it difficult, if not impossible,
 *    to deal with the "complex" cases (cf. points 4. and 5.), so any logic
 *    doing that will have to be very carefully considered.
 *
 * The only common aspects to all Modifications are that:
 *
 * - they can be printed;
 *
 * - they have to report if the :Block should process them;
 *
 * - they can provide a pointer to the Block they were originated from.
 */

class Modification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 Modification( void ) = default;     ///< constructor: does nothing

 virtual ~Modification() = default;  ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
 /// returns the Block this Modification was originated from
 /** Each Modification is ultimately originated from within some Block; since
  * the Modification has to contain information (typically, a pointer) capable
  * of uniquely identifying the part of the Block it refers to, it can also
  * uniquely identify the Block itself. However, according to the specific
  * type of Modification (i.e., the part of the Block it refers to), it may be
  * necessary to jump some hops before being able to reconstruct what the
  * Block exactly was. This pure virtual method provides a uniform interface
  * so that this can be immediately asked to any Modification, whatever its
  * type. */

 [[nodiscard]] virtual Block * get_Block( void ) const = 0;

/*--------------------------------------------------------------------------*/
 /// returns true if the :Block needs to process this Modification
 /** This method must return true if the Modification "has not been seen by
  * the Block already", i.e., it has not been issued by the :Block itself
  * while modifying its PR. This typically means that the Modification has
  * been issued by "someone other than the :Block" (say, a Solver), an
  * therefore the :Block is not already aware of the corresponding changes.
  * If, instead, the Modification has been issued by the :Block, then
  * concerns_Block() should return false. The base Modification class always
  * returns false: this is the right behavior for PMod, which are directly
  * issued by the :Block and that therefore can be of no interest for it. */

 [[nodiscard]] virtual bool concerns_Block( void ) const { return( false ); }

/*--------------------------------------------------------------------------*/
 /// method to set the value returned by concerns_Block( void )
 /** This method sets the value returned by concerns_Block( void ); in the
  * base Modification class, thought to represent "physical" Modification,
  * this cannot be done (i.e., it cannot be set to true). */

 virtual void concerns_Block( bool cB ) {
  if( cB )
   throw( std::invalid_argument(
			    "physical Modification cannot concerns_Block" ) );
  }

/*--------------------------------------------------------------------------*/
 /// friend operator<<(), dispatching to virtual protected print()
 /** Not really a method, but a friend operator<<() that just dispatches the
  * ostream to the protected *pure* virtual method print(). This way the
  * operator<<() is defined for each Modification, but its behavior must be
  * customized by derived classes (since the base class has nothing to
  * print). */

 friend std::ostream &
 operator<<( std::ostream & out, const Modification & b ) {
  b.print( out );
  return( out );
  }

/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// *pure virtual* method for allowing any Modification to print itself
 /** *pure virtual* method intended to provide support for Modifications to
  * print themselves out in human-readable form. The base Modification class
  * does not have anything to print, and this method is precisely what makes
  * it an abstract base class. */

 virtual void print( std::ostream & output ) const = 0;

/*--------------------------------------------------------------------------*/

 };  // end( class( Modification ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS AModification -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// base class for "Abstract" Modification
/** The class AModification derives from Modification and extends it by just
 * allows to set the value returned by the concerns_Block() method.
 * Typically, AMods should derive from AModification. */

class AModification : public Modification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
 /// constructor: takes the value to be returned by concerns_Block()
 /** Constructor of the class; it takes and sets the value to be returned by
  * concerns_Block() (true by default). */

 explicit AModification( bool cB = true ) : f_concerns_Block( cB ) {}

 ~AModification() override = default;  ///< destructor: does nothing

/*--------------------------------------------------------------------------*/
 /// returns the value stored in the f_concerns_Block field

 [[nodiscard]] bool concerns_Block( void ) const override {
  return( f_concerns_Block );
  }

/*--------------------------------------------------------------------------*/
 /// method to set the value returned by concerns_Block( void )
 /** This method allows to set the value returned by concerns_Block( void ),
  *
  *      WHICH IS A VERY DELICATE OPERATION THAT NEEDS TO BE DONE WITH
  *      EXTREME CARE. THERE IS NO REASON (I SEE NOW) WHY A :Block SHOULD
  *      CHANGE THE VALUE FROM false TO true (THIS CAN BE DONE IN Block WHEN
  *      HANDLING CHANNELS). THE ONLY PLACE IN WHICH THE VALUE SHOULD BE
  *      CHANGED FROM true TO false IS INSIDE add_Modification() OF THE
  *      :Block ISSUING THE AModification, AFTER IT HAS DONE WHAT IT
  *      MUST TO ADJUST THE "PHYSICAL" REPRESENTATION AFTER A CHANGE IN THE
  *      "ABSTRACT" ONE, AND PRIOR TO DISPATCHING THE AModification TO THE
  *      Solver AND THE FATHER Block.
  *
  * I see no reasonable way in which this condition can be enforced, hence
  * there is no attempt to enforce it. But concerns_Block( bool ) is a very
  * rare method allowing to change a Modification after it is issued, which
  * should almost never happen, and therefore it should be managed with
  * extreme care. */

 void concerns_Block( bool cB ) override { f_concerns_Block = cB; }

/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

 protected:

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 bool f_concerns_Block;       ///< the returned value

/*--------------------------------------------------------------------------*/

 };  // end( class( AModification ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS NModification -----------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// a "Nuclear" Modification indicating that everything changes
/** The class NModification derives from Modification and basically does
 * nothing to extend it. However, the different class is used to indicate that
 * the Modification of this type are "high priority" ones. While in general
 * Modification should be treated in a rigidly sequential order, a
 * NModification should be given priority to all other ones. This is because
 * it typically indicates that "too much has changed" in some object, so that
 * it is either not convenient, or not even possible, to process the
 * corresponding Modification prior to this one. */

class NModification : public Modification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
 /// constructor: does nothing

 NModification() : Modification() {}

 ~NModification() override = default;  ///< destructor: does nothing

/*--------------------------------------------------------------------------*/

 };  // end( class( NModification ) )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS NBModification ---------------------------*/
/*--------------------------------------------------------------------------*/
/// derived class from NModification for "nuclear Block Modification"
/** Derived class from NModification to describe the "nuclear" Modification
 * to a Block, i.e., everything is changed (say, load() is called). This
 * implies that
  *
  *    ALL EXISTING Modification RELATED TO THIS Block SHOULD BE
  *    IMMEDIATELY DELETED WITHOUT BEING ACTED UPON FIRST
  *
  * This is due to the fact that the, despite being the same physical object
  * with the same memory address, the Block is now in fact a "completely
  * different Block". In particular, handling of Modification can usually rely
  * on the fact that "some aspects" of the Block cannot change, such as the
  * number and type of static Variable and Constraint. However, after a
  * NBModification, this is no longer true: a(n abstract) Modification
  * (pointing to some static Variable / Constraint) may contain invalid
  * pointers to objects that have meanwhile ceased to exist. Therefore, the
  * only safe recourse is to immediately delete them all. This is, anyway,
  * also the cheapest thing to do: there is no point in wasting time on these
  * Modification, since they refer to the status of a Block that is fact no
  * longer exist.
  * Doing all this may be an issue for Solver of "complex" Block where the
  * re-loading of a sub-Block can be faced without a complete reset of all
  * the state of the Solver, but it is in fact quite simple at least for
  * Solver of "leaf" Block that have no sub-block: see the comments to
  * Solver::add_Modification. 
  *
  * Note that NBModification is a "physical" Modification, since it is
  * assumed that the whole of the Block (hence, both the "physical" and the
  * "abstract" representation) change at once. */

class NBModification : public NModification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// constructor: takes the Block

 explicit NBModification( Block * fblock ) : NModification() ,
  f_Block( fblock ) {}

 ~NBModification() override = default;   ///< destructor, does nothing

/*--------------------------------------------------------------------------*/
 /// returns the Block this Modification was originated from

 [[nodiscard]] Block * get_Block( void ) const override { return( f_Block ); }

/*--------------------- PROTECTED PART OF THE CLASS ------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// print the NBModification

 void print( std::ostream & output ) const override {
  output << "NBModification on Block [" << &f_Block << "]" << std::endl;
  }

/*--------------------- PROTECTED FIELDS OF THE CLASS ----------------------*/

 Block * f_Block;  ///< reference to the block to which the Modification refers

/*--------------------------------------------------------------------------*/

 };  // end( class( NBModification ) )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS GroupModification ---------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// concrete class for "a group of logically related Modification"
/** The class GroupModification derives from AModification and provides a STL
 * container of shared pointers to Modification which can be used to
 * implement in a simple way the concept of "a set of Modification logically
 * related to each other". Note that since GroupModification is a
 * Modification, there is nothing preventing some of the sub-Modification of
 * a GroupModification to be GroupModification themselves, allowing to have
 * arbitrarily nested "logically related Modification". Indeed,
 * GroupModification also has a "father" pointer that simplifies traversals
 * of the tree of GroupModification. Note that this is a raw pointer rather
 * than a smart pointer, as it is not supposed to be used for anything else
 * than traversing the data structure.
 *
 * A GroupModification derives from AModification, which means that in general
 * it can be an "abstract" Modification. However, its concerns_Block() value
 * is automatically set as the logical or of the concerns_Block() values of
 * its sub-Modifications (recursively), i.e., it is true if at least one of
 * them is true. */

class GroupModification : public AModification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/
 /// constructor: takes the "father" GroupModification, if any
 /** Constructor of the class; takes (optionally) the "father" of the
  * GroupModification. The concerns_Block() value is initialized to false,
  * and (almost) automatically set to true if any of the Modification
  * inserted in the GroupModification has concerns_Block() == true. */

 explicit GroupModification( GroupModification * father = nullptr )
  : AModification( false ) , f_father( father ) {}

 /// destructor: does nothing
 ~GroupModification() override = default;

/*--------------------------------------------------------------------------*/
 /// returns the Block this Modification was originated from
 /** One expects that all the Modification in the same GroupModification
  * refer to the same Block, hence GroupModification returns the Block of
  * the first one of them (if any, nullptr otherwise). */

 [[nodiscard]] Block * get_Block( void ) const override {
  return( v_sub_Modifications.empty() ?
	  nullptr : v_sub_Modifications.front()->get_Block() );
  }

/*--------------------------------------------------------------------------*/
 /// returns a const reference to the list of sub-Modification

 [[nodiscard]] const std::list< std::shared_ptr< Modification > > &
           sub_Modifications( void ) const { return( v_sub_Modifications ); }

/*--------------------------------------------------------------------------*/
 /// returns (a pointer to) the "father" GroupModification (may be nullptr)

 [[nodiscard]] GroupModification * father( void ) const {
  return( f_father );
  }

/*------------------------ FRIENDS OF THE CLASS ----------------------------*/
 /** Opening, closing, nesting and un-nesting channels, and sending a
  * Modification to a non-default channel, requires modifying a
  * GroupModification (say, add a new Modification to list of
  * sub-Modification). This is something that "most entities" interacting
  * with GroupModification should not be able to do. The methods allowing
  * doing this are therefore made protected, but they have to be called by
  * Block, rather than a derived class. Hence, Block is made friend. */

 friend Block;

/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/
 /// printing the GroupModification
 /** Method for printing the GroupModification, which basically means printing
  * all its sub-Modification. */

 void print( std::ostream & output ) const override {
  output << "GroupModification[";
  if( concerns_Block() )
   output << "t]:";
  else
   output << "f]:";
  output << std::endl;
  for( const auto & mod : v_sub_Modifications )
   output << *mod << std::endl;
 }

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// add a new Modification to the v_sub_Modifications list

 void add( const std::shared_ptr< Modification > & mod ) {
  if( mod->concerns_Block() )  // if the sub-Modification concerns the Block
   concerns_Block( true );     // then the whole GroupModification does
  v_sub_Modifications.push_back( mod );
 }

/*--------------------------------------------------------------------------*/
 /// sets (the pointer to) the "father" GroupModification

 void set_father( GroupModification * father ) { f_father = father; }

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 std::list< std::shared_ptr< Modification > > v_sub_Modifications;
 ///< the std::list of std::shared_ptr< Modification >

 GroupModification * f_father;
 ///< pointer to the "father" GroupModification

/*--------------------------------------------------------------------------*/

 };  // end( class( GroupModification ) )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS VariableGroupMod --------------------------*/
/*--------------------------------------------------------------------------*/
/*--------------------------- GENERAL NOTES --------------------------------*/
/*--------------------------------------------------------------------------*/
/// GroupModification for all the changes related to adding/deleting Variable
/** The Constraint-based "abstract" representation of SMS++ has a significant
 * issue in that it is not well-suited to deal with cases (that are bound to
 * be rather common) where adding or deleting one or more Variable causes
 * significant changes in the Constraint and Objective. The obvious example
 * is that of a MILP, where this entails "just" the addition or removal of a
 * column from the coefficient matrix, an operation that is typically handled
 * very efficiently by MILP solvers. Conversely, in SMS++ this produces a
 * possibly very long list of separate Modification for all the rows the
 * Variable has a nonzero coefficient in. Besides the effort and memory for
 * keeping all them, handling the coefficient modifications one by one can
 * well be much less efficient than the unique column operation.
 *
 * Recognising this issue, the class VariableGroupMod derives from
 * GroupModification and aims at grouping together all Modification
 * corresponding to changes brought about by "significant changes in one or
 * more Variable", typically adding/removing them. :Block may then
 * appropriately issue VariableGroupMod so that :Solver may react to them
 * more efficiently.
 *
 * In general, one should not expect all Solver to be able to recognise a
 * VariableGroupMod and deal with it explicitly. This means that the
 * VariableGroupMod should still contain all the standard Modification for
 * the individual Constraint/Objective, so that is the Solver goes the easy
 * way of unpacking the VariableGroupMod and process the individual changes
 * one by one, it can. However, just having to create the individual
 * Modification can incur a significant cost that one may rather avoid if
 * possible. It could therefore be possible for individual :Block to have
 * some way to allow the individual Modification to be dispensed with in the
 * knowledge that the enclosing  VariableGroupMod is enough to convey all the
 * necessary information to all the :Solver being currently employed. This
 * should be a non-default behaviour for which currently no general mechanism
 * is provided, so it necessarily needs be :Block-specific. It is not clear
 * if this is general enough to make it effective or if a general mechanism
 * will have to be provided for it to work as intended, but for now we keep
 * it minimal. */

class VariableGroupMod : public GroupModification {

/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/

 public:

/*---------------------------- PUBLIC TYPES --------------------------------*/

 /// Definition of the possibles type of VariableGroupMod
 /** This enum specifies what happened to the Variable that triggered the
  * cascade of Modification contained inside the VariableGroupMod. Currently
  * only two values are supported for the obvious added and deleted cases,
  * but the "type" field is kept "int" and the enum is provided to allow for
  * other cases to be considered in the future. */
 enum variable_group_mod_type {
  VariableAdded,          ///< the Variable has been added
  VariableDeleted,        ///< the Variable has been deleted
  VariableGroupModLastParam
  ///< First allowed parameter value for derived classes
  /**< Convenience value for easily allow derived classes to extend
   * the set of types of VariableGroupMod. */
 };

/*--------------------- PUBLIC METHODS OF THE CLASS ------------------------*/

 /// constructor: takes the affected vars, the type and the "father"
 /** Constructor of the class. Takes the set of affected Variable under the
  * form of a std::vector< Variable * >, and the type of changes occurred to
  * them with the meaning set by variable_group_mod_type. Also, it optionally
  * takes the "father" of the GroupModification. */

 explicit VariableGroupMod( std::vector< Variable * > && vars , int type ,
			    GroupModification * father = nullptr )
  : GroupModification( father ) , v_vars( std::move( vars ) ) ,
    f_type( type ) {}

 /// destructor: does nothing
 ~VariableGroupMod() override = default;

/*--------------------------------------------------------------------------*/
 /// accessor to the vector of pointers to affected Variable

 [[nodiscard]] const std::vector< Variable * > & vars( void ) const {
  return( v_vars );
  }

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
 /// accessor to the type of Variable change

 [[nodiscard]] int type( void ) const { return( f_type ); }

/*-------------------- PROTECTED PART OF THE CLASS -------------------------*/

 protected:

/*-------------------------- PROTECTED METHODS -----------------------------*/

 /// printing the VariableGroupMod
 /** Method for printing the VariableGroupMod, which basically means printing
  * all its sub-Modification. */

 void print( std::ostream & output ) const override {
  output << "VariableGroupMod[";
  if( concerns_Block() )
   output << "t]:";
  else
   output << "f]:";
  if( f_type == VariableAdded )
   output << " adding ";
  else
  output << " deleting ";
  output << v_vars.size() << " Variables" << std::endl;
  for( const auto & mod : v_sub_Modifications )
   output << *mod << std::endl;
 }

/*--------------------------- PROTECTED FIELDS -----------------------------*/

 int f_type;  ///< type of Variable change

 std::vector< Variable * > v_vars;
              ///< vector of pointers to affected Variable

/*--------------------------------------------------------------------------*/

 };  // end( class( VariableGroupMod ) )

/** @} end( group( Modification_CLASSES ) ) */
/*--------------------------------------------------------------------------*/
/*--------------------- Modification-RELATED TYPES -------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup Modification_TYPES Modification-related types.
 *  @{ */

using p_Mod = Modification *;  ///< a pointer to Modification

using c_p_Mod = Modification * const;   ///< a const pointer to Modification

using Vec_p_Mod = std::vector< p_Mod >;
///< a vector of pointer to Modification

using Lst_p_Mod = std::list< p_Mod >;
///< a list of pointer to Modification

using sp_Mod = std::shared_ptr< Modification >;
///< a shared pointer to Modification

using Vec_sp_Mod = std::vector< sp_Mod >;
///< a vector of shared pointer to Modification

using Lst_sp_Mod = std::list< sp_Mod >;
///< a list of shared pointer to Modification

/*--------------------------------------------------------------------------*/
/// public type for parameters controlling issuing of Modification
/** Public type for parameters controlling how Modification are issued. It
 * is defined "at global scope" so that all methods of all relevant classes
 * can use a consistent interface. */

using ModParam = unsigned int;

using c_ModParam = const ModParam;  ///< a const ModParam

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/// public enum for ways of issuing a Modification
/** Public enum for describing if and how any method potentially issuing a
 * Modification should actually issue it. The enum is defined "at global
 * scope" so that all methods of all relevant classes can use a consistent
 * interface. */

enum amododification_type {
 eDryRun  = 0 ,  ///< don't do the change, hence issue no Modification
 eNoMod   = 1 ,  ///< do the change but issue no Modification at all
 eNoBlck  = 2 ,  ///< issue the Modification, but concerns_Block() == false
 eModBlck = 3    ///< issue the Modification, and concerns_Block() == true
 };

/*--------------------------------------------------------------------------*/

/** @} end( group( Modification_TYPES ) ) ----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#endif /* Modification.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Modification.h -------------------------*/
/*--------------------------------------------------------------------------*/
