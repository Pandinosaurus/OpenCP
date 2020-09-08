#include "stereoEval.hpp"
#include "depthfilter.hpp"
#include "metrics.hpp"
using namespace std;
using namespace cv;

namespace cp
{
	void createDisparityALLMask(Mat& src, Mat& dest)
	{
		compare(src, 0, dest, CMP_NE);
	}

	void createDisparityNonOcclusionMask(Mat& src, double amp, double thresh, Mat& dest)
	{
		Mat temp = src.clone();
		fastLRCheckDisparity(temp, thresh, amp);
		Mat mask2;
		compare(src, temp, dest, CMP_NE);
		compare(src, 0, mask2, CMP_EQ);
		bitwise_or(dest, mask2, dest);
		bitwise_not(dest, dest);
	}

	double calcBadPixel(InputArray groundtruth_, InputArray disparityImage_, InputArray mask_, double th, double amp)
	{
		Mat groundtruth = groundtruth_.getMat();
		Mat disparityImage = disparityImage_.getMat();
		Mat mask = mask_.getMat();

		if (mask.empty())mask = Mat::ones(groundtruth.size(), CV_8U);
		th *= amp;
		int count = 0;
		int c2 = 0;
		for (int j = 0; j < disparityImage.rows; j++)
		{
			const uchar* src = disparityImage.ptr<uchar>(j);
			const uchar* dest = groundtruth.ptr<uchar>(j);
			const uchar* mk = mask.ptr<uchar>(j);
			for (int i = 0; i < disparityImage.cols; i++)
			{
				if (mk[i] != 0)
				{
					int s = src[i];
					int d = dest[i];
					int diff = s - d;
					if (abs(diff) > th)
					{
						c2++;
					}
					count++;
				}
			}
		}
		return ((double)c2 / count) * 100.0;
	}

	template<typename T>
	double calcBadPixel_(Mat& groundtruth, Mat& disparityImage, Mat& mask, const double th, const double amp, Mat& dstErr)
	{
		dstErr.setTo(0);

		if (mask.empty())mask = Mat::ones(groundtruth.size(), CV_8U);
		double thresh = th * amp;
		int count = 0;
		int c2 = 0;

		for (int j = 0; j < disparityImage.rows; j++)
		{
			const T* src = disparityImage.ptr<T>(j);
			const T* dest = groundtruth.ptr<T>(j);
			const uchar* mk = mask.ptr<uchar>(j);
			for (int i = 0; i < disparityImage.cols; i++)
			{
				if (mk[i] != 0)
				{
					T diff = abs(src[i] - dest[i]);
					if (diff > thresh)
					{
						dstErr.at<uchar>(j, i) = 255;
						c2++;
					}
					count++;
				}
			}
		}
		return ((double)c2 / count) * 100.0;
	}

	double calcBadPixel(InputArray groundtruth_, InputArray disparityImage_, InputArray mask_, double th, double amp, OutputArray outErr)
	{
		CV_Assert(groundtruth_.depth() == disparityImage_.depth());
		CV_Assert(groundtruth_.depth() == CV_8U ||
			groundtruth_.depth() == CV_16U ||
			groundtruth_.depth() == CV_16S ||
			groundtruth_.depth() == CV_32S ||
			groundtruth_.depth() == CV_32F);
		if (outErr.empty() || outErr.size() != groundtruth_.size() || outErr.type() != CV_8U)
			outErr.create(groundtruth_.size(), groundtruth_.type());

		Mat groundtruth = groundtruth_.getMat();
		Mat disparityImage = disparityImage_.getMat();
		Mat mask = mask_.getMat();
		Mat dsterr = outErr.getMat();

		double ret = 0.0;
		if (groundtruth.depth() == CV_8U)
			ret = calcBadPixel_<uchar>(groundtruth, disparityImage, mask, th, amp, dsterr);
		else if (groundtruth.depth() == CV_16S)
			ret = calcBadPixel_<short>(groundtruth, disparityImage, mask, th, amp, dsterr);
		else if (groundtruth.depth() == CV_16U)
			ret = calcBadPixel_<ushort>(groundtruth, disparityImage, mask, th, amp, dsterr);
		else if (groundtruth.depth() == CV_32S)
			ret = calcBadPixel_<int>(groundtruth, disparityImage, mask, th, amp, dsterr);
		else if (groundtruth.depth() == CV_32F)
			ret = calcBadPixel_<float>(groundtruth, disparityImage, mask, th, amp, dsterr);

		return ret;
	}

