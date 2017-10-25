#ifndef FRAME_H_INCLUDED
#define FRAME_H_INCLUDED

#include <deque>

#include "Reprise/Reprise.h"

namespace OPS 
{
	namespace Analysis 
	{
		namespace Frames 
		{
            using OPS::Reprise::ForStatement;
			using OPS::Reprise::StatementBase;

			struct Frame
			{
				typedef std::deque<StatementBase*> TNodes;

				// Содержимое кадра
				TNodes m_Nodes;
				
				// Число от 0.0 до 1.0, характеризующее объем используемых ресурсов
				// 0.0 - ресурсы полностью свободны
				// 1.0 - ресурсы полностью заняты
				double m_resourceUsage;

				virtual ~Frame() {}
			};

			struct HeaderFrame: public Frame
			{
			};

			struct PartitionedByIterationsFrame: public Frame
			{
                std::map<ForStatement*, int> m_partitionedNestIterationsCount;
			};
		}
	}
}
#endif // FRAME_H_INCLUDED
