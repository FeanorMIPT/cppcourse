#include <cstdlib>
#include <iostream>

char* error;

char* StringIncreaseCapacity(char* string, int* capacity) {
  *capacity *= 2;
  char* copy = (char*)realloc(string, sizeof(char) * (*capacity));
  return copy;
}

char** StackIncreaseCapacity(char** stack, int* capacity) {
  *capacity *= 2;
  char** copy = (char**)realloc(stack, sizeof(char*) * *capacity);
  return copy;
}

char** Push(char** stack, int* stack_size, int* stack_capacity,
            const char* string) {
  int data_size = 0;
  int data_capacity = 1;
  char* data = (char*)malloc(sizeof(char) * data_capacity);
  while ((data[data_size] = string[data_size]) != '\0') {
    ++data_size;
    if (data_size == data_capacity) {
      data = StringIncreaseCapacity(data, &data_capacity);
    }
  }
  data[data_size] = string[data_size];
  ++data_size;
  if (data_size == data_capacity) {
    data = StringIncreaseCapacity(data, &data_capacity);
  }
  stack[*stack_size] = data;
  ++*stack_size;
  if (*stack_size >= *stack_capacity) {
    stack = StackIncreaseCapacity(stack, stack_capacity);
  }
  return stack;
}

char* Pop(char** stack, int* stack_size) {
  if (*stack_size >= 1) {
    int data_size = 0;
    int data_capacity = 1;
    char* data = (char*)malloc(sizeof(char) * data_capacity);
    while ((data[data_size] = stack[*stack_size - 1][data_size]) != '\0') {
      ++data_size;
      if (data_size == data_capacity) {
        data = StringIncreaseCapacity(data, &data_capacity);
      }
    }
    free(stack[*stack_size - 1]);
    --*stack_size;
    return data;
  }
  return error;
}

char* Back(char** stack, const int* stack_size) {
  if (*stack_size > 0) {
    return stack[*stack_size - 1];
  }
  return error;
}

int Size(const int* stack_size) { return *stack_size; }

void Clear(char** stack, int* stack_size) {
  while (*stack_size > 0) {
    char* ans = Pop(stack, stack_size);
    free(ans);
  }
}

int main() {
  error = (char*)malloc(sizeof(char) * 6);
  error[0] = 'e';
  error[1] = 'r';
  error[2] = 'r';
  error[3] = 'o';
  error[4] = 'r';
  error[5] = '\0';
  int* stack_size = new int(0);
  int* stack_capacity = new int(1);
  char** stack = (char**)malloc(sizeof(char*) * *stack_capacity);
  int* string_size = new int(0);
  int* string_capacity = new int(1);
  char* string = (char*)malloc(sizeof(char) * *string_capacity);
  char* ans;
  char* request = (char*)malloc(sizeof(char) * 6);
  int inner_idx = 0;
  char cur_char;
  std::cin.get(cur_char);
  while (cur_char != ' ' and cur_char != '\n') {
    request[inner_idx] = cur_char;
    ++inner_idx;
    std::cin.get(cur_char);
  }
  inner_idx = 0;
  while (request[0] != 'e') {
    if (request[0] == 'p' and request[1] == 'u') {
      *string_size = 0;
      std::cin.get(cur_char);
      while (cur_char != ' ' and cur_char != '\n') {
        string[*string_size] = cur_char;
        ++(*string_size);
        if (*string_size == *string_capacity) {
          string = StringIncreaseCapacity(string, string_capacity);
        }
        std::cin.get(cur_char);
      }
      string[*string_size] = '\0';
      ++*string_size;
      if (*string_size == *string_capacity) {
        string = StringIncreaseCapacity(string, string_capacity);
      }
      stack = Push(stack, stack_size, stack_capacity, string);
      std::cout << "ok" << '\n';
    }
    if (request[0] == 'p' and request[1] == 'o') {
      if (*stack_size == 0) {
        std::cout << "error" << '\n';
      } else {
        ans = Pop(stack, stack_size);
        std::cout << ans << '\n';
        free(ans);
      }
    }
    if (request[0] == 'b') {
      ans = Back(stack, stack_size);
      std::cout << ans << '\n';
    }
    if (request[0] == 's') {
      std::cout << Size(stack_size) << '\n';
    }
    if (request[0] == 'c') {
      Clear(stack, stack_size);
      std::cout << "ok" << '\n';
    }
    inner_idx = 0;
    std::cin.get(cur_char);
    while (cur_char != ' ' and cur_char != '\n') {
      request[inner_idx] = cur_char;
      ++inner_idx;
      std::cin.get(cur_char);
    }
  }
  std::cout << "bye";
  Clear(stack, stack_size);
  free(stack);
  free(string);
  free(request);
  free(error);
  delete (stack_size);
  delete (stack_capacity);
  delete (string_size);
  delete (string_capacity);
}
