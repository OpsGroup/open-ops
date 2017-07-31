/************************************************************************************
AST2RepriseConverter - converter of AST to OPS::Reprise
*************************************************************************************/
// CLang & LLVM
#ifdef __GNUC__
#  include <cstddef>
#endif
#include "OPS_Core/disable_llvm_warnings_begin.h"
#include "AST2RepriseConsumers.h"
//#include "../lib/Index/ASTVisitor.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/Lex/Token.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/CodeGen/ModuleBuilder.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Pragma.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Timer.h"
#include "OPS_Core/disable_llvm_warnings_end.h"
#include <cstdio>
#include <map>
#include <stack>
#include <deque>
#include <iostream>
#include <numeric>
// OPS::Reprise
#include "Reprise/Reprise.h"
#include "Reprise/Canto.h"

using namespace clang;


/// AST2RepriseConverter - converter of AST to OPS::Reprise

/* -------------------------------------------------------------------------------------------------- */

namespace OPS
{
    namespace Reprise
    {
        class ConverterInternalError : public OPS::Exception
        {
        public:
            ConverterInternalError(const std::string& message, const SourceLocation& sourceLocation)
                :OPS::Exception(message)
                ,m_sourceLocation(sourceLocation)
            {}

            const SourceLocation& getSourceLocation() { return m_sourceLocation; }
        private:
            SourceLocation m_sourceLocation;
        };

        class Clang2RepriseConverter
        {
            OPS::Reprise::RepriseContext &m_repriseContext;
            ASTContext &m_astContext;
            const clang::TranslationUnitDecl& m_translationUnit;
            /// Stack of nested declarations, so the current declaration is on the stack top
            typedef std::deque<OPS::Reprise::Declarations*> DeclarationsStack;
            typedef std::map<OPS::Reprise::GotoStatement*, std::string> GotoToLabelMap;
            DeclarationsStack m_Declarations;
            /// Stack of nested compound statements, so the current compound stmt is on the stack top
            std::stack<OPS::Reprise::BlockStatement*> m_Contexts;
            /// Stack of nested switch statements, so the current switch stmt is on the stack top
            std::stack<OPS::Reprise::PlainSwitchStatement*> m_plainSwitchStmts;
            /// Per function map that maps label names to the statements of Reprise
            std::map<std::string, OPS::Reprise::StatementBase*> m_labelsToStmtMap;
            /// Per function map that maps goto's onto label names they point to.
            /// Used for forward label using: { goto label1; ...; label1: someStmt; }
            GotoToLabelMap m_gotoToLabelMap;

            typedef std::map<const clang::Decl*, ReprisePtr<OPS::Reprise::DeclarationBase> > ClangToRepriseDeclMap;
            ClangToRepriseDeclMap m_clangToRepriseDeclMap;

            typedef std::multimap<const OPS::Reprise::DeclarationBase*, OPS::Reprise::DeclarationBase*> LinkedDeclMap;
            LinkedDeclMap m_linkedDecls;

            const clang::Decl* m_currentDecl;

            Clang2RepriseConverter& operator=(const Clang2RepriseConverter&);
            Clang2RepriseConverter(const Clang2RepriseConverter&);

            double convertToDoubleSafe(llvm::APFloat floatValue);
            bool convertToInt64(llvm::APInt intValue, sqword& result);
            std::unique_ptr<StrictLiteralExpression> convertToStrictLiteral(llvm::APInt intValue);

            void assignMacroDefinition(const clang::Expr& expr, OPS::Reprise::ExpressionBase* opsExpr);

            void markPragmas(const clang::Stmt& stmt, OPS::Reprise::RepriseBase* opsNode);
            void markPragmas(const clang::Decl& decl, OPS::Reprise::RepriseBase* opsNode);

            void markPragmas(const clang::PresumedLoc beginLocation, OPS::Reprise::RepriseBase* opsNode);

            std::string extractMacroDefinition(SourceLocation Loc);

            //const unsigned int arraySize = (const unsigned int)arrayType.getSizeExpr()->EvaluateAsInt(m_astContext).getZExtValue();
            std::unique_ptr<OPS::Reprise::ExpressionBase> trySimplifyExpression(const clang::Expr &expression);

            /// Tune pointed statements for all the goto stmts in current function.
            //  It's made for handling forward goto labels correctly
            void tuneGotoStmts();

            std::string getIdentifierName(const clang::NamedDecl& decl);

            /// Checks if stmt is a BlockStmt and if not, creates a new block, inserts stmt into it and returns the block
            /// If block exists, returns it
            ReprisePtr<OPS::Reprise::BlockStatement> ensureBlockStmt(OPS::Reprise::StatementBase* stmt);

            OPS::Reprise::DeclarationBase* findConvertedDeclaration(const clang::Decl& decl);

            OPS::Reprise::SubroutineDeclaration* findConvertedSubroutine(const clang::Decl& decl);
            OPS::Reprise::TypeDeclaration* findConvertedType(const clang::Decl& decl);
            OPS::Reprise::VariableDeclaration* findConvertedVariable(const clang::Decl& decl);

            /// Assignes source location of (beginLocation, endLocation) to assignee.
            void assignLocation(const clang::PresumedLoc beginLocation,
                const clang::PresumedLoc endLocation,
                OPS::Reprise::RepriseBase& assignee);

            /// Takes a location info from stmt & reassigns it to assignee.
            void assignLocation(const clang::Stmt& stmt, OPS::Reprise::RepriseBase& assignee);

            /// Takes a location info from stmt & reassigns it to assignee.
            void assignLocation(const clang::Decl& decl, OPS::Reprise::RepriseBase& assignee);

            std::unique_ptr<OPS::Reprise::TypeBase> convertBuiltinType(const clang::BuiltinType& type);
            std::unique_ptr<OPS::Reprise::TypeBase> convertComplexType(const clang::ComplexType& type);

            ReprisePtr<OPS::Reprise::TypeBase> convertType(const clang::Type& type);

            ReprisePtr<OPS::Reprise::TypeBase> convertQualType(const clang::QualType& qualType);

            ReprisePtr<OPS::Reprise::DeclarationBase> convertDecl(const clang::Decl& declaration);

            ReprisePtr<OPS::Reprise::TranslationUnit> convertTranslationUnitDecl(const clang::TranslationUnitDecl& translationUnit);

            ReprisePtr<TypeDeclaration> convertTypedefDecl(const clang::TypedefDecl& decl);

            ReprisePtr<VariableDeclaration> convertVarDecl(const clang::VarDecl& decl);

            ReprisePtr<TypeDeclaration> convertRecordDecl(const clang::RecordDecl& recordDecl);

            ReprisePtr<TypeDeclaration> convertEnumDecl(const clang::EnumDecl& decl);

            ReprisePtr<SubroutineDeclaration> convertFunctionDecl(const clang::FunctionDecl& decl);

            std::unique_ptr<OPS::Reprise::ExpressionBase> convertExpr(const clang::Expr& expr);

            ReprisePtr<OPS::Reprise::StatementBase> convertStmt(const clang::Stmt& stmt);

            void addDeclaration(DeclarationBase* decl);

            ClangParser::Pragmas& m_pragmas;
        public:
            Clang2RepriseConverter(
                OPS::Reprise::RepriseContext& aRepriseContext,
                clang::ASTContext& anAstContext,
                const clang::TranslationUnitDecl& aTranslationUnit,
                ClangParser::Pragmas& pragmas):
            m_repriseContext(aRepriseContext),
                m_astContext(anAstContext),
                m_translationUnit(aTranslationUnit),
                m_pragmas(pragmas)
            {
            }

            ReprisePtr<OPS::Reprise::TranslationUnit> convert()
            {
                ReprisePtr<OPS::Reprise::TranslationUnit> result(convertTranslationUnitDecl(m_translationUnit));
                if (result.get())
                {
                    assignLocation(m_translationUnit, *result);
                }
                return result;
            }
        };

            double Clang2RepriseConverter::convertToDoubleSafe(llvm::APFloat floatValue)
            {
                double result = 0.0;
                bool dummy = false;
                llvm::APFloat::opStatus convertStatus = floatValue.convert(llvm::APFloat::IEEEdouble, llvm::APFloat::rmNearestTiesToEven, &dummy);
                // TODO: Проверить, что llvm::APFloat::opInexact не сильно портит исходную константу.
                // Или использовать константу только, если convertStatus == llvm::APFloat::opOK
                if (convertStatus == llvm::APFloat::opOK || convertStatus == llvm::APFloat::opInexact)
                {
                    result = floatValue.convertToDouble();
                }
                else
                {
                    throw OPS::StateError("This float type constant is not supported yet. Only float and double are supported.");
                    result = std::numeric_limits<double>().quiet_NaN();
                }
                return result;
            }

            bool Clang2RepriseConverter::convertToInt64(llvm::APInt intValue, sqword& result)
            {
                if (intValue.isNegative())
                {
                    if (convertToInt64(intValue.abs(), result))
                    {
                        result = -result;
                        return true;
                    }
                }
                else if (intValue.getActiveBits() < 64)
                {
                    result = (sqword)intValue.getLimitedValue();
                    return true;
                }

                return false;
            }

            std::unique_ptr<StrictLiteralExpression> Clang2RepriseConverter::convertToStrictLiteral(llvm::APInt intValue)
            {
//				OPS_ASSERT(!intValue.isNegative()); // отрицательные значения пока не поддерживаются

				std::unique_ptr<StrictLiteralExpression> result;

				if (intValue.getActiveBits() < 32)
				{
					result.reset(StrictLiteralExpression::createInt32(sdword(intValue.getLimitedValue())));
				}
				else
				{
					if (intValue.isNegative())
					{
						result.reset(StrictLiteralExpression::createInt64(intValue.getLimitedValue()));
					}
					else
					{
						result.reset(StrictLiteralExpression::createUInt64(intValue.getLimitedValue()));
					}
				}

				return result;
			}

			void Clang2RepriseConverter::assignMacroDefinition(const clang::Expr& expr, OPS::Reprise::ExpressionBase* opsExpr)
			{
				const std::string macroDefinition = extractMacroDefinition(expr.getSourceRange().getBegin());
				if (!macroDefinition.empty())
				{
					OPS_ASSERT(opsExpr);
					// #define -> Name Value
					opsExpr->setNote("#define", OPS::Reprise::Note::newString(macroDefinition));
					//std::cout << "[#define " << macroDefinition << "]" << std::endl;
				}
			}

			std::string Clang2RepriseConverter::extractMacroDefinition(SourceLocation Loc)
			{
				OPS_ASSERT(!Loc.isInvalid() && "must have a valid source location here");
				static const std::string defineStr = "#define";
				SourceManager& sourceManager = m_astContext.getSourceManager();
				std::string result;
				// If this is a macro ID, first emit information about where this was
				// instantiated (recursively) then emit information about where. the token was
				// spelled from.
				if (!Loc.isFileID())
				{
					SourceLocation OneLevelUp = sourceManager.getImmediateExpansionRange(Loc).first;
					result = extractMacroDefinition(OneLevelUp);
					if (result.find(defineStr) == 0)
					{
						result = result.substr(defineStr.length(), result.length());
					}
					else
					{
						Loc = sourceManager.getImmediateSpellingLoc(Loc);
						result = extractMacroDefinition(Loc);
						if (result.find(defineStr) == 0)
						{
							result = result.substr(defineStr.length(), result.length());
						}
					}
				}
				else
				{
					// Decompose the location into a FID/Offset pair.
					std::pair<FileID, unsigned> LocInfo = sourceManager.getDecomposedLoc(Loc);
					FileID FID = LocInfo.first;
					unsigned FileOffset = LocInfo.second;

					// Get information about the buffer it points into.
					llvm::StringRef BufferInfo = sourceManager.getBufferData(FID);
					const char *BufStart = BufferInfo.data();

					unsigned ColNo = sourceManager.getColumnNumber(FID, FileOffset);
					// Rewind from the current position to the start of the line.
					const char *TokPtr = BufStart+FileOffset;
					const char *LineStart = TokPtr-ColNo+1; // Column # is 1-based.

					// Compute the line end.  Scan forward from the error position to the end of
					// the line.
					const char *LineEnd = TokPtr;
					while (*LineEnd != '\n' && *LineEnd != '\r' && *LineEnd != '\0')
					{
						++LineEnd;
					}
					// Copy the line of code into an std::string for ease of manipulation.
					std::string sourceLine(LineStart, LineEnd);
					// Scan the source line, looking for tabs.  If we find any, manually expand
					// them to 8 characters and update the CaretLine to match.
					for (unsigned i = 0; i < sourceLine.size(); ++i)
					{
						if (sourceLine[i] == '\t')
						{
							// Replace this tab with at least one space.
							sourceLine[i] = ' ';
						}
					}
					// Emit what we have computed.
					if (sourceLine.find(defineStr) == 0)
					{
						result = sourceLine;
					}
				}
				return OPS::Strings::trim(result);
			}

