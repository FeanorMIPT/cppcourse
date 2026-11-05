#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const int cBase = 1e9;

class BigInteger {
  std::vector<long long> digits;
  bool isNeg = false;

  void delete_leading_zeroes() {
    while (digits.size() > 1 and digits.back() == 0) {
      digits.pop_back();
    }
    if (isNull()) {
      isNeg = false;
    }
  }

public:
  BigInteger() = default;

  BigInteger(const BigInteger& other) = default;

  BigInteger(const std::string& str) {
    size_t first_significant_digit = 0;
    digits.clear();
    isNeg = false;
    if (str[0] == '-') {
      isNeg = true;
      ++first_significant_digit;
    }
    while (first_significant_digit != str.length() and
           str[first_significant_digit] == '0') {
      ++first_significant_digit;
    }
    if (first_significant_digit == str.length()) {
      *this = 0;
      isNeg = false;
    }
    long long digit = 0;
    int i = static_cast<int>(str.length() - 1);
    while (i - 8 >= static_cast<int>(first_significant_digit)) {
      digit = std::stoll(str.substr(i - 8, 9));
      digits.push_back(digit);
      i -= 9;
    }
    if (i >= static_cast<int>(first_significant_digit)) {
      digit = std::stoll(
          str.substr(first_significant_digit, i + 1 - first_significant_digit));
      digits.push_back(digit);
    }
    delete_leading_zeroes();
  }

  BigInteger(const long long& x) {
    isNeg = x < 0;
    long long copy = llabs(x);
    if (copy == 0) {
      digits.push_back(0);
    }
    while (copy > 0) {
      digits.push_back(copy % cBase);
      copy -= copy % cBase;
      copy /= cBase;
    }
    delete_leading_zeroes();
  }

  bool is_negative() const { return isNeg; }

  bool isNull() const {
    bool flag = true;
    for (long long digit : digits) {
      if (digit != 0) {
        flag = false;
        break;
      }
    }
    return flag;
  }

  explicit operator bool() const { return !isNull(); }

  void Swap(BigInteger& other) {
    std::swap(isNeg, other.isNeg);
    digits.swap(other.digits);
  }

  friend bool operator==(const BigInteger& first, const BigInteger& second);
  friend bool operator<(const BigInteger& first, const BigInteger& second);
  friend bool operator>(const BigInteger& first, const BigInteger& second);
  friend bool operator<=(const BigInteger& first, const BigInteger& second);
  friend BigInteger operator%(const BigInteger& first,
                              const BigInteger& second);
  friend std::ostream& operator<<(std::ostream& out, const BigInteger& other);

  BigInteger operator-() const {
    BigInteger copy = *this;
    if (!copy.isNull()) {
      copy.isNeg = !copy.isNeg;
    }
    return copy;
  }

private:
  BigInteger& Add(const BigInteger& other) {
    while (digits.size() < other.digits.size()) {
      digits.push_back(0);
    }
    digits.push_back(0);
    for (size_t i = 0; i < other.digits.size(); ++i) {
      digits[i] += other.digits[i];
    }
    for (size_t i = 0; i < digits.size() - 1; ++i) {
      while (digits[i] >= cBase) {
        digits[i] -= cBase;
        ++digits[i + 1];
      }
    }
    delete_leading_zeroes();
    return *this;
  }

  BigInteger& Subtract(const BigInteger& other) {
    while (digits.size() < other.digits.size()) {
      digits.push_back(0);
    }
    digits.push_back(0);
    for (size_t i = 0; i < other.digits.size(); ++i) {
      digits[i] -= other.digits[i];
    }
    for (size_t i = 0; i < digits.size() - 1; ++i) {
      while (digits[i] >= cBase) {
        digits[i] -= cBase;
        ++digits[i + 1];
      }
      while (digits[i] < 0) {
        digits[i] += cBase;
        --digits[i + 1];
      }
    }
    if (digits.back() < 0) {
      for (long long& digit : digits) {
        digit *= -1;
      }
      isNeg = !isNeg;
      for (size_t i = 0; i < digits.size() - 1; ++i) {
        while (digits[i] >= cBase) {
          digits[i] -= cBase;
          ++digits[i + 1];
        }
        while (digits[i] < 0) {
          digits[i] += cBase;
          --digits[i + 1];
        }
      }
    }
    delete_leading_zeroes();
    return *this;
  }

public:
  BigInteger& operator+=(const BigInteger& other) {
    if (isNeg == other.isNeg) {
      Add(other);
    } else {
      Subtract(other);
    }
    delete_leading_zeroes();
    return *this;
  }

