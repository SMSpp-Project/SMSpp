# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.8.0] - 2026-10-09

### Added

- the inner Block of an easy component that has taken lambda of a
  `MasterProblemBlock` as its size Variable belongs to that master only:
  the master of another Solver does not give it its own lambda, and throws
  if it finds it sized by another master, since the column of one master
  cannot be in the rows that another one solves

- `MasterProblemBlock::aggregate_mass()`, the mass the aggregate of the
  dual master is divided by: lambda + r, i.e., that of the rows of the
  components and of the global lower bound row, or that of the level row in
  the pure level form

- in the dual `MasterProblemBlock` an easy component whose inner Block owns
  no size Variable is given lambda by `Block::set_size_variable()`, and is
  scaled by it if the Block takes it; if it does not, and
  `MasterProblemBlock::use_easy_mirrors( true )` (bit 5, +32, of the
  `DoEasyCmp` of `CreateEmptyMP()`) asks for it, the master puts in its
  place a copy of it (`AbstractBlock::mirror()`) sized by lambda, writes in
  the copy the Modification of the inner Block, and the solutions of the
  copy back into it, and puts the inner Block back, unscaled, if one of its
  Modification is one the copy cannot follow. lambda is free, and with it
  the global lower bound enters the master, when every easy component is
  scaled in one of the three ways, and fixed to 1 otherwise, as by
  default for an inner Block that neither owns nor takes a size Variable.
  `MasterProblemBlock::use_easy_size_variables( false )`, i.e., bit 4 (+16)
  of the `DoEasyCmp` of `CreateEmptyMP()`, fixes lambda to 1 and neither
  gives lambda to an inner Block nor makes copies, for the cheaper master
  without it

- `AbstractBlock::set_size_variable()`: a mirror takes a size Variable v and
  writes into itself the mirrored Block scaled by v, every row
  l <= a x <= u being l v <= a x <= u v (with a second row for a ranged
  one), the finite nonzero sides of the OneVarConstraint (whose copy keeps
  its sides 0, or is moved onto a ColVariable of its own), the bounds
  -1 / 1 of the unitary ColVariable and the value of a fixed ColVariable
  being rows, and the constant of the Objective the term
  of v; nullptr takes it away. An AbstractBlock that is not a mirror does
  not take one, its setters comparing a new value with the current one,
  which would be the scaled one; `Block::set_size_variable( nullptr )` is
  documented as taking back the Variable given, if the Block can.
  `AbstractBlock::mirror_write_duals()` writes the dual values of the copy
  into the mirrored Block

- `AbstractBlock_test` checks the sized mirror
  (`AbstractBlock::set_size_variable()`): feasibility of the scaled points,
  the rows, bounds, unitary and fixed ColVariable and the constant of the
  Objective written with the size Variable, the changes of the original
  followed, dynamic Constraint and ColVariable added and removed, the duals
  written back, and the copy unsized again

- `Block::set_owned_size_variable()`, which declares a Variable that the
  Block has created, and with which it has written its Constraint, as the
  column of its size parameter; the base `get_size_variable()` returns it

- the ModConcern tells the integrality of the Variable (`eModVarType`,
  `changes_integrality()`) from their other data (`eModVarData`: fixing,
  bounds, sign), so that the Solver of a continuous relaxation can leave the
  former out of what it reads; what a change of the state of a Variable
  means is said by the Variable itself (`Variable::state_changes()`), and
  `ColVariable` tells which of its bits is the integrality

- the ModConcern tells the sides of the Constraint (`eModCnsSide`, relaxing
  and enforcing included) from their coefficients (`eModCnsCoef`), since a
  dual solution stays feasible when only the former change
  (`changes_only_sides()`), and a physical Modification may also say what it
  changes in terms of the model; `BendersBFunction` keeps its global pool
  when only the sides of Constraint it does not handle change, the
  constants of its linearizations being computed again (`AlphaChanged`,
  `sides_changed()`), instead of sending the "nuclear" Modification

- `MasterProblemBlock` implements the trust region (`kTrustRegion`) in the
  dual MP too: the box of the multipliers s^+ / s^- is the box [ L , U ]
  intersected with { x : || x - x_bar ||_inf <= t }, Var_z is fixed to 0,
  and the MP is linear; z* is the part of the multipliers that belongs to
  the trust region, and d* the dual value of the coupling rows. In the
  primal MP z* is read from the dual values of the rows, the cuts and the
  absorbed rows of a `BendersBFunction`, rather than from the reduced costs
  of the Variable. `get_conjugate_stabilization( t )` returns D*_t( z* ),
  i.e., t || z* ||_1 under the trust region and ( t / 2 ) || z* ||_2^2
  otherwise; under the trust region the predicted decrease of the dual MP is
  - ( Sigma* + t || z* ||_1 ), that of a component < z*_k , d* > - Sigma*_k,
  and the slope given by `sensitivity_analysis()` - || z* ||_1

- the return code `kCutOff` of `Solver`, placed after `kStopIter` and hence
  in [ `kOK` , `kError` ): the Solver stopped because the cutoff given by
  `dblUpCutOff` or `dblLwCutOff` was reached, so it has something to
  report but not a certified optimal solution

- `AbstractChange`, a Change of the abstract representation of a Block (the
  objective coefficient, the integrality, the fixing and the bounds of a
  ColVariable, the sense of an Objective), each element identified by an
  AbstractPath; fixing a ColVariable that is fixed at another value unfixes
  it first, so that the Block is told of both steps

- the event `eColumnPurged` of `LagBFunction`, whose handlers are called
  before a Solution of the global pool that is no longer feasible for the
  inner Block is deleted, and may take it with
  `release_current_purged_solution()`; `restore_purged_solutions()` puts
  them back, telling the Observer with one Modification of type
  `GlobalPoolAdded`

- `Modification::is_physical()`, `may_shrink_region()` and the other readers
  of a ModConcern are also methods of the Modification itself (e.g.,
  `mod->is_physical()`), which read `changes()`; `LagBFunction` decides
  through them what to do with a Modification of the inner Block it does
  not recognise, which it used to ignore: one that may shrink the feasible
  region has the global pool checked, an abstract one that changes the
  Objective is reported as unknown, any other is ignored

- `Solution::adapt()` adapts a Solution to a Modification of its Block, and
  answers whether it is unchanged, adapted or no longer valid: it drops the
  values of the removed dynamic Variable or Constraint it holds values of
  (`drop_dynamic_values()`) and of what a physical Modification it reads
  removes (`drop_physical_values()`), a `NModification` invalidates it and a
  `GroupModification` is adapted to one sub-Modification at a time;
  `Solution::adapts()` says the kinds whose elements a Solution holds values
  of (`ColVariableSolution` the Variable, `RowConstraintSolution` the
  Constraint, `ColRowSolution` both, the base class anything). The global
  pools of `LagBFunction` and of `BendersBFunction` call it

- `Solver::concerned_by()` says the kinds of Modification a Solver reads
  (all of them in the base class), and a Block passes its Solver only those
  of a kind they read (`Modification::is_of_concern()`);
  `Block::concerned_by()` says those the Block reads itself (none in the
  base class), `Observer::concerned()` the kinds read by anyone listening
  (for a Block, the "or" over its Solver and its ancestors), and
  `anyone_there_for()`, `issue_mod()` and `issue_pmod()` with a kind tell
  whether a Modification of that kind has to be issued at all

- `Modification::changes()` says what a Modification changes and what effect
  it may have on the problem, as a `ModConcern` bit mask: the kind
  (`eModPhys`, the data or the set of the Variable, `eModVarData` and
  `eModVarSet`, the data or the set of the Constraint, `eModCnsData` and
  `eModCnsSet`, and the Objective, `eModObj`) and the possible effects
  (`eRegnShrink`, `eRegnGrow`, `eObjUp`, `eObjDown`), read through static
  methods (`is_physical()`, `lower_bound_stays_valid()`, ...); a physical
  Modification says "any effect", an abstract one "anything abstract, any
  effect", and `VariableMod`, `ConstraintMod`, `ObjectiveMod`, `BlockModAD`,
  `FunctionModVars`, `NModification` and `GroupModification` say what they
  know. `Modification_test` checks them

- with t = infinity and no finite level, the primal master of the pure level
  that `restore_initial_level_objective()` gives back is the cutting-plane
  one, whose value is a lower bound, which the level variant of the integer
  method of `BundleSolver` uses

- `MasterProblemBlock::set_local_branching( kappa )` gives the binary
  coordinates of the primal master in raw form, i.e., the integer ones with
  box [ 0 , 1 ], the local branching constraint Delta( x , x_bar ) <= kappa
  of the stabilized Benders' method of Baena, Castro and Frangioni (Manag.
  Sci. 66, 2020), which follows the stability centre, and the trust region
  then acts on the other coordinates only; `add_reverse_local_branching()`
  excludes the current region with Delta( x , x_bar ) >= kappa + 1, a
  constraint that stays when the centre moves, and
  `clear_reverse_local_branching()` removes them all. `remove_vars()` drops
  them all as well, and writes again the one around the centre

- `MetaBlockSolverConfig` and `MetaBlockConfig`: a BlockSolverConfig and a
  BlockConfig that, besides configuring the Block they are applied to with
  their own fields, carry a map from `classname()` to the
  BlockSolverConfig / BlockConfig of the descendants of that Block, `"*"`
  being that of the classnames not in the map. The map is a
  `SimpleConfiguration< std::map< std::string , Configuration * > >`,
  written after the usual fields (or as `*filename`); clearing the
  `MetaBlockSolverConfig` clears those of the map too, so that it removes
  all the Solver it has registered

