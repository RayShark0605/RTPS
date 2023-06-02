#pragma once
#undef slots
#include <cmath>
#include <Python.h>
#define slots Q_SLOTS
#include <numpy/arrayobject.h>

#include "Base.h"
#include "Database.h"

class CFeatureMatcher
{
public:
	virtual bool Match(std::string ImagePath) = 0;
protected:
	colmap::OptionManager* options;
	void SiftMatch(size_t ImageID, std::vector<size_t>& MatchedImageIDs, size_t& TimeConsuming_MS);
};

class CExhaustiveMatcher :public CFeatureMatcher
{
public:
	CExhaustiveMatcher(colmap::OptionManager* options);
	bool Match(std::string ImagePath);
};
class CRetrievalMatcher :public CFeatureMatcher
{
public:
	CRetrievalMatcher(colmap::OptionManager* options);
	~CRetrievalMatcher();
	void Uninstall();
	bool Match(std::string ImagePath);
private:
	std::vector<std::string> Database;

	CExhaustiveMatcher* ExhaustiveMatcher = nullptr;

	PyObject* MatchFunc;
	PyObject* OutputFunc;
	size_t RetrievalDatabaseNum;
	PyGILState_STATE state;

	bool Initialize();
	
};


