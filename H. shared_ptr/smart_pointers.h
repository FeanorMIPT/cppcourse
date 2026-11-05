#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

template <typename T> class SharedPtr;
template <typename T> class WeakPtr;
template <typename T> class EnableSharedFromThis;

namespace ControlBlocks {
struct BaseCB {
  size_t shared_count_ = 0;
  size_t weak_count_ = 0;
  virtual void destroy_value() = 0;
  virtual void* get_object() = 0;
  virtual void delete_cb() = 0;
  virtual ~BaseCB() = default;
};

template <typename U, typename Deleter = std::default_delete<U>,
          typename Alloc = std::allocator<std::byte>>
struct DefaultCB : BaseCB {
  using CbAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<DefaultCB>;
  using CbTraits = std::allocator_traits<CbAlloc>;

  U* raw_ptr_;
  [[no_unique_address]] Deleter deleter_;
  [[no_unique_address]] CbAlloc alloc_;

  DefaultCB(U* p, Deleter d, const Alloc& a)
      : raw_ptr_(p), deleter_(d), alloc_(a) {}

  void destroy_value() noexcept override { deleter_(raw_ptr_); }

  void* get_object() noexcept override { return raw_ptr_; }

  void delete_cb() noexcept override {
    CbAlloc alloc(alloc_);
    auto* copy = this;
    copy->~DefaultCB();
    alloc.deallocate(copy, 1);
  }

  ~DefaultCB() noexcept override = default;
};

template <typename U, typename Alloc = std::allocator<std::byte>>
struct MakeSharedCB : BaseCB {
  using ValueAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<U>;
  using ValueTraits = std::allocator_traits<ValueAlloc>;
  using CbAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<
      MakeSharedCB>;
  using CbTraits = std::allocator_traits<CbAlloc>;

  alignas(U) unsigned char value_storage_[sizeof(U)];
  [[no_unique_address]] Alloc alloc_;

  template <typename... Args>
  explicit MakeSharedCB(const Alloc& a, Args&&... args) : alloc_(a) {
    ValueAlloc alloc(alloc_);
    ValueTraits::construct(alloc, reinterpret_cast<U*>(&value_storage_),
                           std::forward<Args>(args)...);
  }

  void* get_object() noexcept override {
    return reinterpret_cast<U*>(&value_storage_);
  }

  void destroy_value() noexcept override {
    ValueAlloc alloc(alloc_);
    ValueTraits::destroy(alloc, reinterpret_cast<U*>(&value_storage_));
  }

  void delete_cb() noexcept override {
    CbAlloc alloc(alloc_);
    auto* copy = this;
    copy->~MakeSharedCB();
    alloc.deallocate(copy, 1);
  }

  ~MakeSharedCB() noexcept override = default;
};
}  // namespace ControlBlocks

template <typename T>
class SharedPtr {
  template <typename> friend class SharedPtr;
  template <typename> friend class WeakPtr;
  template <typename> friend class EnableSharedFromThis;
  T* ptr_ = nullptr;
  ControlBlocks::BaseCB* cb_ = nullptr;

  template <typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&... args);

  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc& alloc, Args&&... args);

public:
  SharedPtr() noexcept = default;

  template <typename U, typename Deleter = std::default_delete<U>,
            typename Alloc = std::allocator<std::byte>>
  explicit SharedPtr(U* ptr, Deleter deleter = std::default_delete<U>{},
                     const Alloc& alloc = std::allocator<std::byte>{}) {
    if (!ptr) {
      return;
    }
    using CB = ControlBlocks::DefaultCB<U, Deleter, Alloc>;
    using CbAlloc = typename CB::CbAlloc;
    CbAlloc alloc_(alloc);
    CB* control_block = alloc_.allocate(1);
    try {
      new (control_block) CB(ptr, deleter, alloc);
      cb_ = control_block;
      ptr_ = static_cast<T*>(ptr);
      ++cb_->shared_count_;
      if constexpr (std::is_base_of_v<EnableSharedFromThis<U>, U>) {
        ptr_->weak_esft_ = WeakPtr<U>(*this);
      }
    } catch (...) {
      alloc_.deallocate(control_block, 1);
      throw;
    }
  }

  SharedPtr(const SharedPtr& other) noexcept
      : ptr_(other.ptr_), cb_(other.cb_) {
    if (cb_) {
      ++cb_->shared_count_;
    }
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& other) noexcept
      : ptr_(other.ptr_), cb_(other.cb_) {
    if (cb_) {
      ++cb_->shared_count_;
    }
  }

  SharedPtr(SharedPtr&& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) {
    other.ptr_ = nullptr;
    other.cb_ = nullptr;
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) {
    other.ptr_ = nullptr;
    other.cb_ = nullptr;
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& owner, T* alias_ptr) noexcept
      : ptr_(alias_ptr), cb_(owner.cb_) {
    if (cb_) {
      ++cb_->shared_count_;
    }
  }

  SharedPtr& operator=(const SharedPtr& other) noexcept {
    if (this == &other) {
      return *this;
    }
    SharedPtr copy(other);
    swap(copy);
    return *this;
  }

  template <typename U>
  SharedPtr& operator=(const SharedPtr<U>& other) noexcept {
    if (cb_ == other.cb_ and ptr_ == other.ptr_) {
      return *this;
    }
    SharedPtr copy(other);
    swap(copy);
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    if (cb_ == other.cb_) {
      ptr_ = other.ptr_;
      if (cb_) {
        --cb_->shared_count_;
      }
      other.ptr_ = nullptr;
      other.cb_ = nullptr;
      return *this;
    }
    reset();
    ptr_ = other.ptr_;
    cb_ = other.cb_;
    other.ptr_ = nullptr;
    other.cb_ = nullptr;
    return *this;
  }

  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& other) noexcept {
    if (cb_ == other.cb_) {
      ptr_ = static_cast<T*>(other.ptr_);
      if (cb_) {
        --cb_->shared_count_;
      }
      other.ptr_ = nullptr;
      other.cb_ = nullptr;
      return *this;
    }
    reset();
    ptr_ = other.ptr_;
    cb_ = other.cb_;
    other.ptr_ = nullptr;
    other.cb_ = nullptr;
    return *this;
  }

  size_t use_count() const noexcept { return cb_ ? cb_->shared_count_ : 0; }

  explicit operator bool() const noexcept { return ptr_ != nullptr; }

  T& operator*() const noexcept { return *ptr_; }

  T* operator->() const noexcept { return ptr_; }

  T* get() const noexcept { return ptr_; }

  void reset() noexcept {
    if (!cb_) {
      return;
    }
    if ((cb_->shared_count_)-- == 1) {
      ++cb_->weak_count_;
      cb_->destroy_value();
      if (--cb_->weak_count_ == 0) {
        cb_->delete_cb();
      }
    }
    ptr_ = nullptr;
    cb_ = nullptr;
  }

  template <typename U>
  void reset(U* new_ptr) {
    SharedPtr tmp(new_ptr);
    swap(tmp);
  }

  void swap(SharedPtr& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(cb_, other.cb_);
  }

  ~SharedPtr() { reset(); }
};

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  using CB = ControlBlocks::MakeSharedCB<T>;
  typename CB::CbAlloc CbAlloc{};
  CB* control_block = CB::CbTraits::allocate(CbAlloc, 1);
  try {
    new (control_block)
        CB(std::allocator<std::byte>{}, std::forward<Args>(args)...);
  } catch (...) {
    CB::CbTraits::deallocate(CbAlloc, control_block, 1);
    throw;
  }
  SharedPtr<T> shared_ptr;
  shared_ptr.ptr_ = static_cast<T*>(control_block->get_object());
  shared_ptr.cb_ = control_block;
  ++shared_ptr.cb_->shared_count_;
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    shared_ptr.ptr_->weak_esft_ = WeakPtr<T>(shared_ptr);
  }
  return shared_ptr;
}

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  using CB = ControlBlocks::MakeSharedCB<T, Alloc>;
  typename CB::CbAlloc CbAlloc(alloc);
  CB* control_block = CB::CbTraits::allocate(CbAlloc, 1);
  try {
    new (control_block) CB(alloc, std::forward<Args>(args)...);
  } catch (...) {
    CB::CbTraits::deallocate(CbAlloc, control_block, 1);
    throw;
  }
  SharedPtr<T> shared_ptr;
  shared_ptr.ptr_ = static_cast<T*>(control_block->get_object());
  shared_ptr.cb_ = control_block;
  ++shared_ptr.cb_->shared_count_;
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    shared_ptr.ptr_->weak_esft_ = WeakPtr<T>(shared_ptr);
  }
  return shared_ptr;
}

template <typename T>
class WeakPtr {
  template <typename> friend class WeakPtr;
  template <typename> friend class SharedPtr;
  template <typename> friend class EnableSharedFromThis;

  T* weak_ptr_ = nullptr;
  ControlBlocks::BaseCB* weak_cb_ = nullptr;

public:
  WeakPtr() noexcept = default;

  template <typename U>
  WeakPtr(const SharedPtr<U>& shared_ptr) noexcept
      : weak_ptr_(shared_ptr.ptr_), weak_cb_(shared_ptr.cb_) {
    if (weak_cb_) {
      ++weak_cb_->weak_count_;
    }
  }

  WeakPtr(const WeakPtr& other) noexcept
      : weak_ptr_(other.weak_ptr_), weak_cb_(other.weak_cb_) {
    if (weak_cb_) {
      ++weak_cb_->weak_count_;
    }
  }

  template <typename U>
  WeakPtr(const WeakPtr<U>& other) noexcept
      : weak_ptr_(other.weak_ptr_), weak_cb_(other.weak_cb_) {
    if (weak_cb_) {
      ++weak_cb_->weak_count_;
    }
  }

  WeakPtr(WeakPtr&& other) noexcept
      : weak_ptr_(other.weak_ptr_), weak_cb_(other.weak_cb_) {
    other.weak_ptr_ = nullptr;
    other.weak_cb_ = nullptr;
  }

  template <typename U>
  WeakPtr(WeakPtr<U>&& other) noexcept
      : weak_ptr_(other.weak_ptr_), weak_cb_(other.weak_cb_) {
    other.weak_ptr_ = nullptr;
    other.weak_cb_ = nullptr;
  }

  WeakPtr& operator=(const WeakPtr& other) noexcept {
    WeakPtr copy(other);
    swap(copy);
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& other) noexcept {
    WeakPtr copy(other);
    swap(copy);
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) noexcept {
    if (weak_cb_ == other.weak_cb_) {
      return *this;
    }
    reset();
    weak_ptr_ = other.weak_ptr_;
    weak_cb_ = other.weak_cb_;
    other.weak_ptr_ = nullptr;
    other.weak_cb_ = nullptr;
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& other) noexcept {
    if (weak_cb_ == other.weak_cb_) {
      return *this;
    }
    reset();
    weak_ptr_ = other.weak_ptr_;
    weak_cb_ = other.weak_cb_;
    other.weak_ptr_ = nullptr;
    other.weak_cb_ = nullptr;
    return *this;
  }

  template <typename U>
  WeakPtr& operator=(const SharedPtr<U>& shared_ptr) noexcept {
    WeakPtr copy(shared_ptr);
    swap(copy);
    return *this;
  }

  void reset() noexcept {
    if (!weak_cb_) {
      return;
    }
    if (--weak_cb_->weak_count_ == 0 and weak_cb_->shared_count_ == 0) {
      weak_cb_->delete_cb();
    }
    weak_cb_ = nullptr;
    weak_ptr_ = nullptr;
  }

  std::size_t use_count() const noexcept {
    return weak_cb_ ? weak_cb_->shared_count_ : 0;
  }

  bool expired() const noexcept {
    return !weak_cb_ or weak_cb_->shared_count_ == 0;
  }

  SharedPtr<T> lock() const noexcept {
    if (expired()) {
      return SharedPtr<T>();
    }
    SharedPtr<T> shared_ptr;
    shared_ptr.ptr_ = weak_ptr_;
    shared_ptr.cb_ = weak_cb_;
    ++shared_ptr.cb_->shared_count_;
    return shared_ptr;
  }

  void swap(WeakPtr& other) noexcept {
    std::swap(weak_cb_, other.weak_cb_);
    std::swap(weak_ptr_, other.weak_ptr_);
  }

  ~WeakPtr() { reset(); }
};

template <typename T>
class EnableSharedFromThis {
  template <typename> friend class SharedPtr;
  template <typename> friend class WeakPtr;
  template <typename> friend class EnableSharedFromThis;
  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc& alloc, Args&&... args);
  template <typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&... args);

  WeakPtr<T> weak_esft_;

public:
  EnableSharedFromThis() noexcept = default;
  EnableSharedFromThis(const EnableSharedFromThis&) noexcept {}
  EnableSharedFromThis& operator=(const EnableSharedFromThis&) noexcept {
    return *this;
  }
  ~EnableSharedFromThis() = default;

  SharedPtr<T> shared_from_this() {
    SharedPtr<T> shared_ptr = weak_esft_.lock();
    if (!shared_ptr) {
      throw std::bad_weak_ptr{};
    }
    return shared_ptr;
  }

  SharedPtr<const T> shared_from_this() const {
    SharedPtr<const T> shared_ptr = weak_esft_.lock();
    if (!shared_ptr)
      throw std::bad_weak_ptr{};
    return shared_ptr;
  }
};
