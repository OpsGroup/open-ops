/// R2Clang_main.cpp
/// Создано: 25.02.2013

#include "Reprise/Reprise.h"
#include "Frontend/Frontend.h"

#include "Backends/OutToC/OutToC.h"
#include "Backends/RepriseToClang/ClangGenerator.h"

#include "clang/AST/ASTContext.h"
#include "clang/Basic/TargetOptions.h"

#include "llvm/Support/raw_ostream.h"

#include <iostream>

int main(int nArgC, char* apszArgV[])
{
	using namespace OPS::TransformationsHub;
	using namespace OPS::Reprise;
	using namespace OPS::Frontend;
	using namespace OPS::Backends::Clang;

	using namespace std;

	cout << "RepriseToClang tester" << endl;

	// Обработка командной строки

	if (nArgC != 2)
	{
		cout <<
			"The program requires one parameter:\n"
			"  - the input file name (C);\n"
			//"  - the first output file name (Clang) - optional;\n"
			<< endl;

		return 1;
	}

	// Компиляция C во внутреннее представление

	Frontend frontend;
	frontend.addSourceFile(apszArgV[1]);
	frontend.compile();
	const CompileResult& rcResult = frontend.getResult(0);

	// Обработка результата компиляции

	if (0 == rcResult.errorCount())
	{
		// Обработка внутреннего представления

		cout << "Compilation successfull" << endl;
		OPS::Backends::OutToC writer(cout);
		writer.visit(frontend.getProgramUnit().getUnit(0));
		cout << "\n" << endl;

		// Генерация кода

		cout << "Code generation started..." << endl;
		clang::IntrusiveRefCntPtr <clang::TargetOptions> ptrTargetOptions(
			new clang::TargetOptions);
		ptrTargetOptions->Triple = "i386-pc-win32-eabi";
		Generator generator(*ptrTargetOptions);
		ArgumentValues argValues;
		generator.makeTransform(&(frontend.getProgramUnit()), argValues);

		// Вывод результатов

		cout << "Code generation finished.\n" << endl;

		// OutToC writerRes(cout);
		// writerRes.visit(frontend.getProgramUnit().getUnit(0));

		const ASTContexts& rcContexts =
			generator.getResult();

		// Имя файла пусто?

		//if (i1->first.empty())
		{
			// Вывод на экран

			llvm::errs() <<
				"\n========================================\n\n";

			ASTContexts::const_iterator
				i = rcContexts.begin(),
				e = rcContexts.end();
			for (; i != e; ++ i)
			{
				(*i)->getTranslationUnitDecl()->dump();

				llvm::errs() <<
					"\n----------------------------------------\n\n";
			}
		}
		//else
		//{
			// Сохранение в файл

		//	ofstream ofs(
		//		i1->first.c_str(),
		//		ios_base::out | ios_base::trunc);
		//	if (ofs)
		//	{
		//		ofs << i1->second;
		//		cout << "File saved: " << i1->first << endl;
		//	}
		//}    // if (i1->first.empty()) (else)
	}
	else    // if (0 == rcResult.errorCount())
	{
		// Вывод ошибок

		cout <<
			"Compilation errors: " <<
			endl <<
			rcResult.errorText() <<
			endl;

	}    // if (0 == rcResult.errorCount()) (else)

	llvm::errs() <<
		"\nTetsmod for RepriseToClang generator finished.\n";
	llvm::errs().flush();

}    // main()

// Конец файла