  BigInteger& operator-=(const BigInteger& other) {
    if (isNeg != other.isNeg) {
      Add(other);
    } else {
      Subtract(other);
    }
    delete_leading_zeroes();
    return *this;
  }

  BigInteger& operator*=(const BigInteger& other) {
    if (isNull() == true or other.isNull() == true) {
      digits.clear();
      isNeg = false;
      digits.push_back(0);
      return *this;
    }
    if (other == 1) {
      return *this;
    }
    if (other == -1) {
      isNeg = !isNeg;
      return *this;
    }
    BigInteger answer(0);
    answer.isNeg = isNeg xor other.isNeg;
    answer.digits.resize(digits.size() + other.digits.size() + 1, 0);
    for (size_t i = 0; i < digits.size(); ++i) {
      for (size_t j = 0; j < other.digits.size(); ++j) {
        answer.digits[i + j] += digits[i] * other.digits[j];
        if (answer.digits[i + j] >= cBase) {
          answer.digits[i + j + 1] += answer.digits[i + j] / cBase;
          answer.digits[i + j] %= cBase;
        }
      }
    }
    answer.delete_leading_zeroes();
    *this = answer;
    return *this;
  }

  BigInteger& operator/=(const BigInteger& other) {
    BigInteger copy(other);
    size_t div_sz = copy.digits.size();
    reverse(digits.begin(), digits.end());
    BigInteger ans(0);
    ans.isNeg = isNeg xor copy.isNeg;
    copy.isNeg = false;
    isNeg = false;
    BigInteger cur_slice;
    cur_slice.digits.assign(digits.begin(),
                            digits.begin() + static_cast<int>(div_sz));
    reverse(cur_slice.digits.begin(), cur_slice.digits.end());
    if (cur_slice < copy) {
      if (div_sz == digits.size()) {
        if (ans.isNull()) {
          ans.isNeg = false;
        }
        Swap(ans);
        return *this;
      }
      cur_slice.digits.insert(cur_slice.digits.begin(), digits[div_sz]);
    }
    BigInteger useless;
    for (size_t i = cur_slice.digits.size(); i <= digits.size(); ++i) {
      long long left_border = 0;
      long long right_border = cBase;
      while (right_border - left_border > 1) {
        long long middle = (left_border + right_border) / 2;
        useless = 1;
        useless *= copy;
        useless *= middle;
        if (useless <= cur_slice) {
          left_border = middle;
        } else {
          right_border = middle;
        }
      }
      ans.digits.push_back(left_border);
      useless = 1;
      useless *= copy;
      useless *= left_border;
      cur_slice -= useless;
      if (i != digits.size()) {
        cur_slice.digits.insert(cur_slice.digits.begin(), digits[i]);
      }
      cur_slice.delete_leading_zeroes();
    }

    reverse(ans.digits.begin(), ans.digits.end());
    ans.delete_leading_zeroes();
    if (ans.isNull())
      ans.isNeg = false;
    Swap(ans);
    return *this;
  }

  BigInteger& operator%=(const BigInteger& other) {
    BigInteger copy = *this;
    copy /= other;
    copy *= other;
    *this -= copy;
    return *this;
  }

  std::string toString() const {
    std::string answer;
    if (isNull()) {
      return "0";
    }
    if (isNeg) {
      answer.push_back('-');
    }
    std::string digit = std::to_string(digits.back());
    answer += digit;
    long long num = 0;
    for (int i = static_cast<int>(digits.size() - 2); i >= 0; --i) {
      digit = "";
      num = digits[i];
      while (digit.size() < 9) {
        char next_digit;
        next_digit = static_cast<char>(num % 10) + '0';
        num /= 10;
        digit.push_back(next_digit);
      }
      std::reverse(digit.begin(), digit.end());
      answer += digit;
    }
    return answer;
  }

