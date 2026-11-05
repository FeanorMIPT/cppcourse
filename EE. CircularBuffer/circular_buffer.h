#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

constexpr size_t DYNAMIC_CAPACITY = std::numeric_limits<size_t>::max();

template <typename T, size_t Capacity,
          bool Dynamic = (Capacity == DYNAMIC_CAPACITY)>
class BufferStorage {};

template <typename T, std::size_t Capacity>
class BufferStorage<T, Capacity, false> {
  alignas(T) std::array<std::byte, sizeof(T) * Capacity> buffer_;

public:
  BufferStorage() noexcept = default;

  BufferStorage(BufferStorage const&) noexcept = default;

  BufferStorage& operator=(BufferStorage const&) noexcept = default;

  size_t capacity() const noexcept { return Capacity; }

  T* data() noexcept { return reinterpret_cast<T*>(buffer_.data()); }

  const T* data() const noexcept {
    return reinterpret_cast<const T*>(buffer_.data());
  }

  void destroy_range(size_t begin, size_t size) noexcept {
    for (size_t i = 0; i < size; ++i) {
      size_t index = (begin + i) % Capacity;
      (data() + index)->~T();
    }
  }

  void swap(BufferStorage& other) noexcept {
    std::swap(buffer_, other.buffer_);
  }
};

template <typename T>
class BufferStorage<T, DYNAMIC_CAPACITY, true> {
  std::byte* buffer_ = nullptr;
  size_t capacity_ = 0;

public:
  BufferStorage() noexcept = default;

  BufferStorage(const BufferStorage& other) = delete;

  BufferStorage& operator=(const BufferStorage& other) = delete;

  explicit BufferStorage(size_t capacity)
      : buffer_(static_cast<std::byte*>(operator new(
            sizeof(T) * capacity, std::align_val_t(alignof(T))))),
        capacity_(capacity) {}

  ~BufferStorage() noexcept {
    operator delete(buffer_, std::align_val_t(alignof(T)));
  }

  size_t capacity() const noexcept { return this->capacity_; }

  T* data() noexcept { return reinterpret_cast<T*>(buffer_); }

  const T* data() const noexcept { return reinterpret_cast<const T*>(buffer_); }

  void destroy_range(size_t begin, size_t size) noexcept {
    for (size_t i = 0; i < size; ++i) {
      size_t index = (begin + i) % capacity_;
      (data() + index)->~T();
    }
  }

  void swap(BufferStorage& other) noexcept {
    std::swap(buffer_, other.buffer_);
    std::swap(capacity_, other.capacity_);
  }
};

template <typename T, size_t Capacity = DYNAMIC_CAPACITY>
class CircularBuffer {
  BufferStorage<T, Capacity> storage_;
  size_t begin_ = 0;
  size_t size_ = 0;

  size_t next(size_t index) const noexcept { return (index + 1) % capacity(); }

  size_t prev(size_t index) const noexcept {
    return (index + capacity() - 1) % capacity();
  }

  static BufferStorage<T, Capacity> make_storage(const CircularBuffer& buffer) {
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      return BufferStorage<T, Capacity>(buffer.capacity());
    } else {
      return BufferStorage<T, Capacity>();
    }
  }

