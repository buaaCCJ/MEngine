#pragma once
class JobNode;
class JobBucket;
class JobHandle
{
	friend class JobBucket;
private:
	JobNode* node;
	JobHandle(JobNode* otherNode);
public:
	JobHandle();
	JobHandle(const JobHandle& other);
	JobHandle& operator=(const JobHandle& other);
	void Precede(JobHandle& other);
};