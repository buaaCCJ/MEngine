#include "JobSystem.h"
#include <thread>
#include "ConcurrentQueue.h"
#include <condition_variable>
#include <atomic>
#include "JobBucket.h"
#include "JobNode.h"
void JobSystem::UpdateNewBucket()
{
START:
	if (currentBucketPos >= buckets.size())
	{
		{
			mainThreadFinished = true;
			std::lock_guard<std::mutex> lck(mainThreadWaitMutex);
			mainThreadFinished = true;
			mainThreadWaitCV.notify_all();
		}
		return;
	}

	JobBucket* bucket = buckets[currentBucketPos];
	if (bucket->jobNodesVec.empty() || bucket->sys != this)
	{
		currentBucketPos++;
		goto START;
	}
	bucketMissionCount = bucket->jobNodesVec.size();
	executingNode.ResizeAndClear(bucket->jobNodesVec.size());
	for (int i = 0; i < bucket->jobNodesVec.size(); ++i)
	{
		JobNode* node = bucket->jobNodesVec[i];
		if (node->targetDepending == 0)
		{
			executingNode.Push(node);

		}
	}
	bucket->jobNodesVec.clear();
	currentBucketPos++;
	UINT size = executingNode.GetSize();
	if (executingNode.GetSize() < mThreadCount) {
		for (int64_t i = 0; i < executingNode.GetSize(); ++i) {
			std::lock_guard<std::mutex> lck(threadMtx);
			cv.notify_one();
		}
	}
	else
	{
		std::lock_guard<std::mutex> lck(threadMtx);
		cv.notify_all();
	}
}

class JobThreadRunnable
{
public:
	JobSystem* sys;
	/*bool* JobSystemInitialized;
	std::condition_variable* cv;
	ConcurrentQueue<JobNode*>* executingNode;
	std::atomic<int>* bucketMissionCount;*/
	void operator()()
	{
		int value = (int)-1;
		while (sys->JobSystemInitialized)
		{
			{
				std::unique_lock<std::mutex> lck(sys->threadMtx);
				sys->cv.wait(lck);
			}
			JobNode* node = nullptr;
			while (sys->executingNode.TryPop(&node))
			{
			START_LOOP:
				JobNode* nextNode = node->Execute(sys->executingNode, sys->cv);
				sys->jobNodePool.Delete(node);
				value = sys->bucketMissionCount.fetch_add(-1) - 1;
				if (nextNode != nullptr)
				{
					node = nextNode;
					goto START_LOOP;
				}
				if (value <= 0)
				{
					sys->UpdateNewBucket();
				}
			}

		}
	}
};

JobBucket* JobSystem::GetJobBucket()
{
	if (releasedBuckets.empty())
	{
		JobBucket* bucket = new JobBucket(this);
		usedBuckets.push_back(bucket);
		return bucket;
	}
	else
	{
		auto&& ite = releasedBuckets.end() - 1;
		JobBucket* cur = *ite;
		cur->jobNodesVec.clear();
		releasedBuckets.erase(ite);
		return cur;
	}
}
void JobSystem::ReleaseJobBucket(JobBucket* node)
{
	node->jobNodesVec.clear();
	releasedBuckets.push_back(node);
}

JobSystem::JobSystem(int threadCount)  noexcept :
	executingNode(100),
	mainThreadFinished(true),
	jobNodePool(100),
	vectorPool(100)
{
	mThreadCount = threadCount;
	usedBuckets.reserve(20);
	releasedBuckets.reserve(20);
	allThreads.resize(threadCount);
	for (int i = 0; i < threadCount; ++i)
	{
		JobThreadRunnable j;
		j.sys = this;
		allThreads[i] = new std::thread(j);
	}
}

JobSystem::~JobSystem() noexcept
{
	JobSystemInitialized = false;
	{
		std::lock_guard<std::mutex> lck(threadMtx);
		cv.notify_all();
	}
	for (int i = 0; i < allThreads.size(); ++i)
	{
		allThreads[i]->join();
		delete allThreads[i];
	}
	for (auto ite = usedBuckets.begin(); ite != usedBuckets.end(); ++ite)
	{
		delete *ite;
	}
}