  BigInteger& operator++() {
    *this += 1;
    return *this;
  }

  BigInteger& operator--() {
    *this -= 1;
    return *this;
  }

  BigInteger operator++(int) {
    BigInteger ans(*this);
    *this += 1;
    return ans;
  }

  BigInteger operator--(int) {
    BigInteger ans(*this);
    *this -= 1;
    return ans;
  }

  ~BigInteger() = default;

  static BigInteger gcd(const BigInteger& a, const BigInteger& b) {
    if (b.isNull())
      return a;
    return gcd(b, a % b);
  }
};

bool operator<(const BigInteger& first, const BigInteger& second) {
  if (first.isNeg != second.isNeg) {
    return first.isNeg > second.isNeg;
  }
  bool flag = false;
  if (!first.isNeg) {
    if (first.digits.size() != second.digits.size()) {
      return first.digits.size() < second.digits.size();
    }
    int i = static_cast<int>(first.digits.size()) - 1;
    while (i >= 0 and first.digits[i] == second.digits[i]) {
      --i;
    }
    if (i != -1) {
      flag = first.digits[i] < second.digits[i];
    }
  } else {
    if (first.digits.size() != second.digits.size()) {
      return first.digits.size() > second.digits.size();
    }
    int i = static_cast<int>(first.digits.size()) - 1;
    while (i >= 0 and first.digits[i] == second.digits[i]) {
      --i;
    }
    if (i != -1) {
      flag = first.digits[i] > second.digits[i];
    }
  }
  return flag;
}

bool operator>=(const BigInteger& first, const BigInteger& second) {
  return !(first < second);
}

bool operator>(const BigInteger& first, const BigInteger& second) {
  return second < first;
}

bool operator<=(const BigInteger& first, const BigInteger& second) {
  return !(first > second);
}

bool operator==(const BigInteger& first, const BigInteger& second) {
  if (first.isNeg != second.isNeg) {
    return false;
  }
  if (first.digits.size() != second.digits.size()) {
    return false;
  }
  int i = static_cast<int>(first.digits.size()) - 1;
  while (i >= 0 and first.digits[i] == second.digits[i]) {
    --i;
  }
  return i == -1;
}

bool operator!=(const BigInteger& first, const BigInteger& second) {
  return !(first == second);
}

std::istream& operator>>(std::istream& in, BigInteger& result) {
  std::string input;
  in >> input;
  result = BigInteger(input);
  return in;
}

std::ostream& operator<<(std::ostream& out, const BigInteger& other) {
  out << other.toString();
  return out;
}

BigInteger operator+(const BigInteger& first, const BigInteger& second) {
  BigInteger copy = first;
  copy += second;
  return copy;
}

BigInteger operator-(const BigInteger& first, const BigInteger& second) {
  BigInteger copy = first;
  copy -= second;
  return copy;
}

BigInteger operator*(const BigInteger& first, const BigInteger& second) {
  BigInteger copy = first;
  copy *= second;
  return copy;
}

BigInteger operator/(const BigInteger& first, const BigInteger& second) {
  BigInteger copy = first;
  copy /= second;
  return copy;
}

BigInteger operator%(const BigInteger& first, const BigInteger& second) {
  return first - (first / second) * second;
}

BigInteger operator""_bi(unsigned long long number) {
  if (number > static_cast<unsigned long long>(1LL << 63)) {
    unsigned long long remainder = number % 10;
    long long ans = static_cast<long long>(number) / 10;
    BigInteger result = BigInteger(ans);
    return (result * 10) + static_cast<long long>(remainder);
  }
  return {static_cast<long long>(number)};
}

BigInteger operator""_bi(const char* str, size_t) { return {str}; }

class Rational {
  BigInteger numerator;
  BigInteger denominator;

  void Swap(Rational& other) {
    numerator.Swap(other.numerator);
    denominator.Swap(other.denominator);
  }

