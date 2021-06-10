#include "FaceMorph.hpp"

using namespace cv;
using namespace std;

//Constructor
FaceMorph::FaceMorph(String im0, String im1, double aVal, 
  double bVal, double pVal, string pointFile, int frames)
{
  //Assign weight values
  a = aVal;
  b = bVal;
  p = pVal;

  //save input image filenames
  imf0 = im0;
  imf1 = im1;
 
  //Keeps track of state
  flag = FIRST_IN;


  //Check to see if the image files are valid
  image = imread(imf0,1);
  if (image.empty())
  {
    printf("Error in loading image\n");
    return;
  }
  image = imread(imf1,1);
  if (image.empty())
  {
    printf("Error in loading image\n");
    return;
  }

  //Create opencv window object
  namedWindow(win, WINDOW_AUTOSIZE);

  //connnect window to mouse callback
  setMouseCallback(win, click_event, this);

  //load points file
  if (pointFile.length() > 0)
  {
    if(loadPoints(pointFile))
    {
      //animate video if we are passed a frame number
      if (frames > 0)
      {
        animate(frames);
      }
      else
      {
        //Run the full morph
        full_morph();
      }
    }
    return;
  }

  drawGUI();
  waitKey(0);
}

FaceMorph::~FaceMorph()
{
}


//This is the drawing program that will handle drawing the gui
void FaceMorph::drawGUI()
{
  if (flag == FIRST_IN || flag == SECOND_IN)
  {
    Mat tempIm;

    Mat im0 = imread(imf0,1);
    Mat im1 = imread(imf1,1);

    //TODO: account for differently sized images
    if (im0.cols != im1.cols || im0.rows != im1.rows)
    {
      printf("Please give two same sized images");
      return;
    }
    
    //horizontal buffer for gui for aesthetics
    Mat horizBuff(im0.rows,horizBuffSize,CV_8UC3,bgColor);

    //Add buffer and store in tempIm
    hconcat(im0,horizBuff,image);
    hconcat(image,im1,tempIm);

    //Add bottom gui Space
    Mat verBuff(vertBuffSize,tempIm.cols,CV_8UC3,bgColor);

    vconcat(tempIm,verBuff,image);

    //Add buttons to GUI
    Mat uButton = imread("undo_button.png",1);
    Mat nButton = imread("next_button.png",1);

    uButton.copyTo(image(cv::Rect(10,image.rows-85,75,75)));
    nButton.copyTo(image(cv::Rect(image.cols-85,image.rows-85,75,75)));

    drawPoints();

    //Show final GUI image
    imshow(win,image);
  }
  else
    imshow(win,image);
}

//Draws points and lines onto GUI
void FaceMorph::drawPoints()
{
  int i;
  Mat im0 = imread(imf0,1);

  //Do first image's points and lines
  for (i = 0; i < points0.size(); ++i)
  {
    circle(image,points0[i],pointWidth,lineColor,lineWidth);

    if (i > 0 && (i+1) % 2 == 0)
      line(image,points0[i-1],points0[i],lineColor,lineWidth);
  }

  //Do second image's points and lines
  for (i = 0; i < points1.size(); ++i)
  {
    Point p0(points1[i].x + im0.cols + horizBuffSize,points1[i].y);

    circle(image,p0,pointWidth,lineColor,lineWidth);

    if (i > 0 && (i+1) % 2 == 0)
    {
      Point p1(points1[i-1].x + im0.cols + horizBuffSize,points1[i-1].y);
      line(image,p1,p0,lineColor,lineWidth);
    }
  }

  //Draw guidelines
  if (flag == SECOND_IN && points0.size() != points1.size())
  {
    i = points1.size();
    //Draw current point to input
    circle(image,points0[i],pointWidth,highlightColor,lineWidth);
    if (i > 0 && i % 2 == 1)
    {
      line(image,points0[i-1],points0[i],highlightColor,lineWidth);
    }
  }
}


