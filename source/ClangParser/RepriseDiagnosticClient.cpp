#include "OPS_Core/disable_llvm_warnings_begin.h"

#include "clang/Frontend/ASTConsumers.h"
#include "RepriseDiagnosticClient.h"	
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "llvm/Support/raw_ostream.h"

#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallString.h"

#include "OPS_Core/disable_llvm_warnings_end.h"

#include <algorithm>
#include <iostream>
#include <list>
#include <string>

using namespace clang;

namespace OPS
{
    namespace Reprise
    {
        class RepriseDiagnosticClient: public clang::DiagnosticConsumer
        {
            SourceLocation m_lastWarningLoc;
            std::list<CompilerResultMessage> &m_messageList;
            LangOptions m_langOpts;
			SourceLocation LastLoc;
            bool LastCaretDiagnosticWasNote;

            CompilerResultMessage::CompilerResultKind translateDiagnosticsLevelToReprise(const DiagnosticsEngine::Level diagLevel)
            {
                CompilerResultMessage::CompilerResultKind resultKind = CompilerResultMessage::CRK_NONE;
                switch(diagLevel)
                {
                case DiagnosticsEngine::Ignored:
                    {
                        resultKind = CompilerResultMessage::CRK_IGNORE;
                    }
                    break;
                case DiagnosticsEngine::Note:
                    {
                        resultKind = CompilerResultMessage::CRK_NOTE;
                    }
                    break;
                case DiagnosticsEngine::Warning:
                    {
                        resultKind = CompilerResultMessage::CRK_WARNING;
                    }
                    break;
                case DiagnosticsEngine::Error:
                    {
                        resultKind = CompilerResultMessage::CRK_ERROR;
                    }
                    break;
                case DiagnosticsEngine::Fatal:
                    {
                        resultKind = CompilerResultMessage::CRK_FATAL;
                    }
                    break;
                default:
                    {
                        assert(!"Unknown diagnostic level");
                    }
                }
                return resultKind;
            }

			/*virtual*/ void BeginSourceFile(const LangOptions &LangOpts, const Preprocessor *PP = 0)
			{
				OPS_UNUSED(PP);
				m_langOpts = LangOpts;
			}

            std::string printIncludeStack(clang::SourceLocation location, const clang::SourceManager &sourceManager) 
            {
                std::string result;
                if (!location.isInvalid())
                {
                    PresumedLoc pLoc = sourceManager.getPresumedLoc(location);
                    result += printIncludeStack(pLoc.getIncludeLoc(), sourceManager);
                    result += OPS::Strings::format("In file included from %s: %d\n", pLoc.getFilename(), pLoc.getLine());
                }
                return result;
            }

