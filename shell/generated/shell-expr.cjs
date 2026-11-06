class Lexer {
  constructor(input) {
    this.input = input;
    this.position = 0;
    this.tokens = [];
    this.charClassDepth = 0;
    this.templateDepth = 0;
    this.templateBraceDepthStack = [];
    this.tokenPatterns = [    { type: 'OPEN', regex: /^\(\u0000/ },    { type: 'CLOSE', regex: /^\)\u0000/ },    { type: 'OR', regex: /^\|\u0000/ },    { type: 'AND', regex: /^&\u0000/ },    { type: 'EQ', regex: /^=\u0000/ },    { type: 'NE', regex: /^!=\u0000/ },    { type: 'LE', regex: /^<=\u0000/ },    { type: 'GE', regex: /^>=\u0000/ },    { type: 'LT', regex: /^<\u0000/ },    { type: 'GT', regex: /^>\u0000/ },    { type: 'PLUS', regex: /^\+\u0000/ },    { type: 'MINUS', regex: /^-\u0000/ },    { type: 'STAR', regex: /^\*\u0000/ },    { type: 'SLASH', regex: /^\/\u0000/ },    { type: 'PERCENT', regex: /^%\u0000/ },    { type: 'COLON', regex: /^:\u0000/ },    { type: 'DECIMAL', regex: /^(?:-)?(?:[0-9])+\u0000/ },    { type: 'VALUE', regex: /^(?:[^\u0000])*\u0000/ },    ];
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
    this.parseExpression0();
    this.consume('EOF');

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
  parseExpression0() {
    const __memoStart = this.position;
    const __memoKey = 'Expression0@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Expression0 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Expression0', this.position);
    }
    let __ok = false;
    try {
    this.parseOr0();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Expression0', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Expression0', this.position);
        }
      }
    }
  }
  parseOr0() {
    const __memoStart = this.position;
    const __memoKey = 'Or0@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Or0 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Or0', this.position);
    }
    let __ok = false;
    try {
    this.parseAnd0();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseOrOperator();
    this.parseAnd0();
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
          this.eventHandler.endNonterminal('Or0', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Or0', this.position);
        }
      }
    }
  }
  parseAnd0() {
    const __memoStart = this.position;
    const __memoKey = 'And0@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule And0 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('And0', this.position);
    }
    let __ok = false;
    try {
    this.parseRelation0();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseAndOperator();
    this.parseRelation0();
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
          this.eventHandler.endNonterminal('And0', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('And0', this.position);
        }
      }
    }
  }
  parseRelation0() {
    const __memoStart = this.position;
    const __memoKey = 'Relation0@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Relation0 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Relation0', this.position);
    }
    let __ok = false;
    try {
    this.parseSum0();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseEqual();
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
    this.parseUnequal();
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
    this.parseLess();
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
    this.parseLessEqual();
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
    this.parseGreater();
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
    this.parseGreaterEqual();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseSum0();
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
          this.eventHandler.endNonterminal('Relation0', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Relation0', this.position);
        }
      }
    }
  }
  parseSum0() {
    const __memoStart = this.position;
    const __memoKey = 'Sum0@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Sum0 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Sum0', this.position);
    }
    let __ok = false;
    try {
    this.parseProduct0();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseAdd();
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
    this.parseSubtract();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseProduct0();
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
          this.eventHandler.endNonterminal('Sum0', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Sum0', this.position);
        }
      }
    }
  }
  parseProduct0() {
    const __memoStart = this.position;
    const __memoKey = 'Product0@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Product0 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Product0', this.position);
    }
    let __ok = false;
    try {
    this.parseMatch0();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseMultiply();
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
    this.parseDivide();
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
    this.parseRemainder();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseMatch0();
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
          this.eventHandler.endNonterminal('Product0', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Product0', this.position);
        }
      }
    }
  }
  parseMatch0() {
    const __memoStart = this.position;
    const __memoKey = 'Match0@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Match0 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Match0', this.position);
    }
    let __ok = false;
    try {
    this.parsePrimary0();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseMatchOperator();
    this.parsePattern();
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
          this.eventHandler.endNonterminal('Match0', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Match0', this.position);
        }
      }
    }
  }
  parsePrimary0() {
    const __memoStart = this.position;
    const __memoKey = 'Primary0@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Primary0 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Primary0', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('OPEN');
    this.parseExpression1();
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
    this.parseValue();
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
          this.eventHandler.endNonterminal('Primary0', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Primary0', this.position);
        }
      }
    }
  }
  parseExpression1() {
    const __memoStart = this.position;
    const __memoKey = 'Expression1@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Expression1 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Expression1', this.position);
    }
    let __ok = false;
    try {
    this.parseOr1();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Expression1', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Expression1', this.position);
        }
      }
    }
  }
  parseOr1() {
    const __memoStart = this.position;
    const __memoKey = 'Or1@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Or1 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Or1', this.position);
    }
    let __ok = false;
    try {
    this.parseAnd1();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseOrOperator();
    this.parseAnd1();
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
          this.eventHandler.endNonterminal('Or1', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Or1', this.position);
        }
      }
    }
  }
  parseAnd1() {
    const __memoStart = this.position;
    const __memoKey = 'And1@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule And1 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('And1', this.position);
    }
    let __ok = false;
    try {
    this.parseRelation1();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseAndOperator();
    this.parseRelation1();
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
          this.eventHandler.endNonterminal('And1', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('And1', this.position);
        }
      }
    }
  }
  parseRelation1() {
    const __memoStart = this.position;
    const __memoKey = 'Relation1@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Relation1 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Relation1', this.position);
    }
    let __ok = false;
    try {
    this.parseSum1();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseEqual();
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
    this.parseUnequal();
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
    this.parseLess();
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
    this.parseLessEqual();
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
    this.parseGreater();
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
    this.parseGreaterEqual();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseSum1();
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
          this.eventHandler.endNonterminal('Relation1', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Relation1', this.position);
        }
      }
    }
  }
  parseSum1() {
    const __memoStart = this.position;
    const __memoKey = 'Sum1@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Sum1 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Sum1', this.position);
    }
    let __ok = false;
    try {
    this.parseProduct1();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseAdd();
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
    this.parseSubtract();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseProduct1();
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
          this.eventHandler.endNonterminal('Sum1', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Sum1', this.position);
        }
      }
    }
  }
  parseProduct1() {
    const __memoStart = this.position;
    const __memoKey = 'Product1@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Product1 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Product1', this.position);
    }
    let __ok = false;
    try {
    this.parseMatch1();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseMultiply();
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
    this.parseDivide();
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
    this.parseRemainder();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseMatch1();
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
          this.eventHandler.endNonterminal('Product1', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Product1', this.position);
        }
      }
    }
  }
  parseMatch1() {
    const __memoStart = this.position;
    const __memoKey = 'Match1@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Match1 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Match1', this.position);
    }
    let __ok = false;
    try {
    this.parsePrimary1();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseMatchOperator();
    this.parsePattern();
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
          this.eventHandler.endNonterminal('Match1', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Match1', this.position);
        }
      }
    }
  }
  parsePrimary1() {
    const __memoStart = this.position;
    const __memoKey = 'Primary1@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Primary1 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Primary1', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('OPEN');
    this.parseExpression2();
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
    this.parseValue();
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
          this.eventHandler.endNonterminal('Primary1', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Primary1', this.position);
        }
      }
    }
  }
  parseExpression2() {
    const __memoStart = this.position;
    const __memoKey = 'Expression2@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Expression2 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Expression2', this.position);
    }
    let __ok = false;
    try {
    this.parseOr2();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Expression2', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Expression2', this.position);
        }
      }
    }
  }
  parseOr2() {
    const __memoStart = this.position;
    const __memoKey = 'Or2@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Or2 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Or2', this.position);
    }
    let __ok = false;
    try {
    this.parseAnd2();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseOrOperator();
    this.parseAnd2();
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
          this.eventHandler.endNonterminal('Or2', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Or2', this.position);
        }
      }
    }
  }
  parseAnd2() {
    const __memoStart = this.position;
    const __memoKey = 'And2@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule And2 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('And2', this.position);
    }
    let __ok = false;
    try {
    this.parseRelation2();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseAndOperator();
    this.parseRelation2();
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
          this.eventHandler.endNonterminal('And2', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('And2', this.position);
        }
      }
    }
  }
  parseRelation2() {
    const __memoStart = this.position;
    const __memoKey = 'Relation2@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Relation2 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Relation2', this.position);
    }
    let __ok = false;
    try {
    this.parseSum2();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseEqual();
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
    this.parseUnequal();
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
    this.parseLess();
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
    this.parseLessEqual();
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
    this.parseGreater();
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
    this.parseGreaterEqual();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseSum2();
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
          this.eventHandler.endNonterminal('Relation2', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Relation2', this.position);
        }
      }
    }
  }
  parseSum2() {
    const __memoStart = this.position;
    const __memoKey = 'Sum2@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Sum2 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Sum2', this.position);
    }
    let __ok = false;
    try {
    this.parseProduct2();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseAdd();
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
    this.parseSubtract();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseProduct2();
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
          this.eventHandler.endNonterminal('Sum2', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Sum2', this.position);
        }
      }
    }
  }
  parseProduct2() {
    const __memoStart = this.position;
    const __memoKey = 'Product2@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Product2 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Product2', this.position);
    }
    let __ok = false;
    try {
    this.parseMatch2();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseMultiply();
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
    this.parseDivide();
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
    this.parseRemainder();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseMatch2();
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
          this.eventHandler.endNonterminal('Product2', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Product2', this.position);
        }
      }
    }
  }
  parseMatch2() {
    const __memoStart = this.position;
    const __memoKey = 'Match2@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Match2 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Match2', this.position);
    }
    let __ok = false;
    try {
    this.parsePrimary2();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseMatchOperator();
    this.parsePattern();
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
          this.eventHandler.endNonterminal('Match2', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Match2', this.position);
        }
      }
    }
  }
  parsePrimary2() {
    const __memoStart = this.position;
    const __memoKey = 'Primary2@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Primary2 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Primary2', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('OPEN');
    this.parseExpression3();
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
    this.parseValue();
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
          this.eventHandler.endNonterminal('Primary2', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Primary2', this.position);
        }
      }
    }
  }
  parseExpression3() {
    const __memoStart = this.position;
    const __memoKey = 'Expression3@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Expression3 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Expression3', this.position);
    }
    let __ok = false;
    try {
    this.parseOr3();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Expression3', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Expression3', this.position);
        }
      }
    }
  }
  parseOr3() {
    const __memoStart = this.position;
    const __memoKey = 'Or3@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Or3 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Or3', this.position);
    }
    let __ok = false;
    try {
    this.parseAnd3();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseOrOperator();
    this.parseAnd3();
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
          this.eventHandler.endNonterminal('Or3', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Or3', this.position);
        }
      }
    }
  }
  parseAnd3() {
    const __memoStart = this.position;
    const __memoKey = 'And3@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule And3 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('And3', this.position);
    }
    let __ok = false;
    try {
    this.parseRelation3();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseAndOperator();
    this.parseRelation3();
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
          this.eventHandler.endNonterminal('And3', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('And3', this.position);
        }
      }
    }
  }
  parseRelation3() {
    const __memoStart = this.position;
    const __memoKey = 'Relation3@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Relation3 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Relation3', this.position);
    }
    let __ok = false;
    try {
    this.parseSum3();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseEqual();
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
    this.parseUnequal();
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
    this.parseLess();
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
    this.parseLessEqual();
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
    this.parseGreater();
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
    this.parseGreaterEqual();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseSum3();
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
          this.eventHandler.endNonterminal('Relation3', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Relation3', this.position);
        }
      }
    }
  }
  parseSum3() {
    const __memoStart = this.position;
    const __memoKey = 'Sum3@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Sum3 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Sum3', this.position);
    }
    let __ok = false;
    try {
    this.parseProduct3();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseAdd();
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
    this.parseSubtract();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseProduct3();
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
          this.eventHandler.endNonterminal('Sum3', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Sum3', this.position);
        }
      }
    }
  }
  parseProduct3() {
    const __memoStart = this.position;
    const __memoKey = 'Product3@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Product3 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Product3', this.position);
    }
    let __ok = false;
    try {
    this.parseMatch3();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseMultiply();
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
    this.parseDivide();
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
    this.parseRemainder();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseMatch3();
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
          this.eventHandler.endNonterminal('Product3', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Product3', this.position);
        }
      }
    }
  }
  parseMatch3() {
    const __memoStart = this.position;
    const __memoKey = 'Match3@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Match3 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Match3', this.position);
    }
    let __ok = false;
    try {
    this.parsePrimary3();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseMatchOperator();
    this.parsePattern();
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
          this.eventHandler.endNonterminal('Match3', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Match3', this.position);
        }
      }
    }
  }
  parsePrimary3() {
    const __memoStart = this.position;
    const __memoKey = 'Primary3@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Primary3 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Primary3', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('OPEN');
    this.parseExpression4();
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
    this.parseValue();
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
          this.eventHandler.endNonterminal('Primary3', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Primary3', this.position);
        }
      }
    }
  }
  parseExpression4() {
    const __memoStart = this.position;
    const __memoKey = 'Expression4@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Expression4 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Expression4', this.position);
    }
    let __ok = false;
    try {
    this.parseOr4();

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Expression4', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Expression4', this.position);
        }
      }
    }
  }
  parseOr4() {
    const __memoStart = this.position;
    const __memoKey = 'Or4@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Or4 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Or4', this.position);
    }
    let __ok = false;
    try {
    this.parseAnd4();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseOrOperator();
    this.parseAnd4();
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
          this.eventHandler.endNonterminal('Or4', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Or4', this.position);
        }
      }
    }
  }
  parseAnd4() {
    const __memoStart = this.position;
    const __memoKey = 'And4@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule And4 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('And4', this.position);
    }
    let __ok = false;
    try {
    this.parseRelation4();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseAndOperator();
    this.parseRelation4();
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
          this.eventHandler.endNonterminal('And4', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('And4', this.position);
        }
      }
    }
  }
  parseRelation4() {
    const __memoStart = this.position;
    const __memoKey = 'Relation4@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Relation4 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Relation4', this.position);
    }
    let __ok = false;
    try {
    this.parseSum4();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseEqual();
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
    this.parseUnequal();
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
    this.parseLess();
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
    this.parseLessEqual();
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
    this.parseGreater();
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
    this.parseGreaterEqual();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseSum4();
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
          this.eventHandler.endNonterminal('Relation4', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Relation4', this.position);
        }
      }
    }
  }
  parseSum4() {
    const __memoStart = this.position;
    const __memoKey = 'Sum4@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Sum4 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Sum4', this.position);
    }
    let __ok = false;
    try {
    this.parseProduct4();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseAdd();
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
    this.parseSubtract();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseProduct4();
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
          this.eventHandler.endNonterminal('Sum4', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Sum4', this.position);
        }
      }
    }
  }
  parseProduct4() {
    const __memoStart = this.position;
    const __memoKey = 'Product4@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Product4 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Product4', this.position);
    }
    let __ok = false;
    try {
    this.parseMatch4();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    // Group
    {
      let _matchedAlt = false;
      if (!_matchedAlt) {
        const _altStart = this.position;
        const _altMark = this.markEventState();
        try {
    this.parseMultiply();
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
    this.parseDivide();
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
    this.parseRemainder();
          _matchedAlt = true;
        } catch (e) {
          this.position = _altStart;
          this.restoreEventState(_altMark);
        }
      }
      if (!_matchedAlt) { throw new Error('No group alternative matched'); }
    }
    this.parseMatch4();
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
          this.eventHandler.endNonterminal('Product4', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Product4', this.position);
        }
      }
    }
  }
  parseMatch4() {
    const __memoStart = this.position;
    const __memoKey = 'Match4@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Match4 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Match4', this.position);
    }
    let __ok = false;
    try {
    this.parsePrimary4();
    // Group *
    while (true) {
      const _loopStart = this.position;
      const _loopMark = this.markEventState();
      try {
    this.parseMatchOperator();
    this.parsePattern();
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
          this.eventHandler.endNonterminal('Match4', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Match4', this.position);
        }
      }
    }
  }
  parsePrimary4() {
    const __memoStart = this.position;
    const __memoKey = 'Primary4@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Primary4 at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Primary4', this.position);
    }
    let __ok = false;
    try {
    const _ruleStart = this.position;
    let _matched = false;
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseValue();
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.parseDepthOverflow();
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
          this.eventHandler.endNonterminal('Primary4', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Primary4', this.position);
        }
      }
    }
  }
  parseDepthOverflow() {
    const __memoStart = this.position;
    const __memoKey = 'DepthOverflow@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule DepthOverflow at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('DepthOverflow', this.position);
    }
    let __ok = false;
    try {
    this.consume('OPEN');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('DepthOverflow', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('DepthOverflow', this.position);
        }
      }
    }
  }
  parseOrOperator() {
    const __memoStart = this.position;
    const __memoKey = 'OrOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule OrOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('OrOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('OR');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('OrOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('OrOperator', this.position);
        }
      }
    }
  }
  parseAndOperator() {
    const __memoStart = this.position;
    const __memoKey = 'AndOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule AndOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('AndOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('AND');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('AndOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('AndOperator', this.position);
        }
      }
    }
  }
  parseEqual() {
    const __memoStart = this.position;
    const __memoKey = 'Equal@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Equal at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Equal', this.position);
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
          this.eventHandler.endNonterminal('Equal', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Equal', this.position);
        }
      }
    }
  }
  parseUnequal() {
    const __memoStart = this.position;
    const __memoKey = 'Unequal@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Unequal at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Unequal', this.position);
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
          this.eventHandler.endNonterminal('Unequal', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Unequal', this.position);
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
    this.consume('LT');

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
    this.consume('LE');

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
    this.consume('GT');

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
    this.consume('GE');

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
  parseAdd() {
    const __memoStart = this.position;
    const __memoKey = 'Add@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Add at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Add', this.position);
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
          this.eventHandler.endNonterminal('Add', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Add', this.position);
        }
      }
    }
  }
  parseSubtract() {
    const __memoStart = this.position;
    const __memoKey = 'Subtract@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Subtract at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Subtract', this.position);
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
          this.eventHandler.endNonterminal('Subtract', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Subtract', this.position);
        }
      }
    }
  }
  parseMultiply() {
    const __memoStart = this.position;
    const __memoKey = 'Multiply@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Multiply at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Multiply', this.position);
    }
    let __ok = false;
    try {
    this.consume('STAR');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Multiply', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Multiply', this.position);
        }
      }
    }
  }
  parseDivide() {
    const __memoStart = this.position;
    const __memoKey = 'Divide@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Divide at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Divide', this.position);
    }
    let __ok = false;
    try {
    this.consume('SLASH');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Divide', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Divide', this.position);
        }
      }
    }
  }
  parseRemainder() {
    const __memoStart = this.position;
    const __memoKey = 'Remainder@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Remainder at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Remainder', this.position);
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
          this.eventHandler.endNonterminal('Remainder', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Remainder', this.position);
        }
      }
    }
  }
  parseMatchOperator() {
    const __memoStart = this.position;
    const __memoKey = 'MatchOperator@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule MatchOperator at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('MatchOperator', this.position);
    }
    let __ok = false;
    try {
    this.consume('COLON');

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('MatchOperator', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('MatchOperator', this.position);
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
      throw new Error(`Expected one of: 2 alternatives`);
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
  parsePattern() {
    const __memoStart = this.position;
    const __memoKey = 'Pattern@' + __memoStart;
    if (this.failureMemo.has(__memoKey)) {
      throw new Error('Previously failed rule Pattern at position ' + __memoStart);
    }
    if (this.eventHandler && typeof this.eventHandler.startNonterminal === 'function') {
      this.eventHandler.startNonterminal('Pattern', this.position);
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
    this.consume('OR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('AND');
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
    this.consume('LT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('LE');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('GT');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('GE');
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
    this.consume('STAR');
        _matched = true;
      } catch (e) {
        this.position = _ruleStart;
        this.restoreEventState(_ruleMark);
      }
    }
    if (!_matched) {
      const _ruleMark = this.markEventState();
      try {
    this.consume('SLASH');
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
      throw new Error(`Expected one of: 18 alternatives`);
    }

      __ok = true;
    } finally {
      if (!__ok) {
        this.failureMemo.add(__memoKey);
      }
      if (this.eventHandler) {
        if (__ok && typeof this.eventHandler.endNonterminal === 'function') {
          this.eventHandler.endNonterminal('Pattern', this.position);
        }
        if (!__ok && typeof this.eventHandler.abortNonterminal === 'function') {
          this.eventHandler.abortNonterminal('Pattern', this.position);
        }
      }
    }
  }
}

module.exports = Parser;