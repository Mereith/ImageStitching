#include "precomp.hpp"

using namespace std;

vector<cv::Point2f> points(9);
vector<cv::Point2f> points4(4);

namespace cv {
	namespace detail {


		void ProjectorBa::setCameraParams(const Mat &K, const Mat &R, const Mat &T)
		{
			printf("asasdasd\n");
			CV_Assert(K.size() == Size(3, 3) && K.type() == CV_32F);
			CV_Assert(R.size() == Size(3, 3) && R.type() == CV_32F);
			CV_Assert((T.size() == Size(1, 3) || T.size() == Size(3, 1)) && T.type() == CV_32F);

			Mat_<float> K_(K);
			k[0] = K_(0, 0); k[1] = K_(0, 1); k[2] = K_(0, 2);
			k[3] = K_(1, 0); k[4] = K_(1, 1); k[5] = K_(1, 2);
			k[6] = K_(2, 0); k[7] = K_(2, 1); k[8] = K_(2, 2);

			Mat_<float> Rinv = R.t();
			rinv[0] = Rinv(0, 0); rinv[1] = Rinv(0, 1); rinv[2] = Rinv(0, 2);
			rinv[3] = Rinv(1, 0); rinv[4] = Rinv(1, 1); rinv[5] = Rinv(1, 2);
			rinv[6] = Rinv(2, 0); rinv[7] = Rinv(2, 1); rinv[8] = Rinv(2, 2);

			Mat_<float> R_Kinv = R * K.inv();
			r_kinv[0] = R_Kinv(0, 0); r_kinv[1] = R_Kinv(0, 1); r_kinv[2] = R_Kinv(0, 2);
			r_kinv[3] = R_Kinv(1, 0); r_kinv[4] = R_Kinv(1, 1); r_kinv[5] = R_Kinv(1, 2);
			r_kinv[6] = R_Kinv(2, 0); r_kinv[7] = R_Kinv(2, 1); r_kinv[8] = R_Kinv(2, 2);

			Mat_<float> K_Rinv = K * Rinv;
			k_rinv[0] = K_Rinv(0, 0); k_rinv[1] = K_Rinv(0, 1); k_rinv[2] = K_Rinv(0, 2);
			k_rinv[3] = K_Rinv(1, 0); k_rinv[4] = K_Rinv(1, 1); k_rinv[5] = K_Rinv(1, 2);
			k_rinv[6] = K_Rinv(2, 0); k_rinv[7] = K_Rinv(2, 1); k_rinv[8] = K_Rinv(2, 2);

			Mat_<float> T_(T.reshape(0, 3));
			t[0] = T_(0, 0); t[1] = T_(1, 0); t[2] = T_(2, 0);

		}


		Point2f PlaneWarper2::warpPoint(const Point2f &pt, const Mat &K, const Mat &R)
		{
			printf("WarpPoint\n");
			projector_.setCameraParams(K, R);
			Point2f uv;
			projector_.mapForward(pt.x, pt.y, uv.x, uv.y);
			return uv;
		}


		vector<Point2f> PlaneWarper2::setPoints() {
			return points;
		}

		vector<Point2f> PlaneWarper2::set4Points() {
			return points4;
		}

