#ifndef _ULTL_LEFTIST_HEAP_HPP
#define _ULTL_LEFTIST_HEAP_HPP

#include <cstdint>
#include <memory>
#include <utility>

namespace ultl {
template <class T> struct leftist_heap_node_ {
  using dist_type = std::int32_t;
  using node_ptr = leftist_heap_node_<T> *;

  dist_type dist;
  T value;
  // `lc` means the left child, and `rc` means the right child.
  node_ptr lc, rc;

  leftist_heap_node_() : dist(1), lc(nullptr), rc(nullptr) {}

  T *value_ptr() { return std::addressof(value); }
  const T *value_ptr() const { return std::addressof(value); }
};

template <class Compare> struct leftist_heap_compare_ {
  Compare comp;

  leftist_heap_compare_() : comp() {}

  leftist_heap_compare_(const Compare &_comp) noexcept(
      std::is_nothrow_default_constructible<Compare>::value)
      : comp(_comp) {}

  leftist_heap_compare_(const leftist_heap_compare_ &) = default;

  leftist_heap_compare_(leftist_heap_compare_ &&from) noexcept(
      std::is_nothrow_constructible<Compare>::value)
      : comp(from.comp) {}
};

template <class T> struct leftist_heap_header_ {
  leftist_heap_node_<T> *header;
  std::size_t node_count;

  leftist_heap_header_() noexcept { reset(); }

  leftist_heap_header_(leftist_heap_header_ &&from) noexcept {
    if (from.header != nullptr) {
      move_data(from);
    } else {
      reset();
    }
  }

  void move_data(leftist_heap_header_ &other) {
    header = other.header;
    node_count = other.node_count;

    other.reset();
  }

  void reset() {
    header = nullptr;
    node_count = 0;
  }
};

// A leftist heap (or leftist tree) is a priority queue implemented with a
// variant of a binary heap, which is also a mergeable heap.
template <class T, class Compare = std::less<T>,
          class Allocator = std::allocator<T>>
class leftist_heap {
public:
  using value_type = T;
  using value_compare = Compare;
  using allocator_type = Allocator;

  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;

protected:
  using node_ptr_ = leftist_heap_node_<T> *;
  using const_node_ptr_ = const leftist_heap_node_<T> *;

private:
  using node_allocator_ = typename std::allocator_traits<
      Allocator>::template rebind_alloc<leftist_heap_node_<T>>;
  using allocator_traits_ = std::allocator_traits<node_allocator_>;

protected:
  node_ptr_ get_node_() {
    node_ptr_ p = allocator_traits_::allocate(get_node_allocator_(), 1);
    return p;
  }

  void put_node_(node_ptr_ p) noexcept {
    allocator_traits_::deallocate(get_node_allocator_(), p, 1);
  }

  template <class... Args> void construct_node_(node_ptr_ p, Args &&...args) {
    try {
      ::new (p) leftist_heap_node_<T>;
      allocator_traits_::construct(get_node_allocator_(), p->value_ptr(),
                                   std::forward<Args>(args)...);
    } catch (...) {
      p->~leftist_heap_node_<T>();
      put_node_(p);
      throw;
    }
  }

  template <class... Args> node_ptr_ create_node_(Args &&...args) {
    node_ptr_ p = get_node_();
    construct_node_(p, std::forward<Args>(args)...);
    return p;
  }

  void destroy_node_(node_ptr_ p) noexcept {
    allocator_traits_::destroy(get_node_allocator_(), p->value_ptr());
    p->~leftist_heap_node_<T>();
  }

  void drop_node_(node_ptr_ p) noexcept {
    destroy_node_(p);
    put_node_(p);
  }

  static const_reference value_(const_node_ptr_ p) {
    static_assert(
        std::is_invocable<Compare &, const T &, const T &>{},
        "unable to invoke the comparison with two arguments of value type.");
    if constexpr (std::is_invocable<Compare &, const T &, const T &>{}) {
      static_assert(std::is_invocable_v<const Compare &, const T &, const T &>,
                    "unable to invoke the comparison as const.");
    }

    return std::identity()(*p->value_ptr());
  }