			//const unsigned int arraySize = (const unsigned int)arrayType.getSizeExpr()->EvaluateAsInt(m_astContext).getZExtValue();
			std::unique_ptr<OPS::Reprise::ExpressionBase> Clang2RepriseConverter::trySimplifyExpression(const clang::Expr &expression)
			{
				std::unique_ptr<OPS::Reprise::ExpressionBase> simplifiedExpr;
				clang::Expr::EvalResult evalResult;
				// clang считает, что выражение можно свернуть в константу? Сворачиваем!
				if (expression.EvaluateAsRValue(evalResult, m_astContext) && !evalResult.HasSideEffects)
				{
					switch(evalResult.Val.getKind())
					{
						// Сворачиваем только к int, float, double
					case clang::APValue::Int:
						{
							// TODO: сделать правильный выбор типа для константы
							//simplifiedExpr.reset(OPS::Reprise::StrictLiteralExpression::createInt32(evalResult.Val.getInt().getLimitedValue()));
						}
						break;
					case clang::APValue::Float:
						{
							llvm::APFloat& floatValue = evalResult.Val.getFloat();
							bool dummy = false;
							llvm::APFloat::opStatus convertStatus = floatValue.convert(llvm::APFloat::IEEEdouble, llvm::APFloat::rmNearestTiesToEven, &dummy);
							// TODO: Проверить, что llvm::APFloat::opInexact не сильно портит исходную константу. Или использовать константу только, если convertStatus == llvm::APFloat::opOK
							simplifiedExpr.reset(OPS::Reprise::StrictLiteralExpression::createFloat64(convertToDoubleSafe(floatValue)));
						}
						break;
						// TODO: для комплексных констант
						//case    clang::APValue::ComplexInt:
						//case    clang::APValue::ComplexFloat:
						//case    clang::APValue::LValue:
						//case    clang::APValue::Vector:
						;
					default: break;
					}
				}
				return simplifiedExpr;
			}

			/// Tune pointed statements for all the goto stmts in current function.
			//  It's made for handling forward goto labels correctly
			void Clang2RepriseConverter::tuneGotoStmts()
			{
				for(GotoToLabelMap::iterator it = m_gotoToLabelMap.begin(); it != m_gotoToLabelMap.end(); ++it)
				{
					OPS::Reprise::GotoStatement* gotoStmt = it->first;
					const std::string labelName = it->second;
					OPS::Reprise::StatementBase* labeledStmt = m_labelsToStmtMap[labelName];
					gotoStmt->setPointedStatement(labeledStmt);
				}
			}

			std::string Clang2RepriseConverter::getIdentifierName(const clang::NamedDecl& decl)
			{
				return decl.getIdentifier() ? decl.getIdentifier()->getName() : "";
			}

			/// Checks if stmt is a BlockStmt and if not, creates a new block, inserts stmt into it and returns the block
			/// If block exists, returns it
            ReprisePtr<OPS::Reprise::BlockStatement> Clang2RepriseConverter::ensureBlockStmt(OPS::Reprise::StatementBase* stmt)
			{
                ReprisePtr<OPS::Reprise::BlockStatement> result;
				if (stmt->is_a<OPS::Reprise::BlockStatement>())
				{
					result.reset(static_cast<OPS::Reprise::BlockStatement*>(stmt));
				}
				else
				{
					result.reset(new OPS::Reprise::BlockStatement());
					result->addLast(stmt);
				}
				return result;
			}

			OPS::Reprise::DeclarationBase* Clang2RepriseConverter::findConvertedDeclaration(const clang::Decl& decl)
			{
				ClangToRepriseDeclMap::iterator it = m_clangToRepriseDeclMap.find(&decl);
				if (it != m_clangToRepriseDeclMap.end())
					return it->second.get();

				return 0;
			}

			TypeDeclaration* Clang2RepriseConverter::findConvertedType(const clang::Decl &decl)
			{
				if (DeclarationBase* repriseDecl = findConvertedDeclaration(decl))
					return repriseDecl->cast_ptr<TypeDeclaration>();
				return 0;
			}

			SubroutineDeclaration* Clang2RepriseConverter::findConvertedSubroutine(const clang::Decl &decl)
			{
				if (DeclarationBase* repriseDecl = findConvertedDeclaration(decl))
					return repriseDecl->cast_ptr<SubroutineDeclaration>();
				return 0;
			}

			VariableDeclaration* Clang2RepriseConverter::findConvertedVariable(const clang::Decl &decl)
			{
				if (DeclarationBase* repriseDecl = findConvertedDeclaration(decl))
					return repriseDecl->cast_ptr<VariableDeclaration>();
				return 0;
			}

            /// Assignes source location of (beginLocation, endLocation) to assignee.
			void Clang2RepriseConverter::assignLocation(const clang::PresumedLoc beginLocation,
				const clang::PresumedLoc endLocation,
				OPS::Reprise::RepriseBase& assignee)
			{
				SourceCodeManager& repriseSourceManager = m_repriseContext.getSourceCodeManager();
				if (beginLocation.isValid() && endLocation.isValid())
				{
					// NB!
					// We use beginLocation's file name as a file name for the whole stmt.
					// It's true in most reasonable use cases.
					// TODO: may be it's worth caching the mapping clangFIleId -> repriseFileId.
					int fileId = repriseSourceManager.registerFile(beginLocation.getFilename());

                    //search for HeadIncludeFileId
                    //std::cout << "Start search for HeadIncludeFileId\n";
                    int headIncludeFileId = -1;
                    clang::SourceManager& sourceManager = m_astContext.getSourceManager();
                    std::string lastHeaderFileName = beginLocation.getFilename();
                    clang::SourceLocation location = beginLocation.getIncludeLoc();
                    //std::cout << "loc before while: " << location.printToString(sourceManager)<<"\n";
                    while (true)
                    {
                        clang::PresumedLoc ploc = sourceManager.getPresumedLoc(location);
                        if (!ploc.isValid()) break;
                        location = ploc.getIncludeLoc();
                        if (!location.isValid()) break;
                        //std::cout << location.printToString(sourceManager)<<"\n";
                        lastHeaderFileName = ploc.getFilename();
                    }
                    //std::cout << "lastHeaderFileName: " << lastHeaderFileName <<"\n";
                    if (!lastHeaderFileName.empty())
                        headIncludeFileId = repriseSourceManager.registerFile(lastHeaderFileName.c_str());

                    SourceCodeLocation sourceCodeLocation(fileId, headIncludeFileId,
						beginLocation.getLine(), endLocation.getLine(),
						beginLocation.getColumn(), endLocation.getColumn());

					TSourceCodeLocation assigneeLocationId = repriseSourceManager.addLocation(sourceCodeLocation);
					assignee.setLocationId(assigneeLocationId);
				}
			}

			/// Takes a location info from stmt & reassigns it to assignee.
			void Clang2RepriseConverter::assignLocation(const clang::Stmt& stmt, OPS::Reprise::RepriseBase& assignee)
			{
				clang::SourceManager& clangSourceManager = m_astContext.getSourceManager();
				const clang::PresumedLoc beginLocation = clangSourceManager.getPresumedLoc(stmt.getLocStart());
				const clang::PresumedLoc endLocation = clangSourceManager.getPresumedLoc(stmt.getLocEnd());
				assignLocation(beginLocation, endLocation, assignee);
			}

			/// Takes a location info from stmt & reassigns it to assignee.
			void Clang2RepriseConverter::assignLocation(const clang::Decl& decl, OPS::Reprise::RepriseBase& assignee)
			{
				clang::SourceManager& clangSourceManager = m_astContext.getSourceManager();
				const clang::PresumedLoc beginLocation = clangSourceManager.getPresumedLoc(decl.getLocStart());
				const clang::PresumedLoc endLocation = clangSourceManager.getPresumedLoc(decl.getLocEnd());
				assignLocation(beginLocation, endLocation, assignee);
			}

