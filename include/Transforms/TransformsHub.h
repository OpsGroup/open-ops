#ifndef _TRANSFORMSHUB_H_
#define _TRANSFORMSHUB_H_

#include <memory>
#include "Transforms/TransformArgs.h"

namespace OPS
{
namespace TransformationsHub
{
	class TransformBase
	{
	public:
		/// Возвращает описание параметров преобразования
		const ArgumentsInfo& getArgumentsInfo() const;

        /// Проверяет, применимо ли преобразование к данным аргументам
        virtual bool isApplicable(OPS::Reprise::ProgramUnit* program,
                                  const ArgumentValues& args,
                                  std::string* message = 0) { return true; }

		/// Выполнение преобразования
        void makeTransform(OPS::Reprise::ProgramUnit* program, const ArgumentValues& params);

		virtual ~TransformBase() { }

	protected:

        virtual void makeTransformImpl(OPS::Reprise::ProgramUnit* program, const ArgumentValues& params) = 0;

		ArgumentsInfo	m_argumentsInfo;
	};

	class InformerBase : public TransformBase
	{
	public:

		/// Возвращает информационную строку
		virtual const std::string& getResult() const;

	protected:
		std::string m_result;
	};

	TransformBase* createExprSimplifier();
	TransformBase* createSubstForw();

	class TransformFactory
	{
	public:
		typedef TransformBase* (TransformFactoryFunc)(void);

		struct TransformDescription
		{
			std::string 			category;
			std::string				samplesSuffix;
			TransformFactoryFunc*	factory;

            TransformDescription():factory(0) {}
			TransformDescription(const std::string& category,
								 TransformFactoryFunc,
								 const std::string& samplesSuffix = std::string());
		};

	public:
		/// Получение 
		static TransformFactory& instance();

		void registerTransform(const std::string& name, const TransformDescription& desc);

		/// Создание экземпляра преобразования по имени
		std::unique_ptr<TransformBase> create(const std::string& name) const;

		/// Возвращает категорию преобразования по имени
		std::string getCategory(const std::string& name) const;

		std::string getSamplesSuffix(const std::string& name) const;

		/// Возвращает список имен всех зарегистрированных преобразований
		std::vector<std::string> getNames() const;

		/// Возвращает список всех категорий преобразований
		std::list<std::string> getCategories() const;

	private:

		TransformFactory() {}

		typedef std::map<std::string, TransformDescription> TransformRegistry;

		TransformRegistry	m_transforms;
	};
}
}

#endif