- `for_each_by_classname()`, which dispatches such a map over a tree of
  Block, father-first, looking up the sub-Block of a Block only after it
  has been configured, since configuring it may change them

- `LagBFunction::set_lazy_inner_BlockSolverConfig()`: a BlockSolverConfig
  for the inner Block that the LagBFunction applies, in additive mode, the
  first time it is computed, so that an inner Block that is never computed
  (say, an easy component of a BundleSolver) gets no Solver;
  `lazy_inner_BlockSolverConfig_pending()` tells whether that has happened

- `LagBFunction::intInnrSlvr` set to -1 means that the LagBFunction has no
  inner Solver, whatever Solver its inner Block has: `compute()` then
  throws, rather than returning `kError` as it does when the inner Block
  has no Solver at all, until a BlockSolverConfig that the LagBFunction
  applies itself (that of `set_ComputeConfig()` or the lazy one) registers
  some, the first of which then becomes the inner Solver

- `MasterProblemBlock::add_easy_coupling( easy_id , j , local_i )`, the
  reverse of `drop_easy_coupling()`: the easy component gets the terms of
  its Lagrangian term on the coordinate j, already in the master, in the
  coupling row of j, those left at 0 by a previous drop getting their
  coefficient back, and in the displacement form the x_bar_j part in the
  Objective coefficients of the same Variable. The positions of these
  Objective corrections are kept one per Variable, since a Variable with
  no correction yet gets its term after those of the coordinates

- `MasterProblemBlock::add_vars( n , coefficients )` takes the master problem
  from its coordinates to those plus `n` without building it anew, so that
  the stability centre, t, the level and the references stay where they
  are. The Variable and Constraint that exist once per coordinate (`Var_d`
  and `Bounds_d` in the primal form, `Var_z`, `Var_s_plus` and
  `Var_s_minus` in the dual one) are dynamic groups kept in `std::list`,
  each with a parallel `std::vector` of pointers for the access by index;
  `add_vars()` appends the new coordinates to them, together with their
  coupling rows (dual form), their terms in the level row (primal form) and
  in the Objective, whose positions are kept one per coordinate, and gives
  each hard component the new columns of its matrix, which `coefficients`
  carries indexed by the bundle slot of each cut. A coordinate is born at
  zero in the stability centre as in the point of every cut, hence no
  constant moves and no linearization error changes. Everything travels in
  one channel, so the Solver of the master updates the problem in place.
  `MasterProblemBlock::remove_vars( subset , sz )` does the converse: it
  projects the cuts of the hard components on the remaining coordinates,
  removes the Variable, Constraint and terms of the removed ones and
  compacts the per-coordinate state, again without building the master
  anew

- `LagBFunction` has the string parameter `strChkCfg`, the name of the file
  of the Configuration passed to `is_sol_feasible()` of the inner Block when
  an entry of the global pool is checked (typically, the tolerance of the
  check); empty, the default, leaves the one of the BlockConfig of the
  inner Block

- `MasterProblemBlock::set_integer()` declares which coordinates of the
  primal master in raw form are integer, so that its Solver solves a
  mixed-integer problem, and `solve_master()` does not ask such a master for
  the dual solution it does not have; `get_master_bound()` gives the lower
  bound on the optimal value of the last master, which is less than its
  value when a mixed-integer master stops at a relative gap

- the trust region `kTrustRegion` in the primal `MasterProblemBlock`: the
  master has the linear Objective of the cutting-plane model and the box
  `|| x - x_bar ||_inf <= t`, i.e., a mixed-integer linear master with
  integer coordinates, which any :MILPSolver solves; `set_t( Inf )` removes
  the stabilization of the primal master, both proximal and trust region,
  leaving the cutting-plane one

- `Block::clear_channel( chnl )` empties the current level of an open
  channel, which stays open, and `Block::close_channel( chnl , force ,
  discard )` with `discard == true` deletes the level it would finalize
  instead of shipping it, so that nobody receives anything; with
  `Block::get_default_channel()`, this lets whoever issues an unbounded
  sequence of Modification with no net effect (e.g., a decomposition that
  rewrites the costs of its sub-Block at each iteration and restores them at
  the end) keep them off the other Solver of the Block. `GroupModification`
  gets the protected `clear()` it needs. `BlockModification_unit_test` covers
  the empty channel, the clear followed by new Modification, the discard of
  a nested level, of the whole channel and of the default one, and the path
  from a sub-Block

- `Solution::drop_physical_values( block , mod , dropped )`, the physical
  counterpart of `drop_dynamic_values()`: the :Solution of a Block reads a
  physical Modification of that Block that removed some of the elements it
  holds values of and drops them; the base class returns false

- `MasterProblemBlock::shift_cuts()`, which adds a given delta to the
  subgradient of a set of cuts of a component, the constant of each of them
  staying as it is: this is what a change of the linear part of a component
  does to its linearizations, so that the cuts need not be thrown away and
  asked again, which is what the interface of the master problem of 1.0
  forced one to do. The vertical rows are left alone, the domain not moving
  with the linear part, and the stored constant follows the subgradient
  wherever the representation makes it depend on it: rather than recovering
  the raw constant out of what is stored, which asks for undoing a different
  expression in each of the four representations, the shift is read off
  get_stored_constant() itself, which is affine in ( g , alpha ), so that
  evaluating it on the two subgradients with alpha = 0 leaves exactly the
  term that depends on g

- `Modification_test` checks, for the modifying methods of `ColVariable`,
  `FRowConstraint`, the `OneVarConstraint` family, `FRealObjective`,
  `LinearFunction` and the dynamic `Variable` and `Constraint` of a `Block`,
  whether the change is done and what is issued under each value of the
  `ModParam`, with and without a `Solver` listening and on a channel, the
  type and content of each `Modification`, and the edge cases (empty and full
  `Range`, empty and unordered `Subset`, adding nothing, removing
  everything); and, under eDryRun, that the modifying methods of those
  classes and of `DQuadFunction`, `QuadFunction`, `PolyhedralFunction`,
  `LagBFunction`, `BendersBFunction` and `C05SumFunction` change nothing and
  issue nothing

- `ClassFactory_test` asserts what it used to print: every factory of the
  core (`Block`, `Configuration`, `Solver`, `Solution`, `State`, `Change`)
  gives an object of the class asked for every class the core registers,
  whatever the blanks in the classname, and refuses a name nobody
  registered, or no name, with `std::invalid_argument`

- `MasterProblemBlock::keep_easy_duals()` and `restore_easy_dual()`: the
  duals of the rows of an easy component, and with them the reduced costs of
  its columns, are saved at each solve of the master and written back when
  they are asked for, as the primal of that component already was. The
  sub-Block of an easy component is a Block of the model, which any other
  Solver may write into between the solve and the question, so what it holds
  when asked is not what the master left there; they are saved only if
  whoever drives the master says that it wants them, that being a pass over
  the rows at every solve

- `MasterProblemBlock`, the master problem of a stabilized method as a Block
  of the core: the model a bundle method solves at every iteration is built,
  read and changed through the abstract representation, so that whichever
  Solver is attached to it solves it, and its comments speak of the Solver
  that drives the master and not of one of them in particular

- `C05SumFunction`, the `C05Function` that is the sum of a given set of
  `C05Function`: it computes them, combines their linearizations into its
  own, and is their `Observer`, so that what happens to a member is seen as
  happening to the sum. It looks inside a `GroupModification` of its members
  and says once what the ones that agree do to it, and it takes in a member
  that changes its own Variable by keeping the union of the lists

- `Solver::has_Solver()`, which tells whether the factory holds a `:Solver`
  with a given name, i.e., whether `new_Solver()` would build one rather than
  throwing: which `:Solver` are there depends on the modules the program is
  built with and on the external libraries each of them has found, so that
  whoever applies a configuration naming the `:Solver` of a module that is not
  there can leave that one out, and say so, rather than dying on it

- `Solution::drop_dynamic_values()`, which tells a Solution that the dynamic
  Variable or Constraint that were in some positions of a cell of a group of
  the Block it was read from, or of one nested in it, are no longer there: a
  `Solution` that holds one value per element of the group, as the values of
  the `ColVariable` of a `ColVariableSolution` and the dual values of the
  `RowConstraint` of a `RowConstraintSolution` are, drops the values of those,
  so that the ones that are left keep matching the elements that are left, and
  gives them back to the caller, who is typically holding a dual solution and
  has to know whether the multiplier of a row that is gone was zero. A
  `ColRowSolution` looks for the cell among the groups of the Variable and
  then among those of the Constraint. The cell is named by its address, and it
  is looked for in the Solution of the Block and then in those of the nested
  ones; the default implementation returns false, which says that what the
  Solution holds is only good for the Block as it was

- `AbstractBlock::write_is()`, which `print( out , 'I' )` dispatches: after a
  `CDASolver` has proved the model unfeasible and `get_dual_direction()` has
  written the unbounded dual direction into the Block, it writes the rows
  whose multiplier is not zero, each with its multiplier and its name, and
  the bounds of the columns the same way. It asks nothing of any Solver, the
  ray being in the Constraint of the Block; what it writes is the certificate
  the Solver has left and not the smallest set of rows with that property

- `AbstractBlock::write_mps()`, which writes what the Block holds as the MPS
  file `read_mps()` reads, `print( out , 'M' )` having thrown "not implemented
  yet" as well. It says what `write_lp()` says and in the same names, with the
  two differences the format makes: a row with both sides finite and different
  is one row with its second side in `RANGES` rather than two rows, and a row
  whose two sides are both infinite is left out, the format having no way of
  saying it. A number is written with the fewest digits that read back as
  itself, in both writers, the six digits a stream gives by default not being
  enough to make the trip