	StereoEval::StereoEval(std::string groundtruthPath, std::string maskNonoccPath, std::string maskAllPath, std::string maskDiscPath, double amp_)
	{
		ground_truth = imread(groundtruthPath, 0);
		if (ground_truth.empty())cout << "cannot open" << groundtruthPath << endl;

		mask_all = imread(maskAllPath, 0);
		if (mask_all.empty())cout << "cannot open" << maskAllPath << endl;

		mask_nonocc = imread(maskNonoccPath, 0);
		if (mask_nonocc.empty())cout << "cannot open" << maskNonoccPath << endl;

		Mat temp = imread(maskDiscPath, 0);
		cv::compare(temp, 255, mask_disc, CMP_EQ);

		if (mask_disc.empty())cout << "cannot open" << maskDiscPath << endl;
		amp = amp_;
		threshmap_init();
	}

	void StereoEval::threshmap_init()
	{
		isInit = true;
		all_th = Mat::zeros(ground_truth.size(), CV_8U);
		nonocc_th = Mat::zeros(ground_truth.size(), CV_8U);
		disc_th = Mat::zeros(ground_truth.size(), CV_8U);
		state_all = Mat::zeros(ground_truth.size(), CV_8UC3);
		state_nonocc = Mat::zeros(ground_truth.size(), CV_8UC3);
		state_disc = Mat::zeros(ground_truth.size(), CV_8UC3);
	}

	void StereoEval::init(Mat& ground_truth, const double amp, const int ignoreLeftBoundary)
	{
		CV_Assert(!ground_truth.empty());
		this->ground_truth = ground_truth;
		this->amp = amp;

		createDisparityALLMask(ground_truth, mask_all);
		createDisparityNonOcclusionMask(ground_truth, amp, 1, mask_nonocc);
		skip_disc = true;
		mask_disc = Mat::zeros(ground_truth.size(), ground_truth.type());//under construction
		if (ignoreLeftBoundary > 0)
		{
			Mat mask(ground_truth.size(), CV_8U);
			rectangle(mask, Rect(0, 0, ignoreLeftBoundary, mask.rows), 255, cv::FILLED);
			mask_all.setTo(0, mask);
			mask_nonocc.setTo(0, mask);
			mask_disc.setTo(0, mask);
		}
		threshmap_init();
	}

	void StereoEval::init(Mat& ground_truth_, Mat& mask_nonocc_, Mat& mask_all_, Mat& mask_disc_, double amp_)
	{
		CV_Assert(ground_truth_.channels() == 1);
		ground_truth = ground_truth_;
		mask_all = mask_all_;
		mask_nonocc = mask_nonocc_;
		cv::compare(mask_disc_, 255, mask_disc, CMP_EQ);
		amp = amp_;
		threshmap_init();
	}

	StereoEval::StereoEval()
	{
		isInit = false;
	}

	StereoEval::StereoEval(Mat& ground_truth_, Mat& mask_nonocc_, Mat& mask_all_, Mat& mask_disc_, double amp_)
	{
		init(ground_truth_, mask_nonocc_, mask_all_, mask_disc_, amp_);
	}

	StereoEval::StereoEval(Mat& ground_truth, const double amp, const int ignoreLeftBoundary)
	{
		init(ground_truth, amp, ignoreLeftBoundary);
	}

	string StereoEval::getBadPixel(Mat& src, double threshold, bool isPrint)
	{
		Mat gtf;
		ground_truth.convertTo(gtf, CV_32S);
		all_th.setTo(0);
		nonocc_th.setTo(0);
		disc_th.setTo(0);
		all = calcBadPixel(gtf, src, mask_all, threshold, amp, all_th);
		nonocc = calcBadPixel(gtf, src, mask_nonocc, threshold, amp, nonocc_th);
		if (skip_disc) disc = 0.0;
		else disc = calcBadPixel(gtf, src, mask_disc, threshold, amp, disc_th);

		message = format("nonocc,all,disc: %2.2f, %2.2f, %2.2f", nonocc, all, disc);
		if (isPrint)
		{
			cout << message << endl;
		}
		return message;
	}

	string StereoEval::getMSE(Mat& src, bool isPrint, int disparity_scale)
	{
		all_th.setTo(0);
		nonocc_th.setTo(0);
		disc_th.setTo(0);

		Mat gtf; ground_truth.convertTo(gtf, CV_32F, 1.0 / amp);
		Mat srcf; src.convertTo(srcf, CV_32F, 1.0 / disparity_scale);
		allMSE = cp::getMSE(gtf, srcf, mask_all);
		nonoccMSE = cp::getMSE(gtf, srcf, mask_nonocc);
		if (skip_disc) discMSE = 0.0;
		else discMSE = cp::getMSE(gtf, srcf, mask_disc);

		message = format("nonocc,all,disc: %2.2f, %2.2f, %2.2f", nonoccMSE, allMSE, discMSE);
		if (isPrint)
		{
			cout << message << endl;
		}
		return message;
	}

