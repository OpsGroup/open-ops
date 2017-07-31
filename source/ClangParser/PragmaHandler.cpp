#include "OPS_Core/disable_llvm_warnings_begin.h"
#include "ClangParser/PragmaHandler.h"
#include "clang/Lex/Token.h"
#include "clang/Lex/Preprocessor.h"
#include "OPS_Core/disable_llvm_warnings_end.h"
#include "OPS_Core/Strings.h"

using namespace clang;

namespace OPS
{
namespace ClangParser
{

class BlockArrayDeclareHandler: public PragmaHandlerBase
{
public:
    BlockArrayDeclareHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};
class BlockArrayAllocateHandler: public PragmaHandlerBase
{
public:
    BlockArrayAllocateHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};
class BlockArrayReleaseHandler: public PragmaHandlerBase
{
public:
    BlockArrayReleaseHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};

class SingleAccessPragmaHandler: public PragmaHandlerBase
{
public:
    SingleAccessPragmaHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};

class NestingPragmaHandler: public PragmaHandlerBase
{
public:
    NestingPragmaHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};

class BlockNestingPragmaHandler: public PragmaHandlerBase
{
public:
    BlockNestingPragmaHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};

class DistributionPragmaHandler: public PragmaHandlerBase
{
public:
    DistributionPragmaHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};

class IgnorePragmaHandler: public PragmaHandlerBase
{
public:
    IgnorePragmaHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};

class TestArgumentPragmaHandler: public PragmaHandlerBase
{
public:
    TestArgumentPragmaHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};

class TargetPragmaHandler: public PragmaHandlerBase
{
public:
    TargetPragmaHandler(Pragmas& p);

    void HandlePragma(clang::Preprocessor &PP, clang::PragmaIntroducerKind Introducer,
                      clang::Token &tok);
};

class TilingPragmaHandler : public PragmaHandlerBase
{
public:
    TilingPragmaHandler(Pragmas& p);

    void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer, Token &FirstToken);
};

class VectorizePragmaHandler: public PragmaHandlerBase
{
public:
    VectorizePragmaHandler(Pragmas& p);

    void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer, Token &FirstToken);
};

class LoopDistributionPragmaHandler: public PragmaHandlerBase
{
public:
    LoopDistributionPragmaHandler(Pragmas& p);

    void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer, Token &FirstToken);
};

class TempArraysPragmaHandler: public PragmaHandlerBase
{
public:
    TempArraysPragmaHandler(Pragmas& p);

    void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer, Token &FirstToken);
};

class VarToArrayPragmaHandler: public PragmaHandlerBase
{
public:
    VarToArrayPragmaHandler(Pragmas& p);

    void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer, Token &FirstToken);
};

class LoopFragmentationPragmaHandler: public PragmaHandlerBase
{
public:
    LoopFragmentationPragmaHandler(Pragmas& p);

    void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer, Token &FirstToken);
};

void BlockArrayDeclareHandler::HandlePragma(Preprocessor &PP,PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    PP.Lex(tok);
    std::string argumentString;
    if(tok.isNot(tok::l_paren))
        return;
    PP.Lex(tok);

    if(tok.isNot(tok::identifier))
        return;
    argumentString = tok.getIdentifierInfo()->getName().data();
    PP.Lex(tok);

    if(tok.isNot(tok::comma))
        return;
    PP.Lex(tok);

    if(tok.isNot(tok::numeric_constant))
        return;
    int dimension = 0;
    if (OPS::Strings::fetch(std::string(tok.getLiteralData(), tok.getLength()), dimension) == false
            || dimension <= 0)
        return;
    argumentString += OPS::Strings::format(",%d", dimension);
    PP.Lex(tok);

    for (int i = 0; i < 2*dimension; i++)
    {
        if(tok.isNot(tok::comma))
            return;
        PP.Lex(tok);

        if(tok.is(tok::identifier))
        {
            argumentString += OPS::Strings::format(",%s", tok.getIdentifierInfo()->getName().data());
            PP.Lex(tok);
            continue;
        }

        if(tok.is(tok::numeric_constant))
        {
            int value = 0;
            if (OPS::Strings::fetch(std::string(tok.getLiteralData(), tok.getLength()), value) == false
                    || value <= 0)
                return;
            argumentString += OPS::Strings::format(",%d", value);
            PP.Lex(tok);
            continue;
        }

        return;
    }


    if (tok.isNot(tok::r_paren))
        return;
    registerPragma(pragmaLocation, "block_array_declare", argumentString);
}
void BlockArrayAllocateHandler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind , Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    PP.Lex(tok);

    if(tok.isNot(tok::l_paren))
        return;

    PP.Lex(tok);
    if (tok.isNot(tok::identifier))
        return;
    std::string argument = tok.getIdentifierInfo()->getName().data();
    PP.Lex(tok);

    if (tok.isNot(tok::r_paren))
        return;

    registerPragma(pragmaLocation, "block_array_allocate", argument);
}
void BlockArrayReleaseHandler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind , Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    PP.Lex(tok);

    if(tok.isNot(tok::l_paren))
        return;

    PP.Lex(tok);
    if (tok.isNot(tok::identifier))
        return;
    std::string argument = tok.getIdentifierInfo()->getName().data();
    PP.Lex(tok);

    if (tok.isNot(tok::r_paren))
        return;

    registerPragma(pragmaLocation, "block_array_release", argument);
}