- `AbstractBlock::write_lp()`, which writes what the Block holds as the LP
  file `read_lp()` reads, `print( out , 'L' )` having thrown "not implemented
  yet"; `serialize()` uses it to fill the netCDF variable `Model` that
  `deserialize()` has always read, with `ModelType = 'L'`. The Objective, the
  rows that are `FRowConstraint` on a `LinearFunction`, the bounds a column
  has of its own tightened by the `:OneVarConstraint` written on it and the
  integer columns are written; a row with both sides finite and different
  goes twice, `<name>_up` and `<name>_lo`, the format having no two-sided
  row. The names are those the groups give, with the indices joined by
  underscores, so `name_of_cell()` and `for_each_named_as()` take the two
  separators as a parameter. An LP file has no notion of groups, so what
  travels is the model and not the way it is grouped

- `serialize()` and `deserialize()` of `ColVariableSolution`,
  `RowConstraintSolution` and `ColRowSolution`, which threw "not ready yet":
  the values of the static stuff go in `StaticValues` (`StaticDuals` for the
  duals) with `StaticValuesStart` saying where each group begins, which is
  how SMS++ writes a matrix with rows of different length; the dynamic ones
  have one level more, the cells of all the groups going in one such matrix
  and `DynamicCellsStart` saying which of those cells each group begins at. A
  `ColRowSolution` writes its two halves in the groups `VariableSolution` and
  `ConstraintSolution`, and the Solution of a nested Block goes in
  `NestedSolution_<i>`

- `inspection::name_of()` gives a Variable or a Constraint the name of the
  group it sits in, or the index of that group when it has none, followed by
  the indices of its cell in the grid and, when the cells are collections, by
  its position inside its own cell; `inspection::for_each_named_as()` walks a
  whole group handing over each element with its name, reading the shape of
  the grid once rather than once per element, and
  `inspection::for_each_named_as_any_of()` does it running a cascade of
  concrete types. `AbstractBlock::print()` prints the Variable and the
  Constraint that way, which is what tells which one of them a row of the
  model is written on

- `Block::get_size_variable()` and `Block::set_size_variable()`, through
  which a :Block declares a column standing for a size parameter that it
  writes into its own rows, a column it owns or one it is given, normally by
  its father, a Solver seeing only ordinary Variable and Constraint; the
  size parameter that is a datum goes through the methods factory instead.
  `PolyhedralFunctionBlock` takes one, the multiplier of `set_lambda()`,
  before or after its abstract representation exists, and keeps its
  coefficient in step with the global scale, in place, which
  `PolyhedralFunctionBlock_unit_test` checks

- the methods factory takes the data of a setter as a `std::span` as well
  (`MF_dbl_sp`, `MF_int_sp` and the `MS_sp_*` signatures), which lets the
  setter check the length of what it is given instead of reading past its
  end, and has query families, `QueryType` with the `MS_qry_*` signatures
  and `get_query_fs()`, through which a caller reads data back from a Block
  it knows by name only; the forms taking an iterator stay, and are meant to
  go once the setters of every module take a span

- `Block` keeps its Variable and its Constraint in groups: a group says the
  type of its elements, its shape and its name, and hands them over without the
  caller having to know the type of the container they sit in, the job the
  `un_any_*` machinery used to do. `BaseGroup::for_each_as()` walks them with
  one switch per group and a loop typed on the element, `for_each_run_as()`
  gives them one run of contiguous ones at a time, which is what a caller
  mapping an element back to its position from its address needs,
  `elements_are()` answers the question on the type once for the whole group,
  and `Block::for_each_variable_group()` and `for_each_constraint_group()` walk
  the static groups and then the dynamic ones. The order in which the elements
  come out is the storage order, and that is part of the contract:
  `tests_Group.cpp` fixes it

- a group knows the type of the container it views, not only that of its
  elements, and `get_container_as< C >()` hands it back when it is a `C` and
  `nullptr` when it is not: that is what tells a `std::vector< T >` from a
  `boost::multi_array< T , 1 >`, which have the same elements, the same
  layout and the same rank, and it is what the typed accessors of `Block`
  ask instead of `boost::any_cast`

- a group says how to build a container of its own type and shape in another
  Block, which is what `AbstractBlock::mirror()` needed the `boost::any` for,
  and whoever allocates a container says how it goes, so that a Block
  disposes of what it owns through its own groups

- `std::vector< std::vector< Var > >` can be registered as a group of
  Variable, as it already could be as a group of Constraint: the two sides
  now have the same list of shapes

- `Configuration_unit_test`, `BlockModification_unit_test`,
  `Objective_unit_test`, `Constraint_unit_test`,
  `LinearConstraint_unit_test`, `PolyhedralFunction_unit_test`,
  `LagBFunction_unit_test`, `BendersBFunction_unit_test` and
  `Misc_unit_test`, and new cases in `AbstractBlock_test`,
  `AbstractPath_test`, `C05SumFunction_test`, `Group_test`,
  `LinearFunction_test`, `QuadFunction_test` and `Solution_test`: they check
  the edge cases of the core, i.e., empty and full `Range`, empty and
  unordered `Subset`, Variable and Constraint added and removed (all of them
  at once too), empty groups, Blocks, boxes and files, Blocks with no Solver
  attached, the netCDF round trip of each object that has one and malformed
  input

- `LagBFunction_unit_test` also checks the columns of the Lagrangian term
  that the Lagrangian costs are computed from, as `get_A_by_col()` gives
  them, against the ones computed by hand when the Lagrangian pairs are set
  (once and twice), added and removed (all of them, a `Range`, an ordered
  and an unordered `Subset`, a single one), and on the edge cases (empty
  `Range` and `Subset`, a `Range` past the end, a wrong index, no pair at
  all)

- `PolyhedralFunction_unit_test` also checks the edges of every method of
  `PolyhedralFunction` taking a `Range` or a `Subset` (empty, to the end,
  past the end, covering everything, unordered), with the `Modification`
  each change issues, the functions with no row, no Variable or neither,
  the vertical rows, the names of the global pool, which follow the rows and
  the `State` puts back, and the netCDF round trip of the vertical flags and
  of the functions with no row or no Variable

- `DQuadFunction_test` checks `DQuadFunction`: the value, the gradient whole
  and in part, dense and sparse, the Hessian and the convexity, the
  coefficients and the Variable changed by `Range` and by `Subset` at their
  edges, and the `Modification` each change issues

- `AbstractBlock_test` checks the edges of adding and removing dynamic
  Variable and Constraint: adding nothing, empty, reversed and out-of-range
  `Range`, empty, unordered and out-of-range `Subset`, removing everything,
  lists in the cells of a vector, the group and the `Block` of the elements,
  the `BlockModAdd` and `BlockModRmv*` sent, and the stuff a removed element
  was active in

- `Configuration_unit_test` also compares field by field what is read with
  what was written for the pairs of numbers and the nested
  `SimpleConfiguration`, the meta-configuration mapping a classname to a
  `Configuration` with its `*file.txt` and `*file.txt +` entries, the
  `ComputeConfig` applied to a `ThinComputeInterface`, the ten slots of a
  `BlockConfig` and what its `print()` writes, the `:BlockConfig` with the
  handlers of the Objective, of the Constraint and of the sub-Block, and
  `RBlockSolverConfig`

- `NetCDF_test` writes to a netCDF group and reads back the rows, the bounds
  and the `FRealObjective` of an `AbstractBlock`, the
  `PolyhedralFunctionBlock`, the `BendersBFunction` with its sub-Block and
  the `LagBFunction` with its inner Block and its Lagrangian term, the empty
  cases included, the LP files `read_lp()` has to refuse, and the `State`
  of the `PolyhedralFunction`, of the `BendersBFunction` and of the
  `LagBFunction`, both the one of `get_State()` and the one written by
  `serialize_State()`, with the core alone

- the netCDF format of `LagBFunction`, whose `serialize()` and
  `deserialize()` threw: the inner Block in the group `Block`, with the
  original costs of its Objective, and the Lagrangian term as g( x ) = A x
  + b, A in the sparse format of the matrix of a `BendersBFunction` with an
  `AbstractPath` to the column of each coefficient in place of its index;
  as for the other :Function, the multipliers y are not in the format, and
  the new `LagBFunction::set_variables()` gives them to the functions read.
  `serialize()` also puts back the Lagrangian costs it takes out of the
  inner Block to write it, which it restored as the original ones

### Changed

- `AbstractBlock::mirror_forward_Modification()` takes how the copy issues
  the Modification of its changes (eNoMod by default, as before), follows
  also the Variable added to or removed from a Function, the relaxing and
  enforcing of a Constraint, the Variable of a OneVarConstraint changed, and
  the dynamic Constraint added or removed and the dynamic ColVariable added,
  changes the copy in place rather than replacing its Functions, writes it in
  its sized form under a size Variable, and takes a physical Modification as
  one with nothing to do; `mirror()` copies whether a Constraint is relaxed

- an FRowConstraint and an FRealObjective say what they read of the
  Modification of their Function (`concerned()`): the changes of the set of
  its Variable always, those of its values only if their Block reads the
  coefficients of the Constraint or the Objective; `LinearFunction`,
  `DQuadFunction` and `QuadFunction` ask `issue_mod()` with the kind
  (`Function::eModFVars`, `Function::eModFValues`), so that under eNoBlck
  they do not build a change of values that nobody reads, and a
  `FunctionMod` says `eModFValues` in `changes()`

