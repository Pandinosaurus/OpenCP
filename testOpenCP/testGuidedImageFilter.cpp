#include <opencp.hpp>
#include "test.hpp"

using namespace std;
using namespace cv;
using namespace cp;

void testGuidedImageFilter(Mat& img_p, Mat& img_I)
{
	//const bool isOverwrite = true;
	const bool isOverwrite = img_p.empty() || img_I.empty();
	if (isOverwrite)
	{
		string imgPath_p, imgPath_I;

		imgPath_p = "img/lenna.png";
		//imgPath_p = "fig/flower_foveon8.png";
		//imgPath_p = "fig/kodak/kodim17.png";
		imgPath_I = imgPath_p;

		// Flash/no-flash denoising
	/*{
		imgPath_p = "fig/pot2_noflash.png";
		imgPath_I = "fig/pot2_flash.png";
	}*/

		img_p = imread(imgPath_p, 1);
		img_I = imread(imgPath_I, 1);

		resize(img_p, img_p, img_p.size() / 2);
		resize(img_I, img_I, img_I.size() / 2);
		//guiAlphaBlend(img_p, img_I, true);
	}

	int r = 4;
	int e = 100;
	//int guidedType = GUIDED_XIMGPROC;
	int guidedType = GuidedTypes::GUIDED_SEP_VHI;
	//int guidedType = GuidedTypes::GUIDED_NAIVE;

	int boxType = (int)BoxFilterMethod::OPENCV;
	int parallelType = (int)ParallelTypes::OMP;
	//int parallelType = ParallelTypes::NAIVE;
	int src_GRAY_RGB = 1;
	int guide_GRAY_RGB = 1;

	//#define RANDOM_SHIFT

#define OPENCV
#define FUNCTION_TEST
#define CLASS_TEST
#define CLASS_COLORPARALLEL_TEST
#define CLASS_PRECOMP_TEST
#define FAST_TEST
#define TILE_TEST

	ConsoleImage ci(Size(900, 600), "param");
	namedWindow("param");

	moveWindow("param", 100, 100);
	int loop = 5; createTrackbar("loop", "param", &loop, 100);

	int showIndex = 0; createTrackbar("swowIndex", "param", &showIndex, 5);
	createTrackbar("r", "param", &r, 100);
	createTrackbar("eps", "param", &e, 100000);
	createTrackbar("GuidedType", "param", &guidedType, GuidedTypes::NumGuidedTypes - 1);
	createTrackbar("BoxType", "param", &boxType, (int)BoxFilterMethod::SIZE - 1);
	createTrackbar("parallel", "param", &parallelType, ParallelTypes::NumParallelTypes - 1);
	createTrackbar("src gray:color", "param", &src_GRAY_RGB, 1);
	createTrackbar("guide gray:color", "param", &guide_GRAY_RGB, 1);
#ifdef FAST_TEST
	int fastres = 1;
	createTrackbar("fast res", "param", &fastres, 8);
	int downsample = 0;  createTrackbar("down_method", "param", &downsample, INTER_LANCZOS4);
	int upsample = INTER_CUBIC;  createTrackbar("up_method", "param", &upsample, INTER_LANCZOS4);

#endif
	int h = 2;  createTrackbar("h_div", "param", &h, 6);
	int v = 2; createTrackbar("v_div", "param", &v, 6);
	int bb = 5; createTrackbar("PSNR_BB", "param", &bb, 20);

	Stat stats[8];

	int key = 0;

	cp::UpdateCheck uck(r, guidedType, boxType, parallelType, src_GRAY_RGB, guide_GRAY_RGB, h, v);

	cp::PSNRMetrics psnr;
	GuidedImageFilter gfref;
	GuidedImageFilter gf;
	GuidedImageFilter gfcp;
	GuidedImageFilter gffast;
	GuidedImageFilterTiling gft;
	RNG rng;

	Mat srcf; img_p.convertTo(srcf, CV_32F);
	Mat guidef = srcf.clone();
	Mat src64f; img_p.convertTo(src64f, CV_64F);
	Mat guide64f; guidef.convertTo(guide64f, CV_64F);

	Mat dest = Mat::zeros(img_p.size(), img_p.type());
	Mat destf = Mat::zeros(img_p.size(), srcf.type());
	Mat destfocv = Mat::zeros(img_p.size(), srcf.type());
	Mat dest_class = Mat::zeros(img_p.size(), img_p.type());
	Mat destf_class = Mat::zeros(img_p.size(), srcf.type());
	Mat dest_precomp = Mat::zeros(img_p.size(), img_p.type());
	Mat destf_precomp = Mat::zeros(img_p.size(), srcf.type());
	Mat dest_classcolorpara = Mat::zeros(img_p.size(), img_p.type());
	Mat destf_classcolorpara = Mat::zeros(img_p.size(), srcf.type());
	Mat dest_fast = Mat::zeros(img_p.size(), img_p.type());
	Mat destf_fast = Mat::zeros(img_p.size(), srcf.type());
	Mat dest_tile = Mat::zeros(img_p.size(), img_p.type());
	Mat destf_tile = Mat::zeros(img_p.size(), srcf.type());

	Mat ref64f = Mat::zeros(srcf.size(), src64f.type());
	Mat ref8u = Mat::zeros(img_p.size(), img_p.type());

	Mat show;
	Mat show8u;

	GuidedImageFilter gftemp;
	while (key != 'q')
	{
		int idx = 0;

		Mat src;
		if (src_GRAY_RGB == 0) cvtColor(img_p, src, COLOR_BGR2GRAY);
		else if (src_GRAY_RGB == 1) img_p.copyTo(src);

		Mat guide;
		if (guide_GRAY_RGB == 0) cvtColor(img_I, guide, COLOR_BGR2GRAY);
		else if (guide_GRAY_RGB == 1) img_I.copyTo(guide);

		/*Mat temp;
		bilateralFilter(src,temp , 7, 30, 30);
		bilateralFilter(temp, src, 7, 30, 30);*/

#ifdef RANDOM_SHIFT
		int x = rng.uniform(-10, 10);
		int y = rng.uniform(-10, 10);
		cp::warpShift(src, src, x, y, BORDER_REPLICATE);
		cp::warpShift(guide, guide, x, y, BORDER_REPLICATE);
#endif
		/*
		src.setTo(0);
		src.at<Vec3b>(src.rows / 2 - 1, src.cols / 2 + 1) = Vec3b(255, 255, 255);
		src.at<Vec3b>(src.rows / 2 - 1, src.cols / 2 + 0) = Vec3b(255, 255, 255);
		src.at<Vec3b>(src.rows / 2 - 1, src.cols / 2 - 1) = Vec3b(255, 255, 255);
		src.at<Vec3b>(src.rows / 2 + 0, src.cols / 2 + 1) = Vec3b(255, 255, 255);
		src.at<Vec3b>(src.rows / 2 + 0, src.cols / 2 + 0) = Vec3b(255, 255, 255);
		src.at<Vec3b>(src.rows / 2 + 0, src.cols / 2 - 1) = Vec3b(255, 255, 255);
		src.at<Vec3b>(src.rows / 2 + 1, src.cols / 2 + 1) = Vec3b(255, 255, 255);
		src.at<Vec3b>(src.rows / 2 + 1, src.cols / 2 + 0) = Vec3b(255, 255, 255);
		src.at<Vec3b>(src.rows / 2 + 1, src.cols / 2 - 1) = Vec3b(255, 255, 255);
		*/

		src.convertTo(srcf, CV_32F);
		guide.convertTo(guidef, CV_32F);
		src.convertTo(src64f, CV_64F);
		guide.convertTo(guide64f, CV_64F);

		const float eps = static_cast<float>(e);

		//reference
		{
			ref64f.setTo(0);
			Timer t("", TIME_MSEC, false);
			gfref.setBoxType(BoxFilterMethod::NAIVE_AVX);
			gfref.filterColorParallel(src64f, guide64f, ref64f, r, eps, 1, parallelType);
			stats[idx].push_back(t.getTime());

			ref64f.convertTo(ref8u, CV_8U);
			if (showIndex == idx)ref64f.copyTo(show);
			idx++;
		}
#ifdef OPENCV
		{
			destf.setTo(0);
			Timer t("", TIME_MSEC, false);
			for (int i = 0; i < loop; i++)
			{
				t.start();
				guidedImageFilter(srcf, guidef, destfocv, r, eps, GUIDED_XIMGPROC, (BoxFilterMethod)boxType, (ParallelTypes)parallelType);
				t.getpushLapTime();
			}
			stats[idx].push_back(t.getLapTimeMedian());
		}
		destf.convertTo(dest, CV_8U);
		if (showIndex == idx)destf.copyTo(show);
		idx++;
#endif

#ifdef FUNCTION_TEST
		{
			destf.setTo(0);
			Timer t("", TIME_MSEC, false);
			for (int i = 0; i < loop; i++)
			{
				t.start();
				guidedImageFilter(srcf, guidef, destf, r, eps, (GuidedTypes)guidedType, (BoxFilterMethod)boxType, (ParallelTypes)parallelType);
				t.getpushLapTime();
			}
			stats[idx].push_back(t.getLapTimeMedian());
		}
		destf.convertTo(dest, CV_8U);
		if (showIndex == idx)destf.copyTo(show);
		idx++;
#endif

#ifdef CLASS_TEST
		{
			destf_class.setTo(0);
			Timer t("", TIME_MSEC, false);

			for (int i = 0; i < loop; i++)
			{
				t.start();
				gf.setBoxType((BoxFilterMethod)boxType);
				gf.filter(srcf, guidef, destf_class, r, eps, (GuidedTypes)guidedType, (ParallelTypes)parallelType);
				t.getpushLapTime();
			}
			stats[idx].push_back(t.getLapTimeMedian());
		}
		destf_class.convertTo(dest_class, CV_8U);
		if (showIndex == idx)destf_class.copyTo(show);
		idx++;
#endif

#ifdef CLASS_PRECOMP_TEST
		{
			destf_precomp.setTo(0);
			Timer t("", TIME_MSEC, false);

			for (int i = 0; i < loop; i++)
			{
				t.start();
				gf.setBoxType((BoxFilterMethod)boxType);
				gf.filterGuidePrecomputed(srcf, guidef, destf_precomp, r, eps, (GuidedTypes)guidedType, (ParallelTypes)parallelType);
				t.getpushLapTime();
			}
			stats[idx].push_back(t.getLapTimeMedian());
		}
		destf_precomp.convertTo(dest_precomp, CV_8U);
		if (showIndex == idx)destf_precomp.copyTo(show);
		idx++;
#endif

#ifdef CLASS_COLORPARALLEL_TEST
		{
			destf_classcolorpara.setTo(0);
			Timer t("", TIME_MSEC, false);

			for (int i = 0; i < loop; i++)
			{
				t.start();
				gfcp.setBoxType((BoxFilterMethod)boxType);
				gfcp.filterColorParallel(srcf, guidef, destf_classcolorpara, r, eps, (GuidedTypes)guidedType, (ParallelTypes)parallelType);
				t.getpushLapTime();
			}
			stats[idx].push_back(t.getLapTimeMedian());
		}
		destf_classcolorpara.convertTo(dest_classcolorpara, CV_8U);
		if (showIndex == idx)destf_classcolorpara.copyTo(show);
		idx++;
#endif

#ifdef FAST_TEST
		{
			destf_fast.setTo(0);
			gffast.setUpsampleMethod(upsample);
			gffast.setDownsampleMethod(downsample);
			Mat refsrc;
			double factor = pow(2, fastres);
			double amp = 1.0 / factor;
			resize(srcf, refsrc, Size(), amp, amp);
			Timer t("", TIME_MSEC, false);
			for (int i = 0; i < loop; i++)
			{
				t.start();
				gffast.filterFast(srcf, guidef, destf_fast, r, eps, (int)factor, (GuidedTypes)guidedType, (ParallelTypes)parallelType);
				//gffast.upsample(refsrc, guidef, destf_fast, r, eps, GUIDED_NAIVE, parallelType);
				t.getpushLapTime();
			}
			stats[idx].push_back(t.getLapTimeMedian());
		}
		destf_fast.convertTo(dest_fast, CV_8U);
		if (showIndex == idx)destf_fast.copyTo(show);
		idx++;
#endif

#ifdef TILE_TEST
		{
			destf_tile.setTo(0);
			const int radix = 2;

			Timer t("", TIME_MSEC, false);
			for (int i = 0; i < loop; i++)
			{
				t.start();
				gft.filter(srcf, guidef, destf_tile, r, eps, Size((int)pow(radix, h), (int)pow(radix, v)), (GuidedTypes)guidedType);
				t.getpushLapTime();
			}
			stats[idx].push_back(t.getLapTimeMedian());
		}
		destf_tile.convertTo(dest_tile, CV_8U);
		if (showIndex == idx)destf_tile.copyTo(show);
		idx++;
#endif

		ci("Guided Type(g-f): %s", getGuidedType(guidedType).c_str());
		ci("Box Type        : " + getBoxType((BoxFilterMethod)boxType));
		ci("Parallel Type   : " + getParallelType(parallelType));
		ci("Parallel Type   : " + cp::getInterpolationName(downsample) + " Up: " + cp::getInterpolationName(upsample));
		ci("NUM             : %d", stats[0].getSize());
		ci("======Time======");
		idx = 0;
		ci("0: ref64f   : %f", stats[idx++].getMedian());
#ifdef OPENCV
		ci("1: opencv   : %f", stats[idx++].getMedian());
#endif
#ifdef FUNCTION_TEST
		ci("2: function : %f", stats[idx++].getMedian());
#endif
#ifdef CLASS_TEST
		ci("3: class    : %f", stats[idx++].getMedian());
#endif
#ifdef CLASS_PRECOMP_TEST
		ci("4: precomp  : %f", stats[idx++].getMedian());
#endif
#ifdef CLASS_COLORPARALLEL_TEST
		ci("5: colorPara: %f", stats[idx++].getMedian());
#endif
#ifdef FAST_TEST
		ci("6: fast     : %f", stats[idx++].getMedian());
#endif
#ifdef TILE_TEST
		ci("7: tiling   : %f", stats[idx++].getMedian());
#endif
		{
			//Timer t("PSNR");
			ci("======PSNR between 32F and 64F======");
#ifdef OPENCV
			ci("opencv   : %f", psnr.getPSNR(destfocv, ref64f, bb));
#endif
#ifdef FUNCTION_TEST
			ci("function : %f", psnr.getPSNR(destf, ref64f, bb));
#endif
#ifdef CLASS_TEST
			ci("class    : %f", psnr.getPSNR(destf_class, ref64f, bb));
#endif
#ifdef CLASS_PRECOMP_TEST
			ci("precomp  : %f", psnr.getPSNR(destf_precomp, ref64f, bb));
#endif
#ifdef CLASS_COLORPARALLEL_TEST
			ci("colorPara: %f", psnr.getPSNR(destf_classcolorpara, ref64f, bb));
#endif
#ifdef FAST_TEST
			ci("Fast     : %f", psnr.getPSNR(destf_fast, ref64f, bb));
#endif
#ifdef TILE_TEST
			ci("tiling   : %f", psnr.getPSNR(destf_tile, ref64f, bb));
#endif
		}
		ci.show();

		show.convertTo(show8u, CV_8U);
		imshow("show", show8u);

		//Mat sub;
		//absdiffScale(show, ref64f, sub, subCoeff);
		//imshow("Sub", sub);

		cp::guiDiff(show, ref64f, false);
		//imshowAnalysis("ana", show8u);
		//cout << endl;
		key = waitKey(1);
		if (key == 'g')
		{
			guidedType++;
			guidedType = (guidedType > GuidedTypes::NumGuidedTypes - 1) ? 0 : guidedType;
			setTrackbarPos("GuidedType", "param", guidedType);
		}
		if (key == 'f')
		{
			guidedType--;
			guidedType = (guidedType < 0) ? GuidedTypes::NumGuidedTypes - 2 : guidedType;
			setTrackbarPos("GuidedType", "param", guidedType);
		}
		if (key == 'r')
		{
			for (Stat& s : stats)
			{
				s.clear();
			}
		}

		if (uck.isUpdate(r, guidedType, boxType, parallelType, src_GRAY_RGB, guide_GRAY_RGB, h, v))
		{
			for (Stat& s : stats)
			{
				s.clear();
			}
		}
	}
}

