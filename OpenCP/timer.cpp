#include "timer.hpp"

using namespace std;
using namespace cv;

namespace cp
{

	void CalcTime::start()
	{
		pre = getTickCount();
	}

	void CalcTime::restart()
	{
		start();
	}
	
	int CalcTime::autoTimeMode()
	{
		if (cTime > 60.0*60.0*24.0)
		{
			return TIME_DAY;
		}
		else if (cTime > 60.0*60.0)
		{
			return TIME_HOUR;
		}
		else if (cTime > 60.0)
		{
			return TIME_MIN;
		}
		else if (cTime > 1.0)
		{
			return TIME_SEC;
		}
		else if (cTime > 1.0 / 1000.0)
		{
			return TIME_MSEC;
		}
		else
		{
			return TIME_NSEC;
		}
	}

	void CalcTime::convertTime(bool isPrint, std::string message)
	{
		int mode = timeMode;
		if (mode == TIME_AUTO)
		{
			mode = autoTimeMode();
			autoMode = mode;
		}

		switch (mode)
		{
		case TIME_NSEC:
			cTime *= 1000000.0;
			break;
		case TIME_SEC:
			cTime *= 1.0;
			break;
		case TIME_MIN:
			cTime /= (60.0);
			break;
		case TIME_HOUR:
			cTime /= (60 * 60);
			break;
		case TIME_DAY:
			cTime /= (60 * 60 * 24);
			break;
		case TIME_MSEC:
		default:
			cTime *= 1000.0;
			break;
		}

		if (isPrint)
		{
			if (message == "")message = mes;
			switch (mode)
			{
			case TIME_NSEC:
				cout << message << ": " << cTime << " nsec" << endl;
				break;
			case TIME_SEC:
				cout << message << ": " << cTime << " sec" << endl;
				break;
			case TIME_MIN:
				cout << message << ": " << cTime << " minute" << endl;
				break;
			case TIME_HOUR:
				cout << message << ": " << cTime << " hour" << endl;
				break;
			case TIME_DAY:
				cout << message << ": " << cTime << " day" << endl;
				break;
			case TIME_MSEC:
				cout << message << ": " << cTime << " msec" << endl;
				break;
			default:
				cout << message << ": error" << endl;
				break;
			}
		}
	}

	double CalcTime::getTime(bool isPrint, std::string message)
	{
		cTime = (getTickCount() - pre) / (getTickFrequency());
		convertTime(isPrint, message);
		return cTime;
	}

	double CalcTime::getLapTime(bool isPrint, string message)
	{
		double time = (getTickCount() - pre) / (getTickFrequency());
		stat.push_back(time);

		cTime = time;
		convertTime(isPrint, message);

		restart();
		return cTime;
	}

	double CalcTime::getLapTimeMedian(bool isPrint, string message)
	{
		cTime = stat.getMedian();
		convertTime(isPrint, message);
		return cTime;
	}

	double CalcTime::getLapTimeMean(bool isPrint, string message)
	{
		cTime = stat.getMean();
		convertTime(isPrint, message);
		return cTime;
	}

	void CalcTime::setMessage(string& src)
	{
		mes = src;
	}

	void CalcTime::setMode(int mode)
	{
		timeMode = mode;
	}

	void CalcTime::init(string message, int mode, bool isShow)
	{
		_isShow = isShow;
		timeMode = mode;

		setMessage(message);
		start();
	}

	CalcTime::CalcTime()
	{
		string t = "time ";
		init(t, TIME_AUTO, true);
	}

	CalcTime::CalcTime(char* message, int mode, bool isShow)
	{
		string m = message;
		init(m, mode, isShow);
	}

	CalcTime::CalcTime(string message, int mode, bool isShow)
	{
		init(message, mode, isShow);
	}

	CalcTime::~CalcTime()
	{
		if (_isShow)	getTime(true); 
	}

	void DestinationTimePrediction::init(int DestinationCount)
	{
		pCount = 0;
		destCount = DestinationCount;
		startTime = getTickCount();

		prestamp_for_prediction = startTime;
		prestamp = startTime;

		firstprediction = 0;
	}

	DestinationTimePrediction::DestinationTimePrediction()
	{
		;
	}

	DestinationTimePrediction::DestinationTimePrediction(int DestinationCount)
	{
		init(DestinationCount);
	}

	int DestinationTimePrediction::autoTimeMode(double cTime)
	{
		if (cTime > 60.0*60.0*24.0)
		{
			return TIME_DAY;
		}
		else if (cTime > 60.0*60.0)
		{
			return TIME_HOUR;
		}
		else if (cTime > 60.0)
		{
			return TIME_MIN;
		}
		else
		{
			return TIME_SEC;
		}
	}

