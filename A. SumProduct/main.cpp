#include <iostream>

int main(int argc, char* argv[]) {
  int k = argc - 1;
  int* n = new int[k];
  int t = 0;
  for (int i = 0; i < k; ++i) {
    t = 0;
    n[i] = 0;
    while (argv[i + 1][t] != '\0') {
      n[i] = n[i] * 10 + (argv[i + 1][t] - '0');
      ++t;
    }
  }
  int** arrays = new int*[k];
  for (int i = 0; i < k; ++i) {
    arrays[i] = new int[n[i]];
    for (int j = 0; j < n[i]; ++j) {
      std::cin >> arrays[i][j];
    }
  }
  int maxn = 0;
  for (int i = 0; i < k; ++i) {
    if (maxn < n[i]) {
      maxn = n[i];
    }
  }
  int* counter_index = new int[maxn];
  for (int i = 0; i < maxn; ++i) {
    counter_index[i] = 0;
  }
  int* indexes = new int[k];
  for (int i = 0; i < k; ++i) {
    indexes[i] = 0;
  }
  int count_variables = 1;
  for (int i = 0; i < k; ++i) {
    count_variables *= n[i];
  }
  long long sum = 0;
  bool flag = false;
  long long add = 1;
  for (long long i = 1; i <= count_variables; ++i) {
    for (int j = 0; j < maxn; ++j) {
      counter_index[j] = 0;
    }
    ++indexes[0];
    for (int j = 0; j < k - 1; ++j) {
      if (indexes[j] >= n[j]) {
        indexes[j] = 0;
        ++indexes[j + 1];
      }
    }
    if (indexes[k - 1] >= n[k - 1]) {
      break;
    }
    for (int j = 0; j < k; ++j) {
      ++counter_index[indexes[j]];
    }
    flag = false;
    for (int j = 0; j < maxn; ++j) {
      if (counter_index[j] >= 2) {
        flag = true;
      }
    }
    if (!flag) {
      add = 1;
      for (int j = 0; j < k; ++j) {
        add *= static_cast<long long>(arrays[j][indexes[j]]);
      }
      sum += add;
    }
  }
  std::cout << sum;
  delete[] n;
  for (int i = 0; i < k; ++i) {
    delete[] arrays[i];
  }
  delete[] arrays;
  delete[] counter_index;
  delete[] indexes;
}