		Rect PlaneWarper2::buildMaps(Size src_size, const Mat &K, const Mat &R, Mat &xmap, Mat &ymap)
		{

			projector_.setCameraParams(K, R);

			bool je1 = true;
			bool je2 = true;
			bool je3 = true;
			bool je4 = true;
			bool je5 = true;
			bool je6 = true;
			bool je7 = true;
			bool je8 = true;

			Point dst_tl, dst_br;
			detectResultRoi(src_size, dst_tl, dst_br);
			//printf("y %d %d %d %d\n", dst_tl.x, dst_tl.y, dst_br.x, dst_br.y);
			xmap.create(dst_br.y - dst_tl.y + 1, dst_br.x - dst_tl.x + 1, CV_32F);
			ymap.create(dst_br.y - dst_tl.y + 1, dst_br.x - dst_tl.x + 1, CV_32F);

			float x, y;

			for (int v = dst_tl.y; v <= dst_br.y; ++v)
			{
				for (int u = dst_tl.x; u <= dst_br.x; ++u)
				{
					projector_.mapBackward(static_cast<float>(u), static_cast<float>(v), x, y);

					xmap.at<float>(v - dst_tl.y, u - dst_tl.x) = x;
					ymap.at<float>(v - dst_tl.y, u - dst_tl.x) = y;
					/*	if (je1) {
					if (((x < ((src_size.width / 9) * 1) + 1)) && ((x > ((src_size.width / 9) * 1) - 1)) && (y<((src_size.height / 2) + 1) && y>((src_size.height / 2) - 1))) {
					je1 = false;
					//printf("suradnice 1 %d %d\n", u - dst_tl.x, v - dst_tl.y);
					points[1].x = u - dst_tl.x;
					points[1].y = v - dst_tl.y;
					}
					}
					if (je2) {
					if (((x < ((src_size.width / 9) * 2) + 1)) && ((x > ((src_size.width / 9) * 2) - 1)) && (y<((src_size.height / 2) + 1) && y>((src_size.height / 2) - 1))) {
					je2 = false;
					//printf("suradnice 2 %d %d\n", u - dst_tl.x, v - dst_tl.y);
					points[2].x = u - dst_tl.x;
					points[2].y = v - dst_tl.y;
					}
					}
					if (je3) {
					if (((x < ((src_size.width / 9) * 3) + 1)) && ((x > ((src_size.width / 9) * 3) - 1)) && (y<((src_size.height / 2) + 1) && y>((src_size.height / 2) - 1))) {
					je3 = false;
					//printf("suradnice 3 %d %d\n", u - dst_tl.x, v - dst_tl.y);
					points[3].x = u - dst_tl.x;
					points[3].y = v - dst_tl.y;
					}
					}
					if (je4) {
					if (((x < ((src_size.width / 9) * 4) + 1)) && ((x > ((src_size.width / 9) * 4) - 1)) && (y<((src_size.height / 2) + 1) && y>((src_size.height / 2) - 1))) {
					je4 = false;
					//printf("suradnice 4 %d %d\n", u - dst_tl.x, v - dst_tl.y);
					points[4].x = u - dst_tl.x;
					points[4].y = v - dst_tl.y;
					}
					}
					if (je5) {
					if (((x < ((src_size.width / 9) * 5) + 1)) && ((x > ((src_size.width / 9) * 5) - 1)) && (y<((src_size.height / 2) + 1) && y>((src_size.height / 2) - 1))) {
					je5 = false;
					//printf("suradnice 5 %d %d\n", u - dst_tl.x, v - dst_tl.y);
					points[5].x = u - dst_tl.x;
					points[5].y = v - dst_tl.y;
					}
					}
					if (je6) {
					if (((x < ((src_size.width / 9) * 6) + 1)) && ((x > ((src_size.width / 9) * 6) - 1)) && (y<((src_size.height / 2) + 1) && y>((src_size.height / 2) - 1))) {
					je6 = false;
					//printf("suradnice 6 %d %d\n", u - dst_tl.x, v - dst_tl.y);
					points[6].x = u - dst_tl.x;
					points[6].y = v - dst_tl.y;
					}
					}
					if (je7) {
					if (((x < ((src_size.width / 9) * 7) + 1)) && ((x > ((src_size.width / 9) * 7) - 1)) && (y<((src_size.height / 2) + 1) && y>((src_size.height / 2) - 1))) {
					je7 = false;
					//printf("suradnice 7 %d %d\n", u - dst_tl.x, v - dst_tl.y);
					points[7].x = u - dst_tl.x;
					points[7].y = v - dst_tl.y;
					}
					}
					if (je8) {
					if (((x < ((src_size.width / 9) * 8) + 1)) && ((x > ((src_size.width / 9) * 8) - 1)) && (y<((src_size.height / 2) + 1) && y>((src_size.height / 2) - 1))) {
					je8 = false;
					//printf("suradnice 8 %d %d\n", u - dst_tl.x, v - dst_tl.y);
					points[8].x = u - dst_tl.x;
					points[8].y = v - dst_tl.y;
					}
					}*/

				}
			}

			return Rect(dst_tl, dst_br);
		}



		Point PlaneWarper2::warp(const Mat &src, const Mat &K, const Mat &R, int interp_mode, int border_mode,
			Mat &dst)
		{

			Mat xmap, ymap;
			Rect dst_roi = buildMaps(src.size(), K, R, xmap, ymap);

			dst.create(dst_roi.height + 1, dst_roi.width + 1, src.type());
			remap(src, dst, xmap, ymap, interp_mode, border_mode);

			return dst_roi.tl();
		}


