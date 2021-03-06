// DIP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <math.h> 

#pragma region CONSTANTS


#define M_PI 3.14159265358979323846

enum FILTER_TYPE {BOX3X3, GAUSSIAN3X3, GAUSSIAN5X5};

int box_filter_template3x3[3][3] = {
   {1, 1, 1} ,
   {1, 1, 1} ,
   {1, 1, 1}
};

int gaussian_filter_template3x3[3][3] = {
   {1, 2, 1} ,
   {2, 4, 2} ,
   {1, 2, 1}
};

int gaussian_filter_template5x5[5][5] = {
   {1, 4, 6, 4, 1} ,
   {4,16, 24, 16,4} ,
   {6,24, 36, 24, 6},
   {4,16, 24, 16,4} ,
   {1, 4, 6, 4, 1} 
};

int K1 = 3, K2 = 1;
cv::Mat sourceImg;
cv::Mat fixedDisortion;
#pragma endregion

#pragma region  HELP_FUNCTIONS
cv::Vec3b blurFilter(cv::Mat srcImage, int x, int y, FILTER_TYPE type, int filter_size) {

	

	int rows = filter_size;
	int cols = filter_size;


	int colors[3] = { 0,0,0 };
	int dividerSum = 0;

	int x_start = x - (filter_size / 2);
	int y_start = y - (filter_size / 2);

	cv::Vec3d result = 0;

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			int matrixVal = 0;
			switch (type)
			{
			case BOX3X3:
				matrixVal = box_filter_template3x3[i][j];
				break;
			case GAUSSIAN3X3:
				 matrixVal = gaussian_filter_template3x3[i][j];
				break;
			case GAUSSIAN5X5:
				matrixVal = gaussian_filter_template5x5[i][j];
				break;
			default:
				break;
			}
			dividerSum += matrixVal;
			cv::Vec3d pixelVal = srcImage.at<cv::Vec3b>(cv::Point(x_start, y_start));
			cv::Vec3d data = ( pixelVal * matrixVal);
			for (int k = 0; k < 3; k++)
			{
				colors[k] += data[k];
			}

			y_start += 1;
		}
		y_start = y - (filter_size / 2);
		x_start += 1;
	}
	float divider = dividerSum;
	//int divider
	result = cv::Vec3b(colors[0]/divider, colors[1] / divider, colors[2] / divider);

	//std::cout << "x:  " << x_start << "  y: " << y_start  << "    0:  "<< colors[0] / 9 << "  1: " << colors[1] / 9 << "  2:  " << colors[2] / 9 << "\n";
	return result;

}

void flipQuadrants(cv::Mat &source) {
	// Get half of image rows and cols
	int hRows = source.rows / 2;
	int hCols = source.cols / 2;

	for (int y = 0; y < hRows; y++) {
		for (int x = 0; x < hCols; x++) {
			// Extract px from each quadrant
			double tl = source.at<double>(y, x); // top left px
			double tr = source.at<double>(y, x + hCols); // top right px
			double bl = source.at<double>(y + hRows, x); // bottom left px
			double br = source.at<double>(y + hRows, x + hCols); // bottom right px

			// Compose image
			source.at<double>(y, x) = br;
			source.at<double>(y, x + hCols) = bl;
			source.at<double>(y + hRows, x) = tr;
			source.at<double>(y + hRows, x + hCols) = tl;
		}
	}
}

double anisotropic(cv::Mat srcImage, int x, int y) {
	double sigma = 0.015;
	double delta = 0.1;

	double sigmaPowered = pow(sigma, 2);

	double c_c_value = srcImage.at<double>(x, y);
	double i_n = srcImage.at<double>(x, y - 1);
	double i_s = srcImage.at<double>(x, y + 1);
	double i_w = srcImage.at<double>(x - 1, y);
	double i_e = srcImage.at<double>(x + 1, y);

	double c_s_value = i_s - c_c_value;
	double c_n_value = i_n - c_c_value;
	double c_w_value = i_w - c_c_value;
	double c_e_value = i_e - c_c_value;

	double p = pow(c_s_value, 2);
	double p1 = p / sigmaPowered;
	double c_s = exp2(-p1);
	double c_n = exp2(-(pow(c_n_value, 2) / sigmaPowered));
	double c_w = exp2(-(pow(c_w_value, 2) / sigmaPowered));
	double c_e = exp2(-(pow(c_e_value, 2) / sigmaPowered));


	double result = (c_c_value * (1 - delta * (c_s + c_n + c_w + c_e))) + (delta * ((c_s*i_s) + (c_n * i_n) + (c_e * i_e) + (c_w * i_w)));
	return result;
}