public:
  explicit CircularBuffer(size_t capacity_)
    requires(Capacity == DYNAMIC_CAPACITY)
      : storage_(capacity_) {}

  CircularBuffer()
    requires(Capacity != DYNAMIC_CAPACITY)
  = default;

  explicit CircularBuffer(size_t cap)
    requires(Capacity != DYNAMIC_CAPACITY)
  {
    if (cap != Capacity) {
      throw std::invalid_argument{"Invalid capacity for static buffer"};
    }
  }

  CircularBuffer(const CircularBuffer& other)
      : storage_(make_storage(other)), begin_(other.begin_) {
    size_t idx = 0;
    try {
      for (; idx < other.size_; ++idx) {
        new (storage_.data() + (begin_ + idx) % capacity()) T(other[idx]);
        ++size_;
      }
    } catch (...) {
      storage_.destroy_range(begin_, size_);
      throw;
    }
  }

  CircularBuffer& operator=(const CircularBuffer& other) {
    if (this == &other) {
      return *this;
    }
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      BufferStorage<T, Capacity> newStorage = make_storage(other);
      size_t idx = 0;
      try {
        for (; idx < other.size_; ++idx)
          new (newStorage.data() + idx) T(other[idx]);
      } catch (...) {
        newStorage.destroy_range(0, idx);
        throw;
      }
      storage_.destroy_range(begin_, size_);
      storage_.swap(newStorage);
      begin_ = 0;
      size_ = other.size_;
      return *this;
    }
    storage_.destroy_range(begin_, size_);
    begin_ = 0;
    size_ = 0;
    size_t idx = 0;
    try {
      for (; idx < other.size_; ++idx)
        new (storage_.data() + idx) T(other[idx]);
    } catch (...) {
      storage_.destroy_range(0, idx);
      throw;
    }
    size_ = other.size_;
    return *this;
  }

  size_t capacity() const noexcept { return storage_.capacity(); }

  size_t size() const noexcept { return size_; }

  bool empty() const noexcept { return size_ == 0; }

  bool full() const noexcept { return size_ == capacity(); }

  T& operator[](size_t index) noexcept {
    return storage_.data()[(begin_ + index) % capacity()];
  }

  const T& operator[](size_t index) const noexcept {
    return storage_.data()[(begin_ + index) % capacity()];
  }

  T& at(size_t index) {
    if (index >= size_) {
      throw std::out_of_range("at: index out of range");
    }
    return storage_.data()[(begin_ + index) % capacity()];
  }

  const T& at(size_t index) const {
    if (index >= size_) {
      throw std::out_of_range("at: index out of range");
    }
    return storage_.data()[(begin_ + index) % capacity()];
  }

  void push_back(const T& value) {
    if (full()) {
      T temp(value);
      storage_.data()[begin_] = temp;
      begin_ = next(begin_);
    } else {
      size_t index = (begin_ + size_) % capacity();
      new (storage_.data() + index) T(value);
      ++size_;
    }
  }

  void push_front(const T& value) {
    if (full()) {
      T temp(value);
      size_t new_begin = prev(begin_);
      storage_.data()[new_begin] = temp;
      begin_ = new_begin;
    } else {
      size_t new_begin = prev(begin_);
      new (storage_.data() + new_begin) T(value);
      begin_ = new_begin;
      ++size_;
    }
  }

  void pop_back() {
    if (empty()) {
      throw std::out_of_range("pop_back: empty buffer");
    }
    (storage_.data() + (begin_ + size_ - 1) % capacity())->~T();
    --size_;
  }

  void pop_front() {
    if (empty()) {
      throw std::out_of_range("pop_front: empty buffer");
    }
    (storage_.data() + begin_)->~T();
    begin_ = (begin_ + 1) % capacity();
    --size_;
  }

  template <typename derived, bool IsConst, bool IsReversed>
  class base_iterator {
  public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer_type = std::conditional_t<IsConst, const T*, T*>;
    using reference_type = std::conditional_t<IsConst, const T&, T&>;
    using iterator_category = std::random_access_iterator_tag;

  protected:
    size_t index_ = 0;
    size_t begin_ = 0;
    size_t size_ = 0;
    pointer_type data_ = nullptr;
    base_iterator() = default;

    base_iterator(size_t index, size_t begin, size_t size, pointer_type data)
        : index_(index), begin_(begin), size_(size), data_(data) {}

  public:
    reference_type operator*() const noexcept {
      size_t cap = static_cast<const derived&>(*this).get_capacity();
      size_t real_index =
          (IsReversed ? (begin_ + size_ - 1 - index_) : (begin_ + index_)) %
          cap;
      return data_[real_index];
    }

    pointer_type operator->() const noexcept { return &(**this); }

    derived& operator++() noexcept {
      ++index_;
      return static_cast<derived&>(*this);
    }

    derived operator++(int) noexcept {
      derived copy = static_cast<derived&>(*this);
      ++*this;
      return copy;
    }

    derived& operator--() noexcept {
      --index_;
      return static_cast<derived&>(*this);
    }

    derived operator--(int) noexcept {
      derived copy = static_cast<derived&>(*this);
      --*this;
      return copy;
    }

    derived& operator+=(difference_type n) noexcept {
      index_ += n;
      return static_cast<derived&>(*this);
    }

    derived& operator-=(difference_type n) noexcept {
      index_ -= n;
      return static_cast<derived&>(*this);
    }

    reference_type operator[](difference_type n) const noexcept {
      return *(*this + n);
    }

    size_t index() const noexcept { return index_; }

    friend derived operator+(const derived& it, difference_type n) noexcept {
      derived copy = static_cast<const derived&>(it);
      return copy += n;
    }

    friend derived operator-(const derived& it, difference_type n) noexcept {
      derived copy = static_cast<const derived&>(it);
      return copy -= n;
    }

    friend derived operator+(difference_type n, const derived& it) noexcept {
      return it + n;
    }

    template <typename D1, bool C1>
    friend std::ptrdiff_t operator-(
        const base_iterator<derived, IsConst, IsReversed>& lhs,
        const base_iterator<D1, C1, IsReversed>& rhs) noexcept {
      return static_cast<std::ptrdiff_t>(lhs.index()) -
             static_cast<std::ptrdiff_t>(rhs.index());
    }

    template <typename D1, bool C1>
    friend bool operator==(
        const base_iterator<derived, IsConst, IsReversed>& lhs,
        const base_iterator<D1, C1, IsReversed>& rhs) noexcept {
      return lhs.index() == rhs.index();
    }

    template <typename D1, bool C1>
    friend bool operator!=(
        const base_iterator<derived, IsConst, IsReversed>& lhs,
        const base_iterator<D1, C1, IsReversed>& rhs) noexcept {
      return lhs.index() != rhs.index();
    }

    template <typename D1, bool C1>
    friend bool operator<(
        const base_iterator<derived, IsConst, IsReversed>& lhs,
        const base_iterator<D1, C1, IsReversed>& rhs) noexcept {
      return lhs.index() < rhs.index();
    }

    template <typename D1, bool C1>
    friend bool operator>(
        const base_iterator<derived, IsConst, IsReversed>& lhs,
        const base_iterator<D1, C1, IsReversed>& rhs) noexcept {
      return lhs.index() > rhs.index();
    }

    template <typename D1, bool C1>
    friend bool operator<=(
        const base_iterator<derived, IsConst, IsReversed>& lhs,
        const base_iterator<D1, C1, IsReversed>& rhs) noexcept {
      return lhs.index() <= rhs.index();
    }

    template <typename D1, bool C1>
    friend bool operator>=(
        const base_iterator<derived, IsConst, IsReversed>& lhs,
        const base_iterator<D1, C1, IsReversed>& rhs) noexcept {
      return lhs.index() >= rhs.index();
    }
  };
  template <size_t Cap, bool IsConst, bool IsReversed>
  class static_iterator
      : public base_iterator<static_iterator<Cap, IsConst, IsReversed>, IsConst,
                             IsReversed> {
  public:
    using base_type = base_iterator<static_iterator<Cap, IsConst, IsReversed>,
                                    IsConst, IsReversed>;

    using pointer_type = typename base_type::pointer_type;

    static_iterator(size_t index, size_t begin, size_t size,
                    pointer_type data) noexcept
        : base_type(index, begin, size, data) {}

    operator static_iterator<Cap, true, IsReversed>() const noexcept {
      return static_iterator<Cap, true, IsReversed>(this->index_, this->begin_,
                                                    this->size_, this->data_);
    }

    size_t get_capacity() const noexcept { return Cap; }
  };

  template <bool IsConst, bool IsReversed>
  class dynamic_iterator
      : public base_iterator<dynamic_iterator<IsConst, IsReversed>, IsConst,
                             IsReversed> {
    size_t cap_ = 0;

  public:
    using base_type = base_iterator<dynamic_iterator<IsConst, IsReversed>,
                                    IsConst, IsReversed>;

    using pointer_type = typename base_type::pointer_type;

    dynamic_iterator(size_t index, size_t begin, size_t size, pointer_type data,
                     size_t cap) noexcept
        : base_type(index, begin, size, data), cap_(cap) {}

    operator dynamic_iterator<true, IsReversed>() const noexcept {
      return dynamic_iterator<true, IsReversed>(
          this->index_, this->begin_, this->size_, this->data_, this->cap_);
    }

    size_t get_capacity() const noexcept { return cap_; }
  };

  using iterator = std::conditional_t<Capacity == DYNAMIC_CAPACITY,
                                      dynamic_iterator<false, false>,
                                      static_iterator<Capacity, false, false>>;

  using const_iterator =
      std::conditional_t<Capacity == DYNAMIC_CAPACITY,
                         dynamic_iterator<true, false>,
                         static_iterator<Capacity, true, false>>;

  using reverse_iterator =
      std::conditional_t<Capacity == DYNAMIC_CAPACITY,
                         dynamic_iterator<false, true>,
                         static_iterator<Capacity, false, true>>;

  using const_reverse_iterator =
      std::conditional_t<Capacity == DYNAMIC_CAPACITY,
                         dynamic_iterator<true, true>,
                         static_iterator<Capacity, true, true>>;

  template <typename Iterator>
  Iterator make_iterator(size_t index) const noexcept;

  iterator begin() noexcept { return make_iterator<iterator>(0); }

  const_iterator begin() const noexcept {
    return make_iterator<const_iterator>(0);
  }

  const_iterator cbegin() const noexcept {
    return make_iterator<const_iterator>(0);
  }

  iterator end() noexcept { return make_iterator<iterator>(size_); }

  const_iterator end() const noexcept {
    return make_iterator<const_iterator>(size_);
  }

  const_iterator cend() const noexcept {
    return make_iterator<const_iterator>(size_);
  }

  reverse_iterator rbegin() noexcept {
    return make_iterator<reverse_iterator>(0);
  }

  const_reverse_iterator rbegin() const noexcept {
    return make_iterator<const_reverse_iterator>(0);
  }

  const_reverse_iterator crbegin() const noexcept {
    return make_iterator<const_reverse_iterator>(0);
  }

  reverse_iterator rend() noexcept {
    return make_iterator<reverse_iterator>(size_);
  }

  const_reverse_iterator rend() const noexcept {
    return make_iterator<const_reverse_iterator>(size_);
  }

  const_reverse_iterator crend() const noexcept {
    return make_iterator<const_reverse_iterator>(size_);
  }

  template <size_t OtherCapacity>
  void swap(CircularBuffer<T, OtherCapacity>& other) noexcept(
      Capacity == DYNAMIC_CAPACITY) {
    static_assert(OtherCapacity == Capacity,
                  "Cannot swap buffers with different capacities");
    using std::swap;
    if constexpr (Capacity == DYNAMIC_CAPACITY) {
      storage_.swap(other.storage_);
    } else {
      const size_t max_sz = std::max(size_, other.size_);
      for (size_t i = 0; i < max_sz; ++i) {
        if (i < size_ and i < other.size_) {
          swap((*this)[i], other[i]);
        } else if (i < size_) {
          size_t dst = (other.begin_ + i) % Capacity;
          new (other.storage_.data() + dst) T((*this)[i]);
          (*this)[i].~T();
        } else if (i < other.size_) {
          size_t dst = (begin_ + i) % Capacity;
          new (storage_.data() + dst) T(other[i]);
          other[i].~T();
        }
      }
    }
    swap(begin_, other.begin_);
    swap(size_, other.size_);
  }

  friend void swap(CircularBuffer& a,
                   CircularBuffer& b) noexcept(Capacity == DYNAMIC_CAPACITY) {
    a.swap(b);
  }

  iterator insert(iterator pos, const T& value) {
    size_t idx = pos - begin();
    if (full()) {
      if (idx == 0) {
        return pos;
      }
      (storage_.data() + begin_)->~T();
      begin_ = next(begin_);
      --size_;
      --idx;
    }
    for (size_t i = size_; i > idx; --i) {
      size_t dst = (begin_ + i) % capacity();
      size_t src = (begin_ + i - 1) % capacity();
      if (i == size_) {
        new (storage_.data() + dst) T(*(storage_.data() + src));
      } else {
        *(storage_.data() + dst) = *(storage_.data() + src);
      }
    }
    size_t phys = (begin_ + idx) % capacity();
    if (idx < size_) {
      (storage_.data() + phys)->~T();
    }
    new (storage_.data() + phys) T(value);
    ++size_;
    return make_iterator<iterator>(idx);
  }

  iterator erase(const_iterator index) {
    size_t cur_index = index.index();
    for (size_t i = cur_index; i + 1 < size_; ++i) {
      T& dst = (*this)[i];
      T& src = (*this)[i + 1];
      dst.~T();
      new (&dst) T(src);
    }
    pop_back();
    return make_iterator<iterator>(cur_index);
  }

  ~CircularBuffer() { storage_.destroy_range(begin_, size_); }
};

template <typename T, size_t Capacity>
template <typename Iterator>
Iterator CircularBuffer<T, Capacity>::make_iterator(
    size_t index) const noexcept {
  using CleanIter = std::remove_cv_t<std::remove_reference_t<Iterator>>;
  if constexpr (Capacity == DYNAMIC_CAPACITY) {
    return CleanIter(
        index, begin_, size_,
        const_cast<typename CleanIter::pointer_type>(storage_.data()),
        capacity());
  } else {
    return CleanIter(
        index, begin_, size_,
        const_cast<typename CleanIter::pointer_type>(storage_.data()));
  }
}