- the feasibility checks of `Block::is_feasible()` and
  `Block::is_sol_feasible()` without a Configuration, neither given nor in
  the BlockConfig, accept the relative violation `Block::DefaultFeasTol`
  (1e-12) instead of none, so that a point feasible up to rounding is not
  declared infeasible

- `ColVariable::set_value()` on a fixed ColVariable leaves it as it is when
  the new value equals the fixed one up to a relative 1e-6, and throws
  `std::domain_error` otherwise; `Block::is_sol_feasible()` takes such a
  throw as an infeasible Solution

- the makefile of the library carries `C05SumFunction`, which was built by
  CMake alone

- `tests_Function` discards on purpose what the calls that must throw return,
  the compiler warning that a value was ignored where the point is that the
  call never gets to return one

- a `Solution` that holds a direction, given to a `Block` that does not know
  what a direction of its own is [see `Block::has_directions()`], is declared
  not feasible rather than written in the Variable and checked as if it were
  a solution, which could call a ray that is not one feasible; for the same
  reason `LagBFunction::check_Solution()` drops the entry of the global pool
  it cannot check instead of keeping it, since keeping a wrong entry costs a
  wrong answer while dropping a right one costs finding it again

- the elements of a static group whose cells are `std::vector` are numbered
  cell by cell, the position inside the cell plus the sizes of the cells
  before it, as those of a dynamic group whose cells are `std::list` already
  were and as `Block::ConstraintID` documents, rather than the grid read the
  other way around, `c + i * n` for the i-th element of the c-th of n cells.
  A `ConstraintID` naming a `Constraint` inside such a group therefore names
  another one now; no configuration file did, those groups being recent, and
  the documentation of `Block::ConstraintID` covers them from now on

- a `Variable` and a `Constraint` point to their group: the field that held
  the pointer to the Block holds the pointer to the group the element is in,
  the lowest bit telling the two apart, and `get_Block()` answers through the
  group; `get_Group()` gives the group, `nullptr` for an element in none. The
  Block sets it when it registers a group, when an element is added to a
  dynamic list it has registered, and takes it away when the group is
  replaced or reset and when the element is removed; `set_Block()` with the
  Block of the group leaves the element in it, any other Block takes it out,
  and a copy of a `Variable` has the Block of the original and no group.
  `inspection::get_element_index()` looks in the group of the element only.
  `f_Block` is no longer a protected field of `Variable` and `Constraint`, a
  derived class reading `get_Block()`, and a container has to be there,
  possibly empty, when its group is replaced or reset, since the Block walks
  it to take its elements out of the group: clearing it first, as every
  `:Block` of the umbrella does, is fine, deleting it first is not

- the four `std::vector< boost::any >` of `Block` are gone, and so are the
  four vectors of the names beside them: a Block keeps its Variable and its
  Constraint in its four vectors of groups alone. `get_static_variables()`,
  `get_dynamic_variables()`, `get_static_constraints()`,
  `get_dynamic_constraints()` and the four `get_*_name()` that returned the
  whole vector of the names are gone with them, replaced by
  `get_static_variable_groups()` and its three fellows;
  `get_s_const_name( i )` and `get_s_const_index( name )`, and the six like
  them, stay and read the name of the group. The typed accessors,
  `get_static_variable< T >( i )` and the 23 like them, keep their signature
  and read the group instead of the `boost::any`, so their callers do not
  change; one of them asked for the wrong type answers `nullptr`, which is
  what their documentation has always promised, instead of throwing
  `boost::bad_any_cast`, so that whoever was finding a mistake of type out of
  the exception now gets a null pointer, and finds it out later and elsewhere.
  `Vec_any`, `c_Vec_any` and `Vec_any_it` are gone from `SMSTypedefs.h`,
  which no longer includes `<boost/any.hpp>`

- `PolyhedralFunctionBlock::set_lambda()` is `set_size_variable()` with the
  checks it had, and adds the multiplier to the normalization constraint in
  place instead of giving the constraint a new LinearFunction, so that the
  constraint and its LinearFunction stay the objects they were

- `PolyhedralFunctionBlock`, in the "linearized dual" representation, issues
  the Modification of a row being added or removed inside a
  `VariableGroupMod`: a row of the PolyhedralFunction is a column of the
  abstract representation, i.e., one Variable plus one coefficient in each row
  it appears in, and the group lets a Solver having a column operation of its
  own use it instead of reading the change one row at a time, while a Solver
  that does not recognise the group takes it apart and sees exactly the
  Modification it saw before

- the layout of `Block` has changed, and `add_static_variable()` and the
  other 35 registration methods are templates, hence they live in the
  translation unit of whoever calls them: after updating, everything has to
  be rebuilt, not only `libSMS++`, and a stale object file is not a
  compilation error but a group without the means to copy itself, or a
  library that is a hybrid of two layouts

- `GroupAdapter.h` is gone, having been the scaffolding that read the
  `boost::any` while the consumers of them were converted one at a time, and
  so are `Block::refresh_*_group()`, which existed to rebuild a group after
  it was written into through its `boost::any`, and which nobody calls

- `BoxSolver::get_var_solution()` and `get_dual_solution()` write the
  solution each time they are asked for it, rather than only when `compute()`
  has not written it already, since something else may have written the
  ColVariable or the dual values in between; `get_dual_solution()` writes all
  the dual values whatever `intPDSol` says `compute()` writes, 0 for the
  Constraint that are not box ones, these being relaxed, and 0 for a bound
  that is not tight at the optimum (both of them when the cost is 0), rather
  than leaving what was there

- `Block::set_default_channel()` throws `std::invalid_argument` for a name
  that is not an open channel of the Block or of one of its ancestors, the
  only ones a Modification of the Block can reach; 0 is always accepted

- `AbstractPath::get_number_elements()`, `get_element()` and
  `get_resolved_indices()` throw `std::invalid_argument` for a node of type
  'B' selecting a nested Block that is not there, which was only an
  `assert()`

### Removed

- `Block::access_static_variable()` and its three companions: nobody calls
  them, the abstract copy of a Block asking its groups, and there is no
  boost::any left to hand out

### Fixed

- `PolyhedralFunction::modify_bound()` takes the all-0 linearization of the
  bound out of the global pool when the bound is eliminated (set to - INF
  for a convex function, + INF for a concave one), also when no
  Modification is issued: the pool kept it, with the infinite bound as its
  constant, so that a Solver reading the pool anew, as `BundleSolver` does
  when it is registered again to the Block, put a linearization with an
  infinite constant in its bundle and returned a wrong value

- the Modification of the inner Block of an easy component reach the master
  Solver of every `MasterProblemBlock` that has registered it, so that two
  `BundleSolver` with easy components can be attached to the same Block:
  each `MasterProblemBlock` records the father the inner Block had when it
  registered it, i.e., the `LagBFunction` or `BendersBFunction`, or the
  `MasterProblemBlock` of another Solver that had registered it before, and
  passes it the Modification, while `clear()` gives the inner Block back to
  that father or has the next `MasterProblemBlock` record it in its place.
  Before, each passed them straight to the Function, so that only the
  master that had registered the inner Block last saw them, and the other
  `BundleSolver` kept solving a stale copy of the component after a change
  of its costs, demands or bounds, declaring unbounded an instance that
  has an optimum

- `MasterProblemBlock::set_global_LB()` tells the Solver of the master that
  the multiplier r of the global lower bound row is freed (or pinned to 0
  again), as `set_f_lev()` does with that of the level row: a Solver that
  had loaded the master with r pinned kept it so, and a global lower bound
  set after that had no effect. With r free, the dual
  master divides its aggregate by the mass of all its rows, lambda + r [see
  `aggregate_mass()`], rather than by lambda alone, and the aggregated
  linearization error summed row by row has the term r ( F( x_bar ) - LB )
  of the global lower bound row: with lambda, z* and the errors of the rows
  were divided by 1 - r while the error read off the proximal objective was
  not, so that the model value of the step (`get_FiBLambda()`) came out
  positive and huge as soon as r was. With r = 0 nothing changes

- a Modification of the inner Block that has `LagBFunction` check its
  global pool is forwarded to the father of the `LagBFunction` also when
  nobody listens to the `LagBFunction`, which it was not

- the box of a coordinate that turns finite or infinite after the master
  has been loaded, e.g., because the trust region t does, tells the Solver
  of the master that its multiplier is free or fixed

- `LagBFunction` gives the variables that an inner `Block` adds to its
  `Objective` after the registration, as the original cost in
  `CostMatrix`, the coefficient they have entered the `Objective` with
  rather than 0: the Lagrangian costs of the following `compute()` were
  computed without it, and `cleanup_inner_objective()` brought back 0 in
  place of it (e.g., the weight of the clauses added to a sub-`SATBlock`)

- the LagBFunction reads the parameters of its inner Solver (`get_*_par()`
  and `get_dflt_*_par()`) at the index that the inner Solver gives them:
  they translated it the wrong way, adding the offset that they had to take
  away, so that they read another parameter or none, and an inner Solver
  with parameters of its own threw at the first that it did not have

- `LagBFunction::set_ComputeConfig( nullptr )` takes the LagBFunction back
  to its defaults: it read the parameters of the null ComputeConfig, and
  `set_default_inner_BlockSolverConfig()` now un-registers the Solver that
  the BlockSolverConfig of the last ComputeConfig has registered, while it
  touched none, since a BlockSolverConfig built from the inner Block and
  clear()-ed only removes the Solver that it has registered itself

- setting `intInnrSlvr` of a LagBFunction no longer adds a ComputeConfig
  with no Solver name to the clear()-ed copy of its BlockSolverConfig,
  which then tried to create a Solver "" when the LagBFunction was
  destroyed, i.e., whenever a ComputeConfig gave both a BlockSolverConfig
  and `intInnrSlvr`