	void DestinationTimePrediction::tick2Time(double tick, string mes)
	{
		double cTime = tick / (getTickFrequency());

		int timeMode = autoTimeMode(cTime);

		switch (timeMode)
		{
		case TIME_MSEC:
			cTime *= 1000.0;
			break;
		case TIME_SEC:
		default:
			cTime *= 1.0;
			break;
		case TIME_MIN:
			cTime /= (60.0);
			break;
		case TIME_HOUR:
			cTime /= (60 * 60);
			break;
		case TIME_DAY:
			cTime /= (60 * 60 * 24);
			break;
		}

		switch (timeMode)
		{
		case TIME_SEC:
		default:
			cout << mes << ": " << format("%.2f", cTime) << " sec                 ";
			break;
		case TIME_MIN:
			cout << mes << ": " << format("%.2f", cTime) << " minute              ";
			break;
		case TIME_HOUR:
			cout << mes << ": " << format("%.2f", cTime) << " hour                ";
			break;
		case TIME_DAY:
			cout << mes << ": " << format("%.2f", cTime) << " day                 ";
			break;
		case TIME_MSEC:
			cout << mes << ": " << format("%.2f", cTime) << " msec" << endl;
			break;
		}
	}

	int64 DestinationTimePrediction::getTime(string mes)
	{
		int64 ret = (getTickCount() - startTime);
		double cTime = ret / (getTickFrequency());

		int timeMode = autoTimeMode(cTime);

		switch (timeMode)
		{
		case TIME_MSEC:
			cTime *= 1000.0;
			break;
		case TIME_SEC:
		default:
			cTime *= 1.0;
			break;
		case TIME_MIN:
			cTime /= (60.0);
			break;
		case TIME_HOUR:
			cTime /= (60 * 60);
			break;
		case TIME_DAY:
			cTime /= (60 * 60 * 24);
			break;
		}

		switch (timeMode)
		{
		case TIME_SEC:
		default:
			cout << mes << ": " << format("%.2f", cTime) << " sec                 ";
			break;
		case TIME_MIN:
			cout << mes << ": " << format("%.2f", cTime) << " minute              ";
			break;
		case TIME_HOUR:
			cout << mes << ": " << format("%.2f", cTime) << " hour                ";
			break;
		case TIME_DAY:
			cout << mes << ": " << format("%.2f", cTime) << " day                 ";
			break;
		case TIME_MSEC:
			cout << mes << ": " << format("%.2f", cTime) << " msec" << endl;
			break;
		}
		return ret;
	}

	DestinationTimePrediction::~DestinationTimePrediction()
	{
		int64 a = getTime("actual ");
		tick2Time((double)(firstprediction - a), "diff from 1st prediction ");
	}

	void DestinationTimePrediction::predict()
	{
		pCount++;
		if (pCount < 11)
		{
			int64 v = (int64)predict(pCount, 10);
			firstprediction = (v > 0) ? v : firstprediction;
		}
		else
			predict(pCount);
	}
	double DestinationTimePrediction::predict(int presentCount, int interval)
	{
		double ret = 0.0;
		if ((presentCount% interval) == 0)
		{
			double per = (double)presentCount / destCount;
			//double ctime = (getTickCount()-preTime)/(getTickFrequency());
			int64 cstamp = getTickCount();
			double pret = ((double)destCount / (double)interval) * (cstamp - prestamp_for_prediction);

			pret = (destCount - presentCount) / (double)interval*(cstamp - prestamp_for_prediction);
			ret = pret;
			prestamp_for_prediction = cstamp;

			double cTime = pret / (getTickFrequency());

			int timeMode = autoTimeMode(cTime);

			switch (timeMode)
			{
			case TIME_SEC:
			default:
				cTime *= 1.0;
				break;
			case TIME_MIN:
				cTime /= (60.0);
				break;
			case TIME_HOUR:
				cTime /= (60 * 60);
				break;
			case TIME_DAY:
				cTime /= (60 * 60 * 24);
				break;
			}

			cout << "\r";

			string mes = format("%.3f %% computed, rest ", 100.0*per);

			switch (timeMode)
			{
			case TIME_SEC:
			default:
				cout << mes << ": " << format("%.2f", cTime) << " sec                 ";
				break;
			case TIME_MIN:
				cout << mes << ": " << format("%.2f", cTime) << " minute              ";
				break;
			case TIME_HOUR:
				cout << mes << ": " << format("%.2f", cTime) << " hour                ";
				break;
			case TIME_DAY:
				cout << mes << ": " << format("%.2f", cTime) << " day                 ";
				break;
			}

			prestamp = cstamp;
		}
		return ret;
	}
}