#pragma once
#include <stdio.h>
#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/stitching/stitcher.hpp"
#include "opencv2/stitching/warpers.hpp"
#include "opencv2/stitching/detail/matchers.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/stitching/detail/exposure_compensate.hpp"
#include "opencv2/stitching/detail/seam_finders.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include "opencv2/stitching/detail/camera.hpp"

using namespace cv::detail;
using namespace std;
using namespace cv;

namespace cv {
	class CV_EXPORTS MyStitcher
	{
	public:
		enum { ORIG_RESOL = -1 };

		static MyStitcher setters();

		//setters
		void setWaveCorrection(bool flag) { do_wave_correct_ = flag; }

		void setWaveCorrectKind(detail::WaveCorrectKind kind) { wave_correct_kind_ = kind; }

		void setFeaturesFinder(Ptr<detail::FeaturesFinder> features_finder) { features_finder_ = features_finder; }

		void setFeaturesMatcher(Ptr<detail::FeaturesMatcher> features_matcher) { features_matcher_ = features_matcher; }

		void setMatchingMask(const cv::Mat &mask) { CV_Assert(mask.type() == CV_8U && mask.cols == mask.rows); matching_mask_ = mask.clone(); }

		void setBundleAdjuster(Ptr<detail::BundleAdjusterBase> bundle_adjuster) { bundle_adjuster_ = bundle_adjuster; }

		void setWarper(Ptr<WarperCreator> creator) { warper_ = creator; }

		void setExposureCompensator(Ptr<detail::ExposureCompensator> exposure_comp) { exposure_comp_ = exposure_comp; }

		void setSeamFinder(Ptr<detail::SeamFinder> seam_finder) { seam_finder_ = seam_finder; }

		void setBlender(Ptr<detail::Blender> b) { blender_ = b; }
		void setBlender2(Ptr<detail::Blender> b) { blender_Origo = b; }

		bool checkCameras(string c1, string c2, string c3, string c4, string c5, string c6);
		void setCameras();
		void setNewCombination();
		void setCombination(string comb);
		void stitch(InputArray images, OutputArray pano, const vector<vector<Rect> > &rois);
		void SURF();
		void matchPoints();
		void rotation_homography();
		void warp(int number, int x, int y);
		void points();
		void leftPoints();
		void pointsInWarp();

		std::vector<detail::CameraParams> cameras() const { return cameras_; }


	private:
		MyStitcher() {}

		Ptr<detail::FeaturesFinder> features_finder_;
		Ptr<detail::FeaturesMatcher> features_matcher_;
		Ptr<detail::BundleAdjusterBase> bundle_adjuster_;
		Ptr<detail::ExposureCompensator> exposure_comp_;
		Ptr<detail::SeamFinder> seam_finder_;
		Ptr<detail::Blender> blender_;
		Ptr<detail::Blender> blender_Origo;

		Ptr<WarperCreator> warper_;

		cv::Mat matching_mask_;
		cv::Mat panoO;

		cv::Mat panoOrigo;

		std::vector<detail::CameraParams> cameras_;

		detail::WaveCorrectKind wave_correct_kind_;

		bool do_wave_correct_;
	};
}




