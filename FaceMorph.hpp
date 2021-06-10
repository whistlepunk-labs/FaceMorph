#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <stdlib.h>

#define win "FaceMorph"

#define FIRST_IN 0
#define SECOND_IN 1
#define FINAL 2

#define bgColor Scalar(0,0,100)
#define lineColor Scalar(0,0,255)
#define highlightColor Scalar(0,255,255)

#define horizBuffSize 50
#define vertBuffSize 95

#define iconDim 75
#define iconMargin 10

#define pointWidth 4
#define lineWidth 2



static void click_event(int event, int x, int y, int flags, void * userdata);

//Vecor operations because I didn't want to install a linalg library
void vSub(double * a, double * b, double * ret); //only works on 2 element arrays
void vAdd(double * a, double * b, double * ret); //only works on 2 element arrays
void vScale(double s, double * X, double * ret);
void perpendicular(double * X, double * ret);
double magnitude(double * X);
double dot(double * a, double * b);
double cross(double * va, double * vb);
double distance(double * X, double * P, double * Q); //distance between point & line
double distanceBetween(double * A, double * B); //distance between two points


class FaceMorph {
  public:
    FaceMorph(cv::String,cv::String, double aVal, 
      double bVal, double pVal, std::string, int);
    ~FaceMorph();

    void drawPoint(int x, int y);

  private:
    //Drawing function for GUI component of project
    void drawGUI();
    void drawPoints();

    //handles all mouseclicks from user
    void guiClick(int x, int y);

    bool inIm0(int x, int y); //checks to see if mouse click is in im0
    bool inIm1(int x, int y); //checks to see if mouse click is in im1

    //Check to see if mouse clicks are in the GUI buttons
    bool inNext(int x, int y);
    bool inUndo(int x, int y);

    void undo(); // the undo button

    //Add points to the point vectors
    //Im1 recalculates the position from GUI inputs
    void addPointIm0(int x, int y);
    void addPointIm1(int x, int y);

    //Save points to a file to be reused later
    void savePoints();
    //Load saved points
    int loadPoints(std::string pointfile);

    //save the final image in ./images
    void saveFinal();

    //also saved in ./images
    void saveFrame(cv::Mat,int);


    //Full morph after points are entered
    void full_morph();

    //Generate morphing video
    void animate(int frames);

    //Morph according to given points and filenames
    cv::Mat morph(std::vector<cv::Point>,std::vector<cv::Point>,cv::String,cv::String);

    //Interpolates the points in between the two point arrays
    std::vector<cv::Point> interpolate_points(double,double);

    //Calculate values for the facemorph algorithm
    double getU(double * X, double * P, double * Q);
    double getV(double * X, double * P, double * Q);
    void getX0(double u, double v, double * P0, double * Q0, double * X0);

   
    //Keeps track of state of program
    int flag;

    //Keep track of points
    std::vector<cv::Point>  points0;
    std::vector<cv::Point>  points1;
    
    //Constants used in morphing algorithm
    double a,b,p;

    //Filename
    cv::String imf0;
    cv::String imf1;

    //Final image
    cv::Mat image;
};
