#pragma once
#include <cstddef>
#include <memory>

template <size_t N>
class StackStorage {
  alignas(alignof(std::max_align_t)) std::byte storage_[N]{};
  size_t space_allocated = 0;

public:
  StackStorage() = default;

  StackStorage(const StackStorage&) = delete;

  StackStorage& operator=(const StackStorage&) = delete;

  void* allocate(size_t size, size_t alignment) {
    void* p = static_cast<void*>(storage_ + space_allocated);
    size_t space = N - space_allocated;
    void* aligned = std::align(alignment, size, p, space);
    if (!aligned) {
      throw std::bad_alloc();
    }
    space_allocated =
        static_cast<size_t>(static_cast<std::byte*>(aligned) - storage_) + size;
    return aligned;
  }
};

template <typename T, size_t N>
class StackAllocator {
  StackStorage<N>* storage_ = nullptr;

public:
  template <typename, size_t>
  friend class StackAllocator;

  StackAllocator() noexcept = default;

  explicit StackAllocator(StackStorage<N>& storage) noexcept
      : storage_(&storage) {}

  template <typename U>
  StackAllocator(const StackAllocator<U, N>& other) noexcept
      : storage_(other.storage_) {}

  template <typename U>
  StackAllocator& operator=(const StackAllocator<U, N>& other) noexcept {
    storage_ = other.storage_;
    return *this;
  }

  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  T* allocate(size_t n) {
    if (storage_ == nullptr) {
      throw std::bad_alloc();
    }
    return static_cast<T*>(storage_->allocate(n * sizeof(T), alignof(T)));
  }

  void deallocate(T*, size_t) noexcept {}

  template <typename U>
  bool operator==(const StackAllocator<U, N>& other) const noexcept {
    return storage_ == other.storage_;
  }

  template <typename U>
  bool operator!=(const StackAllocator<U, N>& other) const noexcept {
    return storage_ != other.storage_;
  }

  StackAllocator select_on_container_copy_construction() const noexcept {
    return *this;
  }

  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using void_pointer = void*;
  using const_void_pointer = const void*;
  using size_type = std::size_t;
  using is_always_equal = std::false_type;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_swap = std::false_type;

  ~StackAllocator() = default;
};

template <typename T, typename Alloc = std::allocator<T>>
class List {
public:
  using value_type = T;
  using allocator_type = Alloc;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = typename std::allocator_traits<Alloc>::pointer;
  using const_pointer = typename std::allocator_traits<Alloc>::const_pointer;

private:
  struct BaseNode {
    BaseNode* next = this;
    BaseNode* prev = this;
    BaseNode() = default;
  };
  struct Node : BaseNode {
    T value{};
    Node() = default;

    Node(const T& value) : value(value) {}
  };
  using NodeAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAlloc>;

  BaseNode fake_node_;
  size_t size_ = 0;
  [[no_unique_address]] NodeAlloc alloc_{};

  template <typename... Args>
  Node* create_node(const Args&... args) {
    Node* node = NodeAllocTraits::allocate(alloc_, 1);
    try {
      NodeAllocTraits::construct(alloc_, node, args...);
    } catch (...) {
      NodeAllocTraits::deallocate(alloc_, node, 1);
      throw;
    }
    return node;
  }

  void destroy_node(Node* node) noexcept {
    NodeAllocTraits::destroy(alloc_, node);
    NodeAllocTraits::deallocate(alloc_, node, 1);
  }

  void insert_node(BaseNode* pos, BaseNode* node) noexcept {
    BaseNode* n = pos->next;
    node->next = n;
    node->prev = pos;
    pos->next = node;
    n->prev = node;
  }

  Node* remove_node(BaseNode* node) noexcept {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    return static_cast<Node*>(node);
  }

public:
  List() noexcept = default;

  explicit List(const allocator_type& alloc) noexcept
      : fake_node_(), alloc_(alloc) {}