//Save the individual frames from the animation sequence
void FaceMorph::saveFrame(Mat im, int frame)
{
  string filename = "renders/" + imf0 + "_"
                    + imf1 + to_string(points0.size()/2)
                    +"_points.txt"
                    + imf0 + '+' + imf1 + '_'
                    + format("Frame:%d_",frame)
                    + to_string(points0.size()/2)
                    + "_points_a=" + format("%5.2lf",a)
                    + "_b=" + format("%5.2lf",b)
                    + "_p=" + format("%5.2lf",p)
                    + ".png";
  imwrite(filename,image);
}


//Save the final image render in ./renders
void FaceMorph::saveFinal()
{
  
  string filename = "renders/" + imf0 + "_"
                    + imf1 + to_string(points0.size()/2)
                    +"_points.txt"
                    + imf0 + '+' + imf1 + '_'
                    + to_string(points0.size()/2)
                    + "_points_a=" + format("%5.2lf",a)
                    + "_b=" + format("%5.2lf",b)
                    + "_p=" + format("%5.2lf",p)
                    + ".png";
  imwrite(filename,image);
}


//Save the input points to a .txt file to be reused later 
void FaceMorph::savePoints()
{
  int i;
  ofstream file;

  //Specify file name and location
  string filename = "points/" + imf0 + "_"
                    + imf1 + to_string(points0.size()/2)
                    +"_points.txt";
  /*
                    + imf0 + '+' imf1 + '_'
                    + to_string(points0.size()/2)
                    + "_points_a=" + format("%5.2lf",a)
                    + "_b=" + format("%5.2lf",b)
                    + "_p=" + format("%5.2lf",p)
                    + ".txt";
  */

  file.open(filename);


  for (i = 0; i < points0.size(); ++i)
  {
    file << points0[i].x << ',' << points0[i].y << "\n";
  }

  file << "#\n"; //file delimeter

  for (i = 0; i < points1.size(); ++i)
  {
    file << points1[i].x << ',' << points1[i].y << "\n";
  }

  file.close();
}

//Load the points from the point file
//Highly reccomend against writing your own point file
//returns 1 on success, 0 on failure
int FaceMorph::loadPoints(string pointfile)
{
  int state = 0;
  int x,y;
  ifstream file (pointfile);
  string line;
  string token;

  if (file.is_open())
  {
    while (getline(file,line))
    {
      if (!line.compare("#"))
      {
        state = 1;
      }
      else
      {
        //Split the string and store values in x and y
        token = line.substr(0,line.find(','));
        x = stoi(token);
        token = line.substr(line.find(',')+1);
        y = stoi(token);

        if (!state)
        {
          points0.push_back(Point(x,y));
        }
        else
        {
          points1.push_back(Point(x,y));
        }
      }
    }
    if (points0.size() == points1.size())
    {
      flag = FINAL;
      return 1;
    }
    else
    {
      printf("error loading point file, resuming with point GUI\n");
    }

  }
  else
  {
    printf("error loading point file, resuming with point GUI\n");
  }
  return 0;
}


//Handle click events
void FaceMorph::guiClick(int x, int y)
{
  if (flag == FIRST_IN || flag == SECOND_IN)
  {
    //Add Point if in first in state
    if (inIm0(x,y) && flag == FIRST_IN)
    {
      addPointIm0(x,y);
    }
    //Add point if in second in state
    else if (inIm1(x,y) && flag == SECOND_IN
              && points1.size() < points0.size())
    {
      addPointIm1(x,y);
    }
    //click next button during first state
    else if (inNext(x,y) && flag == FIRST_IN)
    {
      if (points0.size() > 0 && points0.size() % 2 == 0)
      {
        flag = SECOND_IN;
      }
    }
    //click next button during second state
    else if (inNext(x,y) && flag == SECOND_IN)
    {
      if (points0.size() == points1.size())
      {
        flag = FINAL;
        full_morph();
      }
    }
    //use the undo button, which may change the state
    else if (inUndo(x,y))
    {
      undo();
    }
  }
  drawGUI();
}