  void to_prime_form() {
    bool is_neg = numerator.is_negative() xor denominator.is_negative();
    if (numerator.is_negative()) {
      numerator = -numerator;
    }
    if (denominator.is_negative()) {
      denominator = -denominator;
    }
    BigInteger gcd = BigInteger::gcd(numerator, denominator);
    numerator /= gcd;
    denominator /= gcd;
    if (is_neg) {
      numerator *= -1;
    }
  }

public:
  Rational() = default;

  Rational(const BigInteger& p, const BigInteger& q)
      : numerator(p), denominator(q) {
    to_prime_form();
  }

  Rational(const BigInteger& x) : numerator(x), denominator(1) {
    to_prime_form();
  }

  Rational(long long x) : numerator(x), denominator(1) { to_prime_form(); }

  Rational& operator+=(const Rational& other) {
    numerator *= other.denominator;
    numerator += (denominator * other.numerator);
    denominator *= other.denominator;
    to_prime_form();
    return *this;
  }

  Rational& operator*=(const Rational& other) {
    numerator *= other.numerator;
    denominator *= other.denominator;
    to_prime_form();
    return *this;
  }

  Rational& operator/=(const Rational& other) {
    numerator *= other.denominator;
    denominator *= other.numerator;
    to_prime_form();
    return *this;
  }

  Rational operator-() const {
    BigInteger copy = -numerator;
    return {copy, denominator};
  }

  Rational& operator-=(const Rational& other) { return *this += (-other); }

  friend bool operator==(const Rational& first, const Rational& second);
  friend bool operator<(const Rational& first, const Rational& second);

  std::string toString() const {
    Rational ans = *this;
    ans.to_prime_form();
    std::string strans = ans.numerator.toString();
    if (ans.denominator == BigInteger(1)) {
      return strans;
    }
    strans += "/" + ans.denominator.toString();
    return strans;
  }

  std::string asDecimal(size_t precision = 0) {
    if (precision == 0) {
      return (numerator / denominator).toString();
    }
    bool is_neg = numerator.is_negative() xor denominator.is_negative();
    if (numerator.is_negative()) {
      numerator = -numerator;
    }
    if (denominator.is_negative()) {
      denominator = -denominator;
    }
    BigInteger precise_number(numerator);
    for (size_t i = 0; i < precision; ++i) {
      precise_number *= 10;
    }
    std::string int_part;

    if (is_neg and !numerator.isNull()) {
      int_part.push_back('-');
    }
    int_part += (numerator / denominator).toString();
    std::string fractional_part = (precise_number / denominator).toString();
    while (fractional_part.size() < precision) {
      fractional_part.insert(fractional_part.begin(), '0');
    }
    int_part.push_back('.');
    int_part += fractional_part.substr(
        std::max(0, static_cast<int>(fractional_part.size()) -
                        static_cast<int>(precision)),
        precision);
    return int_part;
  }

  explicit operator double() { return std::stod(asDecimal(42)); }

  ~Rational() = default;
};

bool operator==(const Rational& first, const Rational& second) {
  return first.numerator * second.denominator ==
         first.denominator * second.numerator;
}

bool operator<(const Rational& first, const Rational& second) {
  return first.numerator * second.denominator <
         first.denominator * second.numerator;
}

bool operator>=(const Rational& first, const Rational& second) {
  return !(first < second);
}

bool operator>(const Rational& first, const Rational& second) {
  return second < first;
}

bool operator<=(const Rational& first, const Rational& second) {
  return !(first > second);
}

bool operator!=(const Rational& first, const Rational& second) {
  return !(first == second);
}

Rational operator+(const Rational& first, const Rational& second) {
  Rational copy = first;
  copy += second;
  return copy;
}

Rational operator-(const Rational& first, const Rational& second) {
  Rational copy = first;
  copy -= second;
  return copy;
}

Rational operator*(const Rational& first, const Rational& second) {
  Rational copy = first;
  copy *= second;
  return copy;
}

Rational operator/(const Rational& first, const Rational& second) {
  Rational copy = first;
  copy /= second;
  return copy;
}