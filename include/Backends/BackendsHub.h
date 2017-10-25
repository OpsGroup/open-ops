#ifndef BACKENDSHUB_H
#define BACKENDSHUB_H

#include "Transforms/TransformsHub.h"

namespace OPS
{
namespace TransformationsHub
{

	class CodeGeneratorBase : public TransformBase
	{
	public:
		typedef std::map<std::string, std::string> ProgramCode;

		/// Возвращает отображение вида: <имя файла, сгенерированный код>
		virtual const ProgramCode& getGeneratedCode() const;

	protected:
		void setGeneratedCode(OPS::Reprise::ProgramUnit& program);

		ProgramCode	m_generatedCode;
	};

	void registerBackends();

}
}

#endif // BACKENDSHUB_H
