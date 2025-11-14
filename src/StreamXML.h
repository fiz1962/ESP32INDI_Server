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

      // ------------------------------------------------------
      // TEXT
      // ------------------------------------------------------
      case TEXT:
        if (c == '<') {
          if (textBuffer.length()) emitText();
          tagBuffer = "";
          attrName = "";
          attrValue = "";
          tagStarted = false;
          selfClosing = false;
          state = TAG_OPEN;
        } else {
          textBuffer += c;
        }
        break;

      // ------------------------------------------------------
      // <tag ...
      // ------------------------------------------------------
      case TAG_OPEN:
        if (c == '/') {                    // </tag>
          tagBuffer = "";
          state = TAG_CLOSE;
        }
        else if (isWhitespace(c)) {        // <tag␣ → tag name finished
          emitTagStartOnce();
          state = ATTR_NAME;
        }
        else if (c == '>') {               // <tag>
          emitTagStartOnce();
          emitTagEndIfSelfClosing();
          state = TEXT;
        }
        else if (c == '/') {               // <tag/ >
          selfClosing = true;
        }
        else {
          tagBuffer += c;
        }
        break;

      // ------------------------------------------------------
      // attribute name
      // ------------------------------------------------------
      case ATTR_NAME:
        if (c == '>') {                    // end of start-tag
          emitTagStartOnce();
          emitTagEndIfSelfClosing();
          state = TEXT;
        }
        else if (c == '/') {               // <tag .../>
          selfClosing = true;
        }
        else if (c == '=') {
          state = ATTR_VALUE_QUOTE;
        }
        else if (!isWhitespace(c)) {
          attrName += c;
        }
        break;

      // ------------------------------------------------------
      // expecting opening quote
      // ------------------------------------------------------
      case ATTR_VALUE_QUOTE:
        if (c == '"' || c == '\'') {
          quoteChar = c;
          attrValue = "";
          state = ATTR_VALUE;
        }
        break;

      // ------------------------------------------------------
      // inside attribute value
      // ------------------------------------------------------
      case ATTR_VALUE:
        if (c == quoteChar) {
          emitAttribute();
          attrName = "";
          state = ATTR_NAME;
        } else {
          attrValue += c;
        }
        break;

      // ------------------------------------------------------
      // </tag>
      // ------------------------------------------------------
      case TAG_CLOSE:
        if (c == '>') {
          if (onTagEnd) onTagEnd(tagBuffer);
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

  String textBuffer;
  String tagBuffer;
  String attrName;
  String attrValue;

  char quoteChar = '"';
  bool tagStarted = false;
  bool selfClosing = false;

  bool isWhitespace(char c) {
    return c==' ' || c=='\n' || c=='\t' || c=='\r';
  }

  // ----------------------------------------------------------
  // Emit helpers
  // ----------------------------------------------------------

  void emitText() {
    String cleaned = textBuffer;
    cleaned.trim();
    if (cleaned.length() && onText)
      onText(cleaned);
    textBuffer = "";
  }

  void emitTagStartOnce() {
    if (!tagStarted) {
      tagStarted = true;
      if (onTagStart) onTagStart(tagBuffer);
    }
  }

  void emitAttribute() {
    if (onAttribute)
      onAttribute(tagBuffer, attrName, attrValue);
  }

  void emitTagEndIfSelfClosing() {
    if (selfClosing && onTagEnd)
      onTagEnd(tagBuffer);
  }
};
