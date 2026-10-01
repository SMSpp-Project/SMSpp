   //! <b>Requires</b>: p must be a valid iterator of *this, and x must be a
   //!   stable_vector other than *this whose allocator compares equal to the
   //!   one of *this.
   //!
   //! <b>Effects</b>: Transfers all the elements of x to *this, before p,
   //!   without copying, moving or destroying any of them; x is left empty.
   //!
   //! <b>Throws</b>: If the index of *this has to grow and its allocation
   //!   throws, in which case neither *this nor x change.
   //!
   //! <b>Complexity</b>: Linear to x.size() plus the number of elements of
   //!   *this from p to the end.
   //!
   //! <b>Note</b>: Iterators, pointers and references to the elements of x
   //!   remain valid, and refer to the same elements, which are now in *this.
   void splice(const_iterator p, stable_vector &x)
   {  this->splice(p, x, x.cbegin(), x.cend());  }

   //! <b>Effects</b>: Same as splice(p, x), x being an rvalue.
   void splice(const_iterator p, BOOST_RV_REF(stable_vector) x)
   {  this->splice(p, static_cast<stable_vector &>(x));  }

   //! <b>Requires</b>: p must be a valid iterator of *this, i a valid
   //!   dereferenceable iterator of x, and x a stable_vector other than *this
   //!   whose allocator compares equal to the one of *this.
   //!
   //! <b>Effects</b>: Transfers the element pointed by i from x to *this,
   //!   before p, without copying, moving or destroying it.
   //!
   //! <b>Throws</b>: If the index of *this has to grow and its allocation
   //!   throws, in which case neither *this nor x change.
   //!
   //! <b>Complexity</b>: Linear to the number of elements of *this from p to
   //!   the end plus the number of elements of x after i.
   //!
   //! <b>Note</b>: Iterators, pointers and references to the element remain
   //!   valid, and refer to the same element, which is now in *this.
   void splice(const_iterator p, stable_vector &x, const_iterator i)
   {
      const_iterator n(i);
      this->splice(p, x, i, ++n);
   }

   //! <b>Effects</b>: Same as splice(p, x, i), x being an rvalue.
   void splice(const_iterator p, BOOST_RV_REF(stable_vector) x,
               const_iterator i)
   {  this->splice(p, static_cast<stable_vector &>(x), i);  }

   //! <b>Requires</b>: p must be a valid iterator of *this, [first, last) a
   //!   valid range of x, and x a stable_vector other than *this whose
   //!   allocator compares equal to the one of *this.
   //!
   //! <b>Effects</b>: Transfers the elements in [first, last) from x to *this,
   //!   before p, without copying, moving or destroying any of them.
   //!
   //! <b>Throws</b>: If the index of *this has to grow and its allocation
   //!   throws, in which case neither *this nor x change.
   //!
   //! <b>Complexity</b>: Linear to the distance between first and last plus
   //!   the number of elements of *this from p to the end plus the number of
   //!   elements of x from last to the end.
   //!
   //! <b>Note</b>: Iterators, pointers and references to the transferred
   //!   elements remain valid, and refer to the same elements, which are now
   //!   in *this.
   void splice(const_iterator p, stable_vector &x,
               const_iterator first, const_iterator last)
   {
      BOOST_ASSERT(this != &x);
      BOOST_ASSERT(this->priv_node_alloc() == x.priv_node_alloc());
      BOOST_ASSERT(this->priv_in_range_or_end(p));
      BOOST_ASSERT(first == last ||
         (first < last && x.priv_in_range(first) &&
          x.priv_in_range_or_end(last)));
      BOOST_CONTAINER_STABLE_VECTOR_CHECK_INVARIANT;
      const size_type num = static_cast<size_type>(last - first);
      if(!num){
         return;
      }
      const size_type idx = static_cast<size_type>(p - this->cbegin());
      const index_iterator x_first
         (x.index.begin() + static_cast<difference_type>(first - x.cbegin()));
      const index_iterator x_last(x_first + static_cast<difference_type>(num));
      //Make room in the index of *this for the nodes of x, which is the only
      //step that can throw, and fix up the pointers of the nodes that moved
      index_traits_type::initialize_end_node
         (this->index, this->internal_data.end_node, num);
      const node_base_ptr_ptr old_buffer = this->index.data();
      this->index.insert
         (this->index.begin() + static_cast<difference_type>(idx),
          x_first, x_last);
      const index_iterator index_beg = this->index.begin();
      index_traits_type::fix_up_pointers_from
         (this->index, this->index.data() != old_buffer ?
            index_beg : index_beg + static_cast<difference_type>(idx));
      //Now the nodes belong to *this: drop them from the index of x
      const index_iterator e = x.index.erase(x_first, x_last);
      index_traits_type::fix_up_pointers_from(x.index, e);
   }

   //! <b>Effects</b>: Same as splice(p, x, first, last), x being an rvalue.
   void splice(const_iterator p, BOOST_RV_REF(stable_vector) x,
               const_iterator first, const_iterator last)
   {  this->splice(p, static_cast<stable_vector &>(x), first, last);  }

