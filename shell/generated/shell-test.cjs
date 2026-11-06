class Lexer {
  constructor(input) {
    this.input = input;
    this.position = 0;
    this.tokens = [];
    this.charClassDepth = 0;
    this.templateDepth = 0;
    this.templateBraceDepthStack = [];
    this.tokenPatterns = [    { type: 'OPEN', regex: /^\[\u0000/ },    { type: 'CLOSE', regex: /^\]\u0000/ },    { type: 'TEST', regex: /^test\u0000/ },    { type: 'NOT', regex: /^!\u0000/ },    { type: 'NONEMPTY', regex: /^-n\u0000/ },    { type: 'EMPTY', regex: /^-z\u0000/ },    { type: 'EQ', regex: /^=\u0000/ },    { type: 'NE', regex: /^!=\u0000/ },    { type: 'NUM_EQ', regex: /^-eq\u0000/ },    { type: 'NUM_NE', regex: /^-ne\u0000/ },    { type: 'NUM_LT', regex: /^-lt\u0000/ },    { type: 'NUM_LE', regex: /^-le\u0000/ },    { type: 'NUM_GT', regex: /^-gt\u0000/ },    { type: 'NUM_GE', regex: /^-ge\u0000/ },    { type: 'DECIMAL', regex: /^(?:[+-])?(?:[0-9])+\u0000/ },    { type: 'VALUE', regex: /^(?:[^\u0000])*\u0000/ },    ];
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
    this.consume('TEST');
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseNegatedBinary();
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
    this.parseBinary();
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
    this.parseNegatedUnary();
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
    this.parseUnary();
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
    this.parseNegatedTruth();
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
    this.parseTruth();
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
    this.parseEmpty();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('OPEN');
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseBracketNegatedBinary();
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
    this.parseBracketBinary();
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
    this.parseBracketNegatedUnary();
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
    this.parseBracketUnary();
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
    this.parseBracketNegatedTruth();
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
    this.parseBracketTruth();
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
    this.parseBracketEmpty();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
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
          this.eventHandler.endNonterminal('Input', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Input', this.position);
        }
      }
    }
  }
  parseNegatedBinary() {
    const __memoStart = this.position;
    const __memoKey = 'NegatedBinary@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule NegatedBinary at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('NegatedBinary', this.position);
    }
    let __ok = false;
    try {
    this.consume('NOT');
    this.parseBinary();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('NegatedBinary', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('NegatedBinary', this.position);
        }
      }
    }
  }
  parseBinary() {
    const __memoStart = this.position;
    const __memoKey = 'Binary@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Binary at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Binary', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseValue();
    this.parseStringOperator();
    this.parseValue();
    this.consume('EOF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDecimal();
    this.parseNumericOperator();
    this.parseDecimal();
    this.consume('EOF');
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
          this.eventHandler.endNonterminal('Binary', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Binary', this.position);
        }
      }
    }
  }
  parseNegatedUnary() {
    const __memoStart = this.position;
    const __memoKey = 'NegatedUnary@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule NegatedUnary at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('NegatedUnary', this.position);
    }
    let __ok = false;
    try {
    this.consume('NOT');
    this.parseUnary();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('NegatedUnary', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('NegatedUnary', this.position);
        }
      }
    }
  }
  parseUnary() {
    const __memoStart = this.position;
    const __memoKey = 'Unary@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Unary at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Unary', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNonemptyOperator();
    this.parseValue();
    this.consume('EOF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseEmptyOperator();
    this.parseValue();
    this.consume('EOF');
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
          this.eventHandler.endNonterminal('Unary', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Unary', this.position);
        }
      }
    }
  }
  parseNegatedTruth() {
    const __memoStart = this.position;
    const __memoKey = 'NegatedTruth@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule NegatedTruth at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('NegatedTruth', this.position);
    }
    let __ok = false;
    try {
    this.consume('NOT');
    this.parseValue();
    this.consume('EOF');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('NegatedTruth', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('NegatedTruth', this.position);
        }
      }
    }
  }
  parseTruth() {
    const __memoStart = this.position;
    const __memoKey = 'Truth@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Truth at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Truth', this.position);
    }
    let __ok = false;
    try {
    this.parseValue();
    this.consume('EOF');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Truth', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Truth', this.position);
        }
      }
    }
  }
  parseEmpty() {
    const __memoStart = this.position;
    const __memoKey = 'Empty@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Empty at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Empty', this.position);
    }
    let __ok = false;
    try {
    this.consume('EOF');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Empty', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Empty', this.position);
        }
      }
    }
  }
  parseBracketNegatedBinary() {
    const __memoStart = this.position;
    const __memoKey = 'BracketNegatedBinary@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracketNegatedBinary at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracketNegatedBinary', this.position);
    }
    let __ok = false;
    try {
    this.consume('NOT');
    this.parseBracketBinary();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BracketNegatedBinary', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracketNegatedBinary', this.position);
        }
      }
    }
  }
  parseBracketBinary() {
    const __memoStart = this.position;
    const __memoKey = 'BracketBinary@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracketBinary at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracketBinary', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseValue();
    this.parseStringOperator();
    this.parseValue();
    this.consume('CLOSE');
    this.consume('EOF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDecimal();
    this.parseNumericOperator();
    this.parseDecimal();
    this.consume('CLOSE');
    this.consume('EOF');
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
          this.eventHandler.endNonterminal('BracketBinary', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracketBinary', this.position);
        }
      }
    }
  }
  parseBracketNegatedUnary() {
    const __memoStart = this.position;
    const __memoKey = 'BracketNegatedUnary@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracketNegatedUnary at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracketNegatedUnary', this.position);
    }
    let __ok = false;
    try {
    this.consume('NOT');
    this.parseBracketUnary();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BracketNegatedUnary', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracketNegatedUnary', this.position);
        }
      }
    }
  }
  parseBracketUnary() {
    const __memoStart = this.position;
    const __memoKey = 'BracketUnary@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracketUnary at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracketUnary', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNonemptyOperator();
    this.parseValue();
    this.consume('CLOSE');
    this.consume('EOF');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseEmptyOperator();
    this.parseValue();
    this.consume('CLOSE');
    this.consume('EOF');
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
          this.eventHandler.endNonterminal('BracketUnary', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracketUnary', this.position);
        }
      }
    }
  }
  parseBracketNegatedTruth() {
    const __memoStart = this.position;
    const __memoKey = 'BracketNegatedTruth@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracketNegatedTruth at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracketNegatedTruth', this.position);
    }
    let __ok = false;
    try {
    this.consume('NOT');
    this.parseValue();
    this.consume('CLOSE');
    this.consume('EOF');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BracketNegatedTruth', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracketNegatedTruth', this.position);
        }
      }
    }
  }
  parseBracketTruth() {
    const __memoStart = this.position;
    const __memoKey = 'BracketTruth@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracketTruth at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracketTruth', this.position);
    }
    let __ok = false;
    try {
    this.parseValue();
    this.consume('CLOSE');
    this.consume('EOF');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BracketTruth', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracketTruth', this.position);
        }
      }
    }
  }
  parseBracketEmpty() {
    const __memoStart = this.position;
    const __memoKey = 'BracketEmpty@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule BracketEmpty at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('BracketEmpty', this.position);
    }
    let __ok = false;
    try {
    this.consume('CLOSE');
    this.consume('EOF');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('BracketEmpty', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('BracketEmpty', this.position);
        }
      }
    }
  }
  parseStringOperator() {
    const __memoStart = this.position;
    const __memoKey = 'StringOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule StringOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('StringOperator', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseEqualOperator();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseUnequalOperator();
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
          this.eventHandler.endNonterminal('StringOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('StringOperator', this.position);
        }
      }
    }
  }
  parseEqualOperator() {
    const __memoStart = this.position;
    const __memoKey = 'EqualOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule EqualOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('EqualOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('EQ');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('EqualOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('EqualOperator', this.position);
        }
      }
    }
  }
  parseUnequalOperator() {
    const __memoStart = this.position;
    const __memoKey = 'UnequalOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule UnequalOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('UnequalOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('NE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('UnequalOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('UnequalOperator', this.position);
        }
      }
    }
  }
  parseNonemptyOperator() {
    const __memoStart = this.position;
    const __memoKey = 'NonemptyOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule NonemptyOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('NonemptyOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('NONEMPTY');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('NonemptyOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('NonemptyOperator', this.position);
        }
      }
    }
  }
  parseEmptyOperator() {
    const __memoStart = this.position;
    const __memoKey = 'EmptyOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule EmptyOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('EmptyOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('EMPTY');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('EmptyOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('EmptyOperator', this.position);
        }
      }
    }
  }
  parseNumericOperator() {
    const __memoStart = this.position;
    const __memoKey = 'NumericOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule NumericOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('NumericOperator', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNumericEqual();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseNumericUnequal();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseLess();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseLessEqual();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseGreater();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseGreaterEqual();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 6 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('NumericOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('NumericOperator', this.position);
        }
      }
    }
  }
  parseNumericEqual() {
    const __memoStart = this.position;
    const __memoKey = 'NumericEqual@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule NumericEqual at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('NumericEqual', this.position);
    }
    let __ok = false;
    try {
    this.consume('NUM_EQ');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('NumericEqual', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('NumericEqual', this.position);
        }
      }
    }
  }
  parseNumericUnequal() {
    const __memoStart = this.position;
    const __memoKey = 'NumericUnequal@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule NumericUnequal at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('NumericUnequal', this.position);
    }
    let __ok = false;
    try {
    this.consume('NUM_NE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('NumericUnequal', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('NumericUnequal', this.position);
        }
      }
    }
  }
  parseLess() {
    const __memoStart = this.position;
    const __memoKey = 'Less@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Less at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Less', this.position);
    }
    let __ok = false;
    try {
    this.consume('NUM_LT');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Less', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Less', this.position);
        }
      }
    }
  }
  parseLessEqual() {
    const __memoStart = this.position;
    const __memoKey = 'LessEqual@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule LessEqual at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('LessEqual', this.position);
    }
    let __ok = false;
    try {
    this.consume('NUM_LE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('LessEqual', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('LessEqual', this.position);
        }
      }
    }
  }
  parseGreater() {
    const __memoStart = this.position;
    const __memoKey = 'Greater@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Greater at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Greater', this.position);
    }
    let __ok = false;
    try {
    this.consume('NUM_GT');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Greater', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Greater', this.position);
        }
      }
    }
  }
  parseGreaterEqual() {
    const __memoStart = this.position;
    const __memoKey = 'GreaterEqual@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule GreaterEqual at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('GreaterEqual', this.position);
    }
    let __ok = false;
    try {
    this.consume('NUM_GE');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('GreaterEqual', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('GreaterEqual', this.position);
        }
      }
    }
  }
  parseDecimal() {
    const __memoStart = this.position;
    const __memoKey = 'Decimal@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Decimal at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Decimal', this.position);
    }
    let __ok = false;
    try {
    this.consume('DECIMAL');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Decimal', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Decimal', this.position);
        }
      }
    }
  }
  parseValue() {
    const __memoStart = this.position;
    const __memoKey = 'Value@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Value at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Value', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('OPEN');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('CLOSE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('TEST');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('NOT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('NONEMPTY');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EMPTY');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('EQ');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('NE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('NUM_EQ');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('NUM_NE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('NUM_LT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('NUM_LE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('NUM_GT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('NUM_GE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('DECIMAL');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('VALUE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      throw new Error(`Expected one of: 16 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Value', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Value', this.position);
        }
      }
    }
  }
}

module.exports = Parser;