            static void SelectInterestingSourceRegion(std::string &SourceLine,
                std::string &CaretLine,
                std::string &FixItInsertionLine,
                unsigned EndOfCaretToken,
                unsigned Columns) {
                    if (CaretLine.size() > SourceLine.size())
                        SourceLine.resize(CaretLine.size(), ' ');

                    // Find the slice that we need to display the full caret line
                    // correctly.
                    unsigned CaretStart = 0, CaretEnd = CaretLine.size();
                    for (; CaretStart != CaretEnd; ++CaretStart)
                        if (!isspace(CaretLine[CaretStart]))
                            break;

                    for (; CaretEnd != CaretStart; --CaretEnd)
                        if (!isspace(CaretLine[CaretEnd - 1]))
                            break;

                    // Make sure we don't chop the string shorter than the caret token
                    // itself.
                    if (CaretEnd < EndOfCaretToken)
                        CaretEnd = EndOfCaretToken;

                    // If we have a fix-it line, make sure the slice includes all of the
                    // fix-it information.
                    if (!FixItInsertionLine.empty()) {
                        unsigned FixItStart = 0, FixItEnd = FixItInsertionLine.size();
                        for (; FixItStart != FixItEnd; ++FixItStart)
                            if (!isspace(FixItInsertionLine[FixItStart]))
                                break;

                        for (; FixItEnd != FixItStart; --FixItEnd)
                            if (!isspace(FixItInsertionLine[FixItEnd - 1]))
                                break;

                        if (FixItStart < CaretStart)
                            CaretStart = FixItStart;
                        if (FixItEnd > CaretEnd)
                            CaretEnd = FixItEnd;
                    }

                    // CaretLine[CaretStart, CaretEnd) contains all of the interesting
                    // parts of the caret line. While this slice is smaller than the
                    // number of columns we have, try to grow the slice to encompass
                    // more context.

                    // If the end of the interesting region comes before we run out of
                    // space in the terminal, start at the beginning of the line.
                    if (Columns > 3 && CaretEnd < Columns - 3)
                        CaretStart = 0;

                    unsigned TargetColumns = Columns;
                    if (TargetColumns > 8)
                        TargetColumns -= 8; // Give us extra room for the ellipses.
                    unsigned SourceLength = SourceLine.size();
                    while ((CaretEnd - CaretStart) < TargetColumns) {
                        bool ExpandedRegion = false;
                        // Move the start of the interesting region left until we've
                        // pulled in something else interesting.
                        if (CaretStart == 1)
                            CaretStart = 0;
                        else if (CaretStart > 1) {
                            unsigned NewStart = CaretStart - 1;

                            // Skip over any whitespace we see here; we're looking for
                            // another bit of interesting text.
                            while (NewStart && isspace(SourceLine[NewStart]))
                                --NewStart;

                            // Skip over this bit of "interesting" text.
                            while (NewStart && !isspace(SourceLine[NewStart]))
                                --NewStart;

                            // Move up to the non-whitespace character we just saw.
                            if (NewStart)
                                ++NewStart;

                            // If we're still within our limit, update the starting
                            // position within the source/caret line.
                            if (CaretEnd - NewStart <= TargetColumns) {
                                CaretStart = NewStart;
                                ExpandedRegion = true;
                            }
                        }

                        // Move the end of the interesting region right until we've
                        // pulled in something else interesting.
                        if (CaretEnd != SourceLength) {
                            assert(CaretEnd < SourceLength && "Unexpected caret position!");
                            unsigned NewEnd = CaretEnd;

                            // Skip over any whitespace we see here; we're looking for
                            // another bit of interesting text.
                            while (NewEnd != SourceLength && isspace(SourceLine[NewEnd - 1]))
                                ++NewEnd;

                            // Skip over this bit of "interesting" text.
                            while (NewEnd != SourceLength && !isspace(SourceLine[NewEnd - 1]))
                                ++NewEnd;

                            if (NewEnd - CaretStart <= TargetColumns) {
                                CaretEnd = NewEnd;
                                ExpandedRegion = true;
                            }
                        }

                        if (!ExpandedRegion)
                            break;
                    }

                    // [CaretStart, CaretEnd) is the slice we want. Update the various
                    // output lines to show only this slice, with two-space padding
                    // before the lines so that it looks nicer.
                    if (CaretEnd < SourceLine.size())
                        SourceLine.replace(CaretEnd, std::string::npos, "...");
                    if (CaretEnd < CaretLine.size())
                        CaretLine.erase(CaretEnd, std::string::npos);
                    if (FixItInsertionLine.size() > CaretEnd)
                        FixItInsertionLine.erase(CaretEnd, std::string::npos);

                    if (CaretStart > 2) {
                        SourceLine.replace(0, CaretStart, "  ...");
                        CaretLine.replace(0, CaretStart, "     ");
                        if (FixItInsertionLine.size() >= CaretStart)
                            FixItInsertionLine.replace(0, CaretStart, "     ");
                    }
            }