cv::Mat dft(cv::Mat &source) {
	cv::Mat normalized;

	int R = source.rows;
	int C = source.cols;
	double sqrtMN_d = 1.0 / sqrt(R * C);
	double PIPI = M_PI * 2;
	double sumR = 0, sumI = 0;
	double M_d = 1.0 / R, N_d = 1.0 / C;

	source.convertTo(normalized, CV_64FC1, 1.0 / 255.0 * sqrtMN_d);
	cv::Mat furier = cv::Mat(R, C, CV_64FC2);

	for (int k = 0; k < R; k++) {
		for (int l = 0; l < C; l++) {
			sumR = sumI = 0;

			for (int m = 0; m < R; m++) {
				for (int n = 0; n < C; n++) {
					double px = normalized.at<double>(m, n);
					double expX = -PIPI * (m * k * M_d + n * l * N_d);

					sumR += px * cos(expX);
					sumI += px * sin(expX);
				}
			}

			furier.at<cv::Vec2d>(k, l) = cv::Vec2d(sumR, sumI);
		}
	}

	return furier;
}

cv::Mat idft(cv::Mat &furier) {

	int R = furier.rows;
	int C = furier.cols;
	double sqrtMN_d = 1.0 / sqrt(R * C);
	double PIPI = M_PI * 2;
	double sum = 0;
	double M_d = 1.0 / R, N_d = 1.0 / C;

	cv::Mat dest = cv::Mat(R, C, CV_64FC1);

	for (int i = 0; i < R; i++) {
		for (int j = 0; j < C; j++) {
			sum = 0;
			for (int k = 0; k < R; k++) {
				for (int l = 0; l < C; l++) {
					double expX = PIPI * ((k * i * M_d) + (l * j * N_d));

					double realX = cos(expX) * sqrtMN_d;
					double imagX = sin(expX) * sqrtMN_d;

					// Get furier transform
					cv::Vec2d F = furier.at<cv::Vec2d>(k, l);
					
					sum += F[0] * realX - F[1] * imagX;
				}
			}

			dest.at<double>(i, j) = sum;
		}
	}

	cv::normalize(dest, dest, 0.0, 1.0, CV_MINMAX);
	return dest;
}

cv::Mat dftPower(cv::Mat &furier) {
	int M = furier.rows;
	int N = furier.cols;

	// Create result matrix
	cv::Mat dest = cv::Mat(M, N, CV_64FC1);

	for (int m = 0; m < M; m++) {
		for (int n = 0; n < N; n++) {
			cv::Vec2d F = furier.at<cv::Vec2d>(m, n);
			double amp = sqrt(F[0]*F[0] + F[1]*F[1]);
			dest.at<double>(m, n) = log(amp*amp);
		}
	}
	cv::normalize(dest, dest, 0.0, 1.0, CV_MINMAX);
	flipQuadrants(dest);
	return dest;
}

cv::Mat dftPhase(cv::Mat &furier) {
	int M = furier.rows;
	int N = furier.cols;

	cv::Mat dest = cv::Mat(M, N, CV_64FC1);

	for (int m = 0; m < M; m++) {
		for (int n = 0; n < N; n++) {
			cv::Vec2d F = furier.at<cv::Vec2d>(m, n);
			dest.at<double>(m, n) = atan(F[1] / F[0]);
		}
	}

	return dest;
}

void changeQuadrants(cv::Mat &source) {
	int rows = source.rows / 2;
	int cols = source.cols / 2;

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			cv::Vec2d tl = source.at<cv::Vec2d>(y, x);
			cv::Vec2d tr = source.at<cv::Vec2d>(y, x + cols); 
			cv::Vec2d bl = source.at<cv::Vec2d>(y + rows, x); 
			cv::Vec2d br = source.at<cv::Vec2d>(y + rows, x + cols);

			// Compose image
			source.at<cv::Vec2d>(y, x) = br;
			source.at<cv::Vec2d>(y, x + cols) = bl;
			source.at<cv::Vec2d>(y + rows, x) = tr;
			source.at<cv::Vec2d>(y + rows, x + cols) = tl;
		}
	}
}

