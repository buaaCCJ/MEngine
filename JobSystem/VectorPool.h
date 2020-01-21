#pragma once
#include <mutex>
#include <vector>
#include <atomic>
class JobNode;
class VectorPool
{
private:
	struct Array
	{
		std::vector<JobNode*>** objs;
		std::atomic<int64_t> count;
		int64_t capacity;
	};

	Array unusedObjects[2];
	std::mutex mtx;
	bool objectSwitcher = true;
public:
	void UpdateSwitcher();
	void Delete(std::vector<JobNode*>* targetPtr);

	std::vector<JobNode*>* New();

	VectorPool(unsigned int initCapacity);
	~VectorPool();
};