void testGuidedBilateralFilter(Mat& img_)
{
	vector<Mat> v;
	Mat c;
	cvtColor(img_, c, COLOR_BGR2YUV);
	split(c, v);
	Mat img;
	if (img_.channels() == 1) img = img_;
	else cvtColor(img_, img, COLOR_BGR2GRAY);

	string wname = "BF-GIF test";
	namedWindow(wname);
	int sw = 0; createTrackbar("bf(0)-gif(1)", wname, &sw, 2);
	int r = 8; createTrackbar("r", wname, &r, 10);
	int sigma_s = 10; createTrackbar("sigma_s", wname, &sigma_s, 10);
	int sigma_r = 30; createTrackbar("sigma_r", wname, &sigma_r, 200);
	int eps = int(sigma_r / sqrt(2)); createTrackbar("eps", wname, &eps, 200);
	int alphaLEPF10 = 10; createTrackbar("alphaLEPF10", wname, &alphaLEPF10, 200);
	int m10 = 10; createTrackbar("m10", wname, &m10, 200);

	int key = 0;
	Mat destBF, destGIF, destLEPF;
	ConsoleImage ci;
	while (key != 'q')
	{
		guidedImageGaussianFilterGray(img, destGIF, r / 2, float(sigma_s / sqrt(2.0)), float(eps * eps));
		//guidedImageGaussianFilterGrayEnhance(img, destGIF, r / 2, float(sigma_s / sqrt(2.0)), float(eps * eps), -(m10*0.1f));
		localEdgePreservingGaussianFilter(img, destLEPF, r / 2, float(sigma_s / sqrt(2.0)), float(eps * eps), alphaLEPF10*0.1f);
		bilateralFilter(img, destBF, 2 * r + 1, sigma_r, sigma_s);
		ci("PSNR-BF-GIF %f", getPSNR(destBF, destGIF, 20));
		ci("PSNR-GIF-LEPF %f", getPSNR(destGIF, destLEPF, 20));
		ci.show();
		if (sw == 0)imshow(wname, destBF);
		if (sw == 1)imshow(wname, destGIF);
		if (sw == 2)imshow(wname, destLEPF);
		//diffshow("diff", destBF, destGIF, 5);
		diffshow("diff", destGIF, destLEPF, 5);
		//destGIF.copyTo(v[0]); merge(v, c); cvtColor(c, c, COLOR_YUV2BGR); imshow("color", c);
		destLEPF.copyTo(v[0]); merge(v, c); cvtColor(c, c, COLOR_YUV2BGR); imshow("color", c);
		key = waitKey(1);
	}
}