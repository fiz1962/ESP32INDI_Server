#pragma once
#include <Arduino.h>

class StreamXML {
public:
  std::function<void(const String& tag)> onTagStart;
  std::function<void(const String& tag)> onTagEnd;
  std::function<void(const String& text)> onText;
  std::function<void(const String& tag, const String& name, const String& value)> onAttribute;

  void feed(char c) {
    switch (state) {

      case TEXT:
        if (c == '<') {
          if (textBuffer.length()) emitText();
          tagBuffer = "";
          state = TAG_OPEN;
        } else {
          textBuffer += c;
        }
        break;

      case TAG_OPEN:
        if (c == '/') {
          tagBuffer = "";
          state = TAG_CLOSE;
        } else if (c == ' ' || c == '\t' || c == '\n') {
          state = ATTR_NAME;
          attrName = "";
        } else if (c == '>') {
          emitTagStart();
          state = TEXT;
        } else {
          tagBuffer += c;
        }
        break;

      case ATTR_NAME:
        if (c == '>' ) {
          emitTagStart();
          state = TEXT;
        } else if (c == '=') {
          state = ATTR_VALUE_QUOTE;
        } else if (!isWhitespace(c)) {
          attrName += c;
        }
        break;

      case ATTR_VALUE_QUOTE:
        if (c == '"' || c == '\'') {  // accept both " and '
          quoteChar = c;
          attrValue = "";
          state = ATTR_VALUE;
        }
        break;

      case ATTR_VALUE:
        if (c == quoteChar) {
          if (onAttribute) onAttribute(tagBuffer, attrName, attrValue);
          attrName = "";
          state = ATTR_NAME;
        } else {
          attrValue += c;
        }
        break;

      case TAG_CLOSE:
        if (c == '>') {
          emitTagEnd();
          state = TEXT;
        } else {
          tagBuffer += c;
        }
        break;
    }
  }

  void feed(const String& s) {
    for (char c : s) feed(c);
  }

private:
  enum State { TEXT, TAG_OPEN, ATTR_NAME, ATTR_VALUE_QUOTE, ATTR_VALUE, TAG_CLOSE } state = TEXT;
  char quoteChar;
  String textBuffer, tagBuffer, attrName, attrValue;

  bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }

  void emitText() {
    String cleaned = textBuffer;
    cleaned.trim();              // <-- trims whitespace
    if (cleaned.length() && onText)
      onText(cleaned);
    textBuffer = "";
  }

  void emitTagStart() {
    if (onTagStart) onTagStart(tagBuffer);
  }

  void emitTagEnd() {
    if (onTagEnd) onTagEnd(tagBuffer);
  }
};
