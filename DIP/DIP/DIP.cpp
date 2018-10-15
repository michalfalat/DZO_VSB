// DIP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <math.h> 


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


cv::Vec3b boxFilter(cv::Mat srcImage, int x, int y, FILTER_TYPE type) {

	int filter_size = 0;
	switch (type)
	{
	case BOX3X3:
		int filter_size = 3;
		break;
	case GAUSSIAN3X3:
		int filter_size = 3;
		break;
	case GAUSSIAN5X5:
		int filter_size = 5;
		break;
	default:
		break;
	}

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
				int matrixVal = box_filter_template3x3[i][j];
				break;
			case GAUSSIAN3X3:
				int matrixVal = gaussian_filter_template3x3[i][j];
				break;
			case GAUSSIAN5X5:
				int matrixVal = gaussian_filter_template5x5[i][j];
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



void BoxBlur() {
	int border = filter_size / 2;
	cv::Mat src_8uc1_img = cv::imread("images/lena.png", CV_LOAD_IMAGE_COLOR);
	cv::Mat gray_32fc1_img;
	src_8uc1_img.convertTo(gray_32fc1_img, CV_8UC3);
	int rows = src_8uc1_img.rows - border-1;
	int cols = src_8uc1_img.cols - border-1;

	for (int x = border; x < rows; x++) {
		for (int y = border; y < cols; y++) {
			cv::Vec3b resPixel = boxFilter(src_8uc1_img, x, y);
			gray_32fc1_img.at<cv::Vec3b>(cv::Point(x, y)) = resPixel;
		}
	}
	cv::Mat result;
	gray_32fc1_img.convertTo(result, CV_8UC1, 1);
	cv::imshow("Source ", src_8uc1_img);
	cv::imshow("Result box blur ", result);
	cv::waitKey(0);
}

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
	cv::imshow("Gamma " , result);
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

cv::Mat dft(cv::Mat &source) {
	cv::Mat normalized;

	int M = source.cols;
	int N = source.cols;
	double sqrtMN_d = 1.0 / sqrt(M * N);
	double PIPI = M_PI * 2;
	double sumR = 0, sumI = 0;
	double M_d = 1.0 / M, N_d = 1.0 / N;

	source.convertTo(normalized, CV_64FC1, 1.0 / 255.0 * sqrtMN_d);
	cv::Mat furier = cv::Mat(M, N, CV_64FC2);

	for (int k = 0; k < M; k++) {
		for (int l = 0; l < N; l++) {
			sumR = sumI = 0;

			for (int m = 0; m < M; m++) {
				for (int n = 0; n < N; n++) {
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



void FurierTransformation() {
	cv::Mat src_8uc1_img = cv::imread("images/lena64.png", CV_LOAD_IMAGE_GRAYSCALE);
	cv::Mat gray_32fc1_img;
	src_8uc1_img.convertTo(gray_32fc1_img, CV_32FC1, 1.0 / 255.0);

	cv::Mat fourier = dft(gray_32fc1_img);
	cv::Mat power = dftPower(fourier);
	cv::Mat phase = dftPhase(fourier);
	cv::imshow("Source", src_8uc1_img);
	cv::imshow("Discrete fourier transformation - POWER ", power);
	cv::imshow("Discrete fourier transformation - PHASE ", phase);
	cv::waitKey(0);
}

int main()
{
	//cviko 1 
	//Gamma(0.4);

	// cviko 2 
	BoxBlur();

	//cviko 3 
	//AnisotropicFilter();

	//cviko 4
	//FurierTransformation();






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