            void HighlightRange(const CharSourceRange &R, const SourceManager &SM, unsigned LineNo, FileID FID, std::string &CaretLine, const std::string &SourceLine) 
            {
                assert(CaretLine.size() == SourceLine.size() &&
                    "Expect a correspondence between source and caret line!");
                if (!R.isValid()) return;

                SourceLocation Begin = SM.getExpansionLoc(R.getBegin());
                SourceLocation End = SM.getExpansionLoc(R.getEnd());

                // If the End location and the start location are the same and are a macro
                // location, then the range was something that came from a macro expansion
                // or _Pragma.  If this is an object-like macro, the best we can do is to
                // highlight the range.  If this is a function-like macro, we'd also like to
                // highlight the arguments.
                if (Begin == End && R.getEnd().isMacroID())
                    End = SM.getExpansionRange(R.getEnd()).second;

                unsigned StartLineNo = SM.getExpansionLineNumber(Begin);
                if (StartLineNo > LineNo || SM.getFileID(Begin) != FID)
                    return;  // No intersection.

                unsigned EndLineNo = SM.getExpansionLineNumber(End);
                if (EndLineNo < LineNo || SM.getFileID(End) != FID)
                    return;  // No intersection.

                // Compute the column number of the start.
                unsigned StartColNo = 0;
                if (StartLineNo == LineNo) 
                {
                    StartColNo = SM.getExpansionColumnNumber(Begin);
                    if (StartColNo) --StartColNo;  // Zero base the col #.
                }

                // Pick the first non-whitespace column.
                while (StartColNo < SourceLine.size() &&
                    (SourceLine[StartColNo] == ' ' || SourceLine[StartColNo] == '\t'))
                    ++StartColNo;

                // Compute the column number of the end.
                unsigned EndColNo = CaretLine.size();
                if (EndLineNo == LineNo) 
                {
                    EndColNo = SM.getExpansionColumnNumber(End);
                    if (EndColNo) 
                    {
                        --EndColNo;  // Zero base the col #.

                        // Add in the length of the token, so that we cover multi-char tokens.
                        EndColNo += Lexer::MeasureTokenLength(End, SM, m_langOpts);
                    } 
                    else 
                    {
                        EndColNo = CaretLine.size();
                    }
                }

                // Pick the last non-whitespace column.
                if (EndColNo <= SourceLine.size())
                    while (EndColNo-1 &&
                        (SourceLine[EndColNo-1] == ' ' || SourceLine[EndColNo-1] == '\t'))
                        --EndColNo;
                else
                    EndColNo = SourceLine.size();

                // Fill the range with ~'s.
                //assert(StartColNo <= EndColNo && "Invalid range!");
                for (unsigned i = StartColNo; i < EndColNo; ++i)
                    CaretLine[i] = '~';
            }

