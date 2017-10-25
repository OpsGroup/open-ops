#ifndef PARALLELLOOPS_ANALYSERS_H
#define PARALLELLOOPS_ANALYSERS_H

#include "Analysis/ParallelLoops.h"
#include "Analysis/AbstractDepGraph.h"

namespace OPS
{
namespace Analysis
{
namespace ParallelLoops
{

class LoopAnalyserBase : public OPS::NonCopyableMix
{
public:
	LoopAnalyserBase(Target target, AbstractDepGraph& depGraph);
	virtual ~LoopAnalyserBase() {}

	std::tr1::shared_ptr<TargetParallelInfo> analyse(const LoopParallelInfo& loop);

protected:
	virtual void analyseImpl(const LoopParallelInfo& loop, TargetParallelInfo& targetInfo) = 0;

	Target m_target;
	AbstractDepGraph& m_depGraph;
};

class MISMAnalyser : public LoopAnalyserBase
{
public:
	MISMAnalyser(AbstractDepGraph& depGraph);
	void analyseImpl(const LoopParallelInfo &loop, TargetParallelInfo &targetInfo);

private:
	void recognizeReduction(const LoopParallelInfo& loop, AbstractDepGraph& subGraph, TargetParallelInfo& targetInfo);

public:
	static void makePrivatization(const LoopParallelInfo& loop, AbstractDepGraph& subGraph, TargetParallelInfo& targetInfo);
};

class MIDMAnalyser : public LoopAnalyserBase
{
public:
	MIDMAnalyser(AbstractDepGraph& depGraph);
	void analyseImpl(const LoopParallelInfo &loop, TargetParallelInfo &targetInfo);
};

class SIMDAnalyser : public LoopAnalyserBase
{
public:
	enum MemoryKind
	{
		SharedMemory,
		DistributedMemory,
	};
	SIMDAnalyser(AbstractDepGraph& depGraph, MemoryKind memKind);
	void analyseImpl(const LoopParallelInfo &loop, TargetParallelInfo &targetInfo);

private:
	MemoryKind m_memKind;
};

}
}
}

#endif // PARALLELLOOPS_ANALYSERS_H