  List(size_t size, const allocator_type& alloc = allocator_type())
      : fake_node_(), alloc_(alloc) {
    try {
      while (size_ < size) {
        Node* n = create_node();
        insert_node(fake_node_.prev, n);
        ++size_;
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(size_t size, const T& value,
       const allocator_type& alloc = allocator_type())
      : fake_node_(), alloc_(alloc) {
    try {
      while (size_ < size) {
        push_back(value);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List(const List& other)
      : fake_node_(),
        alloc_(NodeAllocTraits::select_on_container_copy_construction(
            other.alloc_)) {
    try {
      for (const T& v : other) {
        push_back(v);
      }
    } catch (...) {
      clear();
      throw;
    }
  }

  List& operator=(const List& other) {
    if (this == &other) {
      return *this;
    }
    if constexpr (NodeAllocTraits::propagate_on_container_copy_assignment::
                      value) {
      List copy(allocator_type(other.alloc_));
      for (const T& v : other) {
        copy.push_back(v);
      }
      std::swap(alloc_, copy.alloc_);
      swap_nodes(copy);
    } else {
      List copy(alloc_);
      for (const T& v : other) {
        copy.push_back(v);
      }
      swap_nodes(copy);
    }
    return *this;
  }

  void swap_nodes(List& other) noexcept {
    std::swap(fake_node_.next, other.fake_node_.next);
    std::swap(fake_node_.prev, other.fake_node_.prev);
    fake_node_.next->prev = &fake_node_;
    fake_node_.prev->next = &fake_node_;
    other.fake_node_.next->prev = &other.fake_node_;
    other.fake_node_.prev->next = &other.fake_node_;
    std::swap(size_, other.size_);
  }

  void swap(List& other) noexcept(
      NodeAllocTraits::propagate_on_container_swap::value) {
    if (this == &other) {
      return;
    }
    if constexpr (NodeAllocTraits::propagate_on_container_swap::value) {
      std::swap(alloc_, other.alloc_);
      swap_nodes(other);
      return;
    } else {
      if (alloc_ == other.alloc_) {
        swap_nodes(other);
        return;
      }
    }
    List copy(alloc_);
    List copy_other(other.alloc_);
    for (const T& v : other) {
      copy.push_back(v);
    }
    for (const T& v : *this) {
      copy_other.push_back(v);
    }
    this->swap_nodes(copy);
    other.swap_nodes(copy_other);
  }

  ~List() { clear(); }

  allocator_type get_allocator() const noexcept {
    return allocator_type(alloc_);
  }

  size_t size() const noexcept { return size_; }

  bool empty() const noexcept { return size_ == 0; }

  void push_front(const T& v) { insert(cbegin(), v); }

  void push_back(const T& v) { insert(cend(), v); }

  void pop_front() noexcept {
    if (empty()) {
      return;
    }
    erase(begin());
  }

  void pop_back() noexcept {
    if (empty()) {
      return;
    }
    erase(--end());
  }

  void clear() noexcept {
    while (!empty())
      pop_back();
  }

private:
  template <bool IsConst>
  class BasicIterator {
    template <bool>
    friend class BasicIterator;
    friend class List;

    using NodePtr = std::conditional_t<IsConst, const BaseNode*, BaseNode*>;
    using DataPtr = std::conditional_t<IsConst, const Node*, Node*>;

    NodePtr ptr_ = nullptr;

  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = std::conditional_t<IsConst, const T*, T*>;
    using reference = std::conditional_t<IsConst, const T&, T&>;

    BasicIterator() = default;

    explicit BasicIterator(NodePtr p) : ptr_(p) {}

    template <bool B = IsConst, typename = std::enable_if_t<B>>
    BasicIterator(const BasicIterator<false>& other) noexcept
        : ptr_(other.ptr_) {}

    reference operator*() const { return static_cast<DataPtr>(ptr_)->value; }

    pointer operator->() const { return &static_cast<DataPtr>(ptr_)->value; }

    BasicIterator& operator++() {
      ptr_ = ptr_->next;
      return *this;
    }

    BasicIterator operator++(int) {
      BasicIterator copy(*this);
      ++(*this);
      return copy;
    }

    BasicIterator& operator--() {
      ptr_ = ptr_->prev;
      return *this;
    }

    BasicIterator operator--(int) {
      BasicIterator copy(*this);
      --(*this);
      return copy;
    }

    friend bool operator==(BasicIterator a, BasicIterator b) {
      return a.ptr_ == b.ptr_;
    }
    friend bool operator!=(BasicIterator a, BasicIterator b) {
      return !(a == b);
    }
  };

public:
  using iterator = BasicIterator<false>;
  using const_iterator = BasicIterator<true>;
  using reverse_iterator = std::reverse_iterator<BasicIterator<false>>;
  using const_reverse_iterator = std::reverse_iterator<BasicIterator<true>>;

  iterator begin() noexcept { return iterator(fake_node_.next); }

  const_iterator cbegin() const noexcept {
    return const_iterator(fake_node_.next);
  }

  const_iterator begin() const noexcept { return cbegin(); }

  iterator end() noexcept { return iterator(&fake_node_); }

  const_iterator cend() const noexcept { return const_iterator(&fake_node_); }

  const_iterator end() const noexcept { return cend(); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(cend());
  }

  const_reverse_iterator rbegin() const noexcept { return crbegin(); }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(cbegin());
  }

  const_reverse_iterator rend() const noexcept { return crend(); }

  iterator insert(const_iterator pos, const T& value) {
    Node* n = create_node(value);
    const BaseNode* p = pos.ptr_;
    insert_node(p->prev, n);
    ++size_;
    return iterator(n);
  }

  iterator erase(const_iterator pos) {
    Node* node = remove_node(const_cast<BaseNode*>(pos.ptr_));
    iterator iter(node->next);
    destroy_node(node);
    --size_;
    return iter;
  }
};
