#include "Header.h"
#include <ctime>
#include <stdio.h>
#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"
std::vector<int> indices_;
Size img_size;

Mat K;
Mat H;

std::string combination;
std::string camName;

std::string cam1 = "cam1";
std::string cam2 = "cam2";
std::string cam3 = "cam3";
std::string cam4 = "cam4";
std::string cam5 = "cam5";
std::string cam6 = "cam6";

double seam_est_resol_;
double compose_resol_;

double seam_scale_ = 1;
double seam_work_aspect_ = 1;
double warped_image_scale_;
double work_scale_ = 1;

std::vector<cv::Mat> imgs_;
std::vector<cv::Mat> seam_est_imgs_;
std::vector<cv::Size> full_img_sizes_;
std::vector<std::vector<cv::Rect> > rois_;
std::vector<detail::ImageFeatures> features_;
std::vector<detail::MatchesInfo> pairwise_matches_;

Mat img_warped, img_warped_s, img_warped_s2;
Mat dilated_mask, seam_mask;
Mat mask_warped, mask_warped2, mask;

bool is_blender_prepared = false;
bool is_compose_scale_set = false;

float movementX = 0;
float movementY = 0;

bool white;

vector<cv::Point2f> pointsFinal;

Point2f pointsF;
Point2f movedFinal;

MyStitcher MyStitcher::setters()
{
	MyStitcher stitcher;
	stitcher.setWaveCorrection(true);
	stitcher.setWaveCorrectKind(detail::WAVE_CORRECT_HORIZ);
	stitcher.setFeaturesMatcher(new detail::BestOf2NearestMatcher());
	stitcher.setBundleAdjuster(new detail::BundleAdjusterRay());
	stitcher.setFeaturesFinder(new detail::SurfFeaturesFinder());
	stitcher.setWarper(new PlaneWarper2());
	stitcher.setSeamFinder(new detail::GraphCutSeamFinder(detail::GraphCutSeamFinderBase::COST_COLOR));
	stitcher.setExposureCompensator(new detail::BlocksGainCompensator());
	stitcher.setBlender(new detail::MultiBandBlender());
	stitcher.setBlender2(new detail::FeatherBlender());

	return stitcher;
}

string type2str(int type) {
	string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth) {
	case CV_8U:  r = "8U"; break;
	case CV_8S:  r = "8S"; break;
	case CV_16U: r = "16U"; break;
	case CV_16S: r = "16S"; break;
	case CV_32S: r = "32S"; break;
	case CV_32F: r = "32F"; break;
	case CV_64F: r = "64F"; break;
	default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	return r;
}

bool MyStitcher::checkCameras(string c1, string c2, string c3, string c4, string c5, string c6) {

	camName = "./XML//X";

	if (c1 != "a") {
		camName.append(c1);
	}
	if (c2 != "a") {
		camName.append(c2);
	}
	if (c3 != "a") {
		camName.append(c3);
	}
	if (c4 != "a") {
		camName.append(c4);
	}
	if (c5 != "a") {
		camName.append(c5);
	}
	if (c6 != "a") {
		camName.append(c6);
	}

	std::string xml = ".xml";
	camName.append(xml);

	//printf("name %s\n", camName.c_str());

	if (FILE *file = fopen(camName.c_str(), "r")) {
		fclose(file);
		return true;
	}
	else {
		return false;
	}
}

void MyStitcher::setCameras() {

	FileStorage fs(camName, FileStorage::WRITE);
	fs.open(camName, FileStorage::WRITE);
	fs.release();

}

void MyStitcher::setNewCombination() {

	combination = camName.c_str();

}

void MyStitcher::setCombination(string comb) {

	combination = "./XML//";
	combination.append(comb);

}

