#include "JobBucket.h"
#include "JobNode.h"
#include "JobSystem.h"
JobBucket::JobBucket(JobSystem* sys) noexcept : 
	sys(sys)
{
	jobNodesVec.reserve(20);
}