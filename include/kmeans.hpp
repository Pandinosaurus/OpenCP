#pragma once

#include "common.hpp"

namespace cp
{
#define KMEANS_MSPP_CENTERS KmeansFlags::KMEANS_PP_CENTERS+2

	//SoA cols>rows
	//AoS rows>cols, cols or channel are dim
	class CP_EXPORT KMeans
	{
	public:

		enum class MeanFunction
		{
			Mean,//box
			MinMax,
			Gauss,
			GaussInv,
			LnNorm,
			Harmonic

		};
		void setSigma(const float sigma) { this->sigma = sigma; }//for Gauss means
		void setSignalMax(const float signal_max) { this->signal_max = signal_max; };//for Gauss means
		void setKMeansPPTraials(const int num) { this->KMEANSPP_TRIALS = num; }//for K-means++
		void setKMeansREPPTraials(const int num) { this->KMEANSREPP_TRIALS = num; }//for K-means++ re
		enum class Schedule
		{
			Auto,
			AoS_NKD,
			SoA_KND,
			AoS_KND,
			SoA_NKD,
			SoAoS_NKD,
			SoAoS_KND,

			SIZE
		};
		double clustering(cv::InputArray data, int K, cv::InputOutputArray _bestLabels, cv::TermCriteria criteria, int attempts, int flags, cv::OutputArray _centers, MeanFunction function = MeanFunction::Mean, Schedule schedule = Schedule::Auto);
		void gui(cv::InputArray data, int K, cv::InputOutputArray _bestLabels, cv::TermCriteria criteria, int attempts, int flags, cv::OutputArray _centers, MeanFunction function = MeanFunction::Mean, Schedule schedule = Schedule::Auto, cv::InputArray additionalData = cv::noArray());
		cv::Mat weight;
		void setWeightMap(const cv::Mat& weight);
	private:
		bool isUseWeight = false;
		int KMEANSREPP_TRIALS = 3;//the number of trials for kmeans++ initialization
		int KMEANSPP_TRIALS = 3;//the number of trials for kmeans++ initialization

		float sigma = 30.f;//for weightedMeanCentroid
		float signal_max = 255.f;//for weightedMeanCentroid
		int weightTableSize = 0;//for weightedMeanCentroid

		cv::Mat labels_internal;
		cv::AutoBuffer<float> distancePP;//for kmeans++

		//initialize
		void generateKmeansRandomBoxInitialCentroidSoA(const cv::Mat& data_points, cv::Mat& dest_centroids, const int K, cv::RNG& rng);
		void generateWeightedKmeansPPInitialCentroidSoA(const cv::Mat& data, const cv::Mat& weight, cv::Mat& _out_centers, int K, cv::RNG& rng, int trials);
		void generateKmeansPPInitialCentroidSoA(const cv::Mat& data, cv::Mat& _out_centers, int K, cv::RNG& rng, int trials);
		void generateKmeansMSPPInitialCentroidSoA(const cv::Mat& data, cv::Mat& _out_centers, int K, cv::RNG& rng, int trials);

		void generateKmeansRandomInitialCentroidAoS(const cv::Mat& data_points, cv::Mat& dest_centroids, const int K, cv::RNG& rng);
		void generateKmeansPPInitialCentroidAoS(const cv::Mat& data, cv::Mat& _out_centers, int K, cv::RNG& rng, int trials);

		//get most outer samples for centroids (not used)
		void getOuterSample(cv::Mat& src_centroids, cv::Mat& dest_centroids, const cv::Mat& data_points, const cv::Mat& labels);

		//computing centroid
		void minmaxCentroidSoA(const cv::Mat& data_points, const int* labels, cv::Mat& dest_centroid, int* counters, const int K);
		void boxMeanCentroidSoA(const cv::Mat& data_points, const int* labels, cv::Mat& dest_centroid, int* counters);//simple average
		void weightedMeanCentroid(const cv::Mat& data_points, const int* labels, const cv::Mat& src_centroid, const float* Table, const int tableSize, cv::Mat& dest_centroid, float* dest_centroid_weight, int* dest_counters);
		void harmonicMeanCentroid(const cv::Mat& data_points, const int* labels, const cv::Mat& src_centroid, cv::Mat& dest_centroid, float* centroid_weight, int* counters);

		void boxMeanCentroidAoS(cv::Mat& data_points, const int* labels, cv::Mat& dest_centroid, int* counters);//N*d simple average

		//data.cols < data.rows transpos data;
		double clusteringSoA(cv::InputArray _data, int K, cv::InputOutputArray _bestLabels, cv::TermCriteria criteria, int attempts, int flags, cv::OutputArray _centers, MeanFunction function = MeanFunction::Mean, int loop = 0);
		double clusteringAoS(cv::InputArray _data, int K, cv::InputOutputArray _bestLabels, cv::TermCriteria criteria, int attempts, int flags, cv::OutputArray _centers, MeanFunction function = MeanFunction::Mean, int loop = 1);
		double clusteringSoAoS(cv::InputArray _data, int K, cv::InputOutputArray _bestLabels, cv::TermCriteria criteria, int attempts, int flags, cv::OutputArray _centers, MeanFunction function = MeanFunction::Mean, int loop = 0);
	};

	CP_EXPORT double kmeans(cv::InputArray _data, int K, cv::InputOutputArray _bestLabels, cv::TermCriteria criteria, int attempts, int flags, cv::OutputArray _centers);
}