void MyStitcher::stitch(InputArray images, OutputArray pano, const vector<vector<Rect> > &rois)
{
	printf("stitch\n");
	printf("Combination destination %s\n", combination.c_str());

	images.getMatVector(imgs_);
	rois_ = rois;

	//imshow("image1", imgs_[0]);
	//imshow("image2", imgs_[1]);

	//waitKey(0);

	bool is_work_scale_set = false;
	bool is_seam_scale_set = false;

	Mat full_img, img;

	features_.resize(imgs_.size());
	seam_est_imgs_.resize(imgs_.size());
	full_img_sizes_.resize(imgs_.size());

	for (size_t i = 0; i < imgs_.size(); ++i)
	{

		full_img = imgs_[i];
		full_img_sizes_[i] = full_img.size();
		img = full_img;

		if (!is_work_scale_set)
		{
			work_scale_ = min(1.0, sqrt(0.6 * 1e6 / full_img.size().area()));
			is_work_scale_set = true;
			//printf("work scale %lf\n", work_scale_);
		}

		if (!is_seam_scale_set)
		{
			seam_scale_ = min(1.0, sqrt(0.1 * 1e6 / full_img.size().area()));
			seam_work_aspect_ = seam_scale_ / work_scale_;
			is_seam_scale_set = true;
			//printf("seam scale %lf\n", seam_scale_);
		}

		features_[i].img_idx = (int)i;
		resize(full_img, img, Size(), seam_scale_, seam_scale_);
		seam_est_imgs_[i] = img.clone();
		//printf("seam img size %d %d\n", seam_est_imgs_[i].cols, seam_est_imgs_[i].rows);

	}

	for (size_t i = 0; i < imgs_.size(); ++i)
	{
		full_img_sizes_[i] = imgs_[i].size();
	}

}