- the MasterProblemBlock saves the dual values of the easy components after
  having asked them to the Solver of the master, and also in the dual form
  of the master: before, it saved whatever their rows held, which in the
  dual form was never written

- the inner Block of a LagBFunction issues its Modification even when no
  Solver is registered to it, as the LagBFunction needs them to keep its
  costs up to date: before, a change of the inner Block made before its
  first Solver was registered did not reach the LagBFunction

- `MasterProblemBlock::add_vars()` in the displacement form of the dual
  gives the easy components the x_bar part of the new coordinates in the
  Objective, which `set_x_bar()` updates from then on; without it, the
  easy terms of a new coordinate were in its coupling row only, and the
  Objective missed their contribution as soon as the centre moved along it

- `PolyhedralFunctionBlock` handles the Variable added to its
  `PolyhedralFunction`: a `C05FunctionModVarsAddd` derives from
  `FunctionModVars`, which is not a `FunctionMod` [see `Function.h`], and
  `add_Modification()` passed on only the latter, so that the code dealing
  with it was never reached. The linearized primal gives its rows the new
  columns, reading them at the end of each row of the matrix and not at
  its beginning, and the dual attaches the theta of the Block to the
  coupling rows of the new coordinates, which the father Block creates
  first [see `MasterProblemBlock::add_vars()`]; conversely, removing a
  Variable in the dual representation requires the father Block to have
  removed its coupling rows first, the surviving rows keeping their theta
  coefficients [see `MasterProblemBlock::remove_vars()`]

- `Block::close_channel()` takes the channel out of the Block before
  shipping its `GroupModification`, so that an exception thrown by whoever
  receives it no longer leaves the Block with a channel whose
  `GroupModification` has already been deleted

- a dynamic `Variable` active in more than one stuff is removed from all of
  them: `Block::remove_variable_from_stuff()` walked the active list of the
  `Variable` by index while each removal took the stuff out of that very
  list, so it skipped the next one, which kept a `Variable` then destroyed

- `ColVariable::is_active()` gives `Inf` for a stuff that is not in the
  active list, and `remove_active()` throws for it: they took the place
  where the stuff would be for the place where it is, giving the index of
  another stuff and removing it

- `Block::set_objective()` issues its `BlockMod` on the channel of the
  `ModParam`, which it ignored, so that on a channel it was dispatched at
  once instead of in the `GroupModification` of the channel

- `BendersBFunction` checks that the Solver of the sub-Block writes the dual
  value of every Constraint the linearization is made of, and throws if it
  does not: a Solver that gives the duals of only a part of the sub-Block
  (e.g., a `LagrangianDualSolver` whose components are solved by a dynamic
  programming) left the others with the value of a previous solve, and the
  cut was silently wrong

- `FRealObjective` and `FRowConstraint` pass to their Block a Modification
  that concerns it also when no Solver is attached: an "abstract" change
  issued with eModBlck before any Solver is registered, such as the
  scaling of the scenario objectives by their probability in
  `TwoStageStochasticBlock`, was dropped, and the "physical" representation
  of the Block (e.g., the start-up costs of a `ThermalUnitBlock`, which its
  DP solvers read) was left out of synch with the Objective

- `MasterProblemBlock::clear()` leaves the `PolyhedralFunctionBlock` of the
  hard components, which the master allocates itself, out of `v_Block`,
  rather than only forgetting the pointers: they used to stay
  in the master, where the next `CreateEmptyMP()` added the new ones beside
  them, and since a stale one carries no linearization its row
  `sum_i theta^k_i + gamma^k = lambda`, with `gamma^k` fixed to 0, forced
  `lambda = 0` and made the master infeasible from the second time it was
  built on. A Solver registered twice on the same Block, as the one that
  learns the step-size is at every epoch, therefore failed with
  "unrecoverable MP failure" on its second solve. They are deleted by the
  destructor, which is the only place that can: a Solver registered on the
  master keeps the `Variable` of the rows it has loaded and reads them again
  when it reloads the problem, so that freeing them any earlier is a read of
  freed memory. They used never to be deallocated at all

- `LagBFunction::set_par( intInnrSlvr , ... )` no longer dereferences the
  BlockSolverConfig of the inner Block when none has been given

- when dynamic Variable are removed from its inner Block, `LagBFunction`
  drops what each entry of its global pool holds for them before checking
  whether the entry is still feasible: what is left would otherwise be
  written on the Variable that have taken their place, and the check would be
  made on a point that is nobody's; an entry that cannot let them go is
  deleted, since what it holds only fits the inner Block as it was

- `RowConstraint::is_feasible()` on a collection of collections, e.g., a
  `std::vector< std::vector< FRowConstraint > >`, did not compile with MSVC,
  whose deduction does not match `C< D< T > >` against containers that also
  take an allocator; the overload now deduces those trailing arguments too

- a group hands out the vertical linearizations of its members one at a time
  from the first request, and no longer answers the first one with their sum:
  a vertical row of a member is a valid inequality of the domain of the group,
  the domain of the group being the intersection of those of the members, and
  the sum of two of them, while valid, is implied by the two of them together
  while the converse fails, hence weaker than either

- `C05SumFunction::set_seed()` sets the seed of the generator that draws the
  combinations of linearizations, which whoever forms the groups now provides:
  the seed was the size of the group, so that two groups of the same size,
  which is what a partition into groups of equal size gives, drew the very
  same sequence

- with a level row next to the proximal term, the primal form of
  `MasterProblemBlock` reads the aggregate linearization error with the mass
  mu = 1 + eta that the rows of each component share, and `get_lambda()`
  returns that mass, rather than 1: the error was smaller than the true one,
  which made the stopping tests optimistic and the noise reduction fire with
  an exact oracle

- the raw aggregate constant of a component counts the vertical rows with
  their multipliers, as the aggregate subgradient already did, so that the
  aggregate row is a valid one

- the dual form of `MasterProblemBlock` divides by that same mass what
  whoever drives the master reads, i.e., the aggregate subgradient, the
  aggregate linearization error and the step it induces: only the pure level
  case did, so with a level row next to the proximal term the aggregate came
  out mu times too large and the aggregate error turned negative, the
  doubly stabilized method failing on problems that the proximal one solves

- a change of an off-diagonal coefficient of a `QuadFunction` issues the
  Modification of a change of the quadratic part and not that of the linear
  one, a Solver reading the wrong one having rebuilt the row it did not have
  to and left the one it had to alone

- `Block::remove_dynamic_constraints()`, asked for the whole list with an
  empty subset and with no Modification to be issued, removed each Constraint
  from its active Variable twice, and the second time threw "remove_active()
  called on non-active stuff"; the second pass `clear()`s them, as the one
  taking a Range already did

- the message of `RowConstraintSolution::write()` named `read()`

- `LagBFunction` left the Lagrangian cost in the Objective of a Variable of
  its inner Block that had lost its last multiplier, whenever a change of
  structure in the same batch of Modification rebuilt the list of the
  coupled positions before the costs were written again: the position was
  dropped from the list while still holding `c + y_k a`, and the function
  kept that cost for good. A position whose cost in the Objective differs
  from its original one now stays in the list until the original cost is
  back

- `LagBFunction` ignored the removal of *all* the Variable of the Objective
  of its inner Block: `LinearFunction::remove_variables()` says it with a
  `C05FunctionModVarsSbst` whose subset is empty, and the LagBFunction read
  the empty subset as "nothing removed". Its table of the costs then went out
  of step with the Objective, and a Variable having a Lagrangian term that had
  to be put back in the Objective was lost, which gave a wrong value of the
  function; the empty subset is now read as the whole range

- `Observer::new_channel_name()`, when reusing a freed name, dereferenced
  `rend()` and erased the lowest free name rather than the one it handed
  out, which only shows when channels are closed out of order, as the
  grouped Modification of UCBlock do, and made the parallel tests of
  InvestmentBlock fail now and then with "Observer: wrong channel name"

- a `boost::multi_array` stored in the order of Fortran, or with indices not
  starting at 0, was accepted as a group and then read as if it were not:
  its cells were named after the wrong indices and its copy had another
  shape. Registering one now throws `std::invalid_argument`; no module
  registers one

- the unit tests check what they assert in every build type: the Release one
  defines NDEBUG, which turned each of their `assert()` into nothing, so that
  they passed whatever happened, and `AbstractPath_test` did not even walk
  the Block, the walk being inside an `assert()`; now that it does, it
  writes and reads back through netCDF one path in 8, many thousands to a
  file, rather than every path in a file of its own, which took 9 minutes

- `RowConstraint::is_feasible()` on a collection of collections of
  RowConstraint, and on a `boost::multi_array` of collections, took them as
  const while each RowConstraint has to be computed, so that it did not
  compile as soon as it was used

- `BendersBFunction` kept the dual solutions of its global pool when the
  Constraint of the sub-Block changed, adding and removing alike, while a
  removal takes a dual variable away and what is left satisfies the dual
  constraints only if that variable was zero. An addition keeps the pool, the
  new dual variable being feasible at zero; a removal keeps the entries whose
  multiplier of the rows that went was zero, dropping it from the dual
  solution they hold [see `Solution::drop_dynamic_values()`], and deletes the
  others. The rows are still alive inside the `BlockModRmv` while it is being
  processed, which is what makes the multiplier readable at all, and the whole
  pool goes when the Modification does not say which rows went or a Solution
  of the pool cannot drop them

- `LagBFunction` marked no Solution as written in the inner Block after
  checking one of the global pool against a Block that is not
  `is_sol_feasible_physical()`: the check goes through the Variable there,
  and what is put back is only as complete as the Solution the Block hands
  out