            std::unique_ptr<OPS::Reprise::TypeBase> Clang2RepriseConverter::convertBuiltinType(const clang::BuiltinType& type)
			{
				OPS::Reprise::Canto::HirCBasicType::HirCBasicKind typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_UNDEFINED;
				switch(type.getKind())
				{
				case clang::BuiltinType::Void:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_VOID;
					}
					break;
				case clang::BuiltinType::Bool:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_BOOL;
					}
					break;
				case clang::BuiltinType::Char_U:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_CHAR;
					}
					break;
				case clang::BuiltinType::UChar:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_UCHAR;
					}
					break;
				case clang::BuiltinType::UShort:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_USHORT;
					}
					break;
				case clang::BuiltinType::UInt:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_UINT;
					}
					break;
				case clang::BuiltinType::ULong:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_ULONG;
					}
					break;
				case clang::BuiltinType::ULongLong:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_ULONG_LONG;
					}
					break;
				case clang::BuiltinType::UInt128:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_UINT128;
					}
					break;
				case clang::BuiltinType::Char_S:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_CHAR;
					}
					break;
				case clang::BuiltinType::SChar:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_SCHAR;
					}
					break;
				case clang::BuiltinType::WChar_U:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_WIDE_CHAR;
					}
					break;
				case clang::BuiltinType::Short:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_SHORT;
					}
					break;
				case clang::BuiltinType::Int:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_INT;
					}
					break;
				case clang::BuiltinType::Long:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_LONG;
					}
					break;
				case clang::BuiltinType::LongLong:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_LONG_LONG;
					}
					break;
				case clang::BuiltinType::Int128:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_INT128;
					}
					break;
				case clang::BuiltinType::Float:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_FLOAT;
					}
					break;
				case clang::BuiltinType::Double:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_DOUBLE;
					}
					break;
				case clang::BuiltinType::LongDouble:
					{
						typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_LONG_DOUBLE;
					}
					break;
				default:
					{
                        throw OPS::StateError("Unknown builtin type kind");
					}
				}
				OPS_ASSERT(typeKind != OPS::Reprise::Canto::HirCBasicType::HCBK_UNDEFINED);
				return std::unique_ptr<OPS::Reprise::TypeBase>(new OPS::Reprise::Canto::HirCBasicType(typeKind));
			}

            std::unique_ptr<OPS::Reprise::TypeBase> Clang2RepriseConverter::convertComplexType(const clang::ComplexType& type)
			{
                ReprisePtr<TypeBase> elementType = convertQualType(type.getElementType());
				OPS_ASSERT(elementType.get() != 0);

				if (Canto::HirCBasicType* hirCBasic = elementType->cast_ptr<Canto::HirCBasicType>())
				{
					OPS::Reprise::Canto::HirCBasicType::HirCBasicKind typeKind = OPS::Reprise::Canto::HirCBasicType::HCBK_UNDEFINED;
					switch(hirCBasic->getKind())
					{
					case Canto::HirCBasicType::HCBK_FLOAT: typeKind = Canto::HirCBasicType::HCBK_COMPLEX_FLOAT; break;
					case Canto::HirCBasicType::HCBK_DOUBLE: typeKind = Canto::HirCBasicType::HCBK_COMPLEX_DOUBLE; break;
					case Canto::HirCBasicType::HCBK_LONG_DOUBLE: typeKind = Canto::HirCBasicType::HCBK_COMPLEX_LONG_DOUBLE; break;
					default:
						{
                            throw OPS::StateError("Unexpected element type kind");
						}
					}
					OPS_ASSERT(typeKind != OPS::Reprise::Canto::HirCBasicType::HCBK_UNDEFINED);
					return std::unique_ptr<OPS::Reprise::TypeBase>(new OPS::Reprise::Canto::HirCBasicType(typeKind));
				}
				else
				{
                    throw OPS::StateError("Unexpected element type");
					return std::unique_ptr<OPS::Reprise::TypeBase>();
				}
			}

            ReprisePtr<OPS::Reprise::TypeBase> Clang2RepriseConverter::convertType(const clang::Type& type)
			{
				ReprisePtr<OPS::Reprise::TypeBase> result;
				const clang::Type::TypeClass typeClass = type.getTypeClass();
				switch(typeClass)
				{
				case clang::Type::Builtin:
					{
						// builtin type
                        result.reset(convertBuiltinType(static_cast<const clang::BuiltinType&>(type)).release());
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::Complex:
					{
                        result.reset(convertComplexType(static_cast<const clang::ComplexType&>(type)).release());
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::Pointer:
					{
						const clang::PointerType& pointer = static_cast<const clang::PointerType&>(type);
						// Type that pointer points to
                        ReprisePtr<OPS::Reprise::TypeBase> pointeeType = convertQualType(pointer.getPointeeType());
						result.reset(new OPS::Reprise::PtrType(pointeeType));
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::BlockPointer:
					{
						const clang::BlockPointerType& blockPointer = static_cast<const clang::BlockPointerType&>(type);
						OPS_UNUSED(blockPointer);
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::LValueReference:
					{
						result.reset(BasicType::voidType());
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::RValueReference:
					{
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::MemberPointer:
					{
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::ConstantArray:
					{
						const clang::ConstantArrayType& arrayType = static_cast<const clang::ConstantArrayType&>(type);
                        ReprisePtr<OPS::Reprise::TypeBase> elementType = convertQualType(arrayType.getElementType());
						const unsigned int arraySize = (const unsigned int)arrayType.getSize().getZExtValue();
						std::unique_ptr<OPS::Reprise::ArrayType> repriseArrayType(new OPS::Reprise::ArrayType(arraySize, elementType.get()));
						result.reset(repriseArrayType.release());
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::IncompleteArray:
					{
						const clang::IncompleteArrayType& arrayType = static_cast<const clang::IncompleteArrayType&>(type);
                        ReprisePtr<OPS::Reprise::TypeBase> elementType = convertQualType(arrayType.getElementType());
						std::unique_ptr<OPS::Reprise::ArrayType> repriseArrayType(new OPS::Reprise::ArrayType(elementType.get()));
						result.reset(repriseArrayType.release());
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::VariableArray:
					{
						/*
						Example:
						void f(int n)
						{
						int a[n];
						}
						*/
						const clang::VariableArrayType& arrayType = static_cast<const clang::VariableArrayType&>(type);
                        ReprisePtr<OPS::Reprise::TypeBase> elementType = convertQualType(arrayType.getElementType());
                        std::unique_ptr<OPS::Reprise::ExpressionBase> sizeExpr( convertExpr(*arrayType.getSizeExpr()) );
						std::unique_ptr<OPS::Reprise::ArrayType> repriseArrayType(new OPS::Reprise::ArrayType(elementType.get()));
						repriseArrayType->setCountExpression(sizeExpr.release());
						result.reset(repriseArrayType.release());
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::FunctionProto:
					{
						/// FunctionProtoType - Represents a prototype with argument type info, e.g.
						/// 'int foo(int)' or 'int foo(void)'.  'void' is represented as having no
						/// arguments, not as having a single void argument. Such a type can have an
						/// exception specification, but this specification is not part of the canonical
						/// type.
						const clang::FunctionProtoType& protoFunc = static_cast<const clang::FunctionProtoType&>(type);

                        ReprisePtr<OPS::Reprise::TypeBase> returnType = convertQualType(protoFunc.getResultType());
						std::unique_ptr<OPS::Reprise::SubroutineType> repriseProtoFunc(new OPS::Reprise::SubroutineType(returnType.get()));

						// Add subproc params
						const unsigned int paramCount = protoFunc.getNumArgs();
						for(unsigned int paramIndex = 0; paramIndex < paramCount; ++paramIndex)
						{
                            ReprisePtr<OPS::Reprise::TypeBase> paramType = convertQualType(protoFunc.getArgType(paramIndex));
							std::unique_ptr<OPS::Reprise::ParameterDescriptor> paramDescriptor(new OPS::Reprise::ParameterDescriptor("", paramType.get()));
							repriseProtoFunc->addParameter(paramDescriptor.get());
							// Release auto_ptrs
							paramDescriptor.release();
						}
						// Устаналвиваем признак того, что ф-я имеет переменное число аргументов (varargs, variadic).
						repriseProtoFunc->setVarArg(protoFunc.isVariadic());
						result.reset(repriseProtoFunc.release());
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::FunctionNoProto:
					{
						/// FunctionNoProtoType - Represents a K&R-style 'int foo()' function, which has
						/// no information available about its arguments.
						const clang::FunctionNoProtoType& protoFunc = static_cast<const clang::FunctionNoProtoType&>(type);
                        ReprisePtr<OPS::Reprise::TypeBase> returnType = convertQualType(protoFunc.getResultType());
						std::unique_ptr<OPS::Reprise::SubroutineType> repriseProtoFunc(new OPS::Reprise::SubroutineType(returnType.get()));
						repriseProtoFunc->setArgsKnown(false);
						result.reset(repriseProtoFunc.release());
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::Paren:
					{
						const clang::ParenType& parenType = static_cast<const clang::ParenType&>(type);
                        result = convertQualType(parenType.getInnerType());
					}
					break;
				case clang::Type::Typedef:
					{
						// TODO: find a typedef declaration in Reprise
						const clang::TypedefType& typedefType = static_cast<const clang::TypedefType&>(type);
						const clang::TypedefNameDecl* typedefDecl = typedefType.getDecl();
						OPS_ASSERT(typedefDecl->getIdentifier() != 0);
                        ReprisePtr<TypeDeclaration> repriseTypedefDecl = convertDecl(*typedefDecl);
						OPS_ASSERT(repriseTypedefDecl.get());
						result.reset(new DeclaredType(*repriseTypedefDecl));
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::TypeOfExpr:
					{
						const clang::TypeOfExprType& typeOfExprType = static_cast<const clang::TypeOfExprType&>(type);
						if (typeOfExprType.isSugared())
						{
                            result.reset(convertQualType(typeOfExprType.desugar()).release());
						}
						else
						{
                            throw OPS::StateError("clang::Type::TypeOfExpr is not sugared");
						}
					}
					break;
				case clang::Type::TypeOf:
					{
                        throw OPS::StateError("clang::Type::TypeOf not implemented");
					}
					break;
				case clang::Type::Decltype:
					{
                        throw OPS::StateError("clang::Type::Decltype not implemented");
					}
					break;
				case clang::Type::Record:
					{
						const clang::RecordType& recordType = static_cast<const clang::RecordType&>(type);
						const clang::RecordDecl* recordDecl = recordType.getDecl();
                        ReprisePtr<TypeDeclaration> repriseTypeDeclaration = convertDecl(*recordDecl);
						OPS_ASSERT(repriseTypeDeclaration.get());

						const DeclContext* declContext = recordDecl->getDeclContext();
						if (declContext != 0 &&
							(declContext->isTranslationUnit() ||
							 declContext->isFunctionOrMethod()))
						{
							result.reset(new OPS::Reprise::DeclaredType(*repriseTypeDeclaration));
						}
						else
						{
							result.reset(&repriseTypeDeclaration->getType());
						};

						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::Enum:
					{
						// TODO: find a typedef declaration in Reprise
						const clang::EnumType& enumType = static_cast<const clang::EnumType&>(type);
						const clang::EnumDecl* enumDecl = enumType.getDecl();
                        ReprisePtr<TypeDeclaration> repriseTypeDeclaration = convertDecl(*enumDecl);
						OPS_ASSERT(repriseTypeDeclaration.get());

						const DeclContext* declContext = enumDecl->getDeclContext();
						if (declContext != 0 &&
							(declContext->isTranslationUnit() ||
							 declContext->isFunctionOrMethod()))
						{
							result.reset(new OPS::Reprise::DeclaredType(*repriseTypeDeclaration));
						}
						else
						{
							result.reset(&repriseTypeDeclaration->getType());
						};
						OPS_ASSERT(result.get());
					}
					break;
				case clang::Type::Elaborated:
					{
						const clang::ElaboratedType& elabType = static_cast<const clang::ElaboratedType&>(type);
                        result.reset(convertQualType(elabType.desugar()).release());
					}
					break;
				case clang::Type::Vector:
					{
						const clang::VectorType& vectType = static_cast<const clang::VectorType&>(type);
                        ReprisePtr<OPS::Reprise::TypeBase> elementType = convertQualType(vectType.getElementType());
						const unsigned int vectorSize = (const unsigned int)vectType.getNumElements();
						std::unique_ptr<OPS::Reprise::VectorType> repriseVectorType(new OPS::Reprise::VectorType(vectorSize, elementType.get()));
						result.reset(repriseVectorType.release());
						OPS_ASSERT(result.get());
					}
					break;
				default:
					{
                        throw OPS::StateError(OPS::Strings::format("Unknown type kind: %s", type.getTypeClassName()));
					}
				}
				OPS_ASSERT(result.get() != 0);
				return result;
			}

            ReprisePtr<OPS::Reprise::TypeBase> Clang2RepriseConverter::convertQualType(const clang::QualType& qualType)
			{
                ReprisePtr<OPS::Reprise::TypeBase> type = convertType(*qualType);

				type->setConst(qualType.isConstQualified());
				type->setVolatile(qualType.isVolatileQualified());
				// set "restrict" only if qualType is a pointer type
				if (OPS::Reprise::PtrType *ptrType = type->cast_ptr<OPS::Reprise::PtrType>())
				{
					const bool isRectrict = qualType.isRestrictQualified();
					ptrType->setRestrict(isRectrict);
				}
				return type;
			}

            ReprisePtr<OPS::Reprise::DeclarationBase> Clang2RepriseConverter::convertDecl(const clang::Decl& declaration)
            {
				switch(declaration.getKind())
				{
				case Decl::Typedef:
                    {
                        ReprisePtr<TypeDeclaration> typeDef = convertTypedefDecl(static_cast<const clang::TypedefDecl&>(declaration));
						assignLocation(declaration, *typeDef);
                        return typeDef;
					}
				case Decl::Var:
					{
						ReprisePtr<OPS::Reprise::VariableDeclaration> variable =
                            convertVarDecl(static_cast<const clang::VarDecl&>(declaration));
						if (!m_Contexts.empty())
						{
							variable->setDefinedBlock(*m_Contexts.top());
						}
						assignLocation(declaration, *variable);
						markPragmas(declaration, variable.get());
                        return variable;
					}
				case Decl::Record:
					{
                        ReprisePtr<TypeDeclaration> record = convertRecordDecl(static_cast<const clang::RecordDecl&>(declaration));
						assignLocation(declaration, *record);
                        return record;
					}
				case Decl::Enum:
					{
						ReprisePtr<TypeDeclaration>  enumTypeDecl =
                            convertEnumDecl(static_cast<const clang::EnumDecl&>(declaration));
						assignLocation(declaration, *enumTypeDecl);
                        return enumTypeDecl;
					}
				case Decl::Function:
					{
						m_currentDecl = &declaration;
						const clang::FunctionDecl& clangFuncDecl = static_cast<const clang::FunctionDecl&>(declaration);
                        ReprisePtr<SubroutineDeclaration> funcDecl = convertFunctionDecl(clangFuncDecl);
						assignLocation(declaration, *funcDecl);
                        return funcDecl;
					}
                case Decl::Empty:
                    {
                        throw OPS::StateError("Empty declarations are not supported");
                    }
				default:
					{
                        throw OPS::StateError(OPS::Strings::format("Unknown declaration kind: %s", declaration.getDeclKindName()));
						return OPS::Reprise::ReprisePtr<DeclarationBase>();
					}
				}
			}

            ReprisePtr<OPS::Reprise::TranslationUnit> Clang2RepriseConverter::convertTranslationUnitDecl(const clang::TranslationUnitDecl& translationUnit)
			{
				ReprisePtr<OPS::Reprise::TranslationUnit> result(new OPS::Reprise::TranslationUnit(OPS::Reprise::TranslationUnit::SL_C));
				m_Declarations.push_back(&result->getGlobals());

				for (DeclContext::decl_iterator declarationIterator = translationUnit.decls_begin();
					declarationIterator != translationUnit.decls_end();
					++declarationIterator)
				{
					const Decl* declaration = *declarationIterator;

                    // Skip empty declarations
                    if (declaration->getKind() == clang::Decl::Empty)
                        continue;

                    addDeclaration(convertDecl(*declaration).get());
				}
				m_Declarations.pop_back();
				OPS_ASSERT(m_Declarations.empty());
				return result;
			}

            ReprisePtr<TypeDeclaration> Clang2RepriseConverter::convertTypedefDecl(const clang::TypedefDecl& decl)
			{
				if (TypeDeclaration* typeDecl = findConvertedType(decl))
				{
					// Этот typedef уже объявлен - переиспользуем.
					return ReprisePtr<Reprise::TypeDeclaration>(typeDecl);
				}

				ReprisePtr<Reprise::TypeDeclaration> resultTypeDecl;
				std::unique_ptr<OPS::Reprise::TypedefType> typeDef;

				const std::string name = getIdentifierName(decl);
				// создаем новое определение типа
				// Reprise не позволяет делать пустые typedef-ы, поэтому делаем typedef для void
				typeDef.reset(new OPS::Reprise::TypedefType(BasicType::voidType()));
				resultTypeDecl.reset(new OPS::Reprise::TypeDeclaration(typeDef.get(), name));
				m_clangToRepriseDeclMap.insert(std::make_pair(&decl, resultTypeDecl));

                ReprisePtr<OPS::Reprise::TypeBase> underlyingType = convertQualType(decl.getUnderlyingType());

				// заменяем void в typedef-е на наш тип
				typeDef->setBaseType(underlyingType.get());
				typeDef.release();

				// !hack! for non-x86 architectures
				if (name == "__va_list_tag")
					addDeclaration(resultTypeDecl.get());

				return resultTypeDecl;
			}

            ReprisePtr<OPS::Reprise::VariableDeclaration> Clang2RepriseConverter::convertVarDecl(const clang::VarDecl& decl)
			{
				// Ищем уже определенную переменную
				if (VariableDeclaration* varDecl = findConvertedVariable(decl))
				{
					// будем использовать уже существующую переменную
					return ReprisePtr<VariableDeclaration>(varDecl);
				}

				const std::string name = getIdentifierName(decl);
                ReprisePtr<OPS::Reprise::TypeBase> varType = convertQualType(decl.getType());
				OPS::Reprise::VariableDeclarators declarators;
				const clang::VarDecl::StorageClass storageClass = decl.getStorageClass();
				switch(storageClass)
				{
				case clang::SC_Register: declarators.set(OPS::Reprise::VariableDeclarators::DECL_REGISTER); break;
				case clang::SC_Extern: declarators.set(OPS::Reprise::VariableDeclarators::DECL_EXTERN); break;
				case clang::SC_Static: declarators.set(OPS::Reprise::VariableDeclarators::DECL_STATIC); break;
				}

				ReprisePtr<VariableDeclaration> varDecl(new OPS::Reprise::VariableDeclaration(varType.get(), name, declarators));
				m_clangToRepriseDeclMap.insert(std::make_pair(&decl, varDecl));

				// Выражение инициализации может содержать вхождение переменной, поэтому
				// конвертируем его только когда переменная уже создана
				if (decl.getInit() != 0)
				{
                    std::unique_ptr<ExpressionBase> initExpr = convertExpr(*decl.getInit());
					varDecl->setInitExpression(*initExpr);
					initExpr.release();
				}

				return varDecl;
			}

            Reprise::ReprisePtr<OPS::Reprise::TypeDeclaration> Clang2RepriseConverter::convertRecordDecl(const clang::RecordDecl& recordDecl)
			{
				if (TypeDeclaration* typeDecl = findConvertedType(recordDecl))
				{
					// Эта структура уже объявлена - переиспользуем.
					return ReprisePtr<Reprise::TypeDeclaration>(typeDecl);
				}

				std::string name = getIdentifierName(recordDecl);

				// анонимная структура - генерируем ей имя
				if (name.empty())
					name = generateUniqueIndentifier("anon");

				ReprisePtr<Reprise::TypeDeclaration> resultTypeDecl;
				std::unique_ptr<OPS::Reprise::StructType> structType;

				structType.reset(new OPS::Reprise::StructType(recordDecl.isUnion()));

				// создаем новое определение типа
				resultTypeDecl.reset(new OPS::Reprise::TypeDeclaration(structType.get(), name));
				m_clangToRepriseDeclMap.insert(std::make_pair(&recordDecl, resultTypeDecl));

				const bool incompleteType = !recordDecl.isCompleteDefinition();
				structType->setIncomplete(incompleteType);
				//clang::QualType recordDeclarationQualType = recordDecl.getTypeForDecl()->getCanonicalTypeInternal().getDesugaredType(m_astContext);
				{
					for(DeclContext::decl_iterator it = recordDecl.decls_begin(); it != recordDecl.decls_end(); ++it)
					{
						/* unsigned */int bitsLimit = 0;
						if (it->getKind() == clang::Decl::Field)
						{
							const clang::FieldDecl& fieldDecl = static_cast<const clang::FieldDecl&>(**it);
							if (fieldDecl.isBitField())
							{
								OPS_ASSERT(fieldDecl.getBitWidth());
								llvm::APSInt result;
								fieldDecl.getBitWidth()->EvaluateAsInt(result, m_astContext);
								bitsLimit = (int)result.getSExtValue();
							}
							const std::string memberName = getIdentifierName(fieldDecl);

                            ReprisePtr<OPS::Reprise::TypeBase> memberType = convertQualType(fieldDecl.getType());
							std::unique_ptr<OPS::Reprise::StructMemberDescriptor> memberDescriptor(new OPS::Reprise::StructMemberDescriptor(memberName, memberType.get(), bitsLimit));
							structType->addMember(memberDescriptor.get());
							memberDescriptor.release();
						}
						else if (it->getDeclContext() != &recordDecl)
						{
							// Лексически, объявление находится внутри структуры, но семантически - где-то в другом контексте
							// Необходимо добавить это объявление в декларации, соответствующие семантическому контексту
                            m_linkedDecls.insert(std::make_pair(resultTypeDecl.get(), convertDecl(**it).get()));
						}
					}
				}

				structType.release();

				// !hack! for non-x86 architectures
				if (name == "__va_list_tag")
					addDeclaration(resultTypeDecl.get());

				return resultTypeDecl;
			}

            ReprisePtr<OPS::Reprise::TypeDeclaration> Clang2RepriseConverter::convertEnumDecl(const clang::EnumDecl& decl)
			{
				// Ищем уже определенный тип
				if (TypeDeclaration* typeDecl = findConvertedType(decl))
				{
					// будем использовать уже существующий тип
					return ReprisePtr<TypeDeclaration>(typeDecl);
				}

				std::string name = getIdentifierName(decl);

				// анонимный енум - генерируем ему имя
				if (name.empty())
					name = generateUniqueIndentifier("anon");

				std::unique_ptr<OPS::Reprise::EnumType> enumType;
				ReprisePtr<OPS::Reprise::TypeDeclaration> resultTypeDecl;

				// Не нашли тип - создаем новый
				enumType.reset(new OPS::Reprise::EnumType());
				resultTypeDecl.reset(new OPS::Reprise::TypeDeclaration(enumType.get(), name));
				m_clangToRepriseDeclMap.insert(std::make_pair(&decl, resultTypeDecl));

				{
					for(clang::EnumDecl::enumerator_iterator it = decl.enumerator_begin(); it != decl.enumerator_end(); ++it)
					{
						const std::string name = it->getDeclName().getAsIdentifierInfo()->getName();
						const llvm::APInt value = it->getInitVal();
						std::unique_ptr<OPS::Reprise::EnumMemberDescriptor> memberDescriptor(new OPS::Reprise::EnumMemberDescriptor(name, (int)value.getSExtValue()));
						enumType->addMember(memberDescriptor.get());
						memberDescriptor.release();
					}
				}

				enumType.release();

				return resultTypeDecl;
			}

            ReprisePtr<OPS::Reprise::SubroutineDeclaration> Clang2RepriseConverter::convertFunctionDecl(const clang::FunctionDecl& decl)
			{
				// Ищем уже определенную функцию
				if (SubroutineDeclaration* subDecl = findConvertedSubroutine(decl))
				{
					// будем использовать уже существующую функцию
					return ReprisePtr<SubroutineDeclaration>(subDecl);
				}

				const std::string name = getIdentifierName(decl);
                ReprisePtr<OPS::Reprise::TypeBase> returnType = convertQualType(decl.getResultType());
				ReprisePtr<OPS::Reprise::SubroutineType> subrType(new OPS::Reprise::SubroutineType(returnType.get()));
				if (decl.hasPrototype())
				{
					const clang::Type* prototypeType = decl.getType().getTypePtr()->getUnqualifiedDesugaredType();
					if (prototypeType->getTypeClass() == clang::Type::FunctionProto)
					{
						const clang::FunctionProtoType *prototype = static_cast<const clang::FunctionProtoType*>(prototypeType);
						OPS_ASSERT(prototype != 0);
						subrType->setVarArg(prototype->isVariadic());
					}
					else if (prototypeType->getTypeClass() == clang::Type::FunctionNoProto)
					{
					}
					else
                        throw ConverterInternalError("Unknown function prototype type", decl.getLocStart());
				}
				// TODO: use storage class!
				ReprisePtr<SubroutineDeclaration> result(new OPS::Reprise::SubroutineDeclaration(subrType.get(), name));
				m_clangToRepriseDeclMap.insert(std::make_pair(&decl, result));

				std::unique_ptr<OPS::Reprise::Declarations> declarations(new OPS::Reprise::Declarations());

				if (decl.getStorageClass() == clang::SC_Static)
				{
					result->declarators().set(VariableDeclarators::DECL_STATIC);
				}
				if (decl.isInlineSpecified())
				{
					result->declarators().set(VariableDeclarators::DECL_INLINE);
				}

				const bool isFunctionDefinition = decl.isThisDeclarationADefinition();

				m_Declarations.push_back(declarations.get());
				const unsigned int paramCount = decl.getNumParams();
				for(unsigned int paramIndex = 0;paramIndex < paramCount; ++paramIndex)
				{
					const clang::ParmVarDecl *param = decl.getParamDecl(paramIndex);
					const std::string paramName = getIdentifierName(*param);
                    ReprisePtr<OPS::Reprise::TypeBase> paramType = convertQualType(param->getType());
					std::unique_ptr<OPS::Reprise::ParameterDescriptor> paramDescriptor(new OPS::Reprise::ParameterDescriptor(paramName, paramType.get()));
					result->getType().addParameter(paramDescriptor.get());

					if (isFunctionDefinition)
					{
						// We have to redeclare params as variables
						ReprisePtr<OPS::Reprise::VariableDeclaration> varDecl;
						varDecl.reset(new OPS::Reprise::VariableDeclaration(paramType.get()->clone(), paramName));
						m_clangToRepriseDeclMap.insert(std::make_pair(param, varDecl));
						OPS_ASSERT(paramDescriptor.get());
						varDecl->setParameterReference(*paramDescriptor);
						m_Declarations.back()->addVariable(varDecl.get());
					}

					// Release auto_ptrs
					paramDescriptor.release();
				}
				// Set function body...
				if (isFunctionDefinition)
				{
					OPS_ASSERT(decl.getBody()->getStmtClass() == clang::Stmt::CompoundStmtClass);
					const clang::CompoundStmt *funcBody = static_cast<const clang::CompoundStmt*>(decl.getBody());

					m_labelsToStmtMap.clear();
					m_gotoToLabelMap.clear();

                    ReprisePtr<OPS::Reprise::StatementBase> baseStmt = convertStmt(*funcBody);
					OPS::Reprise::ReprisePtr<OPS::Reprise::BlockStatement> body(baseStmt->cast_ptr<OPS::Reprise::BlockStatement>());
					result->setBodyBlock(body);
					result->setDeclarations(declarations.get());
					declarations.release();
					tuneGotoStmts();
				}
				else
				{
					// ... or mark it as prototype
					result->setDeclarations(declarations.get());
					declarations.release();
				}
				subrType.release();
				m_Declarations.pop_back();
				return result;
			}

            std::unique_ptr<OPS::Reprise::ExpressionBase> Clang2RepriseConverter::convertExpr(const clang::Expr& expr)
			{
				std::unique_ptr<OPS::Reprise::ExpressionBase> result;
				// Сначала проверяем, нельзя ли свернуть выражение в константу
				/*
				if (trySimplify)
					result = trySimplifyExpression(expr);
				*/

				if (!result.get())
				{
					switch(expr.getStmtClass())
					{
					case Stmt::PredefinedExprClass:
						{
							const clang::PredefinedExpr& predefinedExpr = static_cast<const clang::PredefinedExpr&>(expr);
							//result.reset(StrictLiteralExpression::createString(m_Declarations.front()->getLastSubr()->getName()));
							result.reset(StrictLiteralExpression::createString(PredefinedExpr::ComputeName(predefinedExpr.getIdentType(), m_currentDecl)));
						}
						break;
					case Stmt::DeclRefExprClass:
						{
							const clang::DeclRefExpr& decl_ref_expr = static_cast<const clang::DeclRefExpr&>(expr);
							const clang::NamedDecl& named_decl = *decl_ref_expr.getDecl();
							const clang::Decl::Kind declKind = named_decl.getKind();
							switch(declKind)
							{
							case clang::Decl::Var:
							case clang::Decl::ParmVar:
								{
									OPS::Reprise::VariableDeclaration* pVariableDeclaration = findConvertedVariable(named_decl);
									if (pVariableDeclaration != NULL)
									{
										result.reset(new OPS::Reprise::ReferenceExpression(*pVariableDeclaration));
									}
									else
									{
                                        throw ConverterInternalError("Can't find variable declaration for expression", expr.getLocStart());
									}
								}
								break;
							case clang::Decl::Function:
								{
									OPS::Reprise::SubroutineDeclaration* pSubroutineDeclaration = findConvertedSubroutine(named_decl);
									if (pSubroutineDeclaration == 0)
									{
										// Если функции нет, создаем её прототип без тела
                                        convertFunctionDecl(static_cast<const clang::FunctionDecl&>(named_decl));
										pSubroutineDeclaration = findConvertedSubroutine(named_decl);
									}
									OPS_ASSERT(pSubroutineDeclaration != 0)
										result.reset(new OPS::Reprise::SubroutineReferenceExpression(*pSubroutineDeclaration));
								}
								break;
							case clang::Decl::Enum:
								{
                                    throw ConverterInternalError("clang::Decl::Enum is not implemented yet in DeclRefExprClass", named_decl.getLocStart());
								}
								break;
							case clang::Decl::Record:
								{
                                    throw ConverterInternalError("clang::Decl::Record is not implemented yet in DeclRefExprClass", named_decl.getLocStart());
								}
								break;
							case clang::Decl::EnumConstant:
								{
									const clang::EnumConstantDecl& enum_constant = static_cast<const clang::EnumConstantDecl&>(named_decl);
									const clang::DeclContext& constant_context = *enum_constant.getDeclContext();
									OPS_ASSERT(constant_context.getDeclKind() == clang::Decl::Enum);
									const clang::EnumDecl& enum_declaration = static_cast<const clang::EnumDecl&>(constant_context);
									TypeDeclaration* enumTypeDecl = findConvertedType(enum_declaration);
									OPS_ASSERT(enumTypeDecl != 0);
									const std::string enumConstName = getIdentifierName(enum_constant);
									OPS::Reprise::EnumType* pEnum = &enumTypeDecl->getType().cast_to<EnumType>();
									std::unique_ptr<OPS::Reprise::EnumMemberDescriptor> pDescriptor;

									for(int i = 0; i < pEnum->getMemberCount(); i++)
									{
										if(pEnum->getMember(i).getName() == enumConstName)
										{
											pDescriptor.reset(&pEnum->getMember(i));
											break;
										}
									}
									OPS_ASSERT(pDescriptor.get()!=NULL);
									result.reset(new OPS::Reprise::EnumAccessExpression(*pDescriptor.release()));
								}
								break;
							default:
								{
                                    throw ConverterInternalError(OPS::Strings::format("Unknown declaration reference class: %s", named_decl.getDeclKindName()),
                                                                 named_decl.getLocStart());
								}
								break;
							}
						}
						break;
					case Stmt::IntegerLiteralClass:
						{
							const clang::IntegerLiteral& integer = static_cast<const clang::IntegerLiteral&>(expr);
							result = convertToStrictLiteral(integer.getValue());
							assignMacroDefinition(expr, result.get());
						}
						break;
					case Stmt::FloatingLiteralClass:
						{
							const clang::FloatingLiteral& floating = static_cast<const clang::FloatingLiteral&>(expr);
							result.reset(OPS::Reprise::StrictLiteralExpression::createFloat64(convertToDoubleSafe(floating.getValue())));
							assignMacroDefinition(expr, result.get());
						}
						break;
					case Stmt::ImaginaryLiteralClass:
						{
							const clang::ImaginaryLiteral& imaginary = static_cast<const clang::ImaginaryLiteral&>(expr);
							OPS_UNUSED(imaginary);
                            throw ConverterInternalError("Stmt::ImaginaryLiteralClass is not implemented yet", expr.getLocStart());
							assignMacroDefinition(expr, result.get());
						}
						break;
					case Stmt::StringLiteralClass:
						{
							const clang::StringLiteral& str = static_cast<const clang::StringLiteral&>(expr);
							int length = str.getLength();
							if(!str.isWide())
							{
								std::string literalText(str.getString().data(), length);
								result.reset(OPS::Reprise::StrictLiteralExpression::createString(literalText));
							} else {
								std::wstring literalText;
								literalText.resize(length);
								for(int i = 0; i < length; i++)
								{
									wchar_t nextWChar = 0;
									const char* pNextChar = &(str.getBytes().data()[i * sizeof(wchar_t)]);
									nextWChar = *(reinterpret_cast <const wchar_t*> (pNextChar));
									literalText[i] = nextWChar;
								}
								result.reset(OPS::Reprise::StrictLiteralExpression::createWideString(literalText));
							}
							assignMacroDefinition(expr, result.get());
						}
						break;
					case Stmt::CharacterLiteralClass:
						{
							const clang::CharacterLiteral& character = static_cast<const clang::CharacterLiteral&>(expr);
							if(character.getKind() == clang::CharacterLiteral::Ascii)
							{
								result.reset(OPS::Reprise::StrictLiteralExpression::createChar((char)character.getValue()));
							}
							else
							{
								result.reset(OPS::Reprise::StrictLiteralExpression::createWideChar((wchar_t)character.getValue()));
							}
							assignMacroDefinition(expr, result.get());
						}
						break;
					case Stmt::ParenExprClass:
						{
							// (expr)
							const clang::ParenExpr& parenExpr = static_cast<const clang::ParenExpr&>(expr);
							OPS_ASSERT(parenExpr.getSubExpr());
                            result = convertExpr(*parenExpr.getSubExpr());
						}
						break;
					case Stmt::UnaryOperatorClass:
						{
							const clang::UnaryOperator& unary_expr = static_cast<const clang::UnaryOperator&>(expr);
							OPS_ASSERT(unary_expr.getSubExpr());
							std::unique_ptr<OPS::Reprise::Canto::HirCCallExpression> pHirCallExpr;
							switch (unary_expr.getOpcode())
							{
							case clang::UO_Plus:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_UNARY_PLUS));
								}
								break;
							case clang::UO_Minus:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_UNARY_MINUS));
								}
								break;
							case clang::UO_Deref:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_DE_REFERENCE));
								}
								break;
							case clang::UO_AddrOf:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_TAKE_ADDRESS));
								}
								break;
							case clang::UO_Not:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_BITWISE_NOT));
								}
								break;
							case clang::UO_LNot:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_LOGICAL_NOT));
								}
								break;
							case clang::UO_PreInc:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_PREFIX_PLUS_PLUS));
								}
								break;
							case clang::UO_PreDec:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_PREFIX_MINUS_MINUS));
								}
								break;
							case clang::UO_PostInc:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_POSTFIX_PLUS_PLUS));
								}
								break;
							case clang::UO_PostDec:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_POSTFIX_MINUS_MINUS));
								}
								break;
                            case clang::UO_Real:
                            case clang::UO_Imag:
                                {
									OPS_ASSERT(!"__real and __imag extensions are not supported");
                                }
                                break;
                            case clang::UO_Extension:
                                {
									OPS_ASSERT(!"__extension__ is not supported");
                                }
                                break;
							default:
								{
                                    llvm::StringRef opcodeStr = clang::UnaryOperator::getOpcodeStr(unary_expr.getOpcode());
                                    throw ConverterInternalError("Unsupported unary operator class: " + opcodeStr.str(), unary_expr.getLocStart());
								}
								break;
							}
                            std::unique_ptr<OPS::Reprise::ExpressionBase> exprArg = convertExpr(*unary_expr.getSubExpr());
							pHirCallExpr->addArgument(exprArg.release());
							result = std::move(pHirCallExpr);
						}
						break;
					case Stmt::ArraySubscriptExprClass:
						{
							const clang::ArraySubscriptExpr& arraySubscriptExpr = static_cast<const clang::ArraySubscriptExpr&>(expr);
							OPS_ASSERT(arraySubscriptExpr.getBase());
							OPS_ASSERT(arraySubscriptExpr.getIdx());

                            std::unique_ptr<OPS::Reprise::ExpressionBase> baseConverted = convertExpr(*arraySubscriptExpr.getBase());
							std::unique_ptr<OPS::Reprise::Canto::HirCCallExpression> pHirCallExpr;

							OPS_ASSERT(baseConverted.get());
							if (OPS::Reprise::Canto::HirCCallExpression *callExpr = dynamic_cast<OPS::Reprise::Canto::HirCCallExpression*>(baseConverted.get()))
							{
								if (callExpr->getKind() == OPS::Reprise::Canto::HirCCallExpression::HIRC_ARRAY_ACCESS)
								{
									// callExpr looks like expr[index1], so just add a new index argument
									pHirCallExpr.reset(callExpr);
									baseConverted.release();
								}
							}
							if (!pHirCallExpr.get())
							{
								pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_ARRAY_ACCESS));
								pHirCallExpr->addArgument(baseConverted.release());
							}
                            pHirCallExpr->addArgument(convertExpr(*arraySubscriptExpr.getIdx()).release());
							result = std::move(pHirCallExpr);
						}
						break;
					case Stmt::MemberExprClass:
						{
							//TODO: how to handle anonymous structs?
							const clang::MemberExpr& member_expr = static_cast<const clang::MemberExpr&>(expr);
							OPS_ASSERT(member_expr.getMemberDecl());
							OPS_ASSERT(member_expr.getBase());
							const clang::NamedDecl* member_decl = member_expr.getMemberDecl();
							const clang::Expr& base_expr = *member_expr.getBase();
                            std::unique_ptr<OPS::Reprise::ExpressionBase> pStructReferenceExpression(convertExpr(*member_expr.getBase()));
							OPS::Reprise::StructMemberDescriptor* pStructMember = 0;
							const clang::Type* struct_type = NULL;
							const clang::RecordType* record_type = NULL;
							const clang::RecordDecl* record_decl = NULL;

							{
								const clang::Type* base_expression_type = base_expr.getType().getTypePtr();

								if (member_expr.isArrow())
								{
									while (clang::Type::Typedef == base_expression_type->getTypeClass())
									{
										const clang::TypedefType& typeDef = static_cast<const clang::TypedefType&>(*base_expression_type);
										base_expression_type = typeDef.desugar().getTypePtr();
									}

									if (clang::Type::Pointer == base_expression_type->getTypeClass())
									{
										const clang::PointerType& struct_pointer = static_cast<const clang::PointerType&>(*base_expression_type);
										struct_type = struct_pointer.getPointeeType().getTypePtr();
									}
									else
									{
                                        throw ConverterInternalError("Unexpected type of base expression in Stmt::MemberExprClass", member_expr.getLocStart());
									}
								} else {
									struct_type = base_expression_type;
								}
							}

							OPS_ASSERT(struct_type->getTypeClass() == clang::Type::Record ||
									   struct_type->getTypeClass() == clang::Type::Typedef||
									   struct_type->getTypeClass() == clang::Type::Elaborated)

							if (struct_type->getTypeClass() == clang::Type::Record)
							{
								record_type = static_cast<const clang::RecordType*>(struct_type);
								record_decl = record_type->getDecl();
							}
							else
							{
								while(struct_type->getTypeClass() == clang::Type::Typedef ||
									  struct_type->getTypeClass() == clang::Type::Elaborated)
								{
									if (struct_type->getTypeClass() == clang::Type::Typedef)
									{
										const clang::TypedefType* typeDef = static_cast<const clang::TypedefType*>(struct_type);
										clang::QualType structQualType = typeDef->desugar();
										struct_type = structQualType.getTypePtr();
									}
									else
									{
										const clang::ElaboratedType* elabType = static_cast<const clang::ElaboratedType*>(struct_type);
										clang::QualType structQualType = elabType->desugar();
										struct_type = structQualType.getTypePtr();
									}
								}
								record_type = static_cast<const clang::RecordType*>(struct_type);
								record_decl = record_type->getDecl();
							}

							const std::string memberName = getIdentifierName(*member_decl);
							OPS_ASSERT(struct_type->getTypeClass() == clang::Type::Record);
							TypeDeclaration* recordTypeDecl = findConvertedType(*record_decl);
							OPS_ASSERT(recordTypeDecl != 0);

							OPS::Reprise::StructType* pStructType = &recordTypeDecl->getType().cast_to<StructType>();
							for(int i = 0; i < pStructType->getMemberCount(); i++)
							{
								if(pStructType->getMember(i).getName() == memberName)
								{
									pStructMember = &pStructType->getMember(i);
									break;
								}
							}
                            if (pStructMember == 0)
                                throw ConverterInternalError("Can't find record member declaration for member reference expression", expr.getLocStart());
							if (member_expr.isArrow())
								pStructReferenceExpression.reset(new BasicCallExpression(BasicCallExpression::BCK_DE_REFERENCE, pStructReferenceExpression.release()));
							result.reset(new OPS::Reprise::StructAccessExpression(*pStructReferenceExpression.release(), *pStructMember));
						}
						break;
					case Stmt::CallExprClass:
						{
							const clang::CallExpr& call_expr = static_cast<const clang::CallExpr&>(expr);
                            std::unique_ptr<OPS::Reprise::ExpressionBase> subroutineCallExpr = convertExpr(*call_expr.getCallee());
							std::unique_ptr<OPS::Reprise::SubroutineCallExpression> subroutineCall(new OPS::Reprise::SubroutineCallExpression(subroutineCallExpr.release()));
							for(unsigned int argumentIndex = 0; argumentIndex < call_expr.getNumArgs(); ++argumentIndex)
							{
                                std::unique_ptr<ExpressionBase> argument = convertExpr(*call_expr.getArg(argumentIndex));
								OPS_ASSERT(argument.get() != 0);
								subroutineCall->addArgument(argument.get());
								argument.release();
							}
							result = std::move(subroutineCall);
						}
						break;
					case Stmt::CompoundAssignOperatorClass:
						{
							const clang::CompoundAssignOperator& compound_assign_expr = static_cast<const clang::CompoundAssignOperator&>(expr);
							OPS_ASSERT(compound_assign_expr.getLHS());
							OPS_ASSERT(compound_assign_expr.getRHS());
							std::unique_ptr<OPS::Reprise::Canto::HirCCallExpression> pHirCallExpr;
							switch (compound_assign_expr.getOpcode())
							{
							case clang::BO_AddAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_PLUS_ASSIGN));
								}
								break;
							case clang::BO_SubAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_MINUS_ASSIGN));
								}
								break;
							case clang::BO_MulAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_MULTIPLY_ASSIGN));
								}
								break;
							case clang::BO_DivAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_DIVISION_ASSIGN));
								}
								break;
							case clang::BO_RemAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_MOD_ASSIGN));
								}
								break;
							case clang::BO_OrAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_BOR_ASSIGN));
								}
								break;
							case clang::BO_AndAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_BAND_ASSIGN));
								}
								break;
							case clang::BO_XorAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_BXOR_ASSIGN));
								}
								break;
							case clang::BO_ShlAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_LSHIFT_ASSIGN));
								}
								break;
							case clang::BO_ShrAssign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_RSHIFT_ASSIGN));
								}
								break;
							default:
								{
                                    llvm::StringRef opcodeStr = CompoundAssignOperator::getOpcodeStr(compound_assign_expr.getOpcode());
                                    throw ConverterInternalError("Unsupported compound assignment operator class: " + opcodeStr.str(),
                                                                 compound_assign_expr.getLocStart());
								}
								break;
							}
                            std::unique_ptr<OPS::Reprise::ExpressionBase> exprArg1 = convertExpr(*compound_assign_expr.getLHS());
                            std::unique_ptr<OPS::Reprise::ExpressionBase> exprArg2 = convertExpr(*compound_assign_expr.getRHS());
							pHirCallExpr->addArgument(exprArg1.release());
							pHirCallExpr->addArgument(exprArg2.release());
							result = std::move(pHirCallExpr);
						}
						break;
					case Stmt::BinaryOperatorClass:
						{
							const clang::BinaryOperator& binary_expr = static_cast<const clang::BinaryOperator&>(expr);
							OPS_ASSERT(binary_expr.getLHS());
							OPS_ASSERT(binary_expr.getRHS());
							std::unique_ptr<OPS::Reprise::Canto::HirCCallExpression> pHirCallExpr;
							switch (binary_expr.getOpcode())
							{
							case clang::BO_Assign:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_ASSIGN));
								}
								break;
							case clang::BO_Add:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_BINARY_PLUS));
								}
								break;
							case clang::BO_Sub:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_BINARY_MINUS));
								}
								break;
							case clang::BO_Mul:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_MULTIPLY));
								}
								break;
							case clang::BO_Div:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_DIVISION));
								}
								break;
							case clang::BO_Rem:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_INTEGER_MOD));
								}
								break;
							case clang::BO_LT:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_LESS));
								}
								break;
							case clang::BO_LE:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_LESS_EQUAL));
								}
								break;
							case clang::BO_GT:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_GREATER));
								}
								break;
							case clang::BO_GE:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_GREATER_EQUAL));
								}
								break;
							case clang::BO_EQ:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_EQUAL));
								}
								break;
							case clang::BO_NE:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_NOT_EQUAL));
								}
								break;
							case clang::BO_And:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_BITWISE_AND));
								}
								break;
							case clang::BO_Xor:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_BITWISE_XOR));
								}
								break;
							case clang::BO_Or:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_BITWISE_OR));
								}
								break;
							case clang::BO_LAnd:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_LOGICAL_AND));
								}
								break;
							case clang::BO_LOr:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_LOGICAL_OR));
								}
								break;
							case clang::BO_Shl:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_LEFT_SHIFT));
								}
								break;
							case clang::BO_Shr:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_RIGHT_SHIFT));
								}
								break;
							case clang::BO_Comma:
								{
									pHirCallExpr.reset(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_COMMA));
								}
								break;
							default:
								{
                                    llvm::StringRef opcodeStr = BinaryOperator::getOpcodeStr(binary_expr.getOpcode());
                                    throw ConverterInternalError("Unsupported binary operator class: " + opcodeStr.str(),
                                                                 binary_expr.getLocStart());
								}
								break;
							}
                            std::unique_ptr<OPS::Reprise::ExpressionBase> exprArg1 = convertExpr(*binary_expr.getLHS());
                            std::unique_ptr<OPS::Reprise::ExpressionBase> exprArg2 = convertExpr(*binary_expr.getRHS());
							pHirCallExpr->addArgument(exprArg1.release());
							pHirCallExpr->addArgument(exprArg2.release());
							result = std::move(pHirCallExpr);
						}
						break;
					case Stmt::ConditionalOperatorClass:
						{
							const clang::ConditionalOperator& conditional_expr = static_cast<const clang::ConditionalOperator&>(expr);
							OPS_ASSERT(conditional_expr.getCond());
							OPS_ASSERT(conditional_expr.getLHS());
							OPS_ASSERT(conditional_expr.getRHS());
							std::unique_ptr<OPS::Reprise::Canto::HirCCallExpression> pHirCallExpr(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_CONDITIONAL));
                            std::unique_ptr<OPS::Reprise::ExpressionBase> exprArg0 = convertExpr(*conditional_expr.getCond());
                            std::unique_ptr<OPS::Reprise::ExpressionBase> exprArg1 = convertExpr(*conditional_expr.getLHS());
                            std::unique_ptr<OPS::Reprise::ExpressionBase> exprArg2 = convertExpr(*conditional_expr.getRHS());
							pHirCallExpr->addArgument(exprArg0.release());
							pHirCallExpr->addArgument(exprArg1.release());
							pHirCallExpr->addArgument(exprArg2.release());
							result = std::move(pHirCallExpr);
						}
						break;
					case Stmt::ImplicitCastExprClass:
						{
							// TODO: don't create implicit cast expression node for function call
							// (when function is function prototype)
							const clang::ImplicitCastExpr& castExpr = static_cast<const clang::ImplicitCastExpr&>(expr);
                            std::unique_ptr<OPS::Reprise::ExpressionBase> castExprConverted = convertExpr(*castExpr.getSubExpr());
							//
							// NB!  (by ZinovyNis)  Я закомментировал все неявные преобразования типов. Остаются только явные, вида (int*)arr_ptr
							//

							// Type to cast to
							const clang::QualType cast_qual_type = castExpr.getType();
							const clang::Type* cast_type = cast_qual_type.getTypePtr();
							/*
							if(cast_type->getTypeClass() == clang::Type::Pointer)
							{
							const clang::Type* pointer_type = static_cast<const clang::PointerType*>(cast_type);
							const clang::Type* pointee_type = pointer_type->getPointeeType().getTypePtr();
							if(pointee_type->getTypeClass() == clang::Type::FunctionProto)
							{
							result.reset(castExprConverted.release());
							return result;
							}
							}
							*/
							// FIX: для выражений типа (struct X){1,2} мы делаем каст, чтобы сохранить тип
							if (castExpr.getSubExpr()->getStmtClass() == Stmt::CompoundLiteralExprClass)
							{
                                result.reset(new OPS::Reprise::TypeCastExpression(convertType(*cast_type).release(), castExprConverted.release(), true));
							}
							else
							{
								result.reset(castExprConverted.release());
							}
						}
						break;
					case Stmt::CStyleCastExprClass:
						{
							const clang::CStyleCastExpr& castExpr = static_cast<const clang::CStyleCastExpr&>(expr);
                            std::unique_ptr<OPS::Reprise::ExpressionBase> castExprConverted = convertExpr(*castExpr.getSubExpr());
							// Type to cast to
							const QualType castType = castExpr.getType();
                            result.reset(new OPS::Reprise::TypeCastExpression(convertQualType(castType).release(), castExprConverted.release()));
						}
						break;
					case Stmt::CompoundLiteralExprClass:
						{
							const clang::CompoundLiteralExpr& compound_literal_expr = static_cast<const clang::CompoundLiteralExpr&>(expr);
							OPS_ASSERT(compound_literal_expr.getInitializer());
                            result = convertExpr(*compound_literal_expr.getInitializer());
						}
						break;
					case Stmt::InitListExprClass:
						{
							const clang::InitListExpr& init_list_expr = static_cast<const clang::InitListExpr&>(expr);
							std::unique_ptr<OPS::Reprise::CompoundLiteralExpression> pCompoundLiteral(new OPS::Reprise::CompoundLiteralExpression());
							int initCount = init_list_expr.getNumInits();
							for(int i = 0; i < initCount; i++)
							{
                                std::unique_ptr<ExpressionBase> expr = convertExpr(*init_list_expr.getInit(i));
								OPS_ASSERT(expr.get());
								pCompoundLiteral->addValue(expr.release());
							}
							result = std::move(pCompoundLiteral);
						}
						break;
					case Stmt::DesignatedInitExprClass:
						{
						}
						break;
					case Stmt::ImplicitValueInitExprClass:
						{
							result.reset(new EmptyExpression);
						}
						break;
					case Stmt::CXXBoolLiteralExprClass:
						{
							// Булевская константа: true или false.
							// NB! Эти константы - не макросы, переводимые в 1 или 0, а именно литералы специального типа - bool.
							const clang::CXXBoolLiteralExpr& boolLiteralExpr =
								static_cast<const clang::CXXBoolLiteralExpr&>(expr);

							const bool literalValue = boolLiteralExpr.getValue();
							result.reset(OPS::Reprise::BasicLiteralExpression::createBoolean(literalValue));
						}
						break;
					case Stmt::VAArgExprClass:
						{
							result.reset(new EmptyExpression);
						}
						break;
					case Stmt::UnaryExprOrTypeTraitExprClass:
						{
							const clang::UnaryExprOrTypeTraitExpr& unaryOrTypeTrait =
									static_cast<const clang::UnaryExprOrTypeTraitExpr&>(expr);
							// sizeof(expr) or alignof(expr) or vec_step

                            if (unaryOrTypeTrait.isArgumentType())
							{
								// what to do with types?
								// just compute result of sizeof() and insert it as literal
								llvm::APSInt resultInt;
								unaryOrTypeTrait.EvaluateAsInt(resultInt, m_astContext);
								result.reset(OPS::Reprise::StrictLiteralExpression::createInt32(resultInt.getLimitedValue()));
							}
							else
							{
                                OPS_ASSERT(unaryOrTypeTrait.getKind() == clang::UETT_SizeOf);
								OPS_ASSERT(unaryOrTypeTrait.getArgumentExpr() != 0);
								std::unique_ptr<OPS::Reprise::Canto::HirCCallExpression> pHirCallExpr(new OPS::Reprise::Canto::HirCCallExpression(OPS::Reprise::Canto::HirCCallExpression::HIRC_SIZE_OF));
                                pHirCallExpr->addArgument(convertExpr(*unaryOrTypeTrait.getArgumentExpr()).release());
								result = std::move(pHirCallExpr);
							}
						}
                        break;
                        case Stmt::StmtExprClass:
                        {
                            const clang::StmtExpr& stmtExpr = static_cast<const StmtExpr&>(expr);
                            ReprisePtr<OPS::Reprise::StatementBase> repriseSubStmts = convertStmt(*stmtExpr.getSubStmt());
                            ReprisePtr<OPS::Reprise::BlockStatement> subBlock = ensureBlockStmt(repriseSubStmts.get());

                            result.reset(new Canto::HirCStatementExpression(subBlock.get()));
                        }
                        break;
                        case Stmt::OffsetOfExprClass:
                        {
                            const clang::OffsetOfExpr& offsetOfExpr =
                                    static_cast<const clang::OffsetOfExpr&>(expr);

                            llvm::APSInt resultInt;
                            if (offsetOfExpr.EvaluateAsInt(resultInt, m_astContext))
                            {
                                result.reset(OPS::Reprise::StrictLiteralExpression::createInt32(resultInt.getLimitedValue()));
                            }
                            else
                            {
                                throw OPS::Exception("Could not evaluate offsetof");
                            }
                        }
                        break;
					default:
                        throw ConverterInternalError(OPS::Strings::format(
                                                  "Unknown expression class: %s",
                                                  expr.getStmtClassName()),
                                                     expr.getLocStart());
						break;
					}
				}
				if (result.get())
				{
					assignLocation(expr, *result);
				}
				return result;
			}

            ReprisePtr<StatementBase> Clang2RepriseConverter::convertStmt(const clang::Stmt& stmt)
			{
				ReprisePtr<OPS::Reprise::StatementBase> result;
				// TODO: add *stmtIt to the compound statement
				if (!clang::Expr::classof(&stmt))
				{
					// regular statement
					switch(stmt.getStmtClass())
					{
					case Stmt::NullStmtClass:
						{
							result.reset(new OPS::Reprise::EmptyStatement());
						}
						break;
					case Stmt::CompoundStmtClass:
						{
							const clang::CompoundStmt& comp = static_cast<const clang::CompoundStmt&>(stmt);
                            ReprisePtr<OPS::Reprise::BlockStatement> blockStmt(new OPS::Reprise::BlockStatement());
							// add current compound stmt onto the context stack top
                            m_Contexts.push(blockStmt.get());
							for(clang::CompoundStmt::const_body_iterator stmtIt = comp.body_begin(); stmtIt != comp.body_end(); ++stmtIt)
							{
                                ReprisePtr<OPS::Reprise::StatementBase> compConverted = convertStmt(**stmtIt);
                                if ((*stmtIt)->getStmtClass() != Stmt::DeclStmtClass &&
                                        compConverted.get() == 0)
                                {
                                    throw ConverterInternalError("Error converting block inner statement", (**stmtIt).getLocStart());
                                }

								if (compConverted.get())
								{
									blockStmt->addLast(compConverted.get());
								}
							}
							// restore prev. compound stmt from the context stack
							m_Contexts.pop();
                            result.reset(blockStmt.get());
						}
						break;
					case Stmt::CaseStmtClass:
						{
							const clang::CaseStmt& switchCase = static_cast<const clang::CaseStmt&>(stmt);
							const clang::Stmt* casedStmt = switchCase.getSubStmt();
                            ReprisePtr<OPS::Reprise::StatementBase> casedStatementConverted = convertStmt(*casedStmt);
							sqword caseValue = 0;
							llvm::APSInt resultInt;
							switchCase.getLHS()->EvaluateAsInt(resultInt, m_astContext);
							if (!convertToInt64(resultInt, caseValue))
                                throw ConverterInternalError("plain case value is too large", stmt.getLocStart());

                            OPS::Reprise::PlainSwitchStatement& switchStmt = *m_plainSwitchStmts.top();
                            switchStmt.addLabel(new OPS::Reprise::PlainCaseLabel(caseValue, casedStatementConverted.get()));

                            result.reset(casedStatementConverted.get());
						}
						break;
					case Stmt::DefaultStmtClass:
						{
							const clang::DefaultStmt& defaultStmt = static_cast<const clang::DefaultStmt&>(stmt);
							const clang::Stmt* casedStmt = defaultStmt.getSubStmt();
                            ReprisePtr<OPS::Reprise::StatementBase> casedStatementConverted = convertStmt(*casedStmt);
							std::unique_ptr<OPS::Reprise::PlainCaseLabel> caseLabel(new OPS::Reprise::PlainCaseLabel());
							caseLabel->setDefault(true);
							caseLabel->setStatement(casedStatementConverted.get());

                            OPS::Reprise::PlainSwitchStatement& switchStmt = *m_plainSwitchStmts.top();
                            switchStmt.addLabel(caseLabel.release());

							result.reset(casedStatementConverted.get());
						}
						break;
					case Stmt::LabelStmtClass:
						{
							// Statement with a label
							const clang::LabelStmt& labelStmt = static_cast<const clang::LabelStmt&>(stmt);
							const std::string labelName = labelStmt.getName();
							const clang::Stmt* labeledStatement = labelStmt.getSubStmt();
                            ReprisePtr<OPS::Reprise::StatementBase> labeledStatementConverted = convertStmt(*labeledStatement);
							labeledStatementConverted->setLabel(labelName);
							OPS_ASSERT(labeledStatementConverted.get() != 0);
							m_labelsToStmtMap[labelName] = labeledStatementConverted.get();
							result.reset(labeledStatementConverted.get());
						}
						break;
					case Stmt::IfStmtClass:
						{
							const clang::IfStmt& ifStmt = static_cast<const clang::IfStmt&>(stmt);
                            ReprisePtr<OPS::Reprise::IfStatement> repriseIfStmt(new OPS::Reprise::IfStatement());

                            std::unique_ptr<OPS::Reprise::ExpressionBase> cond = convertExpr(*ifStmt.getCond());
                            repriseIfStmt->setCondition(cond.release());

							if (ifStmt.getThen())
							{
                                ReprisePtr<OPS::Reprise::StatementBase> repriseThenBranch = convertStmt(*ifStmt.getThen());
                                ReprisePtr<OPS::Reprise::BlockStatement> thenBranch;
								thenBranch = ensureBlockStmt(repriseThenBranch.get());
                                repriseIfStmt->setThenBody(thenBranch.get());
							}
							if (ifStmt.getElse())
							{
                                ReprisePtr<OPS::Reprise::StatementBase> repriseElseBranch = convertStmt(*ifStmt.getElse());
                                ReprisePtr<OPS::Reprise::BlockStatement> elseBranch;
								elseBranch = ensureBlockStmt(repriseElseBranch.get());
								repriseIfStmt->setElseBody(elseBranch.get());
							}
							result.reset(repriseIfStmt.get());
						}
						break;
					case Stmt::SwitchStmtClass:
						{
							const clang::SwitchStmt& switchStmt = static_cast<const clang::SwitchStmt&>(stmt);
                            std::unique_ptr<OPS::Reprise::ExpressionBase> cond(convertExpr(*switchStmt.getCond()));
                            ReprisePtr<OPS::Reprise::PlainSwitchStatement> plainSwitchStmt(
                                        new OPS::Reprise::PlainSwitchStatement(cond.release()));

                            m_plainSwitchStmts.push(plainSwitchStmt.get());
                            ReprisePtr<OPS::Reprise::StatementBase> switchBody = convertStmt(*switchStmt.getBody());
                            OPS::Reprise::BlockStatement& switchBlock = switchBody->cast_to<BlockStatement>();
                            plainSwitchStmt->setBody(&switchBlock);
                            m_plainSwitchStmts.pop();

                            result = plainSwitchStmt;
						}
						break;
					case Stmt::WhileStmtClass:
						{
							const clang::WhileStmt& whileStmt = static_cast<const clang::WhileStmt&>(stmt);
							std::unique_ptr<OPS::Reprise::WhileStatement> repriseWhileStmt(new OPS::Reprise::WhileStatement(true));
							std::unique_ptr<OPS::Reprise::ExpressionBase> cond;
							if (whileStmt.getCond())
							{
                                cond = convertExpr(*whileStmt.getCond());
								repriseWhileStmt->setCondition(cond.get());
								cond.release();
							}
							if (whileStmt.getBody() != 0)
							{
                                ReprisePtr<OPS::Reprise::StatementBase> repriseBody = convertStmt(*whileStmt.getBody());
                                ReprisePtr<OPS::Reprise::BlockStatement> body = ensureBlockStmt(repriseBody.get());

								repriseWhileStmt->setBody(body.get());
							}
							result.reset(repriseWhileStmt.get());
							repriseWhileStmt.release();
						}
						break;
					case Stmt::DoStmtClass:
						{
							const clang::DoStmt& doStmt = static_cast<const clang::DoStmt&>(stmt);
							std::unique_ptr<OPS::Reprise::WhileStatement> repriseWhileStmt(new OPS::Reprise::WhileStatement(false));
							std::unique_ptr<OPS::Reprise::ExpressionBase> cond;
							if (doStmt.getCond())
							{
                                cond = convertExpr(*doStmt.getCond());
								repriseWhileStmt->setCondition(cond.get());
								cond.release();
							}
							if (doStmt.getBody() != 0)
							{
                                ReprisePtr<OPS::Reprise::StatementBase> repriseBody = convertStmt(*doStmt.getBody());
                                ReprisePtr<OPS::Reprise::BlockStatement> body = ensureBlockStmt(repriseBody.get());

								repriseWhileStmt->setBody(body.get());
							}
							result.reset(repriseWhileStmt.get());
							repriseWhileStmt.release();
						}
						break;
					case Stmt::ForStmtClass:
						{
							const clang::ForStmt& forStmt = static_cast<const clang::ForStmt&>(stmt);
							const Stmt *initExpr = forStmt.getInit();
							const Expr *condExpr = forStmt.getCond();
							const Expr *incExpr = forStmt.getInc();
							const Stmt *body = forStmt.getBody();

							std::unique_ptr<OPS::Reprise::BlockStatement> baseBlock;
							std::unique_ptr<OPS::Reprise::ForStatement> repriseForStmt(new OPS::Reprise::ForStatement());

							if(initExpr != NULL && initExpr->getStmtClass() != Stmt::NullStmtClass)
							{
								if (initExpr->getStmtClass() == Stmt::DeclStmtClass)
								{
									baseBlock.reset(new BlockStatement);
									m_Contexts.push(baseBlock.get());
								}

								ReprisePtr<OPS::Reprise::StatementBase> initStatementConverted;
                                initStatementConverted = convertStmt(*initExpr);
								OPS_ASSERT(initStatementConverted.get() != NULL);
                                if (ExpressionStatement* exprStmt = initStatementConverted->cast_ptr<ExpressionStatement>())
								{
                                    repriseForStmt->setInitExpression(&exprStmt->get());
								}
								else
								{
                                    if (Canto::HirCVariableInitStatement* varInit = initStatementConverted->cast_ptr<Canto::HirCVariableInitStatement>())
                                    {
                                        varInit->connectToForStmt(*repriseForStmt);
                                    }
									baseBlock->addLast(initStatementConverted.release());
								}
							}

							if(condExpr != NULL)
							{
								std::unique_ptr<OPS::Reprise::ExpressionBase> condExpressionConverted;
                                condExpressionConverted = convertExpr(*condExpr);
								repriseForStmt->setFinalExpression(condExpressionConverted.release());
							}

							if(incExpr != NULL)
							{
								std::unique_ptr<OPS::Reprise::ExpressionBase> incExpressionConverted;
                                incExpressionConverted = convertExpr(*incExpr);
								repriseForStmt->setStepExpression(incExpressionConverted.release());
							}

                            ReprisePtr<OPS::Reprise::StatementBase> repriseBody = convertStmt(*body);
                            ReprisePtr<OPS::Reprise::BlockStatement> bodyBlock = ensureBlockStmt(repriseBody.get());

							repriseForStmt->setBody(bodyBlock.get());

							if (baseBlock.get())
							{
								m_Contexts.pop();
								baseBlock->addLast(repriseForStmt.get());
								result.reset(baseBlock.release());
							}
							else
							{
								result.reset(repriseForStmt.get());
							}

							repriseForStmt.release();
						}
						break;
					case Stmt::GotoStmtClass:
						{
							const clang::GotoStmt& gotoStmt = static_cast<const clang::GotoStmt&>(stmt);
							const clang::LabelStmt *labelStmt = gotoStmt.getLabel()->getStmt();
							OPS_ASSERT(labelStmt != 0);
							const std::string labelName = labelStmt->getName();
							// the label will be tuned after all the function body is parsed
							std::unique_ptr<OPS::Reprise::GotoStatement> repriseGotoStmt(new OPS::Reprise::GotoStatement());
							m_gotoToLabelMap[repriseGotoStmt.get()] = labelName;
							result.reset(repriseGotoStmt.release());
						}
						break;
					case Stmt::IndirectGotoStmtClass:
						{
							// TODO: what is it?
                            throw ConverterInternalError("Stmt::IndirectGotoStmtClass is not implemented yet", stmt.getLocStart());
						}
						break;
					case Stmt::ContinueStmtClass:
						{
							result.reset(new OPS::Reprise::Canto::HirContinueStatement());
						}
						break;
					case Stmt::BreakStmtClass:
						{
							result.reset(new OPS::Reprise::Canto::HirBreakStatement());
						}
						break;
					case Stmt::ReturnStmtClass:
						{
							const clang::ReturnStmt& returnStmt = static_cast<const clang::ReturnStmt&>(stmt);
							const Expr* returnExpr = returnStmt.getRetValue();
							std::unique_ptr<OPS::Reprise::ExpressionBase> returnExprConverted;
							if (returnExpr)
							{
                                returnExprConverted = convertExpr(*returnExpr);
								result.reset(new OPS::Reprise::ReturnStatement(returnExprConverted.release()));
							} else {
								result.reset(new OPS::Reprise::ReturnStatement());
							}
						}
						break;
					case Stmt::DeclStmtClass:
						{
							const clang::DeclStmt& declStmt = static_cast<const clang::DeclStmt&>(stmt);
							// true, if the decl is like "int a;"
							// false if the decl is like "int a, b, c;"
							//const bool isSingleDecl = declStmt.isSingleDecl();
							clang::DeclGroupRef declGroup = declStmt.getDeclGroup();
							ReprisePtr<BlockStatement> initBlock(new BlockStatement);
							for(clang::DeclGroupRef::const_iterator it = declGroup.begin(); it != declGroup.end(); ++it)
							{
								OPS_ASSERT(*it);
                                ReprisePtr<DeclarationBase> decl = convertDecl(**it);
								addDeclaration(decl.get());
								if (VariableDeclaration* varDecl = decl->cast_ptr<VariableDeclaration>())
								{
									if (varDecl->hasNonEmptyInitExpression() &&
										!varDecl->getDeclarators().isStatic())
									{
										initBlock->addLast(new Canto::HirCVariableInitStatement(
														varDecl,
														varDecl->detachInitExpression().get()));
									}
								}
							}

							if (!initBlock->isEmpty())
							{
								if (initBlock->getFirst() == initBlock->getLast())
									result.reset(&*initBlock->getFirst());
								else
									result.reset(initBlock.release());
							}
						}
						break;
					case Stmt::MSAsmStmtClass:
						{
                            const clang::MSAsmStmt& MSASMStmt = static_cast<const clang::MSAsmStmt&>(stmt);
                            if (!MSASMStmt.isSimple())
                            {
                                throw ConverterInternalError("inline assembler with inputs or outputs is not supported yet", stmt.getLocStart());
                            }
                            result.reset(new ASMStatement(MSASMStmt.getAsmString().str(), ASMStatement::ASMTP_MS));
						}
						break;
					case Stmt::GCCAsmStmtClass:
						{
							const clang::GCCAsmStmt& GCCASMStmt = static_cast<const clang::GCCAsmStmt&>(stmt);
                            if(!GCCASMStmt.isSimple())
                            {
                                throw ConverterInternalError("inline assembler with inputs or outputs is not supported yet", stmt.getLocStart());
							}
							result.reset(new OPS::Reprise::ASMStatement(GCCASMStmt.getAsmString()->getString().str(), ASMStatement::ASMTP_GCC));
						}
						break;
					default:
						{
                            throw ConverterInternalError(OPS::Strings::format(
                                                      "Statement class is not implemented: %s", stmt.getStmtClassName()),
                                                         stmt.getLocStart());
						}
					}
				}
				else
				{
					// stmt is a statement-expression
                    std::unique_ptr<OPS::Reprise::ExpressionBase> expr = convertExpr(static_cast<const clang::Expr&>(stmt));
					OPS_ASSERT(expr.get());
					result.reset(new OPS::Reprise::ExpressionStatement(expr.release()));
				}

				if (result.get())
				{
					assignLocation(stmt, *result);
					markPragmas(stmt, result.get());
				}
				return result;
			}

            void Clang2RepriseConverter::addDeclaration(DeclarationBase *decl)
            {
                // Добавляем сначала связанные объявления
                LinkedDeclMap::iterator it = m_linkedDecls.lower_bound(decl);
                while (it != m_linkedDecls.end() && it->first == decl)
                {
                    addDeclaration(it->second);
                    ++it;
                }
                // Потом само..
                if (decl->getParent() == 0)
                    m_Declarations.back()->addLast(decl);
            }

            AST2RepriseConverter::AST2RepriseConverter(
                OPS::Reprise::RepriseContext& repriseContext,
                ReprisePtr<OPS::Reprise::TranslationUnit>& translationUnit,
                DiagnosticsEngine &diagnostic,
                llvm::raw_ostream* outputStream)
                :
            m_repriseContext(repriseContext),
                m_astContext(0),
                m_translationUnit(translationUnit),
                m_diagnostic(diagnostic),
                m_outputStream(outputStream)
            {
            }

            void Clang2RepriseConverter::markPragmas(const clang::PresumedLoc beginLocation, OPS::Reprise::RepriseBase* opsNode)
            {
                SourceCodeManager& repriseSourceManager = m_repriseContext.getSourceCodeManager();
                clang::SourceManager& clangSourceManager = m_astContext.getSourceManager();
                int fileId = repriseSourceManager.registerFile(beginLocation.getFilename());

                std::set<int> pragmaLines;
                ClangParser::Pragmas::iterator pPragma = m_pragmas.begin();
                for(; pPragma != m_pragmas.end(); ++pPragma)
                {
                    clang::SourceLocation loc = pPragma->first;
                    const clang::PresumedLoc pragmaBegin = clangSourceManager.getPresumedLoc(loc);
                    int pragmaFileId = repriseSourceManager.registerFile(pragmaBegin.getFilename());

                    if(fileId == pragmaFileId)
                    {
                        pragmaLines.insert(pragmaBegin.getLine());
                    }
                }

                for(pPragma = m_pragmas.begin(); pPragma != m_pragmas.end(); ++pPragma)
                {
                    clang::SourceLocation loc = pPragma->first;
                    std::string pragmaName = pPragma->second.first;
                    std::string pragmaArgs = pPragma->second.second;

                    const clang::PresumedLoc pragmaBegin = clangSourceManager.getPresumedLoc(loc);

                    int pragmaFileId = repriseSourceManager.registerFile(pragmaBegin.getFilename());

                    int nodeLine = beginLocation.getLine();
                    int line = pragmaBegin.getLine() + 1;
                    for(; line < nodeLine; line++)
                    {
                        if(pragmaLines.find(line) == pragmaLines.end()) break;
                    }
                    if(fileId == pragmaFileId && line == nodeLine)
                    {
                        if(!opsNode->hasNote(pragmaName))
                        {
                            opsNode->setNote(pragmaName, OPS::Reprise::Note::newString(pragmaArgs));
                        } else {
                            std::string note = opsNode->getNote(pragmaName).getString();
                            note += ";";
                            note += pragmaArgs;
                            Note& oldNote = opsNode->getNote(pragmaName);
                            oldNote.setString(note);
                        }
                    }
                }
            }

            void Clang2RepriseConverter::markPragmas(const clang::Decl& decl, OPS::Reprise::RepriseBase* opsNode)
            {
                clang::SourceManager& clangSourceManager = m_astContext.getSourceManager();
                const clang::PresumedLoc beginLocation = clangSourceManager.getPresumedLoc(decl.getLocStart());

                markPragmas(beginLocation, opsNode);
            }

            void Clang2RepriseConverter::markPragmas(const clang::Stmt& stmt, OPS::Reprise::RepriseBase* opsNode)
            {
                clang::SourceManager& clangSourceManager = m_astContext.getSourceManager();
                const clang::PresumedLoc beginLocation = clangSourceManager.getPresumedLoc(stmt.getLocStart());

                markPragmas(beginLocation, opsNode);
            }

            void AST2RepriseConverter::HandleTranslationUnit(ASTContext &astContext)
            {
                // Convert AST iff there were no errors in the source file
                if (!m_diagnostic.hasErrorOccurred())
                {
                    // Store AST Context for its Source Manager
                    m_astContext = &astContext;

                    // TODO: (zzz) remove the block as debug only
                    if (m_outputStream != 0)
                    {
                        PrintingPolicy policy = astContext.getPrintingPolicy();
                        policy.SuppressTag = false;
                        policy.SuppressScope = false;
                        // Print AST dump to the console
                        astContext.getTranslationUnitDecl()->print(*m_outputStream, policy);
                    }

                    try
                    {
                        m_translationUnit = Clang2RepriseConverter(
                            m_repriseContext,
                            astContext,
                            *astContext.getTranslationUnitDecl(),
                            m_pragmas).convert();
                    }
                    catch(OPS::Exception& ex)
                    {
                        unsigned internalErrorDiagId = m_diagnostic.getCustomDiagID(DiagnosticsEngine::Fatal, "Internal error: %0");
                        SourceLocation loc;
                        if (ConverterInternalError* intErr = dynamic_cast<ConverterInternalError*>(&ex))
                            loc = intErr->getSourceLocation();

                        DiagnosticBuilder builder = m_diagnostic.Report(loc, internalErrorDiagId);
                        builder.AddString(ex.getMessage());                        
                    }

                    if (m_translationUnit.get() != 0)
                    {
                        const FileEntry* pFileEntry =
                            astContext
                            .getSourceManager()
                            .getFileEntryForID(astContext.getSourceManager().getMainFileID());

                        if (0 != pFileEntry)
                        {
                            const std::string mainFileName = pFileEntry->getName();
                            m_translationUnit->setSourceFilename(mainFileName);
                        }
                    }
                }
                else
                {
                    // const unsigned num_diags = m_diagnostic.getNumDiagnostics();
                    // const unsigned num_errors = m_diagnostic.getNumErrors();
                    m_translationUnit.reset(0);
                }
            }

        std::unique_ptr<clang::ASTConsumer> CreateAST2RepriseDumper(OPS::Reprise::RepriseContext& context, clang::DiagnosticsEngine& diagnostic,
            OPS::Reprise::ReprisePtr<OPS::Reprise::TranslationUnit>& translationUnit,
            llvm::raw_ostream* outputStream)
        {
            return std::unique_ptr<clang::ASTConsumer>(new AST2RepriseConverter(context, translationUnit, diagnostic, outputStream));
        }

    }
}
