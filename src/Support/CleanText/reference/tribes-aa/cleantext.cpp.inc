#include "cleantext.hpp"
#include "cleantext.hpp"

// Database.

static const char *BadWordList[]=
{
   "shit",
   "fuck",
   "cock",
   "bitch",
   "cunt",
   "nigger",
   "bastard",
   "dick",
   "whore",
   "goddamn",
   "asshole",
   NULL,
};


CleanLanguage::CleanLanguage()
{
}

CleanLanguage::~CleanLanguage()
{
}

// See if the word is 'bad' and return a cleaned up version in good.
// Returns percentage match if any.
xbool CleanLanguage::CheckWord(const char *pString) const
{
    const char **pWordList;

    char    LowerCaseString[128];

    pWordList = BadWordList;

    x_strcpy(LowerCaseString,pString);
    x_strtolower(LowerCaseString);

    while (*pWordList)
    {
        if (x_strstr(LowerCaseString,*pWordList))
        {
            return TRUE;
        }
        pWordList++;
    }
    return FALSE;
}

