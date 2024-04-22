#include "iostream"
#include "long_march.h"

int main() {
  long_march::LogInfo("Hello, {}", "World!");
  long_march::LogInfo(" Assets Path: {}", ASSETS_PATH);
}