void DistributionPragmaHandler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    PP.Lex(tok);
    if (tok.isNot(tok::identifier))
    {
        if (tok.isNot(tok::numeric_constant))
        {
            registerPragma(pragmaLocation, "distribute", "");
            return;
        }
        else
        {
            registerPragma(pragmaLocation, "distribute", std::string(tok.getLiteralData(), tok.getLength()));
            return;
        }
    } else {
        std::string argumentString;
        const IdentifierInfo* II = tok.getIdentifierInfo();
        if (II->isStr("data") /*|| II->isStr("calculation") || II->isStr("results")*/)
        {
            argumentString += II->getName().str();

            PP.Lex(tok);
            // список аргументов начинается со скобки
            if(tok.isNot(tok::l_paren))
            {
                //just skip
                return;
            }
            argumentString += "(";

            {
                // читаем аргументы
                PP.Lex(tok);
                do {
                    if(tok.isNot(tok::numeric_constant) && tok.isNot(tok::identifier))
                    {
                        //just skip
                        return;
                    }
                    if(tok.is(tok::numeric_constant))
                    {
                        argumentString += std::string(tok.getLiteralData(), tok.getLength());
                    }
                    if(tok.is(tok::identifier))
                    {
                        argumentString += std::string(tok.getIdentifierInfo()->getName());
                    }

                    PP.Lex(tok);
                    if(tok.isNot(tok::comma))
                    {
                        continue;
                    }
                    PP.Lex(tok); // eat the comma!
                    argumentString += ",";

                } while(tok.isNot(tok::r_paren));
            }

            // список аргументов заканчивается скобкой
            if(tok.isNot(tok::r_paren))
            {
                //just skip
                return;
            }
            argumentString += ")";

        } else
        {
            argumentString += II->getName().str();
            // прочтем всю строку до конца
            do {
                PP.Lex(tok);
                if(tok.is(tok::numeric_constant))
                {
                    argumentString += std::string(tok.getLiteralData(), tok.getLength());
                }
                if(tok.is(tok::identifier))
                {
                    argumentString += std::string(tok.getIdentifierInfo()->getName());
                }
                if(tok.is(tok::comma))
                {
                    argumentString += ",";
                }
                if(tok.is(tok::l_paren))
                {
                    argumentString += "(";
                }
                if(tok.is(tok::r_paren))
                {
                    argumentString += ")";
                }
            } while(tok.isNot(tok::eod) && tok.isNot(tok::unknown));
        }

        registerPragma(pragmaLocation, "distribute", argumentString);
    }
}

void NestingPragmaHandler::HandlePragma(Preprocessor &PP,PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    registerPragma(pragmaLocation, "nesting", "");
}

void BlockNestingPragmaHandler::HandlePragma(Preprocessor &PP,PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    PP.Lex(tok);
    std::string argumentString;
    if(tok.isNot(tok::l_paren))
    {
        //just skip
        return;
    }
    argumentString += "(";

    {
        // читаем аргументы
        PP.Lex(tok);
        do {
            if(tok.isNot(tok::numeric_constant))
            {
                //just skip
                return;
            }
            argumentString += std::string(tok.getLiteralData(), tok.getLength());

            PP.Lex(tok);
            if(tok.isNot(tok::comma))
            {
                continue;
            }
            PP.Lex(tok); // eat the comma!
            argumentString += ",";

        } while(tok.isNot(tok::r_paren));
    }
    argumentString += ")";

    registerPragma(pragmaLocation, "block", argumentString);
}

