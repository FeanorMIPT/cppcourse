#pragma once
#include <cstddef>
#include <memory>
#include <vector>
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
    T value;
    Node() = default;

    Node(const T& value) : value(value) {}

    Node(T&& value) : value(std::move(value)) {}

    template <typename... Args>
    explicit Node(Args&&... args) : value(std::forward<Args>(args)...) {}
  };
  using NodeAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
  using NodeAllocTraits = std::allocator_traits<NodeAlloc>;

  BaseNode fake_node_;
  size_t size_ = 0;
  [[no_unique_address]] NodeAlloc alloc_{};

  template <typename... Args>
  Node* create_node(Args&&... args) {
    Node* node = NodeAllocTraits::allocate(alloc_, 1);
    try {
      NodeAllocTraits::construct(alloc_, node, std::forward<Args>(args)...);
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

  List(List&& other) noexcept {
    alloc_ = std::move(other.alloc_);
    if (!other.empty()) {
      fake_node_.next = other.fake_node_.next;
      fake_node_.prev = other.fake_node_.prev;
      fake_node_.next->prev = &fake_node_;
      fake_node_.prev->next = &fake_node_;
      size_ = other.size_;
    } else {
      fake_node_.next = &fake_node_;
      fake_node_.prev = &fake_node_;
      size_ = 0;
    }
    other.fake_node_.next = &other.fake_node_;
    other.fake_node_.prev = &other.fake_node_;
    other.size_ = 0;
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

  List& operator=(List&& other) noexcept(
      NodeAllocTraits::propagate_on_container_move_assignment::value or
      NodeAllocTraits::is_always_equal::value) {
    if (this == &other) {
      return *this;
    }
    clear();
    if constexpr (NodeAllocTraits::propagate_on_container_move_assignment::
                      value or
                  NodeAllocTraits::is_always_equal::value) {
      alloc_ = std::move(other.alloc_);
      if (!other.empty()) {
        fake_node_.next = other.fake_node_.next;
        fake_node_.prev = other.fake_node_.prev;
        fake_node_.next->prev = &fake_node_;
        fake_node_.prev->next = &fake_node_;
        size_ = other.size_;
      } else {
        fake_node_.next = &fake_node_;
        fake_node_.prev = &fake_node_;
        size_ = 0;
      }
      other.fake_node_.next = &other.fake_node_;
      other.fake_node_.prev = &other.fake_node_;
      other.size_ = 0;
      return *this;
    }
    clear();
    while (!other.empty()) {
      emplace_back(std::move(other.front()));
      other.pop_front();
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

  template <typename... Args>
  reference emplace_front(Args&&... args) {
    Node* n = create_node(std::forward<Args>(args)...);
    insert_node(&fake_node_, n);
    ++size_;
    return n->value;
  }

  template <typename... Args>
  reference emplace_back(Args&&... args) {
    Node* n = create_node(std::forward<Args>(args)...);
    insert_node(fake_node_.prev, n);
    ++size_;
    return n->value;
  }

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
    while (!empty()) {
      pop_back();
    }
  }

  reference front() { return static_cast<Node*>(fake_node_.next)->value; }

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
  void splice_after(iterator pos, iterator it) noexcept {
    BaseNode* n = const_cast<BaseNode*>(it.ptr_);
    BaseNode* p = const_cast<BaseNode*>(pos.ptr_);
    if (n == &fake_node_) {
      return;
    }
    if (n == p || n == p->next) {
      return;
    }
    BaseNode* n_prev = n->prev;
    BaseNode* n_next = n->next;
    n_prev->next = n_next;
    n_next->prev = n_prev;
    BaseNode* p_next = p->next;
    n->prev = p;
    n->next = p_next;
    p->next = n;
    p_next->prev = n;
  }
};

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
public:
  using key_type = Key;
  using mapped_type = Value;
  using NodeType = std::pair<const Key, Value>;
  using hasher = Hash;
  using key_equal = Equal;
  using allocator_type = Alloc;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using value_type = NodeType;
  using reference = value_type&;
  using const_reference = const value_type&;

private:
  struct Node {
    using PairAlloc =
        typename std::allocator_traits<Alloc>::template rebind_alloc<NodeType>;
    NodeType* kv_ = nullptr;
    size_t hash_;
    [[no_unique_address]] PairAlloc pa;

    template <typename... Args>
    Node(const Alloc& a, size_t hash, Args&&... args) : hash_(hash), pa(a) {
      kv_ = std::allocator_traits<PairAlloc>::allocate(pa, 1);
      try {
        std::allocator_traits<PairAlloc>::construct(
            pa, kv_, std::forward<Args>(args)...);
      } catch (...) {
        std::allocator_traits<PairAlloc>::deallocate(pa, kv_, 1);
        throw;
      }
    }

    Node(Node&& other) noexcept
        : kv_(other.kv_), hash_(other.hash_), pa(std::move(other.pa)) {
      other.kv_ = nullptr;
    }

    Node& operator=(Node&& other) noexcept {
      if (this == &other) {
        return *this;
      }
      if (kv_) {
        std::allocator_traits<PairAlloc>::destroy(pa, kv_);
        std::allocator_traits<PairAlloc>::deallocate(pa, kv_, 1);
      }
      kv_ = other.kv_;
      hash_ = other.hash_;
      pa = std::move(other.pa);
      other.kv_ = nullptr;
      return *this;
    }

    ~Node() {
      if (kv_) {
        std::allocator_traits<PairAlloc>::destroy(pa, kv_);
        std::allocator_traits<PairAlloc>::deallocate(pa, kv_, 1);
      }
    }
    NodeType& kv() noexcept { return *kv_; }

    const NodeType& kv() const noexcept { return *kv_; }
  };

  using NodeAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
  using ListType = List<Node, NodeAlloc>;
  using ListIter = typename ListType::iterator;
  using ListConstIter = typename ListType::const_iterator;

  ListType node_list_;
  std::vector<ListIter> buckets_;
  size_t size_ = 0;
  double max_load_factor_ = 1.0;
  [[no_unique_address]] hasher hash_;
  [[no_unique_address]] key_equal eq_;
  [[no_unique_address]] allocator_type alloc_;

  size_t bucket_count() const noexcept { return buckets_.size(); }

  size_t index_by_hash(std::size_t hash) const noexcept {
    return bucket_count() ? (hash % bucket_count()) : 0;
  }

  size_t index_by_key(const key_type& k) const {
    return index_by_hash(hash_(k));
  }

  template <typename It>
  It next_it(It it) noexcept {
    if (it == node_list_.end()) {
      return node_list_.begin();
    }
    ++it;
    return it;
  }

  template <typename It>
  It next_it(It it) const noexcept {
    if (it == node_list_.end()) {
      return node_list_.begin();
    }
    ++it;
    return it;
  }
  // если не дублировать код а писать через шаблоны, то, как я понял, нужно
  // будет объявлять функцию вообще 3 раза сначала базовую шаблонную, а потом
  // две версии - константную и нет, в которой просто вызов базовой, нельзя
  // же(?) const в сигнатуре функции менять в зависимости от шаблона. Функции в
  // пару строчек, поэтому решил что копипаста не сильно повредит и это лучше
  // чем писать как я описал выше.

  ListIter find_in_bucket(size_t i, const key_type& key) {
    ListIter it = buckets_[i];
    while (it != node_list_.end() and index_by_hash(it->hash_) == i) {
      if (eq_(it->kv().first, key)) {
        return it;
      }
      ++it;
    }
    return node_list_.end();
  }

  ListConstIter find_in_bucket(size_t i, const key_type& key) const {
    ListConstIter it = buckets_[i];
    while (it != node_list_.end() && index_by_hash(it->hash_) == i) {
      if (eq_(it->kv().first, key)) {
        return it;
      }
      ++it;
    }
    return node_list_.end();
  }

  void rehash_relink(size_t size) {
    if (size == bucket_count()) {
      std::vector<ListIter> new_buckets(size, node_list_.end());
      for (ListIter it = node_list_.begin(); it != node_list_.end(); ++it) {
        size_t i = it->hash_ % size;
        if (new_buckets[i] == node_list_.end()) {
          new_buckets[i] = it;
        }
      }
      buckets_.swap(new_buckets);
      return;
    }
    if (size == 0) {
      size = 1;
    }
    std::vector<ListIter> new_buckets(size, node_list_.end());
    ListIter it = node_list_.begin();
    while (it != node_list_.end()) {
      ListIter cur = it;
      ++it;
      size_t i = cur->hash_ % size;
      if (new_buckets[i] == node_list_.end()) {
        node_list_.splice_after(--node_list_.end(), cur);
        new_buckets[i] = cur;
      } else {
        node_list_.splice_after(new_buckets[i], cur);
      }
    }
    buckets_.swap(new_buckets);
  }

public:
  UnorderedMap() : buckets_(7, node_list_.end()) {}

  explicit UnorderedMap(size_t bucket_count, const Hash& hash = Hash(),
                        const Equal& equal = Equal(),
                        const Alloc& alloc = Alloc())
      : buckets_(bucket_count ? bucket_count : 1, node_list_.end()),
        max_load_factor_(1.0),
        hash_(hash),
        eq_(equal),
        alloc_(alloc) {}

  UnorderedMap(const UnorderedMap& other)
      : buckets_(other.bucket_count(), node_list_.end()),
        max_load_factor_(other.max_load_factor_),
        hash_(other.hash_),
        eq_(other.eq_),
        alloc_(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(
                other.alloc_)) {
    for (ListConstIter it = other.node_list_.begin();
         it != other.node_list_.end(); ++it) {
      emplace(it->kv().first, it->kv().second);
    }
  }

  UnorderedMap(UnorderedMap&& other) noexcept
      : node_list_(std::move(other.node_list_)),
        buckets_(std::move(other.buckets_)),
        size_(other.size_),
        max_load_factor_(other.max_load_factor_),
        hash_(std::move(other.hash_)),
        eq_(std::move(other.eq_)),
        alloc_(std::move(other.alloc_)) {
    rehash_relink(bucket_count());

    other.buckets_.assign(7, other.node_list_.end());
    other.size_ = 0;
  }

  UnorderedMap& operator=(const UnorderedMap& other) {
    if (this == &other) {
      return *this;
    }
    if constexpr (std::allocator_traits<
                      Alloc>::propagate_on_container_copy_assignment::value) {
      if (alloc_ != other.alloc_) {
        clear();
      }
      alloc_ = other.alloc_;
      UnorderedMap tmp(other);
      swap(tmp);
      return *this;
    }
    if (std::allocator_traits<Alloc>::is_always_equal::value ||
        alloc_ == other.alloc_) {
      UnorderedMap tmp(other);
      swap(tmp);
      return *this;
    }
    UnorderedMap tmp(other.bucket_count(), hash_, eq_, alloc_);
    tmp.max_load_factor_ = other.max_load_factor_;
    tmp.reserve(other.size());
    for (auto it = other.begin(); it != other.end(); ++it) {
      tmp.emplace(it->first, it->second);
    }
    swap(tmp);
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) noexcept(
      std::allocator_traits<
          Alloc>::propagate_on_container_move_assignment::value) {
    if (this == &other) {
      return *this;
    }
    clear();
    if constexpr (std::allocator_traits<
                      Alloc>::propagate_on_container_move_assignment::value) {
      alloc_ = std::move(other.alloc_);
    }
    buckets_ = std::move(other.buckets_);
    node_list_ = std::move(other.node_list_);
    size_ = other.size_;
    max_load_factor_ = other.max_load_factor_;
    hash_ = std::move(other.hash_);
    eq_ = std::move(other.eq_);
    rehash_relink(bucket_count());
    other.buckets_.assign(7, other.node_list_.end());
    other.size_ = 0;
    return *this;
  }

  ~UnorderedMap() = default;

private:
  template <bool IsConst>
  class basic_iterator {
    using RawIter = std::conditional_t<IsConst, ListConstIter, ListIter>;
    RawIter it_{};

    explicit basic_iterator(RawIter it) : it_(it) {}
    friend class UnorderedMap;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = UnorderedMap::value_type;
    using difference_type = UnorderedMap::difference_type;
    using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;
    using reference =
        std::conditional_t<IsConst, const value_type&, value_type&>;
    basic_iterator() = default;

    template <bool B = IsConst, typename = std::enable_if_t<B>>
    explicit basic_iterator(const basic_iterator<false>& other)
        : it_(other.it_) {}

    reference operator*() const { return it_->kv(); }

    pointer operator->() const { return std::addressof(it_->kv()); }

    basic_iterator& operator++() {
      ++it_;
      return *this;
    }

    basic_iterator operator++(int) {
      basic_iterator copy = *this;
      ++(*this);
      return copy;
    }

    friend bool operator==(basic_iterator a, basic_iterator b) {
      return a.it_ == b.it_;
    }

    friend bool operator!=(basic_iterator a, basic_iterator b) {
      return !(a == b);
    }

    RawIter base() const { return it_; }
  };

public:
  using iterator = basic_iterator<false>;
  using const_iterator = basic_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() noexcept { return iterator(node_list_.begin()); }

  iterator end() noexcept { return iterator(node_list_.end()); }

  const_iterator begin() const noexcept {
    return const_iterator(node_list_.begin());
  }

  const_iterator end() const noexcept {
    return const_iterator(node_list_.end());
  }

  const_iterator cbegin() const noexcept {
    return const_iterator(node_list_.begin());
  }

  const_iterator cend() const noexcept {
    return const_iterator(node_list_.end());
  }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  bool empty() const noexcept { return size_ == 0; }

  size_t size() const noexcept { return size_; }

  double load_factor() const noexcept {
    size_t m = bucket_count();
    return m ? static_cast<double>(size_) / static_cast<double>(m) : 0.0;
  }
  double max_load_factor() const noexcept { return max_load_factor_; }

  void max_load_factor(double x) noexcept {
    max_load_factor_ = (x > 0.0) ? x : 1.0;
  }

  const mapped_type& at(const key_type& key) const {
    size_t i = index_by_key(key);
    ListConstIter it = find_in_bucket(i, key);
    if (it == node_list_.end()) {
      throw std::out_of_range("UnorderedMap::at");
    }
    return it->kv().second;
  }

  mapped_type& at(const key_type& key) {
    return const_cast<mapped_type&>(
        const_cast<const UnorderedMap&>(*this).at(key));
  }

  mapped_type& operator[](const key_type& key) {
    std::pair<iterator, bool> res = emplace(key, mapped_type());
    iterator it = res.first;
    return (*it).second;
  }

  mapped_type& operator[](key_type&& key) {
    std::pair<iterator, bool> res = emplace(std::move(key), mapped_type());
    iterator it = res.first;
    return (*it).second;
  }

  iterator find(const key_type& key) {
    size_t i = index_by_key(key);
    ListIter it = find_in_bucket(i, key);
    return (it == node_list_.end()) ? end() : iterator(it);
  }

  const_iterator find(const key_type& key) const {
    size_t i = index_by_key(key);
    ListConstIter it = find_in_bucket(i, key);
    return (it == node_list_.end()) ? cend() : const_iterator(it);
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    node_list_.emplace_front(alloc_, 0, std::forward<Args>(args)...);
    ListIter new_it = node_list_.begin();
    try {
      new_it->hash_ = hash_(new_it->kv().first);
    } catch (...) {
      node_list_.erase(new_it);
      throw;
    }
    size_t i = index_by_hash(new_it->hash_);
    if (buckets_[i] != node_list_.end()) {
      ListIter exist = find_in_bucket(i, new_it->kv().first);
      if (exist != node_list_.end()) {
        node_list_.erase(new_it);
        return {iterator(exist), false};
      }
      node_list_.splice_after(buckets_[i], new_it);
    } else {
      buckets_[i] = new_it;
    }
    ++size_;
    if (size_ > static_cast<size_t>(max_load_factor_ * bucket_count())) {
      rehash_relink(bucket_count() * 3 + 1);
    }
    return {iterator(new_it), true};
  }

  std::pair<iterator, bool> insert(const value_type& v) { return emplace(v); }

  std::pair<iterator, bool> insert(value_type&& v) {
    return emplace(std::move(v));
  }

  template <typename U>
  std::pair<iterator, bool> insert(U&& u) {
    return emplace(std::forward<U>(u));
  }

  template <typename InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      auto&& v = *first;
      insert(std::forward<decltype(v)>(v));
    }
  }

  iterator erase(iterator pos) {
    if (pos == end()) {
      return end();
    }
    size_t i = index_by_hash(hash_((*pos).first));
    ListIter It = find_in_bucket(i, (*pos).first);
    if (It == node_list_.end()) {
      return end();
    }
    ListIter next_it = It;
    ++next_it;
    if (buckets_[i] == It) {
      if (next_it != node_list_.end() and index_by_hash(next_it->hash_) == i) {
        buckets_[i] = next_it;
      } else {
        buckets_[i] = node_list_.end();
      }
    }
    node_list_.erase(It);
    --size_;
    return iterator(next_it);
  }

  void erase(iterator first, iterator last) {
    while (first != last) {
      first = erase(first);
    }
  }

  void reserve(size_t n) {
    size_t needed =
        static_cast<size_t>(static_cast<double>(n) / max_load_factor_) + 10;
    if (needed > bucket_count()) {
      rehash_relink(needed);
    }
  }

  void rehash(size_t size) {
    if (size == 0) {
      size = 1;
    }
    if (size == bucket_count()) {
      return;
    }
    rehash_relink(size);
  }

  void swap(UnorderedMap& other) noexcept(
      std::allocator_traits<Alloc>::propagate_on_container_swap::value) {
    if constexpr (std::allocator_traits<
                      Alloc>::propagate_on_container_swap::value) {
      std::swap(alloc_, other.alloc_);
    }
    buckets_.swap(other.buckets_);
    node_list_.swap(other.node_list_);
    std::swap(size_, other.size_);
    std::swap(max_load_factor_, other.max_load_factor_);
    std::swap(hash_, other.hash_);
    std::swap(eq_, other.eq_);
  }

  void clear() noexcept {
    node_list_.clear();
    size_ = 0;
    size_t m = bucket_count();
    for (size_t i = 0; i < m; ++i) {
      buckets_[i] = node_list_.end();
    }
  }
};