cv::Mat dftFilter(cv::Mat &source, cv::Mat &mask) {
	cv::Mat furier = dft(source);

	changeQuadrants(furier);

	for (int y = 0; y < furier.rows; y++) {
		for (int x = 0; x < furier.cols; x++) {
			cv::Vec2d &fPx = furier.at<cv::Vec2d>(y, x);
			uchar mPx = mask.at<uchar>(y, x);

			if (mPx == 0) {
				fPx[0] = 0;
				fPx[1] = 0;
			}
		}
	}
	changeQuadrants(furier);

	cv::Mat dest = idft(furier);
	return dest;

}



cv::Vec3b bilinearInterpolation(cv::Mat &src, cv::Point2d point) {
	// Get adjacent pixel coordinates
	double x1 = floor(point.x);
	double y1 = floor(point.y);
	double x2 = ceil(point.x);
	double y2 = ceil(point.y);

	cv::Vec3b f_00 = src.at<cv::Vec3b>(y1, x1);
	cv::Vec3b f_01 = src.at<cv::Vec3b>(y1, x2);
	cv::Vec3b f_10 = src.at<cv::Vec3b>(y2, x1);
	cv::Vec3b f_11 = src.at<cv::Vec3b>(y2, x2);

	// Move x and y into <0,1> square
	double x = point.x - x1;
	double y = point.y - y1;

	// Interpolate
	return (f_00 * (1 - x) * (1 - y)) + (f_01 * x * (1 - y)) + (f_10 * (1 - x) * y) + (f_11 * x * y);
}

void geom_dist(cv::Mat &src, cv::Mat &dst, bool bili, double K1 = 1.0, double K2 = 1.0) {
	int cu = src.cols / 2;
	int cv = src.rows / 2;
	double R = sqrt((cu*cu) + (cv*cv));

	for (int y_n = 0; y_n < src.rows; y_n++) {
		for (int x_n = 0; x_n < src.cols; x_n++) {
			double x_ = (x_n - cu) / R;
			double y_ = (y_n - cv) / R;
			double r_2 = (x_*x_) + (y_*y_);
			double theta = 1 + K1 * r_2 + K2 * (r_2* r_2);

			double x_d = (x_n - cu) * (1 / theta) + cu;
			double y_d = (y_n - cv) * (1 / theta) + cv;

			// Interpolate
			dst.at<cv::Vec3b>(y_n, x_n) = bili ? bilinearInterpolation(src, cv::Point2d(x_d, y_d)) : src.at<cv::Vec3b>(y_d, x_d);
		}
	}
}

void on_trackbar_change(int id, void*) {
	geom_dist(sourceImg, fixedDisortion, true, K1 / 100.0, K2 / 100.0);
	if (fixedDisortion.data) {
		cv::imshow("Geom Dist", fixedDisortion);
	}
}

cv::Mat calcHistogram(cv::Mat &src) {
	cv::Mat hist = cv::Mat::zeros(256, 1, CV_32FC1);

	for (int y = 0; y < src.rows; y++) {
		for (int x = 0; x < src.cols; x++) {
			uchar &srcPx = src.at<uchar>(y, x);
			float &histPx = hist.at<float>(srcPx);
			histPx++;
		}
	}

	return hist;
}

cv::Mat drawHistogram(cv::Mat &hist) {
	cv::Mat histNorm;
	cv::normalize(hist, histNorm, 0.0f, 255.0f, CV_MINMAX);
	cv::Mat dst = cv::Mat::zeros(256,256 , CV_8UC1);

	for (int x = 0; x < dst.cols; x++) {
		int vertical = cv::saturate_cast<int>(histNorm.at<float>(x));
		for (int y = 255; y > (256 - vertical - 1); y--) {
			dst.at<uchar>(y, x) = 255;
		}
	}

	return dst;
}

cv::Mat calcCdf(cv::Mat &hist) {
	cv::Mat cdfInt;
	cv::integral(hist, cdfInt, CV_32F);
	return cdfInt(cv::Rect(1, 1, 1, hist.rows ));
}

int cdfMin(cv::Mat &cdf) {
	int min = 1;
	for (int i = 0; i < cdf.rows; i++) {
		float newMin = cdf.at<float>(i);
		if (newMin < min) 
		{ 
			min = (int)newMin;
		}
	}

	return min;
}

void fillMatrix(double in[8][9], cv::Mat &leftT, cv::Mat &rightT) {
	leftT = cv::Mat(8, 8, CV_64F);
	rightT = cv::Mat(8, 1, CV_64F);

	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			leftT.at<double>(y, x) = in[y][x];
		}

		rightT.at<double>(y) = -in[y][8];
	}
}

