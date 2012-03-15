#include "pch.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include "gtest/gtest.h"
#include "LogCommon/ScanningSections.hpp"

using namespace Instalog;

struct RunningProcessesTest : public testing::Test
{
	RunningProcesses rp;
	std::wostringstream ss;
	std::unique_ptr<IUserInterface> ui;
	ScriptSection section;
	std::vector<std::wstring> options;
	virtual void SetUp()
	{
		ui.reset(new DoNothingUserInterface);
	}
	void Go()
	{
		rp.Execute(ss, ui.get(), section, options);
	}
};

TEST_F(RunningProcessesTest, NameIsRunningProcesses)
{
	ASSERT_EQ(L"Running Processes", rp.GetName());
}

TEST_F(RunningProcessesTest, PriorityIsScanning)
{
	ASSERT_EQ(SCANNING, rp.GetPriority());
}

TEST_F(RunningProcessesTest, SvchostHasFullLine)
{
	std::wstring svcHost = L"C:\\Windows\\system32\\svchost.exe -k netsvcs\n";
	Go();
	ASSERT_PRED2((boost::algorithm::contains<std::wstring, std::wstring>), ss.str(), svcHost);
}

TEST_F(RunningProcessesTest, ExplorerDoesNotHaveFullLine)
{
	std::wstring svcHost = L"C:\\Windows\\Explorer.EXE\n";
	Go();
	ASSERT_PRED2((boost::algorithm::contains<std::wstring, std::wstring>), ss.str(), svcHost);
}