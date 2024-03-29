// ArenaCrop.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <cv.h>
#include <highgui.h>
#include <math.h>

int main()
{
	// Load the image we'll work on
	IplImage* img = cvLoadImage("C:\\goal_arena.jpg");
	CvSize imgSize = cvGetSize(img);

	// This will hold the white parts of the image
	IplImage* detected = cvCreateImage(imgSize, 8, 1);

	// These hold the three channels of the loaded image
	IplImage* imgBlue = cvCreateImage(imgSize, 8, 1);
	IplImage* imgGreen = cvCreateImage(imgSize, 8, 1);
	IplImage* imgRed = cvCreateImage(imgSize, 8, 1);
	cvSplit(img, imgBlue, imgGreen, imgRed, NULL);

	// Extract white parts into detected
	cvAnd(imgGreen, imgBlue, detected);
	cvAnd(detected, imgRed, detected);

	// Morphological opening
	cvErode(detected, detected);
	cvDilate(detected, detected);

	// Thresholding (I knew you wouldn't catch this one... so i wrote a comment here
	// I mean the command can be so decieving at times)
	cvThreshold(detected, detected, 100, 250, CV_THRESH_BINARY);

	// Do the hough thingy
	CvMat* lines = cvCreateMat(100, 1, CV_32FC2);
	cvHoughLines2(detected, lines, CV_HOUGH_STANDARD, 1, 0.001, 100);
	
	// The two endpoints for each boundary line
	CvPoint left1 = cvPoint(0, 0);
	CvPoint left2 = cvPoint(0, 0);
	CvPoint right1 = cvPoint(0, 0);
	CvPoint right2 = cvPoint(0, 0);
	CvPoint top1 = cvPoint(0, 0);
	CvPoint top2 = cvPoint(0, 0);
	CvPoint bottom1 = cvPoint(0, 0);
	CvPoint bottom2 = cvPoint(0, 0);

	// Some numbers we're interested in
	int numLines = lines->rows;
	int numTop = 0;
	int numBottom = 0;
	int numLeft = 0;
	int numRight = 0;

	// Iterate through each line
	for(int i=0;i<numLines;i++)
	{
		// Get the parameters for the current line
		CvScalar dat = cvGet1D(lines, i);
		double rho = dat.val[0];
		double theta = dat.val[1];
		
		if(theta==0.0)
		{
			// This is an obviously vertical line... and we can't approximate it... NEXT
			continue;
		}

		// Convert from radians to degrees
		double degrees = theta*180/(3.1412);
		
		// Generate two points on this line (one at x=0 and one at x=image's width)
		CvPoint pt1 = cvPoint(0, rho/sin(theta));
		CvPoint pt2 = cvPoint(img->width, (-img->width/tan(theta)) + rho/sin(theta));
		
		if(abs(rho)<50)		// Top + left
		{
			if(degrees>45 && degrees<135)	// Top
			{
				numTop++;

				// The line is horizontal and near the top
				top1.x+=pt1.x;
				top1.y+=pt1.y;
			
				top2.x+=pt2.x;
				top2.y+=pt2.y;
			}
			else	// left
			{
				numLeft++;

				//The line is vertical and near the left
				left1.x+=pt1.x;
				left1.y+=pt1.y;
			
				left2.x+=pt2.x;
				left2.y+=pt2.y;
			}
		}
		else // bottom+right
		{
			if(degrees>45 && degrees<135)	// Bottom
			{
				numBottom++;

				//The line is horizontal and near the bottom
				bottom1.x+=pt1.x;
				bottom1.y+=pt1.y;
			
				bottom2.x+=pt2.x;
				bottom2.y+=pt2.y;
			}
			else	// Right
			{
				numRight++;

				// The line is vertical and near the right
				right1.x+=pt1.x;
				right1.y+=pt1.y;
				
				right2.x+=pt2.x;
				right2.y+=pt2.y;
			}
		}
	}

	// we've done the adding... now the dividing to get the "averaged" point
	left1.x/=numLeft;
	left1.y/=numLeft;
	left2.x/=numLeft;
	left2.y/=numLeft;

	right1.x/=numRight;
	right1.y/=numRight;
	right2.x/=numRight;
	right2.y/=numRight;

	top1.x/=numTop;
	top1.y/=numTop;
	top2.x/=numTop;
	top2.y/=numTop;

	bottom1.x/=numBottom;
	bottom1.y/=numBottom;
	bottom2.x/=numBottom;
	bottom2.y/=numBottom;

	// Render these lines onto the image
	cvLine(img, left1, left2, CV_RGB(255, 0,0), 1);
	cvLine(img, right1, right2, CV_RGB(255, 0,0), 1);
	cvLine(img, top1, top2, CV_RGB(255, 0,0), 1);
	cvLine(img, bottom1, bottom2, CV_RGB(255, 0,0), 1);

	// Next, we need to figure out the four intersection points
	double leftA = left2.y-left1.y;
	double leftB = left1.x-left2.x;
	double leftC = leftA*left1.x + leftB*left1.y;

	double rightA = right2.y-right1.y;
	double rightB = right1.x-right2.x;
	double rightC = rightA*right1.x + rightB*right1.y;

	double topA = top2.y-top1.y;
	double topB = top1.x-top2.x;
	double topC = topA*top1.x + topB*top1.y;

	double bottomA = bottom2.y-bottom1.y;
	double bottomB = bottom1.x-bottom2.x;
	double bottomC = bottomA*bottom1.x + bottomB*bottom1.y;

	// Intersection of left and top
	double detTopLeft = leftA*topB - leftB*topA;
	CvPoint ptTopLeft = cvPoint((topB*leftC - leftB*topC)/detTopLeft, (leftA*topC - topA*leftC)/detTopLeft);

	// Intersection of top and right
	double detTopRight = rightA*topB - rightB*topA;
	CvPoint ptTopRight = cvPoint((topB*rightC-rightB*topC)/detTopRight, (rightA*topC-topA*rightC)/detTopRight);

	// Intersection of right and bottom
	double detBottomRight = rightA*bottomB - rightB*bottomA;
	CvPoint ptBottomRight = cvPoint((bottomB*rightC-rightB*bottomC)/detBottomRight, (rightA*bottomC-bottomA*rightC)/detBottomRight);

	// Intersection of bottom and left
	double detBottomLeft = leftA*bottomB-leftB*bottomA;
	CvPoint ptBottomLeft = cvPoint((bottomB*leftC-leftB*bottomC)/detBottomLeft, (leftA*bottomC-bottomA*leftC)/detBottomLeft);

	// Render the points onto the image
	cvLine(img, ptTopLeft, ptTopLeft, CV_RGB(0,255,0), 5);
	cvLine(img, ptTopRight, ptTopRight, CV_RGB(0,255,0), 5);
	cvLine(img, ptBottomRight, ptBottomRight, CV_RGB(0,255,0), 5);
	cvLine(img, ptBottomLeft, ptBottomLeft, CV_RGB(0,255,0), 5);

	// Initialize a mask
	IplImage* imgMask = cvCreateImage(imgSize, 8, 3);
	cvZero(imgMask);

	// Generate the mask
	CvPoint* pts = new CvPoint[4];
	pts[0] = ptTopLeft;
	pts[1] = ptTopRight;
	pts[2] = ptBottomRight;
	pts[3] = ptBottomLeft;
	cvFillConvexPoly(imgMask, pts, 4, cvScalar(255,255,255));

	// Delete anything thats outside the mask
	cvAnd(img, imgMask, img);

	// Show all images in windows
	cvNamedWindow("Original");
	cvNamedWindow("Detected");

	cvShowImage("Original", img);
	cvShowImage("Detected", detected);

	cvWaitKey(0);

	return 0;
}