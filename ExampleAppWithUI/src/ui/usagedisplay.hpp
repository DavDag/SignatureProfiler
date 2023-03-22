#pragma once

class App;

class UsageDisplay {
public:
	UsageDisplay(App& app, float updatePerSec);
	//
	void initialize();
	void update(double dtSec);
	void ui(int w, int h);

private:
	void __update();

private:
	App& _app;
	//
	double _timeFromLastUpdateSec;
	double _updateIntervalSec;
	int _fpsAccumulator;
	//
	int _processId;
	int _runningCoreInd;
	double _fps;
	double _memUsagePercentage;
	double _memUsageMb;
	double _memAvailableMb;
	double _memPhysicalMb;
	double _gpuUsagePercentage;
	double _gpuMemUsageMb;
	double _gpuMemAvailableMb;
	double _gpuMemPhysicalMb;
	double _cpuUsagePercentageAllCores;
	int _coreCount;
	double* _cpuUsagePercentagePerCore;
};