//Check to see if mouseclick is in undo button
bool FaceMorph::inUndo(int x, int y)
{
  Mat im0 = imread(imf0,1);

  int minX = iconMargin;
  int maxX = iconMargin + iconDim;

  int minY = im0.rows + iconMargin;
  int maxY = im0.rows + iconMargin + iconDim;

  if (x >= minX && x <= maxX 
      && y >= minY && y <= maxY)
  {
    return true;
  }

  return false;
}

//undo button for point assignment during user input phase
void FaceMorph::undo()
{
  if (flag == SECOND_IN)
  {
    if (points1.size() == 0)
    {
      flag = FIRST_IN;
    }
    else 
    {
      points1.pop_back();
    }
  } 
  else if (flag == FIRST_IN && points0.size() > 0)
  {
    points0.pop_back();
  }
}

//Determine if a click was inside the next button
bool FaceMorph::inNext(int x, int y)
{
  Mat im0 = imread(imf0,1);
  Mat im1 = imread(imf1,1);

  int minX = im0.cols + horizBuffSize + im1.cols - iconDim - iconMargin;
  int maxX = im0.cols + horizBuffSize + im1.cols - iconMargin;

  int minY = im1.rows + iconMargin;
  int maxY = im1.rows + iconMargin + iconDim;

  if (x >= minX && x <= maxX 
      && y >= minY && y <= maxY)
  {
    return true;
  }


  return false;
}


//check to see if click is in im0
bool FaceMorph::inIm0(int x, int y)
{
  //TODO: Add variables to keep track of image dimensions
  Mat im = imread(imf0,1);

  if (x < im.cols && y < im.rows)
    return true;
  return false;
}

//Add point to im0 according to im0 dimensions
void FaceMorph::addPointIm0(int x, int y)
{
  points0.push_back(Point(x,y));
}

//check to see if click is in im1
bool FaceMorph::inIm1(int x, int y)
{
  //TODO: Add variables to keep track of image dimensions
  Mat im0= imread(imf0,1);
  Mat im1= imread(imf1,1);

  int minX = im0.cols + horizBuffSize;
  int maxY = im1.rows;


  if (x >= minX  && y < maxY)
    return true;
  return false;
}

//Add point to im1 according to im1 dimensions
void FaceMorph::addPointIm1(int x, int y)
{
  Mat im0 = imread(imf0,1);

  int newX = x - horizBuffSize - im0.cols;

  points1.push_back(Point(newX,y));
}

//Function to be called by the click handler
void FaceMorph::drawPoint(int x, int y)
{
  guiClick(x,y);
}


//Render final morph image
void FaceMorph::full_morph()
{
  double alpha = 0.5;
  double beta = 1.0 - alpha;

  vector<Point> interpolated_points = interpolate_points(alpha,beta);

  Mat im0 = morph(interpolated_points,points1,imf0,imf1);
  Mat im1 = morph(interpolated_points,points0,imf1,imf0);

  //Mat im0 = morph(points0,points1,imf0,imf1);
  //Mat im1 = morph(points1,points0,imf1,imf0);

  addWeighted(im1,alpha,im0,beta,0.0,image);
  savePoints();
  saveFinal();
  
}