void fillP3x3(cv::Mat &in, cv::Mat &out) {
	out = cv::Mat(3, 3, CV_64F);

	for (int i = 0; i < 9; i++) {
		if (i == 0) {
			out.at<double>(i) = 1;
			continue;
		}

		out.at<double>(i) = in.at<double>(i - 1);
	}
}

inline cv::Mat fillXYT(cv::Mat &in, int x, int y) {
	if (in.rows == 0) {
		in = cv::Mat(3, 1, CV_64F);
	}

	in.at<double>(0) = x;
	in.at<double>(1) = y;
	in.at<double>(2) = 1;

	return in;
}

cv::Mat genInput(int rows, int cols) {
	cv::Mat input = cv::Mat::zeros(rows, cols, CV_64FC1);

	// Fill input matrix
	cv::circle(input, cv::Point(cols / 2, rows / 2), 60, 1.0, 1);
	cv::rectangle(input, cv::Rect((cols / 2), (rows / 2), 40, 40), 1.0, -1);

	return input;
}

void genProjections(cv::Mat &input, cv::Mat tpls[]) {
	// Get center point
	cv::Mat inputRotated, inputBackup;
	cv::Point2f center(input.rows / 2.0f, input.cols / 2.0f);

	for (int i = 0; i < 360; i++) {
		// Get rotation matrix
		inputBackup = input.clone();
		cv::Mat rotationMat = cv::getRotationMatrix2D(center, 1 * i, 1.0f);
		cv::warpAffine(inputBackup, inputRotated, rotationMat, input.size());
		tpls[i] = cv::Mat::zeros(input.rows, input.cols, CV_64FC1);

		// Create slices
		for (int y = 0; y < inputRotated.rows; y++) {
			for (int x = 0; x < inputRotated.cols; x++) {
				tpls[i].at<double>(y, 0) += inputRotated.at<double>(y, x);
			}
		}

		// Copy slices to other cols
		for (int k = 1; k < tpls[i].cols; k++) {
			tpls[i].col(0).copyTo(tpls[i].col(k));
		}
	};
}

#pragma endregion

#pragma  region FUNCTIONS

