#include "minmaxfilter.hpp"

using namespace std;
using namespace cv;

namespace cp
{
	
	void maxFilter(InputArray src, OutputArray dest, Size kernelSize, int shape)
	{
		Mat element = getStructuringElement(shape, kernelSize);
		dilate(src, dest, element);
	}

	void minFilter(InputArray src, OutputArray dest, Size kernelSize, int shape)
	{
		Mat element = getStructuringElement(shape, kernelSize);
		erode(src, dest, element);
	}

	void minFilter(InputArray src, OutputArray dest, int radius)
	{
		minFilter(src, dest, Size(2 * radius + 1, 2 * radius + 1));
	}

	void maxFilter(InputArray src, OutputArray dest, int radius)
	{
		maxFilter(src, dest, Size(2 * radius + 1, 2 * radius + 1));
	}

	template<class srcType>
	void blurRemoveMinMax_(const Mat& src, Mat& dest, const int r)
	{
		const Size ksize = Size(2 * r + 1, 2 * r + 1);
		if (src.data != dest.data)src.copyTo(dest);

		Mat xv;
		Mat nv;
		Mat element = Mat::ones(2 * r + 1, 2 * r + 1, CV_8U);
		dilate(src, xv, element);
		erode(src, nv, element);

		Mat mind;
		Mat maxd;
		Mat mask;
		absdiff(src, nv, mind);//can move to loop
		absdiff(src, xv, maxd);//
		min(mind, maxd, mask);//

		srcType* n = nv.ptr<srcType>(0);
		srcType* x = xv.ptr<srcType>(0);
		srcType* d = dest.ptr<srcType>(0);
		srcType* nd = mind.ptr<srcType>(0);
		srcType* mk = mask.ptr<srcType>(0);

		int remsize = src.size().area();

#if CV_SSE4_1
		if (src.depth() == CV_8U)
		{

			const int ssesize = src.size().area() / 16;
			remsize = src.size().area() - ssesize * 16;
			for (int i = 0; i < ssesize; i++)
			{
				__m128i mmk = _mm_load_si128((__m128i*)mk);
				__m128i mnd = _mm_load_si128((__m128i*)nd);

				__m128i mmn = _mm_load_si128((__m128i*)n);
				__m128i mmx = _mm_load_si128((__m128i*)x);
				__m128i msk = _mm_cmpeq_epi8(mnd, mmk);
				_mm_stream_si128((__m128i*)d, _mm_blendv_epi8(mmx, mmn, msk));
				nd += 16;
				mk += 16;
				d += 16;
				n += 16;
				x += 16;
			}
		}
		else if (src.depth() == CV_16S || src.depth() == CV_16U)
		{

			const int ssesize = src.size().area() / 8;
			remsize = src.size().area() - ssesize * 8;
			for (int i = 0; i < ssesize; i++)
			{
				__m128i mmk = _mm_load_si128((__m128i*)mk);
				__m128i mnd = _mm_load_si128((__m128i*)nd);

				__m128i mmn = _mm_load_si128((__m128i*)n);
				__m128i mmx = _mm_load_si128((__m128i*)x);
				__m128i msk = _mm_cmpeq_epi16(mnd, mmk);
				_mm_stream_si128((__m128i*)d, _mm_blendv_epi8(mmx, mmn, msk));
				nd += 8;
				mk += 8;
				d += 8;
				n += 8;
				x += 8;
			}
		}
		else if (src.depth() == CV_32F)
		{

			const int ssesize = src.size().area() / 4;
			remsize = src.size().area() - ssesize * 4;
			for (int i = 0; i < ssesize; i++)
			{
				__m128 mmk = _mm_load_ps((float*)mk);
				__m128 mnd = _mm_load_ps((float*)nd);

				__m128 mmn = _mm_load_ps((float*)n);
				__m128 mmx = _mm_load_ps((float*)x);
				__m128 msk = _mm_cmpeq_ps(mnd, mmk);
				_mm_stream_ps((float*)d, _mm_blendv_ps(mmx, mmn, msk));
				nd += 4;
				mk += 4;
				d += 4;
				n += 4;
				x += 4;
			}
		}
		else if (src.depth() == CV_64F)
		{
			const int ssesize = src.size().area() / 2;
			remsize = src.size().area() - ssesize * 2;
			for (int i = 0; i < ssesize; i++)
			{
				__m128d mmk = _mm_load_pd((double*)mk);
				__m128d mnd = _mm_load_pd((double*)nd);

				__m128d mmn = _mm_load_pd((double*)n);
				__m128d mmx = _mm_load_pd((double*)x);
				__m128d msk = _mm_cmpeq_pd(mnd, mmk);
				_mm_stream_pd((double*)d, _mm_blendv_pd(mmx, mmn, msk));
				nd += 2;
				mk += 2;
				d += 2;
				n += 2;
				x += 2;
			}
		}
#endif
		for (int i = 0; i < remsize; i++)
		{
			{
				if (nd[i] == mk[i])
				{
					d[i] = n[i];
				}
				else
				{
					d[i] = x[i];
				}
			}
		}
	}

	void blurRemoveMinMax(const Mat& src, Mat& dest, const int r)
	{
		if (src.channels() == 1)
		{
			if (src.depth() == CV_8U)
				blurRemoveMinMax_<uchar>(src, dest, r);
			else if (src.depth() == CV_16S)
				blurRemoveMinMax_<short>(src, dest, r);
			else if (src.depth() == CV_16U)
				blurRemoveMinMax_<ushort>(src, dest, r);
			else if (src.depth() == CV_32F)
				blurRemoveMinMax_<float>(src, dest, r);
			else if (src.depth() == CV_64F)
				blurRemoveMinMax_<double>(src, dest, r);
		}
		else
		{
			vector<Mat> v;
			split(src, v);

			if (src.depth() == CV_8U)
				for (int i = 0; i < (int)v.size(); i++)
					blurRemoveMinMax_<uchar>(v[i], v[i], r);
			else if (src.depth() == CV_16S)
				for (int i = 0; i < (int)v.size(); i++)
					blurRemoveMinMax_<short>(v[i], v[i], r);
			else if (src.depth() == CV_16U)
				for (int i = 0; i < (int)v.size(); i++)
					blurRemoveMinMax_<ushort>(v[i], v[i], r);
			else if (src.depth() == CV_32F)
				for (int i = 0; i < (int)v.size(); i++)
					blurRemoveMinMax_<float>(v[i], v[i], r);
			else if (src.depth() == CV_64F)
				for (int i = 0; i < (int)v.size(); i++)
					blurRemoveMinMax_<double>(v[i], v[i], r);

			merge(v, dest);
		}
	}
}