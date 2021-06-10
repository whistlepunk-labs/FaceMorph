#include "FaceMorph.cpp"


int main(int argc, char ** argv)
{
  double a = 5.0;
  double b = 1.0;
  double p = 0.25;
  std::string pointfile = "";
  int frames = 0;

  if (argc < 3)
  {
    printf("Enter two image files on command line\n");
    return -1;
  }

  if (argc >= 4)
  {
    sscanf(argv[3],"%lf",&a);
    if (argc >= 5)
    {
      sscanf(argv[4],"%lf",&b);

      if (argc >= 6)
      {
        sscanf(argv[5],"%lf",&p);

        if (argc >= 7)
        {
          pointfile = argv[6];

          if (argc >= 8)
          {
            sscanf(argv[7],"%d",&frames);
          }
        }
      }
    }
  }

  cv::String imf0 = argv[1];
  cv::String imf1 = argv[2];


  FaceMorph fm(imf0,imf1,a,b,p,pointfile,frames);

  //std::cout << cv::getBuildInformation();


  return 0;
}
