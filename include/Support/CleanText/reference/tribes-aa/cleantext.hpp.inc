#ifndef __CLEANTEXT_HPP
#define __CLEANTEXT_HPP

#include <xCore/x_files/x_files.hpp>

// The CleanLanguage class will parse any set of input ascii data
// in the english language, look for objectionable words, and replace them
// with non-objectionable words.  It uses a fuzzy string compare routine to
// detect inappropriate language even if it is not spelled exactly
// correctly.  Certainly users can find ways to defeat this parser, but
// it has proven amusing and harmless in the past.  The word lists and
// intermediate strings  are all converted into another alphabet, so you
// shouldn't find yourself offended by the language in the code, or even
// hackers dissassembling it for that matter.
//
// This code makes use of STL for String operations.
//
// OpenSourced July 25, 2000, by John W. Ratcliff (jratcliff@verant.com)
// for FlipCode.com.


class Translate; // forward reference ascii translation table.

class CleanLanguage
{
public:
    CleanLanguage(void);
    ~CleanLanguage(void);

    xbool             CheckWord(const char *pString) const;

private:
};

#endif // __CLEANTEXT_HPP