void Gamma(float gamma) {
	cv::Mat src_8uc1_img = cv::imread("images/lena.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat gray_32fc1_img;
	src_8uc1_img.convertTo(gray_32fc1_img, CV_32FC1, 1.0 / 255.0);

	for (int y = 0; y < gray_32fc1_img.rows; y++) {
		for (int x = 0; x < gray_32fc1_img.cols; x++) {
			gray_32fc1_img.at<float>(y, x) = 255 * pow(gray_32fc1_img.at<float>(y, x), gamma);
		}
	}
	cv::Mat result;
	gray_32fc1_img.convertTo(result, CV_8UC1, 1);
	cv::imshow("Source ", src_8uc1_img);
	cv::imshow("Gamma ", result);
}

void BlurFilter() {
	int filter_size = 0;
	FILTER_TYPE type = GAUSSIAN5X5;
	switch (type)
	{
	case BOX3X3:
		filter_size = 3;
		break;
	case GAUSSIAN3X3:
		filter_size = 3;
		break;
	case GAUSSIAN5X5:
		filter_size = 5;
		break;
	default:
		break;
	}
	int border = filter_size / 2;
	cv::Mat src_8uc1_img = cv::imread("images/lena.png", CV_LOAD_IMAGE_COLOR);
	cv::Mat gray_32fc1_img;
	src_8uc1_img.convertTo(gray_32fc1_img, CV_8UC3);
	int rows = src_8uc1_img.rows - border - 1;
	int cols = src_8uc1_img.cols - border - 1;
	for (int x = border; x < rows; x++) {
		for (int y = border; y < cols; y++) {
			cv::Vec3b resPixel = blurFilter(src_8uc1_img, x, y, type, filter_size);
			gray_32fc1_img.at<cv::Vec3b>(cv::Point(x, y)) = resPixel;
		}
	}
	cv::Mat result;
	gray_32fc1_img.convertTo(result, CV_8UC1, 1);
	cv::imshow("Source ", src_8uc1_img);
	cv::imshow("Result filter ", result);
	cv::waitKey(0);
}

cv::Mat Convolution(cv::Mat src, cv::Mat mask, float normal)
{
	cv::Mat gray_64fc1_img;
	src.convertTo(gray_64fc1_img, CV_32FC1, 1.0 / 255, 0);

	for (int y = 0; y < gray_64fc1_img.rows; y++)
	{

		for (int x = 0; x < gray_64fc1_img.cols; x++)
		{
			float value = 0;
			for (int i = 0; i < (mask.rows); i++)
			{
				for (int j = 0; j < (mask.cols); j++)
				{
					if (y + i >= 0 && x + j >= 0 && y + i < gray_64fc1_img.rows && x + j < src.cols)
					{
						value += (gray_64fc1_img.at<float>(y + i, x + j) * mask.at<float>(i, j));

					}

				}

			}
			gray_64fc1_img.at<float>(y, x) = value * normal;
		}
	}

	return gray_64fc1_img;
}

void AnisotropicFilter() {
	cv::Mat src_32fc1_img_lena = cv::imread("images/valve.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat src_32fc1_img;
	src_32fc1_img_lena.convertTo(src_32fc1_img, CV_64F, 1.0 / 255.0);
	cv::Mat gray_32fc1_img;
	src_32fc1_img.convertTo(gray_32fc1_img, CV_64F, 1.0 / 255.0);
	int rows = src_32fc1_img.rows - 1;
	int cols = src_32fc1_img.cols - 1;

	for (int i = 0; i < 30; i++)
	{
		for (int x = 1; x < rows; x++) {
			for (int y = 1; y < cols; y++) {
				double resPixel = anisotropic(src_32fc1_img, x, y);
				gray_32fc1_img.at<double>(cv::Point(y, x)) = resPixel;
				//std::cout << x  << " y: " << y  << "\n";
			}
		}

		std::cout << i << "\n";
		src_32fc1_img = gray_32fc1_img;
	}

	cv::imshow("Source ", src_32fc1_img_lena);
	cv::imshow("Result anisotropic  ", gray_32fc1_img);
	cv::waitKey(0);
}

void FurierTransformation() {
	cv::Mat src_8uc1_img = cv::imread("images/lena64.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat gray_32fc1_img;
	src_8uc1_img.convertTo(gray_32fc1_img, CV_64FC1, 1.0 / 255.0);

	cv::Mat fourier = dft(gray_32fc1_img);
	cv::Mat power = dftPower(fourier);
	cv::Mat phase = dftPhase(fourier);


	cv::resize(src_8uc1_img, src_8uc1_img, cv::Size(), 3, 3);
	cv::resize(power, power, cv::Size(), 3, 3);
	cv::resize(phase, phase, cv::Size(), 3, 3);

	cv::imshow("Source", src_8uc1_img);
	cv::imshow("Discrete fourier transformation - POWER ", power);
	cv::imshow("Discrete fourier transformation - PHASE ", phase);


	cv::waitKey(0);
}

void InverseFurierTransformation() {
	cv::Mat src_8uc1_img = cv::imread("images/lena64.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat gray_32fc1_img;
	src_8uc1_img.convertTo(gray_32fc1_img, CV_64FC1, 1.0 / 255.0);

	cv::Mat fourier = dft(gray_32fc1_img);

	cv::Mat inversefourier = idft(fourier);


	cv::resize(src_8uc1_img, src_8uc1_img, cv::Size(), 3, 3);
	cv::resize(inversefourier, inversefourier, cv::Size(), 3, 3);

	cv::imshow("Source", src_8uc1_img);
	cv::imshow("Inverse discrete fourier transformation  ", inversefourier);


	cv::waitKey(0);
}

void DFTFiltering() {
	cv::Mat src_8uc1_img_noise = cv::imread("images/lena64_noise.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat src_8uc1_img_noise_2 = cv::imread("images/lena64_noise.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat src_8uc1_img_noise_lines = cv::imread("images/lena64_bars.png", CV_LOAD_IMAGE_GRAYSCALE);
	int rows = src_8uc1_img_noise.rows;
	int cols = src_8uc1_img_noise.cols;

	cv::Mat gray_32fc1_img;
	// Create Filters
	cv::Mat filter_lowPass = cv::Mat::zeros(64, 64, CV_8UC1);
	cv::Mat filter_bars = cv::Mat::ones(64, 64, CV_8UC1) * 255;
	cv::Mat filter_highPass = cv::Mat::ones(64, 64, CV_8UC1) * 255;

	cv::circle(filter_lowPass, cv::Point(rows / 2, cols / 2), 20, 255, -1);
	cv::circle(filter_highPass, cv::Point(rows / 2, cols / 2), 8, 0, -1);

	for (int i = 0; i < 64; i++) {
		// create  individual bar filter
		if (i > 24 && i <40) continue;
		filter_bars.at<uchar>(32, i) = 0;
	}

	cv::Mat res_lowPass =  dftFilter(src_8uc1_img_noise, filter_lowPass);
	cv::Mat res_high_pass =  dftFilter(src_8uc1_img_noise_2, filter_highPass);
	cv::Mat res_lines =  dftFilter(src_8uc1_img_noise_lines, filter_bars);


	//cv::resize(filter_lowPass, filter_lowPass, cv::Size(), 3, 3);
	//cv::resize(filter_highPass, filter_highPass, cv::Size(), 3, 3);
	//cv::resize(filter_bars, filter_bars, cv::Size(), 3, 3);

	//cv::imshow("Low Pass filter", filter_lowPass);
	//cv::imshow("High Pass filter", filter_highPass);
	//cv::imshow("Bars filter", filter_bars);

	cv::resize(res_lowPass, res_lowPass, cv::Size(), 3, 3);
	cv::resize(res_high_pass, res_high_pass, cv::Size(), 3, 3);
	cv::resize(res_lines, res_lines, cv::Size(), 3, 3);

	cv::imshow("Low Pass", res_lowPass);
	cv::imshow("High Pass", res_high_pass);
	cv::imshow("Lines", res_lines);

	cv::waitKey(0);

}


void Disortion()
{
	sourceImg = cv::imread("images/distorted_window.jpg", CV_LOAD_IMAGE_COLOR);


	std::string windowNameOriginal = "Original Image";
	cv::namedWindow(windowNameOriginal);
	cv::imshow(windowNameOriginal, sourceImg);

	sourceImg.copyTo(fixedDisortion);
	geom_dist(sourceImg, fixedDisortion, true, K1 / 100.0, K2 / 100.0);

	std::string windowNameDist = "Geom Dist";
	cv::namedWindow(windowNameDist);
	cv::imshow(windowNameDist, fixedDisortion);

	cv::createTrackbar("K1", windowNameDist, &K1, 1000, on_trackbar_change);
	cv::createTrackbar("K2", windowNameDist, &K2, 1000, on_trackbar_change);

	cv::waitKey(0);
}

void Histogram() {
	cv::Mat src_8uc1_img, src_32fc1_hist, src_32fc1_cdf;
	src_8uc1_img = cv::imread("images/uneq.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat dst_32fc1_normed = cv::Mat::zeros(src_8uc1_img.rows, src_8uc1_img.cols, CV_32FC1);

	int rows = src_8uc1_img.rows;
	int cols = src_8uc1_img.cols;
	int WxH = rows * cols;
	src_32fc1_hist = calcHistogram(src_8uc1_img);
	src_32fc1_cdf = calcCdf(src_32fc1_hist);
		float min = cdfMin(src_32fc1_cdf);

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {
			int srcPx = cv::saturate_cast<int>(src_8uc1_img.at<uchar>(y, x));
			float cdf = src_32fc1_cdf.at<float>(srcPx);

			dst_32fc1_normed.at<float>(y, x) = roundf(((cdf - min) / (WxH - min)) * 255);
		}
	}

	cv::normalize(dst_32fc1_normed, dst_32fc1_normed, 0.0f, 1.0f, CV_MINMAX);

	cv::imshow("Source", src_8uc1_img);
	cv::imshow("Result", dst_32fc1_normed);
	cv::imshow("Histogram src", drawHistogram(src_32fc1_hist));
	cv::imshow("Histogram result", drawHistogram(src_32fc1_cdf));

	cv::waitKey(0);
}

void Flag() {
	cv::Mat src_8uc3_flag = cv::imread("images/flag.png", CV_LOAD_IMAGE_COLOR);
	cv::Mat src_8uc3_result = cv::imread("images/vsb.jpg", CV_LOAD_IMAGE_COLOR);
	cv::Mat dst_8uc3_result = src_8uc3_result.clone();

	// matrix
	double transformData[8][9] = {
			{107, 1, 0,   0,   0, 0,          0,          0,    69},
			{0,   0, 69,  107, 1, 0,          0,          0,    0},
			{76,  1, 0,   0,   0, -323 * 227, -323 * 76,  -323, 227},
			{0,   0, 227, 76,  1, 0,          0,          0,    0},
			{122, 1, 0,   0,   0, -323 * 228, -323 * 122, -323, 228},
			{0,   0, 228, 122, 1, -215 * 228, -215 * 122, -215, 0},
			{134, 1, 0,   0,   0, 0,          0,          0,    66},
			{0,   0, 66,  134, 1, -215 * 66,  -215 * 134, -215, 0},
	};

	cv::Mat leftT, rightT;
	fillMatrix(transformData, leftT, rightT);
	cv::Mat vecBuilding, vecFlag;
	cv::Mat resultP, P3x3;
	cv::solve(leftT, rightT, resultP);
	fillP3x3(resultP, P3x3);

	for (int y = 0; y < src_8uc3_result.rows; y++) {
		for (int x = 0; x < src_8uc3_result.cols; x++) {
			fillXYT(vecBuilding, x, y);
			vecFlag = P3x3 * vecBuilding;
			vecFlag /= vecFlag.at<double>(2);
			int px = cv::saturate_cast<int>(vecFlag.at<double>(0));
			int py = cv::saturate_cast<int>(vecFlag.at<double>(1));

			if (px > 0 && py > 0 && px < src_8uc3_flag.cols && py < src_8uc3_flag.rows) {
				src_8uc3_result.at<cv::Vec3b>(y, x) = src_8uc3_flag.at<cv::Vec3b>(py, px);
			}
		}
	}

	cv::imshow("Result", src_8uc3_result);
	cv::waitKey(0);
}

void project(cv::Mat tpls[], cv::Mat &out) {
	// Get center point
	cv::Rect cropRect(100, 100, 400, 400);
	cv::Mat rotated, tmTpl, tmp;
	cv::Point2f center(tpls[0].rows / 2.0f, tpls[0].cols / 2.0f);
	out = cv::Mat::zeros(tpls[0].rows, tpls[0].cols, CV_64FC1);

	for (int i = 0; i < 360; i++) {
		// Get rotation matrix
		cv::Mat rotationMat = cv::getRotationMatrix2D(center, -1 * i, 1.0f);
		cv::warpAffine(tpls[i], rotated, rotationMat, tpls[i].size());

		for (int y = 0; y < out.rows; y++) {
			for (int x = 0; x < out.cols; x++) {
				out.at<double>(y, x) += rotated.at<double>(y, x);
			}
		}

		// Show projections
		cv::normalize(out, tmp, 0.0, 1.0, CV_MINMAX);
		cv::normalize(rotated, tmTpl, 0.0, 1.0, CV_MINMAX);

		// Crop image
		tmp = tmp(cropRect);
		tmTpl = tmTpl(cropRect);

		// Show projections
		cv::imshow("Projected", tmp);
		cv::imshow("Projected Tpl", tmTpl);
		cv::waitKey(1);
	}

	// Normalize output
	out = out(cropRect);
	cv::normalize(out, out, 0.0, 1.0, CV_MINMAX);
}


void DoProjection() {
	cv::Mat out;
	cv::Mat input = genInput(600, 600);
	cv::Mat tpls[360];

	// Show input
	cv::imshow("Input", input(cv::Rect(100, 100, 400, 400)));
	cv::waitKey(0);

	// Generate projections
	genProjections(input, tpls);
	project(tpls, out);

	// Show results
	cv::imshow("Projected", out);

	cv::waitKey(0);
}


void CannyEdgeDetector(cv::Mat Gy, cv::Mat Gx)
{
	cv::Mat G = Gx.clone();
	//for ()
	for (int y = 0; y < Gx.rows; y++)
	{
		for (int x = 0; x < Gx.rows; x++)
		{

			G.at<float>(y, x) = sqrt(pow(Gx.at<float>(y, x), 2) + pow(Gy.at<float>(y, x), 2));
		}

	}

	imshow("Edge detector", G);
	cv::waitKey(0);

}


void EdgeDetector() {
	cv::Mat GyMask = (cv::Mat_<float>(3, 3) << 1, 2, 1, 0, 0, 0, -1, -2, -1);
	float normalGy = 1.0 / 9.0;
	cv::Mat GxMask = (cv::Mat_<float>(3, 3) << 1, 0, -1, 2, 0, -2, 1, 0, -1);
	float normalGx = 1.0 / 9.0;
	cv::Mat img = cv::imread("images/lena.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat Gy = Convolution(img, GyMask, normalGy);
	cv::Mat Gx = Convolution(img, GxMask, normalGx);
	CannyEdgeDetector(Gy, Gx);
}

#pragma endregion

int main()
{
	// cviko 1 
	//Gamma(0.4);

	// cviko 2 
	//BlurFilter();

	// cviko 3 
	//AnisotropicFilter();

	// cviko 4
	//FurierTransformation();

	// cviko 5
	//InverseFurierTransformation();

	//cviko 6 
	//DFTFiltering();

	//cviko 7 
	//Disortion();

	//cviko 8
	//Histogram();

	//cviko 9 
	//Flag();

	//cviko 10
	//DoProjection();

	//cviko 11
	EdgeDetector();





	//cv::Mat src_8uc3_img = cv::imread("images/lena.png", CV_LOAD_IMAGE_COLOR); // load color image from file system to Mat variable, this will be loaded using 8 bits (uchar)

	//if (src_8uc3_img.empty()) {
	//	printf("Unable to read input file (%s, %d).", __FILE__, __LINE__);
	//}

	////cv::imshow( "LENA", src_8uc3_img );

	//cv::Mat gray_8uc1_img; // declare variable to hold grayscale version of img variable, gray levels wil be represented using 8 bits (uchar)
	//cv::Mat gray_32fc1_img; // declare variable to hold grayscale version of img variable, gray levels wil be represented using 32 bits (float)

	//cv::cvtColor(src_8uc3_img, gray_8uc1_img, CV_BGR2GRAY); // convert input color image to grayscale one, CV_BGR2GRAY specifies direction of conversion
	//gray_8uc1_img.convertTo(gray_32fc1_img, CV_32FC1, 1.0 / 255.0); // convert grayscale image from 8 bits to 32 bits, resulting values will be in the interval 0.0 - 1.0

	//int x = 10, y = 15; // pixel coordinates

	//uchar p1 = gray_8uc1_img.at<uchar>(y, x); // read grayscale value of a pixel, image represented using 8 bits
	//float p2 = gray_32fc1_img.at<float>(y, x); // read grayscale value of a pixel, image represented using 32 bits
	//cv::Vec3b p3 = src_8uc3_img.at<cv::Vec3b>(y, x); // read color value of a pixel, image represented using 8 bits per color channel

	//// print values of pixels
	//printf("p1 = %d\n", p1);
	//printf("p2 = %f\n", p2);
	//printf("p3[ 0 ] = %d, p3[ 1 ] = %d, p3[ 2 ] = %d\n", p3[0], p3[1], p3[2]);

	//gray_8uc1_img.at<uchar>(y, x) = 0; // set pixel value to 0 (black)

	//// draw a rectangle
	//cv::rectangle(gray_8uc1_img, cv::Point(65, 84), cv::Point(75, 94),
	//	cv::Scalar(50), CV_FILLED);

	//// declare variable to hold gradient image with dimensions: width= 256 pixels, height= 50 pixels.
	//// Gray levels wil be represented using 8 bits (uchar)
	//cv::Mat gradient_8uc1_img(50, 256, CV_8UC1);

	//// For every pixel in image, assign a brightness value according to the x coordinate.
	//// This wil create a horizontal gradient.
	///*for (int y = 0; y < gradient_8uc1_img.rows; y++) {
	//	for (int x = 0; x < gradient_8uc1_img.cols; x++) {
	//		gradient_8uc1_img.at<uchar>(y, x) = x;
	//	}
	//}*/
	////cv::Mat gray_32fc1_img;
	//float gamma = 0.8;
	//src_8uc3_img.convertTo(gray_32fc1_img, CV_32FC1, 1.0 / 255.0);
	//for (int y = 0; y < gray_32fc1_img.rows; y++) {
	//	for (int x = 0; x < gray_32fc1_img.cols; x++) {
	//		gray_32fc1_img.at<float>(y, x) = 255 * pow(gray_32fc1_img.at<float>(y, x), gamma);
	//	}
	//}

	//// diplay images
	//cv::imshow("Gradient", gradient_8uc1_img);
	//cv::imshow("Lena gray", src_8uc3_img);
	//cv::imshow("Lena gray gradiented", gray_32fc1_img);

	//cv::waitKey(0); // wait until keypressed

	/*float gamma = 0.4;
	cv::Mat src_8uc1_img = cv::imread("images/lena.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat gray_32fc1_img;
	src_8uc1_img.convertTo(gray_32fc1_img, CV_32FC1, 1.0 / 255.0);

	for (int y = 0; y < gray_32fc1_img.rows; y++) {
		for (int x = 0; x < gray_32fc1_img.cols; x++) {
			gray_32fc1_img.at<float>(y, x) = 255 * pow(gray_32fc1_img.at<float>(y, x), gamma);
		}
	}
	cv::Mat result;
	gray_32fc1_img.convertTo(result, CV_8UC1, 1);
	cv::imshow("Source ", src_8uc1_img);
	cv::imshow("Gamma " , result);
	cv::waitKey(0);*/

	return 0;
}