void JobSystem::ExecuteBucket(JobBucket** bucket, int bucketCount)
{
	jobNodePool.UpdateSwitcher();
	vectorPool.UpdateSwitcher();
	currentBucketPos = 0;
	buckets.resize(bucketCount);
	memcpy(buckets.data(), bucket, sizeof(JobBucket*) * bucketCount);
	mainThreadFinished = false;
	UpdateNewBucket();
}
void JobSystem::ExecuteBucket(JobBucket* bucket, int bucketCount)
{
	jobNodePool.UpdateSwitcher();
	vectorPool.UpdateSwitcher();
	currentBucketPos = 0;
	buckets.resize(bucketCount);
	for (int i = 0; i < bucketCount; ++i)
	{
		buckets[i] = bucket + i;
	}
	mainThreadFinished = false;
	UpdateNewBucket();
}

void JobSystem::Wait()
{
	std::unique_lock<std::mutex> lck(mainThreadWaitMutex);
	while (!mainThreadFinished)
	{
		mainThreadWaitCV.wait(lck);
	}
}



void VectorPool::UpdateSwitcher()
{
	if (unusedObjects[objectSwitcher].count < 0) unusedObjects[objectSwitcher].count = 0;
	objectSwitcher = !objectSwitcher;
}

void VectorPool::Delete(std::vector<JobNode*>* targetPtr)
{
	Array* arr = unusedObjects + !objectSwitcher;
	int64_t currentCount = arr->count++;
	if (currentCount >= arr->capacity)
	{
		std::lock_guard<std::mutex> lck(mtx);
		//			lock
		if (currentCount >= arr->capacity)
		{
			int64_t newCapacity = arr->capacity * 2;
			std::vector<JobNode*>** newArray = new std::vector<JobNode*>*[newCapacity];
			memcpy(newArray, arr->objs, sizeof(std::vector<JobNode*>*) * arr->capacity);
			delete arr->objs;
			arr->objs = newArray;
			arr->capacity = newCapacity;
		}
	}
	arr->objs[currentCount] = targetPtr;
}

std::vector<JobNode*>* VectorPool::New()
{
	Array* arr = unusedObjects + objectSwitcher;
	int64_t currentCount = --arr->count;
	std::vector<JobNode*>* t;
	if (currentCount >= 0)
	{
		t = (std::vector<JobNode*>*)arr->objs[currentCount];

	}
	else
	{
		t = new std::vector<JobNode*>;
		t->reserve(20);
	}

	return t;
}

VectorPool::VectorPool(unsigned int initCapacity)
{
	if (initCapacity < 3) initCapacity = 3;
	unusedObjects[0].objs = new std::vector<JobNode*>*[initCapacity];
	unusedObjects[0].capacity = initCapacity;
	unusedObjects[0].count = initCapacity / 2;
	for (unsigned int i = 0; i < unusedObjects[0].count; ++i)
	{
		unusedObjects[0].objs[i] = new std::vector<JobNode*>;// (StorageT*)malloc(sizeof(StorageT));
		unusedObjects[0].objs[i]->reserve(20);
	}
	unusedObjects[1].objs = new std::vector<JobNode*>*[initCapacity];
	unusedObjects[1].capacity = initCapacity;
	unusedObjects[1].count = initCapacity / 2;
	for (unsigned int i = 0; i < unusedObjects[1].count; ++i)
	{
		unusedObjects[1].objs[i] = new std::vector<JobNode*>;
		unusedObjects[1].objs[i]->reserve(20);
	}
}
VectorPool::~VectorPool()
{
	for (int64_t i = 0; i < unusedObjects[0].count; ++i)
	{
		delete unusedObjects[0].objs[i];

	}
	delete unusedObjects[0].objs;
	for (int64_t i = 0; i < unusedObjects[1].count; ++i)
	{
		delete unusedObjects[1].objs[i];

	}
	delete unusedObjects[1].objs;
}