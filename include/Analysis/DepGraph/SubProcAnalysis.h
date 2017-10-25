/**
   Функции относящиеся к анализу межпроцедурных зависимостей.
**/
#pragma once

#include "Reprise/Reprise.h"
#include "Reprise/Service/Marker.h"
	
namespace DepGraph
{
	using namespace OPS::Reprise;
	using namespace OPS::Reprise::Service;
	/// Информация о подпрограмме и ее параметрах
	class SubProcInfo //: public OPS::Reprise::RepriseBase
	{
	public:
		
		/// Флаги говоряще об использовании параметров в подпрограмме
		enum ParamAccesType
		{
			/// Параметр читается
			AT_READ = 1,
			/// Параметр записывается
			AT_WRITE = 2
		};

		/// Флаги побочных эффектов
		enum SideEffect
		{
			/// Подпрограмма использует глобальные переменные
			SE_GLOBAL_VAR = 1,
			/// Подпрограмма изменяет передаваемы ей параметры
			SE_WRITE_PARAM = 2,
			/// Подпрограмма вызывает другую подпрограмму с побочными эффектами
			SE_CALL = 4
		};

		typedef std::map<const OPS::Reprise::ParameterDescriptor*, int> ParamAccessList;
		/// Для каждого параметра хранится его тип доступа
		ParamAccessList		m_paramAccessList;

		/// Маска побочных эффектов подпрограммы
		int		m_sideEffects;

		SubProcInfo(const OPS::Reprise::SubroutineDeclaration& subProc);
	};

	class SubroutineMarker
	{
	public:

		~SubroutineMarker()
		{
			clear();
		}
	
		/**
		Clear all marks.
		*/
		void clear(void)
		{
			TMarkedNodes::iterator it = m_marks.begin();
			for(; it != m_marks.end(); ++it)
				delete it->second;

			m_marks.clear();
		}

				
		/**
		Checks that node has at least one mark
		\param	node - Reprise node to check marks of
		\return at least one mark is present
		*/
		bool hasMarks(const RepriseBase& node) const
		{
			return m_marks.find(&node) != m_marks.end();
		}

		/**
		Adds mark to specified node.
		\param	node - Reprise node to add mark to
		\param	kind - mark kind to add to node
		\return	true if added, otherwise - mark already present
		*/
		bool addMark(const RepriseBase* node, SubProcInfo* info)
		{
			bool b = m_marks.find(node) != m_marks.end();
			m_marks[node] = info;
			return !b;
		}
		
		/**
		Marks getter.
		\param	node - Reprise node to get marks storage for
		\return	marks storage for node
		*/
		SubProcInfo* getMark(const RepriseBase& node)
		{
			return m_marks[&node];
		}
		
	private:
		typedef std::map<const RepriseBase*, SubProcInfo*> TMarkedNodes;
		TMarkedNodes m_marks;
	};
	//SubroutineMarker subProcInfoMarker;

	
	/// Получить информацию о подпрограмме
	SubProcInfo getSubProcInfo(OPS::Reprise::SubroutineDeclaration& subProc, bool forceUpdate = false);
}