  template <class Value, typename ValueCompare>
  struct leftist_heap_impl_ : public node_allocator_,
                              public leftist_heap_compare_<ValueCompare>,
                              public leftist_heap_header_<Value> {
    using base_compare = leftist_heap_compare_<ValueCompare>;
    using base_header = leftist_heap_header_<Value>;

    leftist_heap_impl_() noexcept(
        std::is_nothrow_default_constructible<node_allocator_>::value &&
        std::is_nothrow_default_constructible<base_compare>::value)
        : node_allocator_() {}

    leftist_heap_impl_(const leftist_heap_impl_ &other)
        : node_allocator_(
              allocator_traits_::select_on_container_copy_construction(other)),
          base_compare(other.comp), base_header() {}

    leftist_heap_impl_(leftist_heap_impl_ &&from) noexcept(
        std::is_nothrow_move_constructible<base_compare>::value) = default;

    leftist_heap_impl_(node_allocator_ &&from)
        : node_allocator_(std::move(from)) {}

    leftist_heap_impl_(leftist_heap_impl_ &&from, node_allocator_ &&from_a)
        : node_allocator_(std::move(from_a)), base_compare(std::move(from)),
          base_header(std::move(from)) {}

    leftist_heap_impl_(const base_compare &other_c, node_allocator_ &&from_a)
        : node_allocator_(std::move(from_a)), base_compare(other_c) {}
  };

  leftist_heap_impl_<T, Compare> impl_;

  node_ptr_ &root_() noexcept { return this->impl_.header; }

  const_node_ptr_ root_() const noexcept { return this->impl_.header; }

  node_allocator_ &get_node_allocator_() noexcept { return this->impl_; }

  const node_allocator_ &get_node_allocator_() const noexcept {
    return this->impl_;
  }

private:
  // Functor that used for allocating node.
  //
  // An optimization is using an extra functor to handle the situation that
  // there are some nodes waiting to be recycled when allocating, e.g. copy
  // constructors, assignments, just like what SGI STL does in stl_tree.h.
  //
  // For the sake of simplicity, only `alloc_node_` is implemented here, without
  // considering recycling nodes.
  struct alloc_node_ {
    alloc_node_(leftist_heap &other) : heap(other) {}

    template <class... Args> node_ptr_ operator()(Args &&...args) {
      return heap.create_node_(std::forward<Args>(args)...);
    }

  private:
    leftist_heap &heap;
  };

  enum {
    as_lvalue_ = 0,
    as_rvalue_ = 1,
  };

  template <bool Move, typename NodeGen>
  node_ptr_ clone_node_(node_ptr_ p, NodeGen &node_gen) {
    using condition =
        std::conditional_t<Move, value_type &&, const value_type &>;
    node_ptr_ tmp = node_gen(std::forward<condition>(*p->value_ptr()));
    tmp->dist = p->dist;
    return tmp;
  }

  template <bool Move, typename NodeGen>
  node_ptr_ clone_(node_ptr_ other_p, NodeGen &node_gen) {
    node_ptr_ rt = clone_node_<Move>(other_p, node_gen);
    try {
      node_ptr_ p = rt;
      if (other_p->rc != nullptr)
        p->rc = clone_<Move>(other_p->rc, node_gen);
      other_p = other_p->lc;
      while (other_p != nullptr) {
        node_ptr_ q = clone_node_<Move>(other_p, node_gen);
        p->lc = q;
        if (other_p->rc != nullptr)
          q->rc = clone_<Move>(other_p->rc, node_gen);
        p = q;
        other_p = other_p->lc;
      }
    } catch (...) {
      erase_(rt);
      throw;
    }
    return rt;
  }

  template <bool Move, typename NodeGen>
  node_ptr_ clone_(const leftist_heap &other, NodeGen &node_gen) {
    // the root of `other` should not be nullptr
    node_ptr_ rt = clone_<Move>(other.impl_.header, node_gen);
    this->impl_.node_count = other.impl_.node_count;
    return rt;
  }

  node_ptr_ copy_(const leftist_heap &other) {
    alloc_node_ alloc(*this);
    return clone_<as_lvalue_>(other, alloc);
  }

  void copy_assign_(const leftist_heap &other) {
    clear();
    if (other.root_() != nullptr) {
      root_() = copy_(other);
    }
  }

  void move_assign_(leftist_heap &other) {
    clear();
    if (other.root_() != nullptr) {
      alloc_node_ alloc(*this);
      root_() = clone_<as_rvalue_>(other, alloc);
      other.clear();
    }
  }

  void erase_(node_ptr_ x) noexcept {
    while (x != nullptr) {
      erase_(x->rc);
      node_ptr_ y = x->lc;
      drop_node_(x);
      x = y;
    }
  }

  // Merge two nodes and their successor nodes, and return the merged node
  // (either x or y).
  node_ptr_ merge_node_(node_ptr_ x, node_ptr_ y) {
    if (x == nullptr)
      return y;
    if (y == nullptr)
      return x;
    if (comp()(x->value, y->value)) // default behaviour is y->value < x->value
      std::swap(x, y);
    x->rc = merge_node_(x->rc, y);
    if (x->lc == nullptr) {
      std::swap(x->lc, x->rc);
      x->dist = 1;
    } else {
      // x->rc must not be nullptr since y is not nullptr
      if (x->lc->dist < x->rc->dist)
        std::swap(x->lc, x->rc);
      x->dist = 1 + x->rc->dist;
    }
    return x;
  }

public:
  // Constructors

