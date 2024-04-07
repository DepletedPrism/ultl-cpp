#ifndef _ULTL_SPLAY_TREE_HPP
#define _ULTL_SPLAY_TREE_HPP

namespace ultl {
template <typename T>
struct splay_tree_node_base {
  using base_ptr = splay_tree_node_base *;
  using const_base_ptr = const splay_tree_node_base *;

  base_ptr parent;
  // 0 means left child, and 1 means right child.
  base_ptr child[2];
};

} // namespace ultl

#endif