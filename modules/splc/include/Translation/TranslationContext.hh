#ifndef __SPLC_TRANSLATION_TRANSLATIONCONTEXT_HH__
#define __SPLC_TRANSLATION_TRANSLATIONCONTEXT_HH__ 1

#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

#include "Core/splc.hh"

#include "Translation/TranslationBase.hh"

namespace splc {

enum TranslationContextBufferType {
    File,
    MacroExpansion,
};

class TranslationContext {
  public:
    TranslationContext() = delete;

    TranslationContext(TranslationContextBufferType type_,
                       const std::string &name_, const Location &intrLocation_,
                       Ptr<std::istream> inputStream_)
        : type{type_}, name{name_}, intrLocation{intrLocation_}, content{},
          inputStream{inputStream_}
    {
    }

    TranslationContext(TranslationContextBufferType type_,
                       const std::string &name_, const Location &intrLocation_,
                       const std::string &content_,
                       Ptr<std::istream> inputStream_)
        : type{type_}, name{name_},
          intrLocation(intrLocation_), content{content_},
          inputStream{inputStream_}
    {
    }

    const TranslationContextBufferType type;
    const std::string name;
    const Location intrLocation; // Interrupt Location
    const std::string content;
    Ptr<std::istream> inputStream;
};

} // namespace splc

#endif // __SPLC_TRANSLATION_TRANSLATIONCONTEXT_HH__