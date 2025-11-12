#pragma once

#include <vector>
#include <Arduino.h>


struct SwitchEntry {
  String name;
  String label;
  String value;
};

struct SwitchVector {
  String name;
  String label;
  std::vector<SwitchEntry> switches;
};

struct NumberEntry {
  String name;
  String label;
  float value;
};

struct NumberVector {
  String name;
  String label;
  std::vector<NumberEntry> numbers;
};

struct TextEntry {
  String name;
  String label;
  String value;
};

struct TextVector {
  String name;
  String label;
  std::vector<TextEntry> texts;
};

struct Group {
  String name;
  std::vector<SwitchVector> switchVectors;
  std::vector<NumberVector> numberVectors;
  std::vector<TextVector> textVectors;
};