//Render an animation sequence of a smooth morph and save it to a .avi file
void FaceMorph::animate(int frames)
{
  double alpha;
  double beta;
  vector<Point> interpolated_points;

  double fadeDiff = 1.0f/((double)frames);

  Size frame_size(image.cols,image.rows);

  string filename = "animations/" + imf0 + "_"
                    + imf1 + to_string(points0.size()/2)
                    +"_points.txt"
                    + imf0 + '+' + imf1 + '_'
                    + format("%d",frames)
                    + "_frames_"
                    + to_string(points0.size()/2)
                    + "_points_a=" + format("%5.2lf",a)
                    + "_b=" + format("%5.2lf",b)
                    + "_p=" + format("%5.2lf",p)
                    + ".avi";

  VideoWriter vW(filename,VideoWriter::fourcc('M','J','P','G'),30,frame_size,true);

  Mat im0 = imread(imf0,1);
  Mat im1 = imread(imf1,1);

  if (vW.isOpened() == false)
  {
    printf("Error opening video file\n");
    return;
  }
  
  for (int f = 0; f < frames; ++f)
  {
    if (f == 0)
    {
      image = im0;
    } else if (f == frames-1)
    {
      image = im1;
    } else
    {
      alpha = fadeDiff * ((double)f);
      beta = 1.0f - alpha;

      interpolated_points = interpolate_points(alpha,beta);

      Mat im0 = morph(interpolated_points,points1,imf0,imf1);
      Mat im1 = morph(interpolated_points,points0,imf1,imf0);

      addWeighted(im1,alpha,im0,beta,0.0,image);
    }
    vW.write(image);
    saveFrame(image,f);
    printf("Frame %d complete\n",f);
  }

  vW.release();
}


//Returns interpolated points vector of the two point vectors
//Alpha and beta signify how far it's to be warped through the alpha
//variable and beta should be (1-alpha)
vector<Point> FaceMorph::interpolate_points(double alpha,double beta)
{
  int x,y;
  vector<Point> intPoints;

  for (int i = 0; i < points0.size(); ++i)
  {
    x = (static_cast<double>(points0[i].x) * beta) + (static_cast<double>(points1[i].x) * alpha),
    y = (static_cast<double>(points0[i].y) * beta) + (static_cast<double>(points1[i].y) * alpha);

    intPoints.push_back(Point(x,y));
  }

  return intPoints;
}


//Morphing algorithm to be used to warp individual images to be crossfaded
Mat FaceMorph::morph(vector<Point> points_source, vector<Point> points_dest, String sourceFile, String destFile)
{
  double DSUM[2];
  double X[2],X0[2];
  double P[2], Q[2];
  double D[2];
  double P0[2], Q0[2];
  double temp[2], temp0[2];
  double dist, length; 
  double weight, weightsum;
  int xF,yF;

  double u,v;


  Mat dImage = imread(destFile,1);
  Mat sImage = imread(sourceFile,1);


  for (int x = 0; x < dImage.cols; ++x)
  {
    for (int y = 0; y < dImage.rows; ++y)
    {
      X[0] = static_cast<double>(x);
      X[1] = static_cast<double>(y);

      DSUM[0] = 0.0; DSUM[1] = 0.0;

      weightsum = 0.0;


      for (int l = 0; l < (points0.size() / 2); ++l)
      {
        P[0] = static_cast<double>(points_dest[l*2].x);
        P[1] = static_cast<double>(points_dest[l*2].y);
        Q[0] = static_cast<double>(points_dest[l*2 + 1].x);
        Q[1] = static_cast<double>(points_dest[l*2 + 1].y);
        P0[0] = static_cast<double>(points_source[l*2].x);
        P0[1] = static_cast<double>(points_source[l*2].y);
        Q0[0] = static_cast<double>(points_source[l*2 + 1].x);
        Q0[1] = static_cast<double>(points_source[l*2 + 1].y);


        u = getU(X,P,Q);
        v = getV(X,P,Q);


        getX0(u,v,P0,Q0,X0);

        vSub(X0,X,D);

        if (u < 0.0)
        {
          dist = distanceBetween(X,P);
        } else if (u > 1.0)
        {
          dist = distanceBetween(X,Q);
        }
        else
        {
          dist = abs(distance(X,P,Q));
        }
          
        length = sqrt(pow(Q[0]-P[0],2)+pow(Q[1]-P[1],2));

        weight = pow(pow(length,p)/(a+dist),b);


        if (!isnan(weight))
        {
          vScale(weight,D,temp);
          vAdd(DSUM,temp,temp0);
          DSUM[0] = temp0[0];
          DSUM[1] = temp0[1];
          weightsum += weight;
        }

      }

      vScale(1.0/weightsum,DSUM,temp);
      vAdd(X,temp,X0);


      xF = (int)round(X0[0]);
      yF = (int)round(X0[1]);

      if (xF < 0)
        xF = 0;
      if (xF >= dImage.cols)
        xF = dImage.cols-1;
      if (yF < 0)
        yF = 0;
      if (yF >= dImage.rows)
        yF = dImage.rows-1;

  
      dImage.at<Vec3b>(Point(x,y)) = sImage.at<Vec3b>(Point(xF,yF));

    }
  }

  return dImage;
}

