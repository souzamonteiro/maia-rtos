class Lexer {
  constructor(input) {
    this.input = input;
    this.position = 0;
    this.tokens = [];
    this.charClassDepth = 0;
    this.templateDepth = 0;
    this.templateBraceDepthStack = [];
    this.tokenPatterns = [    { type: 'CAPTURE_OPEN', regex: /^\(/ },    { type: 'CAPTURE_CLOSE', regex: /^\)/ },    { type: 'CAPTURE_SPACE', regex: /^(?: |\u0009)/ },    { type: 'CAPTURE_CONTROL', regex: /^(?:&|;|<|>)/ },    { type: 'LETTER', regex: /^[A-Za-z_]/ },    { type: 'DIGIT', regex: /^[0-9]/ },    { type: 'EQUAL', regex: /^=/ },    { type: 'HASH', regex: /^#/ },    { type: 'LBRACE', regex: /^\{/ },    { type: 'RBRACE', regex: /^\}/ },    { type: 'BRACED_DOLLAR', regex: /^\$(?=\{)/ },    { type: 'SUBSTITUTION_DOLLAR', regex: /^\$(?=\()/ },    { type: 'DOLLAR', regex: /^\$/ },    { type: 'COLON', regex: /^:/ },    { type: 'MINUS', regex: /^-/ },    { type: 'PLUS', regex: /^\+/ },    { type: 'PERCENT', regex: /^%/ },    { type: 'QUESTION', regex: /^\?/ },    { type: 'SQUOTE', regex: /^'/ },    { type: 'DQUOTE', regex: /^"/ },    { type: 'BACKSLASH', regex: /^\\/ },    { type: 'BACKTICK', regex: /^`/ },    { type: 'LF', regex: /^\u000a/ },    { type: 'RAW', regex: /^(?:[\u0000-\u0008]|[\u000b-\u001f]|!|\*|,|[\u002e-\u002f]|@|\[|[\u005d-\u005e]|[\u007e-\ud7ff]|(?:[\ue000-\uffff]|[\ud800][\udc00-\udfff]|[\ud801-\udbfe][\udc00-\udfff]|[\udbff][\udc00-\udfff]))/ },    ];
  }

  isTemplateSpanPattern(pos, kind) {
    // Deterministic scan to avoid regex escaping issues in generated code.
    if (this.input[pos] !== '}') return false;
    const BACKTICK = String.fromCharCode(96);
    const max = Math.min(this.input.length, pos + 256);
    let i = pos + 1;
    while (i < max) {
      const ch = this.input[i];
      const next = this.input[i + 1];

      if (ch === '\\') {
        i += 2;
        continue;
      }

      if (ch === '$' && next === '{') {
        return kind === 'TemplateMiddle';
      }

      if (ch === BACKTICK) {
        return kind === 'TemplateTail';
      }

      i++;
    }
    return false;
  }

  enterTemplateSpan() {
    this.templateDepth++;
    this.templateBraceDepthStack.push(0);
  }

  exitTemplateSpan() {
    if (this.templateDepth > 0) {
      this.templateDepth--;
    }
    if (this.templateBraceDepthStack.length > 0) {
      this.templateBraceDepthStack.pop();
    }
  }

  currentTemplateBraceDepth() {
    return this.templateBraceDepthStack.length === 0
      ? 0
      : this.templateBraceDepthStack[this.templateBraceDepthStack.length - 1];
  }

  incrementTemplateBraceDepth() {
    if (this.templateBraceDepthStack.length > 0) {
      this.templateBraceDepthStack[this.templateBraceDepthStack.length - 1] += 1;
    }
  }

  decrementTemplateBraceDepth() {
    const index = this.templateBraceDepthStack.length - 1;
    if (index >= 0 && this.templateBraceDepthStack[index] > 0) {
      this.templateBraceDepthStack[index] -= 1;
    }
  }
  
  tokenize() {
    while (this.position < this.input.length) {
      let bestPattern = null;
      let bestMatch = null;
      const candidates = [];

      const isGenericNameType = (type) => (
        type === 'Name' || type === 'NameChar' || type === 'NameStartChar'
      );

      for (const pattern of this.tokenPatterns) {
        // Template spans are synthesized only when a closing brace ends the
        // active interpolation. Their raw patterns must not consume ordinary
        // braces from nested expressions.
        if (pattern.type === 'TemplateMiddle' || pattern.type === 'TemplateTail') {
          continue;
        }

        const regex = pattern.regex;
        const match = this.input.substring(this.position).match(regex);

        if (match && match.index === 0 && match[0].length > 0) {
          let effectivePattern = pattern;
          let effectiveMatch = match;
          // When parsing template expressions, disambiguate closing brace as template span boundary.
          if (this.templateDepth > 0
            && pattern.type === 'TOKEN__7D_'
            && this.currentTemplateBraceDepth() === 0) {
            if (this.isTemplateSpanPattern(this.position, 'TemplateMiddle')) {
              const templateMiddle = this.tokenPatterns.find((candidate) => candidate.type === 'TemplateMiddle');
              const templateMatch = templateMiddle
                ? this.input.substring(this.position).match(templateMiddle.regex)
                : null;
              if (templateMatch && templateMatch.index === 0 && templateMatch[0].length > 0) {
                effectivePattern = templateMiddle;
                effectiveMatch = templateMatch;
              }
            } else if (this.isTemplateSpanPattern(this.position, 'TemplateTail')) {
              const templateTail = this.tokenPatterns.find((candidate) => candidate.type === 'TemplateTail');
              const templateMatch = templateTail
                ? this.input.substring(this.position).match(templateTail.regex)
                : null;
              if (templateMatch && templateMatch.index === 0 && templateMatch[0].length > 0) {
                effectivePattern = templateTail;
                effectiveMatch = templateMatch;
              }
            }
          }

          candidates.push({ pattern: effectivePattern, match: effectiveMatch });
          if (!bestMatch
              || effectiveMatch[0].length > bestMatch[0].length
              || (effectiveMatch[0].length === bestMatch[0].length && effectivePattern.skip && !bestPattern.skip)
              || (effectiveMatch[0].length === bestMatch[0].length
                  && bestPattern
                  && isGenericNameType(bestPattern.type)
                  && !isGenericNameType(effectivePattern.type))) {
            bestPattern = effectivePattern;
            bestMatch = effectiveMatch;
          }
        }
      }

      // Inside character classes, prefer Char/CharCode/CharRange-like tokens
      // over generic global terminals such as '?>' that can overmatch.
      if (this.charClassDepth > 0 && candidates.length > 0) {
        const preferredTypes = new Set(['CharCodeRange', 'CharRange', 'CharCode', 'Char', 'TOKEN__5D_']);
        const preferred = candidates.filter(c => preferredTypes.has(c.pattern.type));
        if (preferred.length > 0) {
          let localBest = preferred[0];
          for (const c of preferred) {
            if (c.match[0].length > localBest.match[0].length) {
              localBest = c;
            }
          }
          bestPattern = localBest.pattern;
          bestMatch = localBest.match;
        }
      }

      // If current input starts with whitespace and a skip token is available,
      // prefer skipping whitespace first instead of consuming it as grammar data.
      if (candidates.length > 0 && /^\s/.test(this.input.substring(this.position, this.position + 1))) {
        const skipCandidates = candidates.filter(c => c.pattern.skip);
        if (skipCandidates.length > 0) {
          let localBest = skipCandidates[0];
          for (const c of skipCandidates) {
            if (c.match[0].length > localBest.match[0].length) {
              localBest = c;
            }
          }
          bestPattern = localBest.pattern;
          bestMatch = localBest.match;
        }
      }

      if (!bestMatch) {
        throw new Error(`Unexpected character at position ${this.position}: '${this.input[this.position]}'`);
      }

      if (!bestPattern.skip) {
        const matchedToken = {
          type: bestPattern.type,
          value: bestMatch[0],
          start: this.position,
          end: this.position + bestMatch[0].length
        };
        this.tokens.push(matchedToken);

        if (bestPattern.type === 'TOKEN__5B_' || bestPattern.type === 'TOKEN__5B__5E_') {
          this.charClassDepth++;
        } else if (bestPattern.type === 'TOKEN__5D_' && this.charClassDepth > 0) {
          this.charClassDepth--;
        } else if (bestPattern.type === 'TOKEN__7B_' && this.templateDepth > 0) {
          this.incrementTemplateBraceDepth();
        } else if (bestPattern.type === 'TOKEN__7D_' && this.templateDepth > 0) {
          this.decrementTemplateBraceDepth();
        } else if (bestPattern.type === 'TemplateHead') {
          this.enterTemplateSpan();
        } else if (bestPattern.type === 'TemplateTail') {
          this.exitTemplateSpan();
        }
      }

      this.position += bestMatch[0].length;
    }
    
    // Add EOF token
    this.tokens.push({
      type: 'EOF',
      value: '',
      start: this.position,
      end: this.position
    });
    
    return this.tokens;
  }
}

class Parser {
  constructor(input, eventHandler = null) {
    this.lexer = new Lexer(input);
    this.tokens = this.lexer.tokenize();
    this.position = 0;
    this.errors = [];
    this.eventHandler = eventHandler;
    this.failureMemo = new Set();
  }
  
  peek(offset = 0) {
    return this.tokens[this.position + offset];
  }
  
  consume(expectedType) {
    const token = this.peek();
    if (!token || token.type !== expectedType) {
      this.errors.push({
        expected: expectedType,
        found: token ? token.type : 'EOF',
        position: this.position
      });
      throw new Error(`Expected '${expectedType}', got '${token ? token.type : 'EOF'}'`);
    }
    if (this.eventHandler && typeof this.eventHandler.terminal === 'function') {
      this.eventHandler.terminal(expectedType, token.value, this.position);
    }
    this.position++;
    return token;
  }
  
  match(expectedType) {
    const token = this.peek();
    if (token && token.type === expectedType) {
      if (this.eventHandler && typeof this.eventHandler.terminal === 'function') {
        this.eventHandler.terminal(expectedType, token.value, this.position);
      }
      this.position++;
      return true;
    }
    return false;
  }

  consumeContextual(expectedType) {
    const token = this.peek();
    if (token && token.type === expectedType) {
      return this.consume(expectedType);
    }

    const pattern = this.lexer.tokenPatterns.find((candidate) => candidate.type === expectedType);
    const match = pattern && token ? token.value.match(pattern.regex) : null;
    if (!match || match.index !== 0 || match[0].length !== token.value.length) {
      this.errors.push({
        expected: expectedType,
        found: token ? token.type : 'EOF',
        position: this.position
      });
      throw new Error('Expected contextual ' + expectedType + ', got ' + (token ? token.type : 'EOF'));
    }

    if (this.eventHandler && typeof this.eventHandler.terminal === 'function') {
      this.eventHandler.terminal(token.type, token.value, this.position);
    }
    this.position++;
    return token;
  }

  markEventState() {
    if (this.eventHandler && typeof this.eventHandler.checkpoint === 'function') {
      return this.eventHandler.checkpoint();
    }
    return null;
  }

  restoreEventState(mark) {
    if (mark !== null && this.eventHandler && typeof this.eventHandler.restore === 'function') {
      this.eventHandler.restore(mark);
    }
  }
  
  getErrorMessage() {
    if (this.errors.length === 0) return 'No errors';
    const err = this.errors[0];
    return `Syntax error: expected ${err.expected}, got ${err.found}`;
  }
  parse() {
    const result = this.parseInput();
    const next = this.peek();
    if (!next && this.position === this.tokens.length) {
      return result;
    }
    if (!next || next.type !== 'EOF') {
      throw new Error(`Unexpected token at end: ${next ? next.type : 'EOF(consumed)'}`);
    }
    return result;
  }
  parseInput() {
    const __memoStart = this.position;
    const __memoKey = 'Input@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Input at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Input', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseAssignmentForm();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBareName();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseWordForm();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 3 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Input', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Input', this.position);
        }
      }
    }
  }
  parseAssignmentForm() {
    const __memoStart = this.position;
    const __memoKey = 'AssignmentForm@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule AssignmentForm at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('AssignmentForm', this.position);
    }
    let __ok = false;
    try {
    this.parseVariableName();
    this.consume('EQUAL');
    this.parseAssignmentValue();
    this.consume('EOF');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('AssignmentForm', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('AssignmentForm', this.position);
        }
      }
    }
  }
  parseAssignmentValue() {
    const __memoStart = this.position;
    const __memoKey = 'AssignmentValue@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule AssignmentValue at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('AssignmentValue', this.position);
    }
    let __ok = false;
    try {
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parsePart();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('AssignmentValue', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('AssignmentValue', this.position);
        }
      }
    }
  }
  parseBareName() {
    const __memoStart = this.position;
    const __memoKey = 'BareName@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BareName at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BareName', this.position);
    }
    let __ok = false;
    try {
    this.parseVariableName();
    this.consume('EOF');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BareName', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BareName', this.position);
        }
      }
    }
  }
  parseWordForm() {
    const __memoStart = this.position;
    const __memoKey = 'WordForm@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule WordForm at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('WordForm', this.position);
    }
    let __ok = false;
    try {
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parsePart();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    this.consume('EOF');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('WordForm', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('WordForm', this.position);
        }
      }
    }
  }
  parseVariableName() {
    const __memoStart = this.position;
    const __memoKey = 'VariableName@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule VariableName at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('VariableName', this.position);
    }
    let __ok = false;
    try {
    this.consume('LETTER');
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.consume('LETTER');
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.consume('DIGIT');
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
      } catch (e) {
        this.position = _loopStart;
        this.restoreEventState(_loopMark);
        break;
      }
      if (this.position === _loopStart) break;
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('VariableName', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('VariableName', this.position);
        }
      }
    }
  }
  parsePart() {
    const __memoStart = this.position;
    const __memoKey = 'Part@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Part at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Part', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseCommandSubstitution();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parsePrefixParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseSuffixParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDefaultParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseParameterLength();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseOpaqueBraced();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseSingleQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parsePlain();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 14 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Part', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Part', this.position);
        }
      }
    }
  }
  parseCommandSubstitution() {
    const __memoStart = this.position;
    const __memoKey = 'CommandSubstitution@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CommandSubstitution at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CommandSubstitution', this.position);
    }
    let __ok = false;
    try {
    this.consume('SUBSTITUTION_DOLLAR');
    this.consume('CAPTURE_OPEN');
    this.parseCaptureCommand();
    this.consume('CAPTURE_CLOSE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CommandSubstitution', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CommandSubstitution', this.position);
        }
      }
    }
  }
  parseCaptureCommand() {
    const __memoStart = this.position;
    const __memoKey = 'CaptureCommand@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CaptureCommand at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CaptureCommand', this.position);
    }
    let __ok = false;
    try {
    while (this.match('CAPTURE_SPACE')) { /* zero or more */ }
    this.parseCaptureWord();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
    while (this.match('CAPTURE_SPACE')) { /* one or more */ }
    this.parseCaptureWord();
      } catch (e) {
        this.position = _loopStart;
        this.restoreEventState(_loopMark);
        break;
      }
      if (this.position === _loopStart) break;
    }
    while (this.match('CAPTURE_SPACE')) { /* zero or more */ }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CaptureCommand', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CaptureCommand', this.position);
        }
      }
    }
  }
  parseCaptureWord() {
    const __memoStart = this.position;
    const __memoKey = 'CaptureWord@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CaptureWord at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CaptureWord', this.position);
    }
    let __ok = false;
    try {
    this.parseCaptureInitialPart();
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseCapturePart();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CaptureWord', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CaptureWord', this.position);
        }
      }
    }
  }
  parseCaptureInitialPart() {
    const __memoStart = this.position;
    const __memoKey = 'CaptureInitialPart@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CaptureInitialPart at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CaptureInitialPart', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseParameterLength();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseSingleQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseCaptureDoubleQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseCaptureInitialLiteral();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 9 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CaptureInitialPart', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CaptureInitialPart', this.position);
        }
      }
    }
  }
  parseCapturePart() {
    const __memoStart = this.position;
    const __memoKey = 'CapturePart@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CapturePart at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CapturePart', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseParameterLength();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseSingleQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseCaptureDoubleQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseCaptureLiteral();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 9 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CapturePart', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CapturePart', this.position);
        }
      }
    }
  }
  parseCaptureInitialLiteral() {
    const __memoStart = this.position;
    const __memoKey = 'CaptureInitialLiteral@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CaptureInitialLiteral at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CaptureInitialLiteral', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 12 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CaptureInitialLiteral', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CaptureInitialLiteral', this.position);
        }
      }
    }
  }
  parseCaptureLiteral() {
    const __memoStart = this.position;
    const __memoKey = 'CaptureLiteral@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CaptureLiteral at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CaptureLiteral', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 13 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CaptureLiteral', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CaptureLiteral', this.position);
        }
      }
    }
  }
  parseCaptureDoubleQuoted() {
    const __memoStart = this.position;
    const __memoKey = 'CaptureDoubleQuoted@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CaptureDoubleQuoted at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CaptureDoubleQuoted', this.position);
    }
    let __ok = false;
    try {
    this.consume('DQUOTE');
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseCaptureDoublePiece();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    this.consume('DQUOTE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CaptureDoubleQuoted', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CaptureDoubleQuoted', this.position);
        }
      }
    }
  }
  parseCaptureDoublePiece() {
    const __memoStart = this.position;
    const __memoKey = 'CaptureDoublePiece@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CaptureDoublePiece at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CaptureDoublePiece', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseParameterLength();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleSlash();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseCaptureDoubleLiteral();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 8 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CaptureDoublePiece', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CaptureDoublePiece', this.position);
        }
      }
    }
  }
  parseCaptureDoubleLiteral() {
    const __memoStart = this.position;
    const __memoKey = 'CaptureDoubleLiteral@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CaptureDoubleLiteral at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CaptureDoubleLiteral', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 19 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CaptureDoubleLiteral', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CaptureDoubleLiteral', this.position);
        }
      }
    }
  }
  parseStatusParameter() {
    const __memoStart = this.position;
    const __memoKey = 'StatusParameter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule StatusParameter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('StatusParameter', this.position);
    }
    let __ok = false;
    try {
    this.consume('DOLLAR');
    this.consume('QUESTION');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('StatusParameter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('StatusParameter', this.position);
        }
      }
    }
  }
  parseNamedParameter() {
    const __memoStart = this.position;
    const __memoKey = 'NamedParameter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule NamedParameter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('NamedParameter', this.position);
    }
    let __ok = false;
    try {
    this.consume('DOLLAR');
    this.parseVariableName();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('NamedParameter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('NamedParameter', this.position);
        }
      }
    }
  }
  parseBracedStatusParameter() {
    const __memoStart = this.position;
    const __memoKey = 'BracedStatusParameter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracedStatusParameter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracedStatusParameter', this.position);
    }
    let __ok = false;
    try {
    this.consume('BRACED_DOLLAR');
    this.consume('LBRACE');
    this.consume('QUESTION');
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BracedStatusParameter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracedStatusParameter', this.position);
        }
      }
    }
  }
  parseBracedNamedParameter() {
    const __memoStart = this.position;
    const __memoKey = 'BracedNamedParameter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracedNamedParameter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracedNamedParameter', this.position);
    }
    let __ok = false;
    try {
    this.consume('BRACED_DOLLAR');
    this.consume('LBRACE');
    this.parseVariableName();
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BracedNamedParameter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracedNamedParameter', this.position);
        }
      }
    }
  }
  parseParameterLength() {
    const __memoStart = this.position;
    const __memoKey = 'ParameterLength@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule ParameterLength at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('ParameterLength', this.position);
    }
    let __ok = false;
    try {
    this.consume('BRACED_DOLLAR');
    this.consume('LBRACE');
    this.consume('HASH');
    this.parseVariableName();
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('ParameterLength', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('ParameterLength', this.position);
        }
      }
    }
  }
  parsePrefixParameter() {
    const __memoStart = this.position;
    const __memoKey = 'PrefixParameter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule PrefixParameter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('PrefixParameter', this.position);
    }
    let __ok = false;
    try {
    this.consume('BRACED_DOLLAR');
    this.consume('LBRACE');
    this.parseVariableName();
    this.parsePrefixOperator();
    this.parsePatternValue();
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('PrefixParameter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('PrefixParameter', this.position);
        }
      }
    }
  }
  parsePrefixOperator() {
    const __memoStart = this.position;
    const __memoKey = 'PrefixOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule PrefixOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('PrefixOperator', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseLongestPrefixOperator();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseShortestPrefixOperator();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 2 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('PrefixOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('PrefixOperator', this.position);
        }
      }
    }
  }
  parseLongestPrefixOperator() {
    const __memoStart = this.position;
    const __memoKey = 'LongestPrefixOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule LongestPrefixOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('LongestPrefixOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('HASH');
    this.consume('HASH');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('LongestPrefixOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('LongestPrefixOperator', this.position);
        }
      }
    }
  }
  parseShortestPrefixOperator() {
    const __memoStart = this.position;
    const __memoKey = 'ShortestPrefixOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule ShortestPrefixOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('ShortestPrefixOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('HASH');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('ShortestPrefixOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('ShortestPrefixOperator', this.position);
        }
      }
    }
  }
  parseSuffixParameter() {
    const __memoStart = this.position;
    const __memoKey = 'SuffixParameter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule SuffixParameter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('SuffixParameter', this.position);
    }
    let __ok = false;
    try {
    this.consume('BRACED_DOLLAR');
    this.consume('LBRACE');
    this.parseVariableName();
    this.parseSuffixOperator();
    this.parsePatternValue();
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('SuffixParameter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('SuffixParameter', this.position);
        }
      }
    }
  }
  parseSuffixOperator() {
    const __memoStart = this.position;
    const __memoKey = 'SuffixOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule SuffixOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('SuffixOperator', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseLongestSuffixOperator();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseShortestSuffixOperator();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 2 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('SuffixOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('SuffixOperator', this.position);
        }
      }
    }
  }
  parseLongestSuffixOperator() {
    const __memoStart = this.position;
    const __memoKey = 'LongestSuffixOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule LongestSuffixOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('LongestSuffixOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('PERCENT');
    this.consume('PERCENT');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('LongestSuffixOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('LongestSuffixOperator', this.position);
        }
      }
    }
  }
  parseShortestSuffixOperator() {
    const __memoStart = this.position;
    const __memoKey = 'ShortestSuffixOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule ShortestSuffixOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('ShortestSuffixOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('PERCENT');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('ShortestSuffixOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('ShortestSuffixOperator', this.position);
        }
      }
    }
  }
  parsePatternValue() {
    const __memoStart = this.position;
    const __memoKey = 'PatternValue@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule PatternValue at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('PatternValue', this.position);
    }
    let __ok = false;
    try {
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseDefaultPart();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('PatternValue', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('PatternValue', this.position);
        }
      }
    }
  }
  parseDefaultParameter() {
    const __memoStart = this.position;
    const __memoKey = 'DefaultParameter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DefaultParameter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DefaultParameter', this.position);
    }
    let __ok = false;
    try {
    this.consume('BRACED_DOLLAR');
    this.consume('LBRACE');
    this.parseVariableName();
    this.parseDefaultOperator();
    this.parseDefaultValue();
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DefaultParameter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DefaultParameter', this.position);
        }
      }
    }
  }
  parseDoubleDefaultParameter() {
    const __memoStart = this.position;
    const __memoKey = 'DoubleDefaultParameter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoubleDefaultParameter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoubleDefaultParameter', this.position);
    }
    let __ok = false;
    try {
    this.consume('BRACED_DOLLAR');
    this.consume('LBRACE');
    this.parseVariableName();
    this.parseDefaultOperator();
    this.parseDoubleDefaultValue();
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoubleDefaultParameter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoubleDefaultParameter', this.position);
        }
      }
    }
  }
  parseDefaultOperator() {
    const __memoStart = this.position;
    const __memoKey = 'DefaultOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DefaultOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DefaultOperator', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDefaultNullOperator();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDefaultUnsetOperator();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseAlternateNonemptyOperator();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseAlternateSetOperator();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 4 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DefaultOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DefaultOperator', this.position);
        }
      }
    }
  }
  parseDefaultNullOperator() {
    const __memoStart = this.position;
    const __memoKey = 'DefaultNullOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DefaultNullOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DefaultNullOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('COLON');
    this.consume('MINUS');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DefaultNullOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DefaultNullOperator', this.position);
        }
      }
    }
  }
  parseDefaultUnsetOperator() {
    const __memoStart = this.position;
    const __memoKey = 'DefaultUnsetOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DefaultUnsetOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DefaultUnsetOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('MINUS');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DefaultUnsetOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DefaultUnsetOperator', this.position);
        }
      }
    }
  }
  parseAlternateNonemptyOperator() {
    const __memoStart = this.position;
    const __memoKey = 'AlternateNonemptyOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule AlternateNonemptyOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('AlternateNonemptyOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('COLON');
    this.consume('PLUS');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('AlternateNonemptyOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('AlternateNonemptyOperator', this.position);
        }
      }
    }
  }
  parseAlternateSetOperator() {
    const __memoStart = this.position;
    const __memoKey = 'AlternateSetOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule AlternateSetOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('AlternateSetOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('PLUS');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('AlternateSetOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('AlternateSetOperator', this.position);
        }
      }
    }
  }
  parseDefaultValue() {
    const __memoStart = this.position;
    const __memoKey = 'DefaultValue@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DefaultValue at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DefaultValue', this.position);
    }
    let __ok = false;
    try {
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseDefaultPart();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DefaultValue', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DefaultValue', this.position);
        }
      }
    }
  }
  parseDefaultPart() {
    const __memoStart = this.position;
    const __memoKey = 'DefaultPart@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DefaultPart at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DefaultPart', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseParameterLength();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseSingleQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseFallbackQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDefaultLiteral();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 9 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DefaultPart', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DefaultPart', this.position);
        }
      }
    }
  }
  parseDefaultLiteral() {
    const __memoStart = this.position;
    const __memoKey = 'DefaultLiteral@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DefaultLiteral at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DefaultLiteral', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 17 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DefaultLiteral', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DefaultLiteral', this.position);
        }
      }
    }
  }
  parseDoubleDefaultValue() {
    const __memoStart = this.position;
    const __memoKey = 'DoubleDefaultValue@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoubleDefaultValue at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoubleDefaultValue', this.position);
    }
    let __ok = false;
    try {
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseDoubleDefaultPart();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoubleDefaultValue', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoubleDefaultValue', this.position);
        }
      }
    }
  }
  parseDoubleDefaultPart() {
    const __memoStart = this.position;
    const __memoKey = 'DoubleDefaultPart@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoubleDefaultPart at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoubleDefaultPart', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseParameterLength();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseFallbackQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDefaultBraceEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleSlash();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleDefaultLiteral();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 10 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoubleDefaultPart', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoubleDefaultPart', this.position);
        }
      }
    }
  }
  parseDoubleDefaultLiteral() {
    const __memoStart = this.position;
    const __memoKey = 'DoubleDefaultLiteral@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoubleDefaultLiteral at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoubleDefaultLiteral', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 18 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoubleDefaultLiteral', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoubleDefaultLiteral', this.position);
        }
      }
    }
  }
  parseDefaultBraceEscape() {
    const __memoStart = this.position;
    const __memoKey = 'DefaultBraceEscape@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DefaultBraceEscape at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DefaultBraceEscape', this.position);
    }
    let __ok = false;
    try {
    this.consume('BACKSLASH');
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DefaultBraceEscape', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DefaultBraceEscape', this.position);
        }
      }
    }
  }
  parseFallbackQuoted() {
    const __memoStart = this.position;
    const __memoKey = 'FallbackQuoted@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule FallbackQuoted at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('FallbackQuoted', this.position);
    }
    let __ok = false;
    try {
    this.consume('DQUOTE');
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseFallbackDoublePart();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    this.consume('DQUOTE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('FallbackQuoted', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('FallbackQuoted', this.position);
        }
      }
    }
  }
  parseFallbackDoublePart() {
    const __memoStart = this.position;
    const __memoKey = 'FallbackDoublePart@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule FallbackDoublePart at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('FallbackDoublePart', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseParameterLength();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleSlash();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseFallbackDoubleLiteral();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 8 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('FallbackDoublePart', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('FallbackDoublePart', this.position);
        }
      }
    }
  }
  parseFallbackDoubleLiteral() {
    const __memoStart = this.position;
    const __memoKey = 'FallbackDoubleLiteral@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule FallbackDoubleLiteral at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('FallbackDoubleLiteral', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 19 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('FallbackDoubleLiteral', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('FallbackDoubleLiteral', this.position);
        }
      }
    }
  }
  parseSingleQuoted() {
    const __memoStart = this.position;
    const __memoKey = 'SingleQuoted@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule SingleQuoted at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('SingleQuoted', this.position);
    }
    let __ok = false;
    try {
    this.consume('SQUOTE');
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseSinglePiece();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    this.consume('SQUOTE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('SingleQuoted', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('SingleQuoted', this.position);
        }
      }
    }
  }
  parseSinglePiece() {
    const __memoStart = this.position;
    const __memoKey = 'SinglePiece@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule SinglePiece at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('SinglePiece', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SUBSTITUTION_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BRACED_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKSLASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKTICK');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 23 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('SinglePiece', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('SinglePiece', this.position);
        }
      }
    }
  }
  parseDoubleQuoted() {
    const __memoStart = this.position;
    const __memoKey = 'DoubleQuoted@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoubleQuoted at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoubleQuoted', this.position);
    }
    let __ok = false;
    try {
    this.consume('DQUOTE');
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseDoublePiece();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    this.consume('DQUOTE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoubleQuoted', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoubleQuoted', this.position);
        }
      }
    }
  }
  parseDoublePiece() {
    const __memoStart = this.position;
    const __memoKey = 'DoublePiece@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoublePiece at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoublePiece', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseCommandSubstitution();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parsePrefixParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseSuffixParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleDefaultParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseParameterLength();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseStatusParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNamedParameter();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseOpaqueBraced();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleSlash();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleLiteral();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 13 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoublePiece', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoublePiece', this.position);
        }
      }
    }
  }
  parseDoubleEscape() {
    const __memoStart = this.position;
    const __memoKey = 'DoubleEscape@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoubleEscape at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoubleEscape', this.position);
    }
    let __ok = false;
    try {
    this.parseEscapeMarker();
    this.parseDoubleEscapedCharacter();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoubleEscape', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoubleEscape', this.position);
        }
      }
    }
  }
  parseDoubleEscapedCharacter() {
    const __memoStart = this.position;
    const __memoKey = 'DoubleEscapedCharacter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoubleEscapedCharacter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoubleEscapedCharacter', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SUBSTITUTION_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BRACED_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKTICK');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKSLASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 8 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoubleEscapedCharacter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoubleEscapedCharacter', this.position);
        }
      }
    }
  }
  parseDoubleSlash() {
    const __memoStart = this.position;
    const __memoKey = 'DoubleSlash@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoubleSlash at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoubleSlash', this.position);
    }
    let __ok = false;
    try {
    this.consume('BACKSLASH');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoubleSlash', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoubleSlash', this.position);
        }
      }
    }
  }
  parseDoubleLiteral() {
    const __memoStart = this.position;
    const __memoKey = 'DoubleLiteral@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DoubleLiteral at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DoubleLiteral', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BRACED_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKTICK');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 20 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DoubleLiteral', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DoubleLiteral', this.position);
        }
      }
    }
  }
  parseOpaqueBraced() {
    const __memoStart = this.position;
    const __memoKey = 'OpaqueBraced@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule OpaqueBraced at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('OpaqueBraced', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseCompleteOpaqueBraced();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BRACED_DOLLAR');
    this.consume('LBRACE');
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseBracedPiece();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    if (this.match('RBRACE')) { /* optional token matched */ }
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 2 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('OpaqueBraced', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('OpaqueBraced', this.position);
        }
      }
    }
  }
  parseCompleteOpaqueBraced() {
    const __memoStart = this.position;
    const __memoKey = 'CompleteOpaqueBraced@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CompleteOpaqueBraced at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CompleteOpaqueBraced', this.position);
    }
    let __ok = false;
    try {
    this.consume('BRACED_DOLLAR');
    this.consume('LBRACE');
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseCompleteBracedPiece();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CompleteOpaqueBraced', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CompleteOpaqueBraced', this.position);
        }
      }
    }
  }
  parseCompleteBracedPiece() {
    const __memoStart = this.position;
    const __memoKey = 'CompleteBracedPiece@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CompleteBracedPiece at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CompleteBracedPiece', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SUBSTITUTION_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseCompleteBracedNested();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseOpaqueQuoted();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BRACED_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleSlash();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKTICK');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 25 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CompleteBracedPiece', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CompleteBracedPiece', this.position);
        }
      }
    }
  }
  parseCompleteBracedNested() {
    const __memoStart = this.position;
    const __memoKey = 'CompleteBracedNested@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule CompleteBracedNested at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('CompleteBracedNested', this.position);
    }
    let __ok = false;
    try {
    this.consume('LBRACE');
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseCompleteBracedPiece();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    this.consume('RBRACE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('CompleteBracedNested', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('CompleteBracedNested', this.position);
        }
      }
    }
  }
  parseOpaqueQuoted() {
    const __memoStart = this.position;
    const __memoKey = 'OpaqueQuoted@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule OpaqueQuoted at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('OpaqueQuoted', this.position);
    }
    let __ok = false;
    try {
    this.consume('DQUOTE');
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseOpaqueQuotedPiece();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    this.consume('DQUOTE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('OpaqueQuoted', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('OpaqueQuoted', this.position);
        }
      }
    }
  }
  parseOpaqueQuotedPiece() {
    const __memoStart = this.position;
    const __memoKey = 'OpaqueQuotedPiece@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule OpaqueQuotedPiece at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('OpaqueQuotedPiece', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SUBSTITUTION_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BRACED_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleSlash();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKTICK');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 24 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('OpaqueQuotedPiece', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('OpaqueQuotedPiece', this.position);
        }
      }
    }
  }
  parseBracedPiece() {
    const __memoStart = this.position;
    const __memoKey = 'BracedPiece@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracedPiece at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracedPiece', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SUBSTITUTION_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BRACED_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseBracedNested();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleEscape();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDoubleSlash();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKTICK');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 22 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BracedPiece', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracedPiece', this.position);
        }
      }
    }
  }
  parseBracedNested() {
    const __memoStart = this.position;
    const __memoKey = 'BracedNested@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracedNested at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracedNested', this.position);
    }
    let __ok = false;
    try {
    this.consume('LBRACE');
    while (true) {
      const savePos = this.position;
      const saveMark = this.markEventState();
      try {
        this.parseBracedPiece();
        if (this.position === savePos) break;
      } catch(e) {
        this.position = savePos;
        this.restoreEventState(saveMark);
        break;
      }
    }
    if (this.match('RBRACE')) { /* optional token matched */ }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BracedNested', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracedNested', this.position);
        }
      }
    }
  }
  parseEscape() {
    const __memoStart = this.position;
    const __memoKey = 'Escape@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Escape at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Escape', this.position);
    }
    let __ok = false;
    try {
    this.parseEscapeMarker();
    this.parseEscapedCharacter();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Escape', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Escape', this.position);
        }
      }
    }
  }
  parseEscapeMarker() {
    const __memoStart = this.position;
    const __memoKey = 'EscapeMarker@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule EscapeMarker at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('EscapeMarker', this.position);
    }
    let __ok = false;
    try {
    this.consume('BACKSLASH');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('EscapeMarker', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('EscapeMarker', this.position);
        }
      }
    }
  }
  parseEscapedCharacter() {
    const __memoStart = this.position;
    const __memoKey = 'EscapedCharacter@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule EscapedCharacter at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('EscapedCharacter', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SUBSTITUTION_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BRACED_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DQUOTE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKSLASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKTICK');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 23 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('EscapedCharacter', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('EscapedCharacter', this.position);
        }
      }
    }
  }
  parsePlain() {
    const __memoStart = this.position;
    const __memoKey = 'Plain@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Plain at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Plain', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CONTROL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CAPTURE_SPACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PERCENT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('PLUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('COLON');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('MINUS');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BRACED_DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('HASH');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LETTER');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DIGIT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RAW');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQUAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('RBRACE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DOLLAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('QUESTION');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('BACKTICK');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 20 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Plain', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Plain', this.position);
        }
      }
    }
  }
}

module.exports = Parser;