void IgnorePragmaHandler::HandlePragma(Preprocessor &PP,PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    registerPragma(pragmaLocation, "ignore", "");
}

void TestArgumentPragmaHandler::HandlePragma(Preprocessor &PP,PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    PP.Lex(tok);
    std::string argumentName;
    if (tok.isNot(tok::identifier))
    {
        return;
    } else {
        const IdentifierInfo* II = tok.getIdentifierInfo();
        argumentName = II->getName().str();
    }

    registerPragma(pragmaLocation, "test_argument", argumentName);
}

void VectorizePragmaHandler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    registerPragma(pragmaLocation, "vectorize", "");
}


void VarToArrayPragmaHandler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    registerPragma(pragmaLocation, "vartoarray", "");
}


void LoopDistributionPragmaHandler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    registerPragma(pragmaLocation, "distribute_loop", "");
}


void TempArraysPragmaHandler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    registerPragma(pragmaLocation, "temparrays", "");
}

void SingleAccessPragmaHandler::HandlePragma(Preprocessor &PP,PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    PP.Lex(tok);
    if (tok.isNot(tok::numeric_constant))
    {
        registerPragma(pragmaLocation, "single_access", "");
        return;
    }
    else
    {
        registerPragma(pragmaLocation, "single_access", std::string(tok.getLiteralData(), tok.getLength()));
        return;
    }
}

void TargetPragmaHandler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind, Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    std::string params;
    PP.Lex(tok); // collapse
    if (tok.isAnyIdentifier() && tok.getIdentifierInfo()->getName().str() == "collapse")
    {
        PP.Lex(tok);
        if (tok.is(tok::l_paren))
        {
            PP.Lex(tok);
            if (tok.isLiteral()) params = std::string(tok.getLiteralData(), tok.getLength());
        }
    }
    registerPragma(pragmaLocation, "target", params);
}

void TilingPragmaHandler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind , Token &tok)
{
    SourceLocation pragmaLocation = tok.getLocation();
    PP.Lex(tok);
    std::string argumentString;
    if(tok.isNot(tok::l_paren))
    {
        // just skip
        return;
    }

    {
        PP.Lex(tok);
        do
        {
            if (tok.isNot(tok::numeric_constant))
            {
                // just skip
                return;
            }

            argumentString += std::string(tok.getLiteralData(), tok.getLength());

            PP.Lex(tok);
            if(tok.isNot(tok::comma))
            {
                continue;
            }
            PP.Lex(tok); // eat the comma!
            argumentString += ",";

        } while(tok.isNot(tok::r_paren));
    }

    registerPragma(pragmaLocation, "transform_tile", argumentString);
}

BlockArrayDeclareHandler::BlockArrayDeclareHandler(Pragmas& p)
    :PragmaHandlerBase("block_array_declare", p) {}
BlockArrayAllocateHandler::BlockArrayAllocateHandler(Pragmas& p)
    :PragmaHandlerBase("block_array_allocate", p) {}
BlockArrayReleaseHandler::BlockArrayReleaseHandler(Pragmas& p)
    :PragmaHandlerBase("block_array_release", p) {}
void LoopFragmentationPragmaHandler::HandlePragma(Preprocessor &PP,
												  PragmaIntroducerKind,
												  Token &token)
{
	SourceLocation pragmaLocation = token.getLocation();
	PP.Lex(token);
	std::string argumentString;

	// read '('
	if (token.isNot(tok::l_paren))
		return;

	PP.Lex(token);

	// read sizes of blocks (size1, size2, ...)
	size_t blockCt = 0;
	
	do
	{
		// next number - next block size
		if (token.isNot(tok::numeric_constant)) return;
		
		argumentString
			+= std::string(token.getLiteralData(), token.getLength()) + " ";
		PP.Lex(token);
		blockCt++;
		
		// read ','
		if (token.is(tok::comma))
			PP.Lex(token);
		else if (token.isNot(tok::r_paren))
			return;
	}
	while (token.isNot(tok::r_paren) && token.isNot(tok::eod)
		&& token.isNot(tok::unknown));

	// read ')'
	if (token.isNot(tok::r_paren)) return;

	// add count of blocks in begin of string
	argumentString = std::to_string(blockCt) + " " + argumentString;

	// read additional parameters
	PP.Lex(token);
	bool intoNewBlock = false;
	bool disDistrCheck = false;
	bool disInterCheck = false;
	bool deleteTails = false;
	
	while (token.isNot(tok::eod) && token.isNot(tok::unknown))
	{
		if (!token.isAnyIdentifier()) return;
		
		std::string paramName = token.getIdentifierInfo()->getName().str();
		
		if (paramName == "INTO_NEW_BLOCK")
			intoNewBlock = true;
		else if (paramName == "DISABLE_DISTRIBUTION_CHECK")
			disDistrCheck = true;
		else if (paramName == "DISABLE_INTERCHANGE_CHECK")
			disInterCheck = true;
		else if (paramName == "DISABLE_ALL_CHECKS")
			disDistrCheck = disInterCheck = true;
		else if (paramName == "DELETE_TAILS")
			deleteTails = true;
		else return;
		
		PP.Lex(token);
	}
	
	// add additional parameters to argument string
	argumentString = argumentString + " " + std::to_string(intoNewBlock);
	argumentString = argumentString + " " + std::to_string(disDistrCheck);
	argumentString = argumentString + " " + std::to_string(disInterCheck);
	argumentString = argumentString + " " + std::to_string(deleteTails);

	registerPragma(pragmaLocation, "block_nest", argumentString);
}


