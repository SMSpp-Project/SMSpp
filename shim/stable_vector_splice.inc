   //! <b>Requires</b>: p must be a valid iterator of *this, and x must be a
   //!   stable_vector other than *this whose allocator compares equal to the
   //!   one of *this.
   //!
   //! <b>Effects</b>: Transfers all the elements of x to *this, before p,
   //!   without copying, moving or destroying any of them; x is left empty.
   //!
   //! <b>Throws</b>: If size() + x.size() > max_size(), or if the index of
   //!   *this has to grow and its allocation throws, in which case neither
   //!   *this nor x change.
   //!
   //! <b>Complexity</b>: Linear to x.size() plus the number of elements of
   //!   *this from p to the end.
   //!
   //! <b>Note</b>: Iterators, pointers and references to the elements of x
   //!   remain valid, and refer to the same elements, which are now in *this.
   void splice(const_iterator p, stable_vector &x)
   {
      BOOST_ASSERT(this != &x);
      this->splice(p, x, x.cbegin(), x.cend());
   }

   //! <b>Effects</b>: Same as splice(p, x), x being an rvalue.
   void splice(const_iterator p, BOOST_RV_REF(stable_vector) x)
   {  this->splice(p, static_cast<stable_vector &>(x));  }

   //! <b>Requires</b>: p must be a valid iterator of *this, i a valid
   //!   dereferenceable iterator of x, and x a stable_vector whose allocator
   //!   compares equal to the one of *this; x can be *this.
   //!
   //! <b>Effects</b>: Transfers the element pointed by i from x to *this,
   //!   before p, without copying, moving or destroying it; if p == i or
   //!   p == i + 1, nothing changes.
   //!
   //! <b>Throws</b>: If x is *this, nothing; otherwise, if size() ==
   //!   max_size(), or if the index of *this has to grow and its allocation
   //!   throws, in which case neither *this nor x change.
   //!
   //! <b>Complexity</b>: If x is *this, linear to the distance between p and
   //!   i; otherwise, linear to the number of elements of *this from p to the
   //!   end plus the number of elements of x after i.
   //!
   //! <b>Note</b>: Iterators, pointers and references to the element remain
   //!   valid, and refer to the same element.
   void splice(const_iterator p, stable_vector &x, const_iterator i)
   {
      BOOST_ASSERT(x.priv_in_range(i));
      const_iterator n(i);
      this->splice(p, x, i, ++n);
   }

   //! <b>Effects</b>: Same as splice(p, x, i), x being an rvalue.
   void splice(const_iterator p, BOOST_RV_REF(stable_vector) x,
               const_iterator i)
   {  this->splice(p, static_cast<stable_vector &>(x), i);  }

   //! <b>Requires</b>: p must be a valid iterator of *this, [first, last) a
   //!   valid range of x, and x a stable_vector whose allocator compares
   //!   equal to the one of *this; x can be *this, and then p must not be in
   //!   (first, last).
   //!
   //! <b>Effects</b>: Transfers the elements in [first, last) from x to *this,
   //!   before p, without copying, moving or destroying any of them.
   //!
   //! <b>Throws</b>: If x is *this, nothing; otherwise, if size() +
   //!   distance(first, last) > max_size(), or if the index of *this has to
   //!   grow and its allocation throws, in which case neither *this nor x
   //!   change.
   //!
   //! <b>Complexity</b>: If x is *this, linear to the distance between the
   //!   first and the last of p, first and last; otherwise, linear to the
   //!   distance between first and last plus the number of elements of *this
   //!   from p to the end plus the number of elements of x from last to the
   //!   end.
   //!
   //! <b>Note</b>: Iterators, pointers and references to the transferred
   //!   elements remain valid, and refer to the same elements.
   void splice(const_iterator p, stable_vector &x,
               const_iterator first, const_iterator last)
   {
      BOOST_ASSERT(this->priv_in_range_or_end(p));
      BOOST_ASSERT(first == last ||
         (first < last && x.priv_in_range(first) &&
          x.priv_in_range_or_end(last)));
      BOOST_ASSERT(this->priv_node_alloc() == x.priv_node_alloc());
      if(this == &x){
         BOOST_ASSERT(p <= first || last <= p);
         this->priv_splice_self(p, first, last);
      }
      else{
         this->priv_splice_other(p, x, first, last);
      }
   }

   //! <b>Effects</b>: Same as splice(p, x, first, last), x being an rvalue.
   void splice(const_iterator p, BOOST_RV_REF(stable_vector) x,
               const_iterator first, const_iterator last)
   {  this->splice(p, static_cast<stable_vector &>(x), first, last);  }

   private:

   //the pointers of the index in [first, last) put in reverse order
   static void priv_splice_reverse(index_iterator first, index_iterator last)
   {
      while((first != last) && (first != --last)){
         ::boost::adl_move_swap(*first, *last);
         ++first;
      }
   }

   //[first, last) of *this moved before p, by a rotation of the index alone
   //(three reversals), which needs no allocation
   void priv_splice_self(const_iterator p,
                         const_iterator first, const_iterator last)
   {
      BOOST_CONTAINER_STABLE_VECTOR_CHECK_INVARIANT;
      const const_iterator cbeg(this->cbegin());
      const difference_type dp = p - cbeg;
      const difference_type df = first - cbeg;
      const difference_type dl = last - cbeg;
      difference_type a = 0, b = 0, c = 0;
      if(dp < df){
         a = dp;
         b = df;
         c = dl;
      }
      else if(dl < dp){
         a = df;
         b = dl;
         c = dp;
      }
      if((a == b) || (b == c)){
         return;
      }
      const index_iterator ib(this->index.begin());
      priv_splice_reverse(ib + a, ib + b);
      priv_splice_reverse(ib + b, ib + c);
      priv_splice_reverse(ib + a, ib + c);
      index_traits_type::fix_up_pointers(ib + a, ib + c);
   }

   //[first, last) of x, which is not *this, moved before p
   void priv_splice_other(const_iterator p, stable_vector &x,
                          const_iterator first, const_iterator last)
   {
      BOOST_CONTAINER_STABLE_VECTOR_CHECK_INVARIANT;
      const size_type num = static_cast<size_type>(last - first);
      if(!num){
         return;
      }
      if(num > size_type(this->max_size() - this->size())){
         throw_length_error("stable_vector::splice max_size() exceeded");
      }
      const difference_type idx = p - this->cbegin();
      const index_iterator x_first(x.index.begin() + (first - x.cbegin()));
      const index_iterator x_last(x_first + difference_type(num));
      //The next two steps are the only ones that can throw, and then neither
      //container has changed: initialize_end_node() allocates only the index
      //of an empty container, with room for the num new pointers, and the
      //insertion has the strong guarantee
      index_traits_type::initialize_end_node
         (this->index, this->internal_data.end_node, num);
      const node_base_ptr_ptr old_buffer = this->index.data();
      const index_iterator ins =
         this->index.insert(this->index.begin() + idx, x_first, x_last);
      index_traits_type::fix_up_pointers_from
         (this->index,
          this->index.data() != old_buffer ? this->index.begin() : ins);
      //Now the nodes belong to *this: drop them from the index of x
      const index_iterator e = x.index.erase(x_first, x_last);
      index_traits_type::fix_up_pointers_from(x.index, e);
      //The pool of a stable_vector holds at most as many nodes as the spare
      //capacity of its index: those of *this that no longer fit go to the
      //pool of x, where the erase has just made room for them
      const size_type spare =
         size_type(this->index.capacity() - this->index.size());
      if(this->internal_data.pool_size > spare){
         multiallocation_chain holder;
         for(size_type n = size_type(this->internal_data.pool_size - spare);
             n; --n){
            holder.push_front(this->priv_get_from_pool());
         }
         x.priv_put_in_pool(holder);
      }
   }

   public:

