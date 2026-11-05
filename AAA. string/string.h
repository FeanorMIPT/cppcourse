#include <cstring>
#include <iostream>

class String {
  char* str = nullptr;
  size_t sz = 0;
  size_t cap = 1;

  void Swap(String& other) {
    std::swap(str, other.str);
    std::swap(sz, other.sz);
    std::swap(cap, other.cap);
  }

public:
  explicit String(size_t size)
      : str(new char[size + 1]), sz(size), cap(size + 1) {}

  String() : str(new char[1]), sz(0), cap(1) { str[0] = '\0'; }

  String(const char a) : str(new char[2]), sz(1), cap(2) {
    str[0] = a;
    str[1] = '\0';
  }

  String(const char* c)
      : str(new char[strlen(c) + 1]), sz(strlen(c)), cap(strlen(c) + 1) {
    memcpy(str, c, sz);
    str[sz] = '\0';
  }

  String(size_t n, char c) : String(n) {
    memset(str, c, n);
    str[sz] = '\0';
  }

  String(const String& other) : String(other.sz) {
    memcpy(str, other.str, sz + 1);
  }

  String& operator=(const String& other) {
    if (str == other.str) {
      return *this;
    }
    if (cap <= other.sz) {
      String copy = other;
      Swap(copy);
      return *this;
    }
    memcpy(str, other.str, other.sz + 1);
    sz = other.sz;
    return *this;
  }

  char& operator[](size_t index) { return str[index]; }

  const char& operator[](size_t index) const { return str[index]; }

  size_t length() const { return sz; }

  size_t size() const { return sz; }

  size_t capacity() const { return cap - 1; }

  char* data() { return str; }

  const char* data() const { return str; }

  bool empty() const { return sz == 0; }

  void clear() {
    sz = 0;
    str[0] = '\0';
  }

  void shrink_to_fit() {
    String copy = *this;
    Swap(copy);
  }

  const char& front() const { return str[0]; }

  char& front() { return str[0]; }

  const char& back() const { return str[sz - 1]; }

  char& back() { return str[sz - 1]; }

  String& operator+=(const String& other) {
    if (cap >= sz + other.sz + 1) {
      memcpy(str + (sizeof(char) * sz), other.str, other.sz + 1);
      sz += other.sz;
      return *this;
    }
    String res = String(sz + other.sz);
    memcpy(res.str, str, sz);
    memcpy(res.str + (sizeof(char) * sz), other.str, other.sz + 1);
    Swap(res);
    return *this;
  }

  String& operator+=(char c) {
    if (cap > sz + 1) {
      str[sz] = c;
      str[sz + 1] = '\0';
      ++sz;
    } else {
      String res = String(2 * cap);
      res.sz = sz + 1;
      memcpy(res.str, str, sz);
      res.str[sz] = c;
      res.str[res.sz] = '\0';
      Swap(res);
    }
    return *this;
  }

  void push_back(char c) { *this += c; }

  void pop_back() {
    if (sz > 0) {
      --sz;
      str[sz] = '\0';
    }
  }

  size_t find(const String& substr) const {
    if (substr.sz == 0 or sz < substr.sz) {
      return sz;
    }
    bool flag = false;
    for (size_t i = 0; i <= sz - substr.sz; i++) {
      flag = memcmp(str + sizeof(char) * i, substr.str, substr.sz) == 0;
      if (flag) {
        return i;
      }
    }
    return sz;
  }

  size_t rfind(const String& substr) const {
    if (substr.sz == 0 || sz < substr.sz) {
      return sz;
    }
    bool flag = false;
    for (int i = sz - substr.sz; i >= 0; i--) {
      flag = memcmp(str + sizeof(char) * i, substr.str, substr.sz) == 0;
      if (flag) {
        return i;
      }
    }
    return sz;
  }

  String substr(size_t start, size_t count) const {
    String res = String(count);
    memcpy(res.str, str + sizeof(char) * start, count);
    res.str[count] = '\0';
    return res;
  }

  ~String() { delete[] str; }
};

String operator+(const String& first, const String& second) {
  String copy = first;
  copy += second;
  return copy;
}

bool operator==(const String& first, const String& second) {
  if (first.size() != second.size()) {
    return false;
  }
  if (first.data() == second.data()) {
    return true;
  }
  return memcmp(first.data(), second.data(), first.size()) == 0;
}

bool operator!=(const String& first, const String& second) {
  return !(first == second);
}

bool operator<(const String& first, const String& second) {
  if (first.data() == second.data()) {
    return false;
  }
  size_t border = std::min(first.size(), second.size());
  int val = memcmp(first.data(), second.data(), sizeof(char) * border);
  return val < 0 or (val == 0 and first.size() < second.size());
}

bool operator<=(const String& first, const String& second) {
  return first < second or first == second;
}

bool operator>(const String& first, const String& second) {
  return !(first <= second);
}

bool operator>=(const String& first, const String& second) {
  return first > second or first == second;
}

std::istream& operator>>(std::istream& in, String& string) {
  string.clear();
  char next_symbol;
  next_symbol = in.peek();
  while (isspace(next_symbol) && next_symbol != EOF) {
    in.get();
    next_symbol = in.peek();
  }
  while (!isspace(next_symbol) && next_symbol != EOF) {
    in.get(next_symbol);
    string += next_symbol;
    next_symbol = in.peek();
  }
  return in;
}

std::ostream& operator<<(std::ostream& out, const String& string) {
  for (size_t i = 0; i < string.size(); i++) {
    out << string.data()[i];
  }
  return out;
}