//Get the X0 value in the morphing algorithm
void FaceMorph::getX0(double u, double v, double * P0, double * Q0, double * X0)
{
  double t0[2], t1[2], t2[2], t3[2];

  vSub(Q0,P0,t0);
  vScale(u,t0,t1);

  perpendicular(t0,t3);
  vScale(v,t3,t2);

  vScale(1.0/magnitude(t0),t2,t3);

  vAdd(P0,t1,t2);
  vAdd(t2,t3,X0);
}

//gets the u value for the beier-neely algorithm
double FaceMorph::getU(double * X, double * P, double * Q)
{
  //intermediat points because I didn't feel like installing linalg tools
  double t0[2], t1[2];
  double top,bottom;
  
  vSub(X,P,t0);
  vSub(Q,P,t1);


  top = dot(t0,t1);
  bottom = pow(magnitude(t1),2.0);


  return top/bottom;
}


//Get the V value for the morphing algorithm
double FaceMorph::getV(double * X, double * P, double * Q)
{
  double t0[2], t1[2], t2[2];
  double top, bottom;

  vSub(X,P,t0);
  vSub(Q,P,t1);
  perpendicular(t1,t2);

  top = dot(t0,t2);
  bottom = magnitude(t1);

  return top/bottom;
}

//Return array is the perpendicular to X
void perpendicular(double * X, double * ret)
{

  ret[0] = -1.0 * X[1];
  ret[1] = X[0];
}

//Return array is the scaled version of X
void vScale(double s, double * X, double * ret)
{
  ret[0] = (s * X[0]);
  ret[1] = (s * X[1]);
}

//Dot product of two 2d vectors
double dot(double * a, double * b)
{
  double d;

  d = (a[0] * b[0]) + (a[1] * b[1]);

  return d;
}

//Cross product of two 2d vectors
double cross(double * va, double * vb)
{
  double d;

  d = (va[0] * vb[1]) - (va[1] * vb[0]);

  return d;
}

//distance between point x and line P,Q
double distance(double * X, double * P, double * Q)
{
  double top, bottom;

  top =  abs( ( (Q[0] - P[0]) * (P[1] - X[1]) )
            - ( (P[0] - X[0]) * (Q[1] - P[1]) ) );


  bottom = sqrt(pow(Q[0]-P[0],2.0) + pow(Q[1]-P[1],2.0));

  return top/bottom;
}

//magnitude of a vector
double magnitude(double * X)
{
  double mag = sqrt(pow(X[0],2) + pow(X[1],2));
  
  return mag;
}


//Subtraction of two 2d vectors
void vSub(double * a, double * b, double * ret)//only works on 2 element arrays
{
  ret[0] = a[0] - b[0];
  ret[1] = a[1] - b[1];
}

//Addition of two 2d vectors
void vAdd(double * a, double * b, double * ret)//only works on 2 element arrays
{
  ret[0] = (a[0] + b[0]);
  ret[1] = (a[1] + b[1]);
}

//Distance between two points
double distanceBetween(double * A, double * B)
{
  return sqrt(pow(B[0]-A[0],2) + pow(B[1]-A[1],2));
}

//Click event function to handle mouse clicks
void click_event(int event, int x, int y, int flags, void * userdata)
{
  if (event == EVENT_LBUTTONDOWN)
  {
    FaceMorph * fm = reinterpret_cast<FaceMorph*>(userdata);
    //TODO: add try/except clause here for safety
    fm->drawPoint(x,y);
  }
}
