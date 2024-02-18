#pragma once

#include <set>
#include <map>
#include <vector>
#include <exception>

template<typename T>
class MemPool {
public:
	MemPool(unsigned pageNum, unsigned pageSize);

	~MemPool();

	T*							alloc();

	void						free(T*);

private:
	void						realloc();

private:
	typedef T*					Page;

	typedef std::vector<Page>	Pool;

	Pool						_pool;
	
	unsigned					_pageNum;
	
	unsigned					_pageSize;

	unsigned					_cnt;

	std::set<unsigned>			_allocated;
	
	std::map<void*, unsigned>	_idxTable;

};

template<typename T>
MemPool<T>::MemPool(unsigned pageNum, unsigned pageSize)
	: _pageNum(0), _pageSize(pageSize)
{
	for (unsigned idx = 0; idx < pageNum; ++idx) {
		realloc();
	}
}

template<typename T>
MemPool<T>::~MemPool()
{
	for (auto page : _pool) {
		free(page);
	}
}

template<typename T>
T* 
MemPool<T>::alloc()
{
	if (_allocated.size() == _pageSize)
	{
		realloc();
	}

	++_cnt;
	
	for (unsigned idx = 0; idx < _cnt; ++idx) {
		if (_allocated.find(idx) == _allocated.end()) {
			_allocated.insert(idx);
			T* addr = &(_pool[idx / _pageSize][idx % _pageSize]);
			_idxTable[addr] = idx;
			return addr;
		}
	}
	
	throw std::bad_alloc();
}

template<typename T>
void				
MemPool<T>::free(T* ptr)
{
	--_cnt;
	_allocated.erase(_idxTable[ptr]);
	_idxTable.erase(ptr);
}

template<typename T>
void
MemPool<T>::realloc()
{
	Page page = (Page)calloc(_pageSize, sizeof(T));
	_pool.push_back(page);
	++_pageNum;
}