  leftist_heap() = default;

  explicit leftist_heap(const Compare &comp,
                        const allocator_type &alloc = allocator_type())
      : impl_(comp, alloc) {}

  explicit leftist_heap(const allocator_type &alloc)
      : impl_(node_allocator_(alloc)) {}

  leftist_heap(const leftist_heap &other) : impl_(other.impl_) {
    if (other.root_() != nullptr)
      root_() = copy_(other);
  }

  leftist_heap(const leftist_heap &other, const allocator_type &alloc)
      : impl_(other.impl_, node_allocator_(alloc)) {
    if (other.root_() != nullptr)
      root_() = copy_(other);
  }

  leftist_heap(leftist_heap &&from) = default;

  leftist_heap(leftist_heap &&from, const allocator_type &alloc)
      : leftist_heap(std::move(from), node_allocator_(alloc)) {}

  leftist_heap(std::initializer_list<value_type> init,
               const Compare &comp = Compare(),
               const allocator_type &alloc = allocator_type())
      : leftist_heap(comp, alloc) {
    for (auto value : init)
      push(value);
  }

  leftist_heap(std::initializer_list<value_type> init,
               const allocator_type &alloc)
      : leftist_heap(init, Compare(), alloc) {}

  // Deconstructor

  ~leftist_heap() noexcept { clear(); }

  // Assignment operator

  leftist_heap &operator=(const leftist_heap &other) {
    if (this != std::addressof(other)) {
      this->impl_.comp = other.impl_.comp;
      copy_assign_(other);
    }
    return *this;
  }

  leftist_heap &operator=(leftist_heap &&from) {
    this->impl_.comp = std::move(from.impl_.comp);
    move_assign_(from);
    return *this;
  }

  // Element access

  // Return the top element.
  const_reference top() const { return value_(root_()); }

  // Capacity

  // Check whether the leftist heap is empty.
  bool empty() const { return this->impl_.node_count == 0; }

  // Return the number of elements in the leftist heap.
  size_type size() const { return this->impl_.node_count; }

  // Modifiers

  // Insert an element into the leftist heap.
  void push(const value_type &value) {
    root_() = merge_node_(root_(), create_node_(value));
    ++this->impl_.node_count;
  }

  void push(value_type &&value) {
    root_() =
        merge_node_(root_(), create_node_(std::forward<value_type &&>(value)));
    ++this->impl_.node_count;
  }

  // Construct an element in-place and insert it to the leftist heap.
  template <class... Args> void emplace(Args &&...args) {
    root_() = merge_node_(root_(), create_node_(args...));
    ++this->impl_.node_count;
  }

  // Remove the top element.
  void pop() {
    node_ptr_ tmp = merge_node_(root_()->lc, root_()->rc);
    drop_node_(root_());
    root_() = tmp;
    --this->impl_.node_count;
  }

  // Remove all elements.
  void clear() noexcept {
    erase_(root_());
    this->impl_.reset();
  }

  // Swap two heaps.
  void swap(leftist_heap &other) noexcept(
      std::is_nothrow_swappable<Compare>::value) {
    // There might be some performance issues.
    std::swap(this->impl_, other.impl_);
  }

  // Merge two heaps.
  void merge(const leftist_heap &other) {
    if (other.root_() != nullptr) {
      // will spend extra time copying nodes in `other`
      root_() = merge_node_(root_(), copy_(other));
      this->impl_.node_count += other.impl_.node_count;
    }
  }

  // time complexity is about O(max_dist(this) + max_dist(from)), while the
  // maximum dist in a leftist heap is not greater than
  // ceil(log(node_count + 1))
  void merge(leftist_heap &&from) {
    root_() = merge_node_(root_(), from.root_());
    this->impl_.node_count += from.impl_.node_count;
    from.impl_.reset();
  }

  // Observers

  // Return the function that compares values
  value_compare comp() const { return this->impl_.comp; }
};
} // namespace ultl

namespace std {
// Specialize the `std::swap` algorithm with `ultl::leftist_heap::swap`.
template <class T, class Compare, class Allocator>
void swap(ultl::leftist_heap<T, Compare, Allocator> &lhs,
          ultl::leftist_heap<T, Compare, Allocator> &rhs) noexcept {
  lhs.swap(rhs);
}
} // namespace std

#endif // _ULTL_LEFTIST_HEAP_HPP