- the check of a Solution against a Block, which is what the default
  `is_sol_feasible()` and `is_sol_optimal()` do, silently skipped putting
  back what the Variable held when the Block could not hand out its current
  Solution, leaving it with the checked Solution written in it; it now
  throws instead

- `BoxSolver` writes the dual value of a tight bound as minus the derivative
  of the Objective there, which it wrote with the wrong sign, and in the two
  linear cases of the minimization it writes it on the bound that is tight,
  which was the other one

- `BoxSolver` adds `x ( a x + b )` to the value for a Variable that does not
  belong to its Block, which stays fixed, rather than `a ( a x + b )`

- `BoxSolver::compute()` on an empty box sets the maximum and the minimum
  value to - INF and INF, those over an empty set, rather than leaving those
  of the previous solve

- `BoxSolver` forgets its solution when the sense of the Objective changes,
  which it kept as the solution of the new sense

- `BoxSolver::get_var_solution()` on a problem that is `kUnbounded`, for
  which `has_var_solution()` is true, gives a feasible value of each
  unbounded Variable, as `compute()` does, rather than throwing "unexpected
  unboundedness"

- `Block::open_channel()` leaks no `GroupModification` when it throws

- `Block::close_channel()` passes on the outermost `GroupModification` of the
  channel, the nested ones being owned by the one they are nested into,
  rather than the current one, which was then owned twice

- `BlockConfig`'s move constructor moves the Configuration of the structure
  too, which it left in the moved-from object

- `BlockConfig::serialize()` and `BlockSolverConfig::serialize()` on a
  problem file write the groups `Prob_<i>/BlockConfig` and
  `Prob_<i>/BlockSolver` that `Configuration::deserialize()` reads, rather
  than `Config_<i>/BlockConfig` and `Config_<i>/SolverConfig`, and
  `BlockSolverConfig::deserialize()` reads them there

- `Configuration::deserialize()` on a problem file reads the BlockSolver of
  `idx < 0` in the group `Prob_<-idx-1>`, as the documentation now says,
  rather than `Prob_<1-idx>`

- `Configuration::deserialize()` on a stream gives nullptr for a `*` followed
  by the end of the stream, as for one followed by whitespace

- `SimpleConfiguration< std::map< std::string , Configuration * >
  >::deserialize()` empties the map before reading, and reads as many entries
  as the file has, rather than as many as the map had, whose deleted
  Configuration it kept

- `SimpleConfiguration< std::vector< Configuration * > >` writes and reads
  its classname, which the factory needs to read it back, and a nullptr in it
  is written as a missing group and read back as nullptr, rather than
  dereferenced

- `ComputeConfig::set_par()` of a vector parameter that is not there yet
  writes into the new entry, rather than past the end of the list

- `ComputeConfig::load()` of a stream that ends before the flags leaves
  `f_diff` false, as `clear()` does, rather than true

- `OCRBlockConfig::get_right_BlockConfig()` builds the BlockConfig out of the
  Block, while the `const Block *` it has was converted to the `bool` of the
  constructor that takes `diff`, so that nothing was got

- `AbstractBlock::write_lp()` writes the lower bound of every column,
  `-infinity` included, since the format reads a column whose lower bound is
  not written as one with lower bound 0

- `AbstractBlock::read_lp()` reads the constant term of the Objective and of
  the rows, the latter moved to the side, a column that is only named in the
  Bounds, General or Binary sections, an Objective with no term and a file
  that ends right after its End

- `AbstractBlock::read_lp()` throws `std::invalid_argument` on a file that
  ends before its End, where it looped forever

- `AbstractBlock::deserialize()` gives the nested Blocks it reads the
  AbstractBlock as their father

- `AbstractPath::set_last_node_subset()` with an empty subset selects
  nothing, rather than what the node selected before, an empty subset being
  how a node says it has none

- `ColVariableSolution::deserialize()` and
  `RowConstraintSolution::deserialize()` read the dynamic groups out of
  `DynamicCellsStart` also when no group has a cell, in which case
  `DynamicValues` (`DynamicDuals`) is not written, rather than losing them

- the empty `clone()` of `ColVariableSolution`, `RowConstraintSolution` and
  `ColRowSolution` keeps `is_direction()`, as
  the documentation of `Solution::is_direction()` says a clone does

- `map_active()` of `ThinVarDepInterface`, `LinearFunction` and
  `DQuadFunction`, with `ordered == true`, maps a Variable only to the active
  one that is that very Variable, and throws only if one of those given is
  not active, rather than mapping a Variable that is not active to the next
  one and throwing when the active Variable are more than those given

- the copy and move assignments of the iterators of `ThinVarDepInterface`
  delete the iterator they replace, which leaked, and do nothing on a
  self-assignment

- `DQuadFunction::get_hessian_approximation()` and
  `QuadFunction::get_hessian_approximation()` put the i-th diagonal term in
  position ( i , i ), rather than all of them in ( 0 , 0 ), in a matrix of
  the right size

- `DQuadFunction::get_linearization_constant()` subtracts `a x^2` rather than
  `a^2 x`

- `DQuadFunctionModSbst` sorts its coefficients together with its unordered
  subset, so that each stays with its Variable

- `QuadFunction`'s constructor refuses a non-diagonal term whose larger index
  is out of range, which it checked on the smaller one

- `QuadFunction::add_nd_term()` and `modify_term()` refuse a diagonal term,
  and issue a `QuadFunctionModSbst` whose subset is increasing, as they say
  it is, with the Variable in the same order

- `PolyhedralFunction::get_linearization_coefficients()` on a Range starts
  from `range.first` rather than from 0

- `PolyhedralFunction::get_linearization_coefficients()` on a Subset reads
  the coefficient of each index in the subset, rather than the next one, and
  writes the dense output in the order of the subset rather than at the index

- `PolyhedralFunction::remove_variables()` on a Subset compacts the Variable
  and the rows of A, which `compact()`, taking its vector by value, left as
  they were

- `PolyhedralFunction::modify_rows()` and `modify_constants()` on an
  unordered Subset permute the rows and the constants along with it, which
  were left in the order given

- `PolyhedralFunction::delete_rows()` on a Subset takes the index
  `get_A().size()` as the bound and resets it, as documented, rather than
  throwing

- `PolyhedralFunction::put_State()` of a State with a smaller global pool
  empties the names that the State does not have, and updates the largest
  name in the pool

- `PolyhedralFunction::compute_new_linearization()` does not give the flat
  row of a bound that is not set as a linearization

- `PolyhedralFunction`'s constructor sets `AAccMlt` to its default, which was
  not initialized

- the documentation of `PolyhedralFunction` gives the `type()` of the
  Modification that `modify_rows()`, `modify_constants()` and `delete_rows()`
  issue, `NothingChanged` when no modified row is in the global pool and
  `GlobalPoolRemoved` for a deletion, and says that `delete_rows()` on a
  Range leaves the bound alone

- `LinearConstraint::add_variables()` and `add_variable()` with no
  Modification issued register the row in its new Variable, which is
  otherwise done by whoever reads the Modification

- `LinearConstraint::print()` compiles, having called a `value()` that no
  class has, which no build noticed since no file of the library includes the
  header

- `FRealObjective::remove_variables()` and
  `FRowConstraint::remove_variables()` on an empty Subset with no
  Modification issued take the Objective or the row out of the active list of
  all its Variable, the empty Subset meaning all of them

- the `remove_variable*()` of `FRealObjective` and `FRowConstraint` do not
  dereference a missing Block

- `LBConstraint`, `UBConstraint`, `NNConstraint`, `NPConstraint` and
  `ZOConstraint` read their Variable through `lb()` and `ub()`, which give 0
  when there is none, rather than dereferencing it

- `abs_viol()` and `rel_viol()` of `LBConstraint`, `UBConstraint`,
  `NNConstraint`, `NPConstraint` and `ZOConstraint` are 0 on a satisfied or
  infinite side, rather than negative or - INF, and `rel_viol()` divides by
  `max( 1 , |side| )`

- `Observer::issue_pmod()` is false for `eNoMod`, which it took as asking for
  a physical Modification

- `BendersBFunction::deserialize()` reads a matrix in sparse form into the
  matrix it builds, rather than into the rows of the current one

- `BendersBFunction::delete_rows()` finds the rows of A to delete by their
  position, rather than by their being empty, since with no active Variable
  every row is empty and all of them went

- `BendersBFunction::serialize()` in sparse form writes the number of
  nonzeros of every row, including the rows with none after the last nonzero

- the documentation of `BendersBFunction::modify_constants()` and
  `modify_constant()` says that the shift is `NaNshift`, which is what they
  issue, the sign of the change depending on the side of the row and on the
  sense of the inner Block

- `LagBFunction` clears the multipliers in the columns of the cost matrix
  when it deletes all its Lagrangian terms, which kept referring to the
  deleted ones

- the documentation of `Block::remove_dynamic_constraints()` says that a
  removed Constraint is `clear()`-ed in the destructor of the `BlockModRmv`
  when one is issued, so that whoever reads it can still read the Variable
  the Constraint was active in, rather than saying that this information is
  gone

- the default `ThinVarDepInterface::remove_variables( Range )` accepts a
  Range that ends at the last active Variable, as its documentation says,
  while it threw for it

- the modifying methods of the core that take a `ModParam` honour eDryRun,
  returning at once without changing anything and without issuing anything:
  those of `Variable`, `ColVariable`, `Constraint`, the `OneVarConstraint`
  family, `FRowConstraint`, `Objective`, `FRealObjective`, `LinearFunction`,
  `DQuadFunction`, `QuadFunction`, `PolyhedralFunction`, `LagBFunction`,
  `BendersBFunction`, `C05Function`, `C05SumFunction`, and
  `Block::set_objective()`, `add_dynamic_*()` and `remove_dynamic_*()`, the
  latter leaving also the stuff of the removed Variable alone. They looked
  at the `ModParam` only to decide whether to issue the Modification, so
  that under eDryRun the change was done all the same, silently

- the `remove_variable*()` of `FRowConstraint` and of `FRealObjective` keep
  eModBlck: the Modification of the Function is issued, and reaches the
  Block, also when no Solver is listening, while they turned eModBlck into
  eNoMod when the Block had no one there, so that the Block never learnt of
  a change it was concerned by

- the `remove_variables( Range )` of `FRowConstraint` and of
  `FRealObjective`, when no Modification is issued, cut the Range to the
  number of active Variable, as the Function does, while a Range up to
  `Inf` made them read past the end of the Function

- removing Variable from a `PolyhedralFunction` also takes their columns out
  of the aggregated linearizations of the global pool, which kept them and so
  gave the coefficients of other Variable

- `PolyhedralFunction::modify_rows( Subset )` checks the size of all the
  rows before changing any

- `PolyhedralFunction::delete_rows( Subset )` deletes the rows by position:
  it marked them by emptying them and then erased every empty row, so that
  with no Variable, where all rows are empty, it erased all the rows of A
  from the first deleted one on while b lost only the right ones

- `PolyhedralFunction::add_rows()` given no row returns silently, as the
  other methods given nothing to do, rather than issuing a
  `PolyhedralFunctionModAddd` with no row

- `PolyhedralFunction::set_par( intGPMaxSz , ... )` shrinking the pool to
  just the names in use loses nothing and issues nothing: it took the last
  name in use for lost, and the changes of the rows then no longer followed
  it

- `PolyhedralFunction::add_variables()` to a function with Variable but no
  row only adds the Variable, where an assert aborted a build without NDEBUG;
  there and in `add_variable()` the new coefficients must be one per row, and
  are refused otherwise, `add_variable()` having read past the end of a short
  vector and made rows of the wrong size in a function with no row

- the netCDF format of `PolyhedralFunction` has the optional variable
  `PolyFunction_Vert` with the vertical rows, which `serialize()` writes when
  some row is vertical and `deserialize()` reads, a file without it giving
  all the rows diagonal: they used to come back all diagonal

- `DQuadFunction::remove_variables( Range )` of a part of the Variable with
  the Modification issued no longer writes past the end of the vector of the
  removed Variable, whose loop tested an iterator that never moved

- `DQuadFunction::modify_linear_coefficients()` and
  `LinearFunction::modify_coefficients()` pass to the Modification only the
  part of `NCoef` they use, with a `Range` cut at the end as with a longer
  `NCoef`: the Modification refused a `delta()` longer than its Variable and
  the call threw after the coefficients had been changed. With a `Subset`,
  they and `DQuadFunction::modify_terms()` check all the indices before
  changing anything

- the `load()` of `OBlockConfig`, `CBlockConfig` and `RBlockSolverConfig`
  accept the `*` they document for no `ComputeConfig` of the Objective, of a
  Constraint and no `BlockSolverConfig` of a sub-Block, on which they threw

- `Configuration::deserialize( std::istream )` deletes the `Configuration` it
  has built when reading its body, or merging the overrides of a `*file.txt
  +`, throws, while it leaked it

- every string of a netCDF file is read with the new `get_var_values()` of
  SMSTypedefs.h, which reads the `char *` that the netCDF library allocates for
  each string, copies it and gives it back with `nc_free_string()`, and which
  the `deserialize()` helpers of the scalars, of the vectors, of the
  multi-dimensional arrays and of the matrices use too: so are read the names
  and values of the parameters of `ComputeConfig`, the text of the model of an
  `AbstractBlock`, the keys of the meta-configuration, the name of the group of
  each Constraint of a `CBlockConfig` and the ids of the sub-Block of
  `RBlockConfig` and of `RBlockSolverConfig`, which were read into the
  `std::string` objects themselves, and the name of each Solver of
  `BlockSolverConfig`, which leaked

- the extra slot of the body of a `*file.txt +` override is optional also
  inside a meta-configuration: `ComputeConfig::merge_overrides()` leaves in
  the stream a next token that is neither a `*` nor the name of a
  `Configuration`, i.e., the next key of the map, which it read as the
  classname of the extra `Configuration` and threw

- `BlockConfig::print()` writes the format that `load()` reads, with the
  version of the format, a `*` for each empty slot and the name of each slot
  as a comment

- `BlockConfig` and `BlockSolverConfig` written in an eProbFile go in the
  `Prob_<i>` group of the last Block written if it has no `BlockConfig`
  (`BlockSolver`) yet, and in a new one otherwise, rather than always in a
  new one; `Configuration::deserialize( netCDF::NcFile , idx )` and
  `BlockSolverConfig::deserialize( netCDF::NcFile , idx )` also take the
  groups `Config_<i>/BlockConfig` and `Config_<i>/SolverConfig` that older
  eProbFile have

- `AbstractBlock::write_lp()` writes every column in the Objective, with a
  zero cost if it has none, so that `read_lp()`, which numbers the columns
  in the order it meets them, gives them back in the order they have in the
  Block, which an `AbstractPath` to one of them relies on

- `AbstractBlock::read_lp()` throws `std::invalid_argument` also on a word
  out of its place, e.g., a row with no name or a sign with no term after
  it, and on a number that is not one; it reads a bound with the sense
  turned (`u >= x >= l`), `-inf` and `inf` in any case, and the rows in
  their order rather than by name, and it deletes what it has built when it
  throws

- `BendersBFunction::deserialize()` takes the number of active Variable out
  of `NumVar` when it has none, as its documentation says, so that
  `Block::new_Block()` of a `BendersBFunction` with columns works: it threw
  unless `NumVar` was the number it had, 0 in a new one; a sparse A whose
  `NumNonzeroAtRow` adds up to more than `NumNonzero`, or whose `Column`
  goes past `NumVar`, is refused rather than read out of bounds

- `put_State()` of `BendersBFunction` leaves no linearization in the places
  of its global pool past those of the State (the copy kept their
  constants), and moving a State in a `BendersBFunction` whose global pool
  is smaller no longer writes past the end of it

- `LagBFunction::serialize_State()` writes `LagBFunction_Value` and
  `LagBFunction_Convexified` as `LagBFunctionState::serialize()` does: the
  constants of the linearizations were read back as 0

- `LagBFunction::put_State()` leaves `LastSolution` undefined: a global pool
  that grew to the size of the State took the Solution of the inner Block
  for that of the first entry, and computed its constant from the Block
  rather than taking the one of the State

## [0.7.1] - 2026-09-13

### Fixed

- a direction is checked against a quadratic row too, the sign of d^T Q d
  deciding whether the row bounds it

## [0.7.0] - 2026-09-12

### Added

- `AbstractBlock::mirror()`, which builds the AbstractBlock as a copy of the
  abstract representation of any other Block: one ColVariable per
  ColVariable, one Constraint per Constraint and an Objective, the groups
  keeping their shape and their order and the inner Block being copied
  recursively, with a Constraint of the copy written in the Variable of the
  copy and a Variable living outside the mirrored subtree shared with the
  original. The copy knows which object of the original each of its own
  corresponds to, hence it moves solution information in both directions
  [see `mirror_read()` and `mirror_write()`] and applies to itself a
  Modification the original issues [see `mirror_forward_Modification()`],
  which is what an `UpdateSolver` attached to the original does by itself.
  What cannot be written on other Variable, which is any Function but a
  LinearFunction and a DQuadFunction, is left out and recorded [see
  `get_mirror_issues()`], so that the copy is then a relaxation and whoever
  asked for it can see that it is; the objects of each group are counted on
  both sides, so that a group of a shape the copy does not reproduce is
  recorded as well rather than quietly holding fewer objects

- the default implementation of `Block::map_back_solution()`,
  `Block::map_forward_solution()` and `Block::map_forward_Modification()`
  serves the AbstractBlock that has mirrored the Block, so that every Block
  has a R3 Block of itself without having had to write a line for it

- `Block::access_static_variable()` and its three companions, which give the
  group of Variable or Constraint as the boost::any holding it, so that code
  building a Block out of another one can install a group whose type it only
  knows at run time

- `ThinComputeInterface::print_parameters()`, which prints the name, the
  current value and the default one of every parameter, walking the six
  index spaces; for a class whose index space extends over that of a
  wrapped ThinComputeInterface it shows what the wrapped one has been
  given, which is what one needs to see when a parameter is suspected of
  not arriving where it was meant to

- `QuadFunction::remove_variables( Subset )`, so that a bunch of Variable can
  be removed in one call rather than one at a time

- `Block::set_structure()`, which decides the *structure* of a Block, i.e.,
  its tree of sub-Block, as opposed to its *formulation*, i.e., what its
  abstract representation encodes; `set_BlockConfig()` calls it as soon as it
  has digested the BlockConfig, hence before anything is generated and before
  any Solver is attached

- `BlockConfig::f_structure_Configuration`, the Configuration that
  `set_structure()` reads, which is the first one of a BlockConfig, since the
  structure comes before everything else

### Changed

- the text format of a BlockConfig carries a version number right after the
  differential flag, so that a file in the previous format is refused with a
  message rather than being read with all its Configuration shifted by one
  slot; every BlockConfig file has to be converted

