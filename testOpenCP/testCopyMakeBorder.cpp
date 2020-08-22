#include <opencp.hpp>

using namespace std;
using namespace cv;
using namespace cp;

void copyMakeBorderTest(Mat& src)
{
	//cv::setNumThreads(1);
	//cv::ipp::setUseIPP(true);
	//cv::setUseOptimized(false);

	const int r = 30;
	const int iteration = 100000;
	print_debug2(r, iteration);
	const int borderType = BORDER_REPLICATE;

	CV_Assert(src.channels() == 3);
	Mat gray;
	cvtColor(src, gray, COLOR_BGR2GRAY);

	Mat src32fc1 = convert(gray, CV_32F);
	Mat src32fc3 = convert(src, CV_32F);

	bool isGrayTest = false;
	bool isColorTest = true;

	if(isGrayTest)
	{
		Mat dstcv8u;
		Mat dstcp8u;
		Mat dstcv32f;
		Mat dstcp32f;
		cout << "gray" << endl;
		{
			Timer t("cv 8u");
			for (int i = 0; i < iteration; i++)
				cv::copyMakeBorder(gray, dstcv8u, r, r, r, r, borderType);
		}
		{
			Timer t("cp 8u");
			for (int i = 0; i < iteration; i++)
				cp::copyMakeBorderReplicate(gray, dstcp8u, r, r, r, r);
		}
		cout << getPSNR(dstcv8u, dstcp8u) << "dB" << endl;
		//guiAlphaBlend(dstcv8u, dstcp8u);
		{
			Timer t("cv 32f");
			for (int i = 0; i < iteration; i++)
				cv::copyMakeBorder(src32fc1, dstcv32f, r, r, r, r, borderType);
		}
		{
			Timer t("cp 32f");
			for (int i = 0; i < iteration; i++)
				cp::copyMakeBorderReplicate(src32fc1, dstcp32f, r, r, r, r);
		}
		cout << getPSNR(dstcv32f, dstcp32f) << "dB" << endl;
		//guiAlphaBlend(dstcv32f, dstcp32f);
	}

	if (isColorTest)
	{
		Mat dstcv32f;
		Mat dstcp32f;
		cout << "color" << endl;
		{
			Timer t("cv 32f");
			for (int i = 0; i < iteration; i++)
				cv::copyMakeBorder(src32fc3, dstcv32f, r, r, r, r, borderType);
		}
		{
			Timer t("cp 32f");
			for (int i = 0; i < iteration; i++)
				cp::copyMakeBorderReplicate(src32fc3, dstcp32f, r, r, r, r);
		}
		{
			Timer t("cv 32f");
			for (int i = 0; i < iteration; i++)
				cv::copyMakeBorder(src32fc3, dstcv32f, r, r, r, r, borderType);
		}
		{
			Timer t("cp 32f");
			for (int i = 0; i < iteration; i++)
				cp::copyMakeBorderReplicate(src32fc3, dstcp32f, r, r, r, r);
		}
		cout << getPSNR(dstcv32f, dstcp32f) << "dB" << endl;
		guiAlphaBlend(dstcv32f, dstcp32f);
	}

}