	string StereoEval::operator() (InputArray src_, double threshold, bool isPrint, int input_disparity_scale)
	{
		Mat src = src_.getMat();
		Mat cnv;
		src.convertTo(cnv, CV_32S, amp / input_disparity_scale);
		return getBadPixel(cnv, threshold, isPrint);
	}

	void StereoEval::compare(Mat& before, Mat& after, double threshold, bool isPrint)
	{
		if (isPrint)cout << "before: ";
		getBadPixel(before, threshold, isPrint);
		Mat pall = all_th.clone();
		Mat pnonocc = nonocc_th.clone();
		Mat pdisc = disc_th.clone();
		if (isPrint)cout << "after : ";
		getBadPixel(after, threshold, isPrint);

		Mat mask;
		Mat v;
		vector<Mat> sa;
		//improve: green
		//degrade: red
		//nochange and bad pixel: blue
		//no change: black
		state_nonocc.setTo(Scalar(0, 0, 255), ~pnonocc);
		state_nonocc.setTo(Scalar(0, 255, 0), ~nonocc_th);
		cv::compare(pnonocc, nonocc_th, mask, cv::CMP_EQ);
		state_nonocc.setTo(0, mask);
		nonocc_th.copyTo(v, mask);
		state_nonocc.setTo(Scalar(255, 0, 0), v);
		split(state_nonocc, sa);
		{
			int tcount = countNonZero(mask_nonocc);
			cv::compare(sa[2], 255, mask, cv::CMP_EQ);
			int degrade = countNonZero(mask);
			cv::compare(sa[1], 255, mask, cv::CMP_EQ);
			int  improve = countNonZero(mask);
			cv::compare(sa[0], 255, mask, cv::CMP_EQ);
			int  remainbad = countNonZero(mask);
			cout << format("nonocc: deg: -%2.2f, imp: %2.2f, rmn: %2.2f, rmn+deg: %2.2f \n", (100.0 * degrade / (double)tcount), (100.0 * improve / (double)tcount), (100.0 * remainbad / (double)tcount), 100.0 * (remainbad + degrade) / (double)tcount);
		}

		state_all.setTo(Scalar(0, 0, 255), ~pall);
		state_all.setTo(Scalar(0, 255, 0), ~all_th);
		cv::compare(pall, all_th, mask, cv::CMP_EQ);
		state_all.setTo(0, mask);
		all_th.copyTo(v, mask);
		state_all.setTo(Scalar(255, 0, 0), v);
		split(state_all, sa);
		{
			int tcount = countNonZero(mask_all);
			cv::compare(sa[2], 255, mask, cv::CMP_EQ);
			int degrade = countNonZero(mask);
			cv::compare(sa[1], 255, mask, cv::CMP_EQ);
			int  improve = countNonZero(mask);
			cv::compare(sa[0], 255, mask, cv::CMP_EQ);
			int  remainbad = countNonZero(mask);
			cout << format("all   : deg: -%2.2f, imp: %2.2f, rmn: %2.2f, rmn+deg: %2.2f \n", (100.0 * degrade / (double)tcount), (100.0 * improve / (double)tcount), (100.0 * remainbad / (double)tcount), 100.0 * (remainbad + degrade) / (double)tcount);
		}

		state_disc.setTo(Scalar(0, 0, 255), ~pdisc);
		state_disc.setTo(Scalar(0, 255, 0), ~disc_th);
		cv::compare(pdisc, disc_th, mask, cv::CMP_EQ);
		state_disc.setTo(0, mask);
		disc_th.copyTo(v, mask);
		state_disc.setTo(Scalar(255, 0, 0), v);
		split(state_disc, sa);
		{
			int tcount = countNonZero(mask_disc);
			cv::compare(sa[2], 255, mask, cv::CMP_EQ);
			int degrade = countNonZero(mask);
			cv::compare(sa[1], 255, mask, cv::CMP_EQ);
			int  improve = countNonZero(mask);
			cv::compare(sa[0], 255, mask, cv::CMP_EQ);
			int  remainbad = countNonZero(mask);
			cout << format("disc  : deg: -%2.2f, imp: %2.2f, rmn: %2.2f, rmn+deg: %2.2f \n", (100.0 * degrade / (double)tcount), (100.0 * improve / (double)tcount), (100.0 * remainbad / (double)tcount), 100.0 * (remainbad + degrade) / (double)tcount);
		}
		//
	}
}