DistributionPragmaHandler::DistributionPragmaHandler(Pragmas& p)
    :PragmaHandlerBase("distribute", p) {}

IgnorePragmaHandler::IgnorePragmaHandler(Pragmas& p)
    :PragmaHandlerBase("ignore", p) {}

NestingPragmaHandler::NestingPragmaHandler(Pragmas& p)
    :PragmaHandlerBase("parallel_loop_nesting", p) {}

BlockNestingPragmaHandler::BlockNestingPragmaHandler(Pragmas& p)
    :PragmaHandlerBase("parallel_loop_block", p) {}

SingleAccessPragmaHandler::SingleAccessPragmaHandler(Pragmas& p)
    :PragmaHandlerBase("single_access", p) {}

TestArgumentPragmaHandler::TestArgumentPragmaHandler(Pragmas& p)
    :PragmaHandlerBase("test_argument", p) {}

TargetPragmaHandler::TargetPragmaHandler(Pragmas& p)
    :PragmaHandlerBase("target", p) {}

TilingPragmaHandler::TilingPragmaHandler(Pragmas &p)
    :PragmaHandlerBase("tile", p) {}

VectorizePragmaHandler::VectorizePragmaHandler(Pragmas &p)
    :PragmaHandlerBase("vectorize", p) {}

VarToArrayPragmaHandler::VarToArrayPragmaHandler(Pragmas &p)
    :PragmaHandlerBase("vartoarray", p) {}

LoopDistributionPragmaHandler::LoopDistributionPragmaHandler(Pragmas &p)
    :PragmaHandlerBase("distribute_loop", p) {}

TempArraysPragmaHandler::TempArraysPragmaHandler(Pragmas &p)
    :PragmaHandlerBase("temparrays", p) {}

LoopFragmentationPragmaHandler::LoopFragmentationPragmaHandler(Pragmas &p)
    :PragmaHandlerBase("block_nest", p) {}


PragmaHandlerBase::PragmaHandlerBase(llvm::StringRef name, Pragmas &p)
    :PragmaHandler(name)
    ,pragmas(p)
{
}

void PragmaHandlerBase::registerPragma(const SourceLocation &location, const std::string &name, const std::string &value)
{
    pragmas.push_back(make_pair(location, std::make_pair(name, value)));
}

clang::PragmaHandler* createOpsPragmaHandlers(Pragmas& p)
{
    PragmaNamespace* opsNamespaceHandler = new PragmaNamespace("ops");
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::DistributionPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::SingleAccessPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::NestingPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::BlockNestingPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::IgnorePragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::TestArgumentPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::VectorizePragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::LoopDistributionPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::VarToArrayPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::TempArraysPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::TargetPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::LoopFragmentationPragmaHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::BlockArrayDeclareHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::BlockArrayAllocateHandler(p));
    opsNamespaceHandler->AddPragma(new OPS::ClangParser::BlockArrayReleaseHandler(p));

    PragmaNamespace* transformNamespace = new PragmaNamespace("transform");
    transformNamespace->AddPragma(new OPS::ClangParser::TilingPragmaHandler(p));

    opsNamespaceHandler->AddPragma(transformNamespace);
    return opsNamespaceHandler;
}

}
}
