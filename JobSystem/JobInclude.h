#pragma once
#include "JobSystem.h"
#include "JobBucket.h"
#include "JobNode.h"

template <typename Func>
constexpr JobHandle JobBucket::GetTask(const Func& func)
{
	JobNode* node = sys->jobNodePool.New();
	jobNodesVec.emplace_back(node);
	node->Create<Func>(func, &sys->vectorPool, &sys->threadMtx);
	JobHandle retValue(node);
	return retValue;
}


template <typename Func>
constexpr void JobNode::Create(const Func& func, VectorPool* vectorPool, std::mutex* threadMtx)
{
	this->threadMtx = threadMtx;
	this->vectorPool = vectorPool;
	dependingEvent = vectorPool->New();
	if (sizeof(Func) >= sizeof(FuncStorage))	//Create in heap
	{
		assert(false);
		ptr = new Func{
			std::move((Func&)func)
		};
		destructorFunc = [](void* currPtr)->void
		{
			delete ((Func*)currPtr);
		};
	}
	else
	{
		ptr = &stackArr;
		new (ptr)Func{
			std::move((Func&)func)
		};
		destructorFunc = [](void* currPtr)->void
		{
			((Func*)currPtr)->~Func();
		};
	}
	executeFunc = [](void* currPtr)->void
	{
		(*(Func*)currPtr)();
	};
}