            std::string EmitCaretDiagnostic(SourceLocation Loc, CharSourceRange *Ranges, 
                unsigned NumRanges, SourceManager &SM, 
				const FixItHint *Hints, unsigned NumHints, unsigned Columns) 
            {
                assert(!Loc.isInvalid() && "must have a valid source location here");

                std::string result;

                // If this is a macro ID, first emit information about where this was
                // instantiated (recursively) then emit information about where. the token was
                // spelled from.
                if (!Loc.isFileID()) 
                {
                    SourceLocation OneLevelUp = SM.getImmediateExpansionRange(Loc).first;
                    // FIXME: Map ranges?
                    result += EmitCaretDiagnostic(OneLevelUp, Ranges, NumRanges, SM, 0, 0, Columns);

                    Loc = SM.getImmediateSpellingLoc(Loc);

                    // Map the ranges.
                    for (unsigned i = 0; i != NumRanges; ++i) 
                    {
                        SourceLocation S = Ranges[i].getBegin(), E = Ranges[i].getEnd();
                        if (S.isMacroID()) S = SM.getImmediateSpellingLoc(S);
                        if (E.isMacroID()) E = SM.getImmediateSpellingLoc(E);
						Ranges[i] = CharSourceRange(SourceRange(S, E), Ranges[i].isTokenRange());
                    }

                    if (true) 
                    {
                        std::pair<FileID, unsigned> IInfo = SM.getDecomposedExpansionLoc(Loc);

                        // Emit the file/line/column that this expansion came from.
                        result += OPS::Strings::format("%s:%d:%d: ", SM.getBuffer(IInfo.first)->getBufferIdentifier(), 
                            SM.getLineNumber(IInfo.first, IInfo.second), SM.getColumnNumber(IInfo.first, IInfo.second));
                    }
                    result += "note: instantiated from:\n";
                    result += EmitCaretDiagnostic(Loc, Ranges, NumRanges, SM, Hints, NumHints, Columns);
                    return result;
                }

                // Decompose the location into a FID/Offset pair.
                std::pair<FileID, unsigned> LocInfo = SM.getDecomposedLoc(Loc);
                FileID FID = LocInfo.first;
                unsigned FileOffset = LocInfo.second;

                // Get information about the buffer it points into.
				llvm::StringRef BufferInfo = SM.getBufferData(FID);
                const char *BufStart = BufferInfo.data();

                unsigned ColNo = SM.getColumnNumber(FID, FileOffset);
                unsigned CaretEndColNo
                    = ColNo + Lexer::MeasureTokenLength(Loc, SM, m_langOpts);

                // Rewind from the current position to the start of the line.
                const char *TokPtr = BufStart+FileOffset;
                const char *LineStart = TokPtr-ColNo+1; // Column # is 1-based.


                // Compute the line end.  Scan forward from the error position to the end of
                // the line.
                const char *LineEnd = TokPtr;
                while (*LineEnd != '\n' && *LineEnd != '\r' && *LineEnd != '\0')
                    ++LineEnd;

                // FIXME: This shouldn't be necessary, but the CaretEndColNo can extend past
                // the source line length as currently being computed. See
                // test/Misc/message-length.c.
                CaretEndColNo = std::min(CaretEndColNo, unsigned(LineEnd - LineStart));

                // Copy the line of code into an std::string for ease of manipulation.
                std::string SourceLine(LineStart, LineEnd);

                // Create a line for the caret that is filled with spaces that is the same
                // length as the line of source code.
                std::string CaretLine(LineEnd-LineStart, ' ');

                // Highlight all of the characters covered by Ranges with ~ characters.
                if (true) 
                {
                    unsigned LineNo = SM.getLineNumber(FID, FileOffset);

                    for (unsigned i = 0, e = NumRanges; i != e; ++i)
                        HighlightRange(Ranges[i], SM, LineNo, FID, CaretLine, SourceLine);
                }

                // Next, insert the caret itself.
                if (ColNo-1 < CaretLine.size())
                    CaretLine[ColNo-1] = '^';
                else
                    CaretLine.push_back('^');

                // Scan the source line, looking for tabs.  If we find any, manually expand
                // them to 8 characters and update the CaretLine to match.
                for (unsigned i = 0; i != SourceLine.size(); ++i) 
                {
                    if (SourceLine[i] != '\t') continue;

                    // Replace this tab with at least one space.
                    SourceLine[i] = ' ';

                    // Compute the number of spaces we need to insert.
                    unsigned NumSpaces = ((i+8)&~7) - (i+1);
                    assert(NumSpaces < 8 && "Invalid computation of space amt");

                    // Insert spaces into the SourceLine.
                    SourceLine.insert(i+1, NumSpaces, ' ');

                    // Insert spaces or ~'s into CaretLine.
                    CaretLine.insert(i+1, NumSpaces, CaretLine[i] == '~' ? '~' : ' ');
                }

                // If we are in -fdiagnostics-print-source-range-info mode, we are trying to
                // produce easily machine parsable output.  Add a space before the source line
                // and the caret to make it trivial to tell the main diagnostic line from what
                // the user is intended to see.
                if (true) 
                {
                    SourceLine = ' ' + SourceLine;
                    CaretLine = ' ' + CaretLine;
                }

                std::string FixItInsertionLine;
                if (true && true) 
                {
					for (const FixItHint *Hint = Hints, *LastHint = Hints + NumHints;
                        Hint != LastHint; ++Hint) 
                    {
						if (Hint->RemoveRange.getBegin().isValid()) 
                        {
                            // We have an insertion hint. Determine whether the inserted
                            // code is on the same line as the caret.
                            std::pair<FileID, unsigned> HintLocInfo
                                = SM.getDecomposedExpansionLoc(Hint->RemoveRange.getBegin());
                            if (SM.getLineNumber(HintLocInfo.first, HintLocInfo.second) ==
                                SM.getLineNumber(FID, FileOffset)) 
                            {
                                // Insert the new code into the line just below the code
                                // that the user wrote.
                                unsigned HintColNo
                                    = SM.getColumnNumber(HintLocInfo.first, HintLocInfo.second);
                                unsigned LastColumnModified
                                    = HintColNo - 1 + Hint->CodeToInsert.size();
                                if (LastColumnModified > FixItInsertionLine.size())
                                    FixItInsertionLine.resize(LastColumnModified, ' ');
                                std::copy(Hint->CodeToInsert.begin(), Hint->CodeToInsert.end(),
                                    FixItInsertionLine.begin() + (HintColNo - 1));
                            } 
                            else 
                            {
                                FixItInsertionLine.clear();
                                break;
                            }
                        }
                    }
                }

                // If the source line is too long for our terminal, select only the
                // "interesting" source region within that line.
                if (Columns && SourceLine.size() > Columns)
                    SelectInterestingSourceRegion(SourceLine, CaretLine, FixItInsertionLine,
                    CaretEndColNo, Columns);

                // Finally, remove any blank spaces from the end of CaretLine.
                while (CaretLine[CaretLine.size()-1] == ' ')
                    CaretLine.erase(CaretLine.end()-1);

                // Emit what we have computed.
                result +=  SourceLine + '\n';

                result += CaretLine + '\n';

                if (!FixItInsertionLine.empty()) 
                {
                    result += ' ';
                    result += FixItInsertionLine + '\n';
                }
                return result;
            }

