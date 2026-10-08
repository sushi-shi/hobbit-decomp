"""Lexical removal of reconstruction annotations without touching literals."""

import re


def tokens(text):
    i = 0
    while i < len(text):
        start = i
        if text.startswith('//', i):
            i = text.find('\n', i)
            if i < 0:
                i = len(text)
            while i < len(text) and text[i - 1] == '\\':
                end = text.find('\n', i + 1)
                i = len(text) if end < 0 else end
            yield 'comment', text[start:i]
        elif text.startswith('/*', i):
            end = text.find('*/', i + 2)
            if end < 0:
                raise ValueError('unterminated comment')
            i = end + 2
            yield 'comment', text[start:i]
        elif text[i] in '\"\'':
            quote = text[i]
            i += 1
            while i < len(text) and text[i] != quote:
                i += 2 if text[i] == '\\' else 1
            if i >= len(text):
                raise ValueError('unterminated literal')
            i += 1
            yield 'literal', text[start:i]
        elif text[i].isalpha() or text[i] == '_':
            i += 1
            while i < len(text) and (text[i].isalnum() or text[i] == '_'):
                i += 1
            yield 'word', text[start:i]
        else:
            i += 1
            yield 'punctuation', text[start:i]


ARITY = {'RVA': 2, 'DATA': 1, 'RVA_COMPGEN': 3, 'RVA_DYNINIT': 3,
         'DATA_MESSAGE_MAP': 2, 'DATA_COMPGEN': 2}


def clean_source(text):
    # Translation phase 2 precedes comment recognition, including split tokens.
    text = re.sub(r'\\\r?\n', '', text)
    text = ''.join((' ' + '\n' * word.count('\n'))
                   if kind == 'comment' and not re.search(
                       r'copyright|SPDX-License|permission is hereby', word, re.I)
                   else word for kind, word in tokens(text))
    # Hobbit rva.h supplies annotations only.
    text = re.sub(r'(?m)^(\s*#\s*include\s*)[<"]rva\.h[>"]',
                  '', text)
    stream = list(tokens(text))

    def rewrite(parts):
        output = []
        i = 0
        while i < len(parts):
            kind, word = parts[i]
            if kind == 'word' and word == 'OVERRIDE':
                output.append(' ')
                i += 1
                continue
            if kind != 'word' or word not in ARITY:
                # DATA_* is also an original engine identifier family (for
                # example DATA_VAULT_HPP and DATA_TYPE_NONE), not a reserved
                # annotation namespace. Known DATA macros are handled above.
                if kind == 'word' and (word.startswith('RVA_')
                                       or word == 'HOBBIT_EMIT_META'):
                    raise ValueError(f'unknown reconstruction annotation: {word}')
                output.append(word)
                i += 1
                continue
            opening = i + 1
            while opening < len(parts) and parts[opening][1].isspace():
                opening += 1
            if opening == len(parts) or parts[opening][1] != '(':
                raise ValueError(f'{word}: expected macro invocation')
            stack, args = ['('], []
            start = cursor = opening + 1
            while cursor < len(parts):
                token_kind, spelling = parts[cursor]
                if token_kind == 'punctuation':
                    if spelling in '([{':
                        stack.append(spelling)
                    elif spelling in ')]}':
                        if not stack or stack.pop() != {')': '(', ']': '[', '}': '{'}[spelling]:
                            raise ValueError(f'{word}: unbalanced arguments')
                        if not stack:
                            args.append(parts[start:cursor])
                            break
                    elif spelling == ',' and len(stack) == 1:
                        args.append(parts[start:cursor])
                        start = cursor + 1
                cursor += 1
            else:
                raise ValueError(f'{word}: unterminated invocation')
            if len(args) != ARITY[word]:
                raise ValueError(f'{word}: expected {ARITY[word]} arguments, got {len(args)}')
            output.append(rewrite(args[1]).strip() if word == 'DATA_COMPGEN' else ' ')
            i = cursor + 1
        return ''.join(output)

    text = rewrite(stream)
    # Protect literals and license comments while tidying whitespace.
    protected = []
    if '\0' in text:
        raise ValueError('NUL in source')
    def protect(kind, word):
        if kind in ('literal', 'comment'):
            protected.append(word)
            return f'\0{len(protected) - 1}\0'
        return word
    text = ''.join(protect(kind, word) for kind, word in tokens(text))
    text = '\n'.join(line.rstrip() for line in text.splitlines())
    text = re.sub(r'\n{3,}', '\n\n', text).strip() + '\n'
    return re.sub(r'\0(\d+)\0', lambda match: protected[int(match[1])], text)
