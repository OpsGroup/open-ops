/// ClangGenerator.h
/// Создано: 19.02.2013

#ifndef CLANG_GENERATOR_H__
#define CLANG_GENERATOR_H__

#include "Backends/BackendsHub.h"

#include "ClangGeneratorTypes.h"

#include <vector>

namespace OPS
{
	namespace Backends
	{
		namespace Clang
		{
			class Generator : public OPS::TransformationsHub::CodeGeneratorBase
			{
			public:
				//
				explicit Generator(
					clang::TargetOptions& rTargetOptions);
				explicit Generator(
					clang::CompilerInstance& rCompilerInstance);
				//
			public:
				//
				const ASTContexts& getResult() const;
				//
			public:
				//
				virtual void makeTransformImpl(
					OPS::Reprise::ProgramUnit* pProgramUnit,
					const OPS::TransformationsHub::ArgumentValues& rcParams);
				//
			private:
				//
				clang::CompilerInstance* m_pCompilerInstance;
				Internal::ASTContextParams m_ASTContextParams;
				ASTContexts m_Contexts;
			};
		}    // namespace Clang
	}    // namespace Backends
}    // namespace OPS

#endif    // CLANG_GENERATOR_H__

// Конец файла