        public:
            RepriseDiagnosticClient(std::list<CompilerResultMessage> &messageList): 
              m_messageList(messageList), LastCaretDiagnosticWasNote(false)
            {
            }

            void HandleDiagnostic(DiagnosticsEngine::Level diagLevel, const Diagnostic &info)
            {
                const CompilerResultMessage::CompilerResultKind resultKind = translateDiagnosticsLevelToReprise(diagLevel);
                std::string message;
                std::string codeSnippet;
                std::string fileName;
                std::pair<unsigned int, unsigned int> location;
                std::string additionalLocationInfo;
                std::string fixItInfo;

                llvm::SmallString<1024> messageStr;
                info.FormatDiagnostic(messageStr);
                message = messageStr.str();
                if (info.getLocation().isValid()) 
                {
					const SourceManager &sourceManager = info.getSourceManager();
                    PresumedLoc pLoc = sourceManager.getPresumedLoc(info.getLocation());
                    unsigned lineNo = pLoc.getLine();

                    // First, if this diagnostic is not in the main file, print out the
                    // "included from" lines.
                    if (m_lastWarningLoc != pLoc.getIncludeLoc()) 
                    {
                        m_lastWarningLoc = pLoc.getIncludeLoc();
                        additionalLocationInfo = OPS::Strings::trim(printIncludeStack(m_lastWarningLoc, sourceManager));
                    }
                    fileName = pLoc.getFilename();
                    location = std::pair<unsigned int, unsigned int>(lineNo, pLoc.getColumn());
                }

                if (info.getLocation().isValid() && ((LastLoc != info.getLocation()) || info.getNumRanges() ||
                    (LastCaretDiagnosticWasNote && diagLevel != DiagnosticsEngine::Note) ||
					info.getNumFixItHints())) 
                {
                        // Cache the LastLoc, it allows us to omit duplicate source/caret spewage.
                        LastLoc = info.getLocation();
                        LastCaretDiagnosticWasNote = (diagLevel == DiagnosticsEngine::Note);

                        // Get the ranges into a local array we can hack on.
                        CharSourceRange Ranges[20];
                        unsigned NumRanges = info.getNumRanges();
                        assert(NumRanges < 20 && "Out of space");
                        for (unsigned i = 0; i != NumRanges; ++i)
							Ranges[i] = info.getRange(i);

						unsigned NumHints = info.getNumFixItHints();
                        for (unsigned idx = 0; idx < NumHints; ++idx) 
                        {
							const FixItHint &Hint = info.getFixItHint(idx);
                            if (Hint.RemoveRange.isValid()) 
                            {
                                assert(NumRanges < 20 && "Out of space");
								Ranges[NumRanges++] = Hint.RemoveRange;
                            }
                        }

						codeSnippet = EmitCaretDiagnostic(LastLoc, Ranges, NumRanges, info.getSourceManager(),
							info.getFixItHints(), info.getNumFixItHints(), 0);
                }

                const OPS::Reprise::CompilerResultMessage compilerResultMessage(resultKind, message, codeSnippet, fileName, location, additionalLocationInfo);
                m_messageList.push_back(compilerResultMessage);
            }

            RepriseDiagnosticClient* clone(DiagnosticsEngine &Diags) const
            {
                return new RepriseDiagnosticClient(*this);
            }
        };

        std::unique_ptr<DiagnosticConsumer> createRepriseDiagnosticConsumer(std::list<CompilerResultMessage> &messageList)
        {
            return std::unique_ptr<DiagnosticConsumer>(new RepriseDiagnosticClient(messageList));
        }
    }
}