		Rect PlaneWarper2::warpRoi(Size src_size, const Mat &K, const Mat &R)
		{

			projector_.setCameraParams(K, R);

			Point dst_tl, dst_br;
			detectResultRoi(src_size, dst_tl, dst_br);

			return Rect(dst_tl, Point(dst_br.x + 1, dst_br.y + 1));
		}


		void PlaneWarper2::detectResultRoi(Size src_size, Point &dst_tl, Point &dst_br)
		{
			detectResultRoiByBorder(src_size, dst_tl, dst_br);

			float tl_uf = static_cast<float>(dst_tl.x);
			float tl_vf = static_cast<float>(dst_tl.y);
			float br_uf = static_cast<float>(dst_br.x);
			float br_vf = static_cast<float>(dst_br.y);

			float x = projector_.rinv[1];
			float y = projector_.rinv[4];
			float z = projector_.rinv[7];
			if (y > 0.f)
			{
				float x_ = (projector_.k[0] * x + projector_.k[1] * y) / z + projector_.k[2];
				float y_ = projector_.k[4] * y / z + projector_.k[5];
				if (x_ > 0.f && x_ < src_size.width && y_ > 0.f && y_ < src_size.height)
				{
					tl_uf = min(tl_uf, 0.f); tl_vf = min(tl_vf, static_cast<float>(CV_PI * projector_.scale));
					br_uf = max(br_uf, 0.f); br_vf = max(br_vf, static_cast<float>(CV_PI * projector_.scale));
				}
			}

			x = projector_.rinv[1];
			y = -projector_.rinv[4];
			z = projector_.rinv[7];
			if (y > 0.f)
			{
				float x_ = (projector_.k[0] * x + projector_.k[1] * y) / z + projector_.k[2];
				float y_ = projector_.k[4] * y / z + projector_.k[5];
				if (x_ > 0.f && x_ < src_size.width && y_ > 0.f && y_ < src_size.height)
				{
					tl_uf = min(tl_uf, 0.f); tl_vf = min(tl_vf, static_cast<float>(0));
					br_uf = max(br_uf, 0.f); br_vf = max(br_vf, static_cast<float>(0));
				}
			}

			dst_tl.x = static_cast<int>(tl_uf);
			dst_tl.y = static_cast<int>(tl_vf);
			dst_br.x = static_cast<int>(br_uf);
			dst_br.y = static_cast<int>(br_vf);

			/*float tl_uf = numeric_limits<float>::max(); //maximalne a minimalne floaty
			float tl_vf = numeric_limits<float>::max();
			float br_uf = -numeric_limits<float>::max();
			float br_vf = -numeric_limits<float>::max();

			float u, v;

			projector_.mapForward(0, 0, u, v);
			tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
			br_uf = max(br_uf, u); br_vf = max(br_vf, v);

			projector_.mapForward(0, static_cast<float>(src_size.height - 1), u, v);
			tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
			br_uf = max(br_uf, u); br_vf = max(br_vf, v);

			projector_.mapForward(static_cast<float>(src_size.width - 1), 0, u, v);
			tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
			br_uf = max(br_uf, u); br_vf = max(br_vf, v);

			projector_.mapForward(static_cast<float>(src_size.width - 1), static_cast<float>(src_size.height - 1), u, v);
			tl_uf = min(tl_uf, u); tl_vf = min(tl_vf, v);
			br_uf = max(br_uf, u); br_vf = max(br_vf, v);


			projector_.mapForward(static_cast<float>(300), static_cast<float>(300), u, v);
			//printf("points: %lf %lf\n", u, v);

			dst_tl.x = static_cast<int>(tl_uf);
			dst_tl.y = static_cast<int>(tl_vf);
			dst_br.x = static_cast<int>(br_uf);
			dst_br.y = static_cast<int>(br_vf);*/

			printf("4 body %d %d %d %d\n", dst_tl.x, dst_tl.y, dst_br.x, dst_br.y);
			points4[0].x = dst_tl.x;
			points4[0].y = dst_tl.y;
			points4[1].x = dst_br.x;
			points4[1].y = dst_br.y;
		}

	} // namespace detail
} // namespace cv