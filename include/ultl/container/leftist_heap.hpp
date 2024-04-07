#ifndef _ULTL_LEFTIST_HEAP_HPP
#define _ULTL_LEFTIST_HEAP_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>

namespace ultl {
struct leftist_heap_node_base_ {
  using base_ptr = leftist_heap_node_base_ *;

  base_ptr pre;
};

template <class T> struct leftist_heap_node_ : public leftist_heap_node_base_ {
  using dist_type = std::int32_t;
  using node_ptr = leftist_heap_node_<T> *;

  dist_type dist;
  T value;
  // `lc` means the left child, and `rc` means the right child.
  node_ptr lc, rc;

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

struct leftist_heap_header_ {
  leftist_heap_node_base_ header;
  std::size_t node_count;

  leftist_heap_header_() noexcept {
    reset();
  }

  leftist_heap_header_(leftist_heap_header_ &&from) noexcept {
    if (from.header.pre != nullptr) {
      move_data(from);
    } else {
      reset();
    }
  }

  void move_data(leftist_heap_header_ &other) {
    header.pre = other.header.pre;
    header.pre->pre = &header;
    node_count = other.node_count;

    other.reset();
  }

  void reset() {
    header.pre = nullptr;
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
  using base_ptr_ = leftist_heap_node_base_ *;
  using const_base_ptr_ = const leftist_heap_node_base_ *;
  using node_ptr_ = leftist_heap_node_<T> *;
  using const_node_ptr_ = const leftist_heap_node_<T> *;

private:
  using node_allocator_ = typename std::allocator_traits<
      Allocator>::template rebind_alloc<leftist_heap_node_<T>>;
  using allocator_traits_ = std::allocator_traits<node_allocator_>;

protected:
  node_ptr_ get_node_() {
    return allocator_traits_::allocate(get_node_allocator_(), 1);
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

  template <bool Move, typename NodeGen>
  node_ptr_ clone_node_(node_ptr_ p, NodeGen &node_gen) {
    using condition = std::conditional<Move, value_type &&, const value_type &>;
    node_ptr_ tmp = node_gen(std::forward<condition>(*p->value_ptr()));
    tmp->dist = p->dist;
    tmp->lc = nullptr;
    tmp->rc = nullptr;
    return tmp;
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

  static const_reference value_(const_base_ptr_ p) {
    return value_(static_cast<const_node_ptr_>(p));
  }

  template <typename ValueCompare>
  struct leftist_heap_impl_ : public node_allocator_,
                              public leftist_heap_compare_<ValueCompare>,
                              public leftist_heap_header_ {
    using base_compare = leftist_heap_compare_<ValueCompare>;

    leftist_heap_impl_() noexcept(
        std::is_nothrow_default_constructible<node_allocator_>::value &&
        std::is_nothrow_default_constructible<base_compare>::value)
        : node_allocator_() {}

    leftist_heap_impl_(const leftist_heap_impl_ &other)
        : node_allocator_(
              allocator_traits_::select_on_container_copy_construction(other)),
          base_compare(other.comp), leftist_heap_header_() {}

    leftist_heap_impl_(leftist_heap_impl_ &&from) noexcept(
        std::is_nothrow_move_constructible<base_compare>::value) = default;

    leftist_heap_impl_(node_allocator_ &&from)
        : node_allocator_(std::move(from)) {}

    leftist_heap_impl_(leftist_heap_impl_ &&from, node_allocator_ &&from_a)
        : node_allocator_(std::move(from_a)), base_compare(std::move(from)),
          leftist_heap_header_(std::move(from)) {}

    leftist_heap_impl_(const base_compare &other_c, node_allocator_ &&from_a)
        : node_allocator_(std::move(from_a)), base_compare(other_c) {}
  };

  leftist_heap_impl_<Compare> impl_;

  base_ptr_ &root_() noexcept { return this->impl_.header.pre; }

  const_base_ptr_ root_() const noexcept { return this->impl_.header.pre; }

  node_allocator_ &get_node_allocator_() noexcept { return this->impl_; }

  const node_allocator_ &get_node_allocator_() const noexcept {
    return this->impl_;
  }

private:
  void erase_(node_ptr_ x) {
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
    if (x->rc == nullptr) {
      x->dist = 1;
    } else {
      x->dist = 1 + x->rc->dist;
      x->rc->pre = x;
      if (x->lc != nullptr && x->lc->dist < x->rc->dist)
        std::swap(x->lc, x->rc);
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

  leftist_heap(const leftist_heap &other) {
    // TODO
  }

  leftist_heap(const leftist_heap &other, const allocator_type &alloc) {
    // TODO
  }

  leftist_heap(leftist_heap &&from) = default;

  leftist_heap(leftist_heap &&from, const allocator_type &alloc)
      : leftist_heap(std::move(from), node_allocator_(alloc)) {}

  leftist_heap(std::initializer_list<value_type> init,
               const Compare &comp = Compare(),
               const allocator_type &alloc = allocator_type()) {
    // TODO
  }

  leftist_heap(std::initializer_list<value_type> init,
               const allocator_type &alloc = allocator_type())
      : leftist_heap(init, Compare(), alloc) {}

  // Deconstructor

  ~leftist_heap() noexcept {
    erase_(static_cast<node_ptr_>(root_()));
  }

  // Assignment operator

  leftist_heap &operator=(const leftist_heap &other) {
    // TODO
  }

  leftist_heap &operator=(leftist_heap &&from) {
    // TODO
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
    root_() = merge_node_(static_cast<node_ptr_>(root_()), create_node_(value));
    ++this->impl_.node_count;
  }

  void push(value_type &&value) {
    root_() = merge_node_(static_cast<node_ptr_>(root_()),
                          create_node_(std::forward<value_type &&>(value)));
    ++this->impl_.node_count;
  }

  // Construct an element in-place and insert it to the leftist heap.
  template <class... Args> void emplace(Args &&...args) {
    root_() =
        merge_node_(static_cast<node_ptr_>(root_()), create_node_(args...));
    ++this->impl_.node_count;
  }

  // Remove the top element.
  void pop() {
    node_ptr_ heap = static_cast<node_ptr_>(root_());
    node_ptr_ tmp = merge_node_(heap.lc, heap.rc);
    destroy_node_(heap);
    root_() = tmp;
    --this->impl_.node_count;
  }

  // Swap two heaps.
  void swap(leftist_heap &other) noexcept {
    // TODO
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