- the version of the module is the git tag of its repository, or the
  VERSION.txt of a release tarball, and the shared library carries it: its
  SONAME is major.minor while the major is 0, and it is installed with an
  RPATH relative to itself, so that an installed tree keeps working wherever
  it is moved

### Fixed

- `LagBFunction::cleanup_inner_objective()` restored the original costs by
  rewriting the whole vector of coefficients, i.e. a Range spanning every
  variable of the inner Block. A :Block is entitled to refuse a change on
  some of its own, and ThermalUnitBlock does so for the schedule-deviation
  variables on the range rather than on the value: it therefore refused a
  restore that left those coefficients exactly where they were, which is
  what the AC instances of the test battery died on. Only the coefficients
  that actually differ are written now, as a Range when they are contiguous
  and as a Subset otherwise, mirroring how the Lagrangian costs are written

- `QuadFunction::remove_variables( Range )` computed the shift of the
  non-diagonal terms out of one Variable more than it was removing, kept the
  terms of the last removed one and, whenever an Observer was listening, spun
  in an infinite loop while collecting the names of the removed Variable

## [0.6.0] - 2025-12-12

### Added

- SimpleConfiguration< std::pair< std::string ,
  Configuration * > >

- set\_caller() method to DataMapping

- strLogFileName string parameter in Solver

- ::deserialize for std::vector< std::string >

- [huge] new parameter intPushCostToOwner in LagBFunction
  allows to decide whether the Lagrangian term is added
  to the "root" inner Block (as previously) or rather
  in the [Linear/DQuad]Function of the Block where the
  ColVariable is defined

- added full netCDF file support for State

- un\_any\_thing\_OneVarConstraint\_*

- added full netCDF file support for Configuration

- updated interface to handle two-nested std::vector

- range_index field in AbstractPath

- updated SMSTypedefs to handle
  boost::multi\_array< std::vector< T > , K >

- added couple SimpleConfiguration

- added full netCDF file support for Solution

- support for arm64 under MacOS

- parameter to select the Solver of the inner Block in
  BendersBFunctio

- Block::almost\_deserialize()

- [huge] QuadFunction for non-diagonal general quadratic
  functions

- DQuadFunctionModVarsAddd

- support for reading .lp and .mps files in AbstractBlock,
  comprised handling of QP problems

### Changed

- parameter in set\_ComputeConfig() is now const

- extended CLANG_1200_0_32_27_PATCH to all
  un\_any\_thing\_*, not only un\_any\_thing\_0

- Solution is no longer pure virtual (useful to define
  empty placeholder Solution)

- removed support for x86-64 under MacOS

- almost complete rehaul of makefiles (but more is needed)

- moved vectors for parameters inside methods

### Fixed

- fixed conceptual flaw in LagBFunction: getting an empty
  Solution from the Block and accumulating all the
  Solution of a convex combination was not the right
  approach according to Solution definition, and in fact
  it was miserably failing with UnitBlockSolution

- removed memory leak in LagBFunction

- catastrophic bug in RBlockSolverConfig::apply()

- g++ issue with SMSpp_ensure_load

- dynamic destructor of SimpleConfiguration< stuff >

- map_active() in most *Function (added in BendersBFunction)

- many minor and major flaws

## [0.5.3] - 2024-02-29

### Added

- "father of LagBFunction" mechanism

- AbstractBlock::read_lp()

- AbstractBlock::read_mps()

### Changed

- adapted to new CMake / makefile organisation

### Fixed

- flaw in DQuadFunction

- template arguments handling in SMSTypedefs.h

- BendersBFunction::delete_rows()

- badly mangled LagBFunction::InnerSolver

## [0.5.2] - 2023-05-17

### Added

- C05Function uses Function::set_par

- RowConstraint::is_feasible()

### Changed

- definition of RowConstraint::rel_viol()

- feasibility check in AbstractBlock

### Removed

- Constraint::is_feasible()

### Fixed

- dynamic cast in put_State() (BendersBFunction, LagBFunction, and
  PolyhedralFunction)

- copy constructor of BlockConfig

## [0.5.1] - 2022-06-28

### Added

- added ThinVarDepInterface::map\_index()

- added `is_feasible()` and `clear()` also for std::list,
  std::vector< std::list > and boost::multi_array< std::list >

- added generic `is_feasible` methods for Variable and Constraint types

- added RelaxationSolve concept

- added Change concept

- added ::deserialize() for std::string

- added Objective::eUndef as default value for get\_objective\_sense()

- added not\_ModBlock and un\_ModBlock

- added `clear_constraints` methods

- added INFRange

- added Solver::pop() and Solver::mod_clear()

- added ::deserialize() for matrix with fixed-length and variable-length rows

- added get\_constant\_term() in RealObjective

- computation of Lipschitz constant in LagBFunction

- passthrough" feature in LagBFunction where the set_par() and related
  mechanisms can be used to directly set the parameters in the inner Solver

### Changed

- dynamic Constraint not clear()-ed on deletion if they are shipped to a
  BlockModRmv; clearing is now done in the destructor of the BlockModRmv,
  so that the information about the set of active variable can still be
  "seen" by the Solver handling it. this turns out to be crucial for
  CPXMILPSolver to handle deletion of OneVarConstraint, but may be useful
  in general

- reworked channel management: now open\_channel() and close\_channel()
  suffice. added open\_if\_needed() and close\_if\_needed() versions

- avoided un-necessary Modification in PolyhedralFunctionBlock

- changed load/print stream interface

- restored default true to optional in SMSTypedefs.h

- significant rehaul of ::deserialize() functions

- better management of ::deserialize() functions with checks about the
  types that are being deserialized

- complete redesign and significant enhancement in BoxSolver: now properly
  handles multi-level f_Block and Variable not belonging to it

### Fixed

- too many bugs to count

## [0.5.0] - 2021-12-08

### Added

- AbstractBlock can read MPS files.

- State (representation of the state of a ThinComputeInterface).

- BendersBFunctionState, LagBFunctionState, and PolyhedralFunctionState.

- Two new SimpleConfiguration.

- VariableGroupMod.

### Changed

- Computation of linearization constant in BendersBFunction.

### Fixed

- Bugs in LagBFunction.

- Multiplier in sum() of RowConstraintSolution and ColVariableSolution.

- get_*_index()/element() in BlockInspection.

- Bugs in BoxSolver.

- Flaw in AbstractBlock::is_feasible().

## [0.4.0] - 2021-02-05

### Added

- Some unit tests.

- Configuration header SMSppConfig.h.

- Rather large changes in LagBFunction, added LagBFunctionMod.

- Added VariableMod::old_type() and supporting methods.

- Cleaner style for un_any_thing macros.

- Implemented AbstractBlock::is_correct().

- Significant rehaul of handling "stealth" obj variables addition in
  LagBFunction: Variable addition is now performed batch in compute()
  rather than real-time when dealing with Modification.

- Added ColRowSolution.

- Significant improvements in load()-ing of Configurations,
  graciously terminating if the stream eof()-s.

- Added vectors in Configurations.

- Defined SMSpp\_classname\_normalise().

- LagBFunction now supports DQuadFunction Objective.

- Added InnrSlvr parameter in LagBFunction.

- Updated DataMapping Dealing with the in which SetFrom is empty when
  producing error messages.

- ThinVarDepInterface now has get_Block().

- Added BoxSolver.

- Updated serialization of BendersBFunction.

### Fixed

- Compilation error in DataMapping with GCC.

- Too many individual fixes to list.

## [0.3.2] - 2020-09-16

### Fixed

- Compilation error in DataMapping.

## [0.3.1] - 2020-09-16

### Fixed

- Bug in *BlockConfig::clear().

## [0.3.0] - 2020-09-16

### Added

- Support for concurrency.

- [O][C][R]BlockConfig for configuring also the Objective, Constraint, and
  sub-Block, recursively.

- RBlockSolverConfig for configuring the Solver of the sub-Block, recursively.

### Changed

- New configuration framework.

## [0.2.0] - 2020-03-06

### Added

- Name to Block

## [0.1.1] - 2020-02-10

### Fixed

- Compilation error in unit tests.

## [0.1.0] - 2020-02-10

### Added

- First test release.

[Unreleased]: https://gitlab.com/smspp/smspp/-/compare/0.8.0...develop
[0.8.0]: https://gitlab.com/smspp/smspp/-/compare/0.7.1...0.8.0
[0.7.1]: https://gitlab.com/smspp/smspp/-/compare/0.7.0...0.7.1
[0.7.0]: https://gitlab.com/smspp/smspp/-/compare/0.6.0...0.7.0
[0.6.0]: https://gitlab.com/smspp/smspp/-/compare/0.5.2...0.6.0
[0.5.2]: https://gitlab.com/smspp/smspp/-/compare/0.5.1...0.5.2
[0.5.1]: https://gitlab.com/smspp/smspp/-/compare/0.5.0...0.5.1
[0.5.0]: https://gitlab.com/smspp/smspp/-/compare/0.4.0...0.5.0
[0.4.0]: https://gitlab.com/smspp/smspp/-/compare/0.3.2...0.4.0
[0.3.2]: https://gitlab.com/smspp/smspp/-/compare/0.3.1...0.3.2
[0.3.1]: https://gitlab.com/smspp/smspp/-/compare/0.3.0...0.3.1
[0.3.0]: https://gitlab.com/smspp/smspp/-/compare/0.2.0...0.3.0
[0.2.0]: https://gitlab.com/smspp/smspp/-/compare/0.1.1...0.2.0
[0.1.1]: https://gitlab.com/smspp/smspp/-/compare/0.1.0...0.1.1
[0.1.0]: https://gitlab.com/smspp/smspp/-/tags/0.1.0