void MyStitcher::SURF()
{
	Mat full_img, img;

	bool is_work_scale_set = false;
	bool is_seam_scale_set = false;
	features_.resize(imgs_.size());
	seam_est_imgs_.resize(imgs_.size());
	full_img_sizes_.resize(imgs_.size());

	printf("Finding features...\n");

	for (size_t i = 0; i < imgs_.size(); ++i)
	{
		full_img = imgs_[i];
		full_img_sizes_[i] = full_img.size();
		img = full_img;

		//////////////////nie pre cam3 cam4
		if (!is_work_scale_set)
		{
			work_scale_ = min(1.0, sqrt(0.6 * 1e6 / full_img.size().area()));
			is_work_scale_set = true;
		}

		resize(full_img, img, Size(), work_scale_, work_scale_);

		if (!is_seam_scale_set)
		{
			seam_scale_ = min(1.0, sqrt(0.1 * 1e6 / full_img.size().area()));
			seam_work_aspect_ = seam_scale_ / work_scale_;
			is_seam_scale_set = true;
		}
		/////////////////////////////
		std::clock_t start;
		double duration;

		start = std::clock();
		(*features_finder_)(img, features_[i]);
		duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		printf("pocet %d\n", features_[i].keypoints.size());
		std::cout << "printf: " << duration << '\n';

		////////////////////nie pre cam3 cam4
		features_[i].img_idx = (int)i;
		resize(full_img, img, Size(), seam_scale_, seam_scale_);
		seam_est_imgs_[i] = img.clone();
		///////////////////////

	}

	Mat outImg1, outImg2;
	drawKeypoints(imgs_[0], features_[0].keypoints, outImg1, Scalar(255, 255, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	drawKeypoints(imgs_[1], features_[1].keypoints, outImg2, Scalar(255, 255, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

	full_img.release();
	img.release();

	imwrite("./ImagesReal//surf1.jpg", outImg1);
	imwrite("./ImagesReal//surf2.jpg", outImg2);

}

void MyStitcher::matchPoints() {

	features_finder_->collectGarbage();

	(*features_matcher_)(features_, pairwise_matches_);

	Mat matched;

	drawMatches(imgs_[0], features_[0].keypoints, imgs_[1], features_[1].keypoints, pairwise_matches_[1].matches, matched, Scalar(255, 255, 255));

	imwrite("./ImagesReal//matched.jpg", matched);



}

void MyStitcher::rotation_homography()
{

	FileStorage fs1(combination, FileStorage::WRITE);
	fs1.open(combination, FileStorage::WRITE);

	detail::HomographyBasedEstimator estimator;
	estimator(features_, pairwise_matches_, cameras_);

	for (size_t i = 0; i < cameras_.size(); ++i)
	{
		Mat R;
		cameras_[i].R.convertTo(R, CV_32F);
		cameras_[i].R = R;
	}

	bundle_adjuster_->setConfThresh(1);
	(*bundle_adjuster_)(features_, pairwise_matches_, cameras_);

	// Find median focal length and use it as final image scale
	vector<double> focals;

	for (size_t i = 0; i < cameras_.size(); ++i)
	{
		focals.push_back(cameras_[i].focal);
	}

	std::sort(focals.begin(), focals.end());

	if (focals.size() % 2 == 1)
		warped_image_scale_ = static_cast<float>(focals[focals.size() / 2]);
	else
		warped_image_scale_ = static_cast<float>(focals[focals.size() / 2 - 1] + focals[focals.size() / 2]) * 0.5f;

	printf("warped images scale %.20lf\n", warped_image_scale_);



	if (do_wave_correct_)
	{
		vector<Mat> rmats;

		for (size_t i = 0; i < cameras_.size(); ++i) {
			rmats.push_back(cameras_[i].R);
		}

		detail::waveCorrect(rmats, wave_correct_kind_);

		for (size_t i = 0; i < cameras_.size(); ++i) {
			cameras_[i].R = rmats[i];
			/*cameras_[i].focal *= 1 / work_scale_;
			cameras_[i].ppx *= 1 / work_scale_;
			cameras_[i].ppy *= 1 / work_scale_;*/

		}
	}

	for (size_t i = 0; i < imgs_.size(); ++i)
	{
		cameras_[i].K().convertTo(K, CV_32F);

		std::string a = "K";
		std::string b = "R";

		int temp = 0;
		temp = i;

		int temp2 = 0;
		temp2 = i;

		a.append(std::to_string(temp));
		fs1 << a << K;

		a.clear();

		b.append(std::to_string(temp2));
		fs1 << b << cameras_[i].R;

		b.clear();
	}

	fs1 << "W" << warped_image_scale_;



	for (size_t i = 0; i < imgs_.size(); ++i) {
		cameras_[i].focal *= 1 / work_scale_;
		cameras_[i].ppx *= 1 / work_scale_;
		cameras_[i].ppy *= 1 / work_scale_;
	}

	for (size_t i = 0; i < imgs_.size(); ++i)
	{
		cameras_[i].K().convertTo(K, CV_32F);

		std::string a = "K2";
		std::string b = "R2";

		int temp = 0;
		temp = i;

		int temp2 = 0;
		temp2 = i;

		a.append(std::to_string(temp));
		fs1 << a << K;

		a.clear();

		b.append(std::to_string(temp2));
		fs1 << b << cameras_[i].R;

		b.clear();
	}

	fs1.release();
}

void MyStitcher::warp(int number, int x, int y)
{
	printf("number %d x %d y %d\n", number, x, y);

	FileStorage fs(combination, FileStorage::READ);
	fs.open(combination, FileStorage::READ);

	Mat t(3, 3, CV_64FC1);

	Mat u(3, 3, CV_64FC1);

	Mat t2(3, 3, CV_64FC1);

	Mat u2(3, 3, CV_64FC1);

	fs["W"] >> warped_image_scale_;

	printf("warped images scale %.20lf\n", warped_image_scale_);

	vector<Point> corners(imgs_.size());
	vector<Size> sizes(imgs_.size());
	vector<Mat> images_warped(imgs_.size());
	vector<Mat> masks(imgs_.size());
	vector<Mat> masks_warped(imgs_.size());

	Mat full_img, img;

	for (size_t i = 0; i < imgs_.size(); ++i)
	{

		resize(imgs_[i], img, Size(), seam_scale_, seam_scale_);
		seam_est_imgs_[i] = img.clone();
	}

	/*vector<Mat> seam_est_imgs_subset;
	vector<Mat> imgs_subset;

	for (size_t i = 0; i < indices_.size(); ++i)
	{
	imgs_subset.push_back(imgs_[indices_[i]]);
	seam_est_imgs_subset.push_back(seam_est_imgs_[indices_[i]]);
	}

	seam_est_imgs_ = seam_est_imgs_subset;
	imgs_ = imgs_subset;*/

	imwrite("./ImagesReal//seamEstFirst.jpg", seam_est_imgs_[0]);
	imwrite("./ImagesReal//seamEstSecond.jpg", seam_est_imgs_[1]);

	// Prepare image masks
	for (size_t i = 0; i < imgs_.size(); ++i)
	{
		masks[i].create(seam_est_imgs_[i].size(), CV_8U);
		masks[i].setTo(Scalar::all(255));
	}

	Ptr<detail::RotationWarper> w;
	Ptr<detail::RotationWarper> ue = warper_->create(float(warped_image_scale_ * seam_work_aspect_));

	for (size_t i = 0; i < imgs_.size(); ++i)
	{
		std::string a = "K";
		std::string b = "R";

		int temp = 0;
		temp = i;

		int temp2 = 0;
		temp2 = i;

		a.append(std::to_string(temp));
		fs[a] >> u;

		b.append(std::to_string(temp2));
		fs[b] >> t;

		u.at<float>(0, 0) *= (float)seam_work_aspect_;
		u.at<float>(0, 2) *= (float)seam_work_aspect_;
		u.at<float>(1, 1) *= (float)seam_work_aspect_;
		u.at<float>(1, 2) *= (float)seam_work_aspect_;

		corners[i] = ue->warp(seam_est_imgs_[i], u, t, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
		sizes[i] = images_warped[i].size();

		ue->warp(masks[i], u, t, INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);

	}

	vector<Mat> images_warped_f(imgs_.size());

	for (size_t i = 0; i < imgs_.size(); ++i)
		images_warped[i].convertTo(images_warped_f[i], CV_32F);

	// Find seams
	exposure_comp_->feed(corners, images_warped, masks_warped);
	seam_finder_->find(images_warped_f, corners, masks_warped);

	printf("prvy corner %d %d\n", corners[0].x, corners[0].y);
	printf("druhy corner %d %d\n", corners[1].x, corners[1].y);
	imwrite("./ImagesReal//seam1.jpg", masks_warped[0]);
	imwrite("./ImagesReal//seam2.jpg", masks_warped[1]);

	// Release unused memory
	seam_est_imgs_.clear();
	images_warped.clear();
	images_warped_f.clear();
	masks.clear();
	double compose_work_aspect = 1;

	for (size_t img_idx = 0; img_idx < imgs_.size(); ++img_idx)
	{

		// Read image and resize it if necessary
		full_img = imgs_[img_idx];

		if (!is_compose_scale_set)
		{

			is_compose_scale_set = true;
			compose_work_aspect = 1 / work_scale_;
			warped_image_scale_ *= static_cast<float>(compose_work_aspect);
			w = warper_->create((float)warped_image_scale_); //scalovanie warpnuteho img

															 // Update corners and sizes
			for (size_t i = 0; i < imgs_.size(); ++i)
			{

				// Update corner and size
				Size sz = full_img_sizes_[i];

				std::string a = "K2";
				std::string b = "R2";

				int temp = 0;
				temp = i;

				int temp2 = 0;
				temp2 = i;

				a.append(std::to_string(temp));
				fs[a] >> u2;

				b.append(std::to_string(temp2));
				fs[b] >> t2;

				Rect roi;

				roi = w->warpRoi(sz, u2, t2);

				corners[i] = roi.tl();
				sizes[i] = roi.size();

			}
		}

		img = full_img;
		full_img.release();
		img_size = img.size();

		std::string a = "K2";
		std::string b = "R2";

		int temp = 0;
		temp = img_idx;

		int temp2 = 0;
		temp2 = img_idx;

		a.append(std::to_string(temp));
		fs[a] >> u2;

		b.append(std::to_string(temp2));
		fs[b] >> t2;

		Point2f ae(0, 0); //311 230 -- 432 310
		Point2f test = w->warpPoint(ae, u2, t2);
		printf("last %lf %lf\n", test.x, test.y);

		// Warp the current image
		w->warp(img, u2, t2, INTER_LINEAR, BORDER_REFLECT, img_warped);

		// Warp the current image mask
		mask.create(img_size, CV_8U);
		mask.setTo(Scalar::all(255));

		Point2f ss;
		ss.x = x;
		ss.y = y;

		/*if (number - 1 == img_idx) {
		cv::circle(mask, ss, 8, (0, 0, 0), -1);
		printf("mask size %d %d\n", mask.cols, mask.rows);
		imshow("mask", mask);
		}*/

		Mat warpNormal = img;

		Mat warped;
		warpNormal.convertTo(warped, CV_8U);

		w->warp(img, u2, t2, INTER_NEAREST, BORDER_CONSTANT, warped);

		if (number - 1 == img_idx) {

			Point2f ae(x, y);
			Point2f coord = w->warpPoint(ae, u2, t2);

			vector<cv::Point2f> points;
			points = w->set4Points();

			printf("coord x %lf coord y %lf\n", coord.x, coord.y);
			printf("new x %lf new y %lf\n", -points[0].x + coord.x, -points[0].y + coord.y);

			Mat original = img;

			Mat waaaarp;
			warped.copyTo(waaaarp);

			Point2f pointsOriginal;

			pointsOriginal.x = x;
			pointsOriginal.y = y;

			Point2f pointsFinal;

			pointsFinal.x = -points[0].x + coord.x;
			pointsFinal.y = -points[0].y + coord.y;

			//pointsF.x = pointsFinal.x/waaaarp.cols;
			//pointsF.y = pointsFinal.y/waaaarp.rows;

			pointsF.x = pointsFinal.x;
			pointsF.y = pointsFinal.y;

			cv::circle(original, pointsOriginal, 8, (0, 0, 255), -1);
			cv::circle(waaaarp, pointsFinal, 8, (0, 0, 255), -1);

			imwrite("./ImagesReal//pointOriginal.jpg", original);
			imwrite("./ImagesReal//pointWarped.jpg", waaaarp);
		}

		w->warp(mask, u2, t2, INTER_NEAREST, BORDER_CONSTANT, mask_warped);
		w->warp(mask, u2, t2, INTER_NEAREST, BORDER_CONSTANT, mask_warped2);

		/*if (img_idx == 0) {

		vector<cv::Point2f> points;
		points = w->setPoints();
		pointsFinal = points;

		}*/

		std::string index = std::to_string(img_idx + 1);
		std::string warp = "warp" + index;
		imwrite("./ImagesReal//" + warp + ".jpg", warped);

		//exposure_comp_->apply((int)img_idx, corners[img_idx], img_warped, mask_warped);

		// Make sure seam mask has proper size
		dilate(masks_warped[img_idx], dilated_mask, Mat());
		resize(dilated_mask, seam_mask, mask_warped.size());

		mask_warped = seam_mask & mask_warped;
		mask_warped2 = seam_mask & mask_warped2; //seam_mask &

												 //imshow("maska", mask_warped)
		img_warped.convertTo(img_warped_s, CV_16S);
		img_warped.convertTo(img_warped_s2, CV_16S);
		img_warped.release();
		img.release();
		mask.release();

		if (!is_blender_prepared)
		{
			blender_->prepare(corners, sizes);
			blender_Origo->prepare(corners, sizes);

			is_blender_prepared = true;
		}
		/*if (number - 1 == img_idx) {

		//printf("rgb %d %d %d on coordinates %lf %lf\n", mask_warped.at<Vec2b>(pointsF)[0], mask_warped.at<Vec2b>(pointsF)[1], mask_warped.at<Vec2b>(pointsF)[2],pointsF.x,pointsF.y);
		//printf("mask size %dx%d\n", mask_warped.cols, mask_warped.rows);

		printf("coordinates %lf %lf\n", pointsF.x, pointsF.y);

		Point2f plusX = pointsF;

		plusX.x += 50;

		Point2f minusX = pointsF;

		minusX.x -= 50;

		Point2f plusY = pointsF;

		plusY.y += 50;

		Point2f minusY = pointsF;

		minusY.y -= 50;

		//string ty = type2str(mask_warped.type());
		//printf("Matrix: %s %dx%d \n", ty.c_str(), mask_warped.cols, mask_warped.rows);

		Mat rgb;
		cvtColor(mask_warped, rgb, CV_GRAY2BGR);

		//Scalar intensity = mask_warped.at<uchar>(Point(pointsF.y,pointsF.x));

		printf("rgb %d %d %d on coordinates %lf %lf\n", rgb.at<Vec3b>(pointsF)[0], rgb.at<Vec3b>(pointsF)[1], rgb.at<Vec3b>(pointsF)[2], pointsF.x, pointsF.y);
		printf("new rgb %d %d %d on coordinates %lf %lf\n", rgb.at<Vec3b>(plusX)[0], rgb.at<Vec3b>(plusX)[1], rgb.at<Vec3b>(plusX)[2], plusX.x, plusX.y);
		int test = 1;
		if (test == 0 && rgb.at<Vec3b>(pointsF)[0] == 255 && rgb.at<Vec3b>(pointsF)[1] == 255 && rgb.at<Vec3b>(pointsF)[2] == 255 && (rgb.at<Vec3b>(plusX)[0] == 255 && rgb.at<Vec3b>(plusX)[1] == 255 && rgb.at<Vec3b>(plusX)[2] == 255) && (rgb.at<Vec3b>(plusY)[0] == 255 && rgb.at<Vec3b>(plusY)[1] == 255 && rgb.at<Vec3b>(plusY)[2] == 255) && (rgb.at<Vec3b>(minusX)[0] == 255 && rgb.at<Vec3b>(minusX)[1] == 255 && rgb.at<Vec3b>(minusX)[2] == 255) && (rgb.at<Vec3b>(minusY)[0] == 255 && rgb.at<Vec3b>(minusY)[1] == 255 && rgb.at<Vec3b>(minusY)[2] == 255)) {

		printf("in white area\n");
		//	blender_Origo->feed(img_warped_s, mask_warped, corners[img_idx]);
		cv::circle(mask_warped, pointsF, 50, (0, 0, 0), -1);

		white = true;

		}

		else {

		printf("in black area\n");

		//blender_Origo->feed(img_warped_s, mask_warped, corners[img_idx]);

		white = false;

		movedFinal = pointsF;

		int quartal = 0;

		if (pointsF.x <= mask_warped.cols / 2 && pointsF.y <= mask_warped.rows / 2) {
		//printf("lavy horny\n");
		quartal = 1;
		}
		if (pointsF.x <= mask_warped.cols / 2 && pointsF.y > mask_warped.rows / 2) {
		//printf("lavy dolny\n");
		quartal = 3;
		}
		if (pointsF.x > mask_warped.cols / 2 && pointsF.y <= mask_warped.rows / 2) {
		//printf("pravy horny\n");
		quartal = 2;
		}
		if (pointsF.x > mask_warped.cols / 2 && pointsF.y > mask_warped.rows / 2) {
		//printf("pravy dolny\n");
		quartal = 4;
		}

		Point2f middle;
		middle.x = mask_warped.cols / 2;
		middle.y = mask_warped.rows / 2;

		cv::circle(mask_warped, middle, 50, (0, 0, 0), -1);

		movementX = pointsF.x - middle.x;
		movementY = pointsF.y - middle.y;
		}

		printf("movements %lf %lf\n", movementX, movementY);
		imwrite("./ImagesReal//mask.jpg", mask_warped);
		//imshow("mask feed", mask_warped);
		}
		else {
		//blender_Origo->feed(img_warped_s, mask_warped, corners[img_idx]);
		//imshow("mask origo", mask_warped);
		}*/
		// Blend the current image

		//blender_->feed(img_warped_s, mask_warped, corners[img_idx]);
		blender_Origo->feed(img_warped_s2, mask_warped2, corners[img_idx]);
	}

	Mat result, result_mask;
	//blender_->blend(result, result_mask);

	result.convertTo(panoO, CV_8U);

	Mat resultOrigo, result_maskOrigo;
	blender_Origo->blend(resultOrigo, result_maskOrigo);

	resultOrigo.convertTo(panoOrigo, CV_8U);

	Mat substract = panoOrigo - panoO;

	//imshow("original", substract);

	Mat src, src_gray;

	substract.copyTo(src);

	//cvtColor(src, src_gray, CV_BGR2GRAY);
	//GaussianBlur(src_gray, src_gray, Size(9, 9), 2, 2);
	//vector<Vec3f> circles;

	/// Apply the Hough Transform to find the circles
	//HoughCircles(src_gray, circles, CV_HOUGH_GRADIENT, 1, 41, 80, 34, 10, 170);

	/*/// Draw the circles detected
	for (size_t i = 0; i < circles.size(); i++)
	{
	printf("circle\n");
	Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
	int radius = cvRound(circles[i][2]);
	// circle center
	circle(src, center, 3, Scalar(0, 255, 0), -1, 8, 0);
	// circle outline
	circle(src, center, radius, Scalar(0, 0, 255), 3, 8, 0);
	}

	/// Show your results
	//namedWindow("Hough Circle Transform Demo", CV_WINDOW_AUTOSIZE);
	//imshow("Hough Circle Transform Demo", src);

	Point center(cvRound(circles[0][0]), cvRound(circles[0][1]));
	if (movementX == 0 && movementY == 0) {

	//circle(panoOrigo, center, 3, Scalar(0, 255, 0), -1, 8, 0);

	Point2f centerPlus;
	Point2f centerMinus;

	centerPlus.x = center.x + 100;
	centerPlus.y = center.y + 100;

	centerMinus.x = center.x - 100;
	centerMinus.y = center.y - 100;

	rectangle(panoOrigo, centerMinus, centerPlus, (0, 255, 0), 8);
	}
	else {

	center.x += movementX;
	center.y += movementY;

	//circle(panoOrigo, center, 3, Scalar(0, 255, 0), -1, 8, 0);

	Point2f centerPlus;
	Point2f centerMinus;

	centerPlus.x = center.x + 50;
	centerPlus.y = center.y + 50;

	centerMinus.x = center.x - 50;
	centerMinus.y = center.y - 50;

	rectangle(panoOrigo, centerMinus, centerPlus, (0, 255, 0), 2);
	}*/

	imwrite("./ImagesReal//Result.jpg", panoOrigo);
}

void MyStitcher::points() {

	vector<Mat> original(imgs_.size());

	for (int i = 0; i < imgs_.size(); i++) {

		original[i] = imgs_[i];

		for (int j = 1; j <= 8; j++) {

			Point2f a((original[i].cols / 9)*j, original[i].rows / 2);

			cv::circle(original[i], a, 8, (0, 0, 255), -1);
		}
	}

	imwrite("./ImagesReal//pointImage1.jpg", original[0]);
	imwrite("./ImagesReal//pointImage2.jpg", original[1]);

}

void MyStitcher::leftPoints() {

	Mat original = panoO;

	cv::circle(original, pointsFinal[1], 8, (0, 0, 255), -1);
	cv::circle(original, pointsFinal[2], 8, (0, 0, 255), -1);
	cv::circle(original, pointsFinal[3], 8, (0, 0, 255), -1);
	cv::circle(original, pointsFinal[4], 8, (0, 0, 255), -1);
	cv::circle(original, pointsFinal[5], 8, (0, 0, 255), -1);
	cv::circle(original, pointsFinal[6], 8, (0, 0, 255), -1);
	cv::circle(original, pointsFinal[7], 8, (0, 0, 255), -1);
	cv::circle(original, pointsFinal[8], 8, (0, 0, 255), -1);

	imwrite("./ImagesReal//resultPointsLeft.jpg", original);

}

void MyStitcher::pointsInWarp() {
	//nothing maybe
}


