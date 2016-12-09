#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdio.h>
#include <math.h>
#include "dirtool_.hpp"
#include "get_config.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <algorithm>


using namespace std;
using namespace cv;

typedef struct {
int x,y;  //位置
float dis_square;   //离矩阵中心的距离
}point;
bool cmp(point a,point b){
    return a.dis_square>b.dis_square;
}

/**
 * 填充矩阵模板
 * @param arr 源数组，数据的来源
 * @param mat 模板矩阵
 * @param size 模板矩阵的大小
 */
void charge_masktemp(int arr[], Mat & mat, int size) {

    point points[size*size];
    //初始化矩阵数组
    for(int i=0;i<size;i++)
        for(int j=0;j<size;j++){
            points[i*size+j].x=i;
            points[i*size+j].y=j;
        }

    double midx=1.0*(size-1)/2;  //计算中心位置(x,y)
    double midy=1.0*(size-1)/2;

    //计算每个点与矩阵中心的距离^2
    for(int i=0;i<size*size;i++)
        points[i].dis_square=pow(points[i].x-midx,2)+pow(points[i].y-midy,2);

    sort(points,points+size*size,cmp);

    for(int k=0;k<size*size;k++)
        mat.at<uchar>(points[k].x,points[k].y)=arr[k];  //按距离进行赋值，离矩阵中心越远的点值越小

}

/**
 *抖动函数
 *
 * @param str 图像路径或存储图像的目录
 * @param Sizex,Sizey 目标图像大小，目标图像非常可能是个更大的图
 * @param Scale 原图放大多少倍以后再放入目标图像,原图放大总是保持X，Y同步放大的
 * @param OffsetX,OffsetY 原图放入目标图像后相对于目标图像左上角的偏移
 * @param Background 目标图像中没有放入原图像的地方应该是什么颜色，建议是0，或者255
 * @param TemplateSize 抖动模板的大小
 */
Mat dither_single(const char* str,int Sizex,int Sizey,double Scale,int OffsetX,int OffsetY,Scalar Background,int TemplateSize){
    Mat imgSource;
    imgSource=imread(str,0);
//	imshow("1",imgSource);

    Mat re(Sizex,Sizey,CV_8UC1,Background);
    Mat img=Mat::zeros(imgSource.rows*Scale ,imgSource.cols*Scale, CV_8UC1 );
    Size dsize = Size(imgSource.cols*Scale,imgSource.rows*Scale);
    resize(imgSource, img, dsize);   //缩放原图
    int rows=img.rows;
    int cols=img.cols;

    if(rows>Sizey||cols>Sizex){
        cout<<"目标图像尺寸大于原图像。。"<<endl;
    }
    //初始化模板大小
    int SIZE=TemplateSize,length=SIZE*SIZE;
    Mat maskTemp(SIZE,SIZE,CV_8UC1,Scalar::all(255));
    //生成0-255均匀分布的数组
    int maskData[length];
    for(int i=0;i<length;i++){
        maskData[i]=rand()%256;
//		cout<<maskData[i]<<"  ";
    }
    //升序排序
    sort(maskData,maskData+length,less<int>());

    //填充矩阵模板
    charge_masktemp(maskData,maskTemp,SIZE);

    imshow("tmp",maskTemp);


    int lastrows=0;
    //进行像素二值化处理
    uchar* maskSet[SIZE];
    for (int i = 0; i < rows - SIZE; i += SIZE) {
        for (int j = 0; j < SIZE; j++)
            maskSet[j] = img.ptr<uchar>(i + j);
        for (int k = 0; k < SIZE; k++) {
            uchar* rp = maskSet[k];
            int flag = 0;  //flag 判断是否达到边缘
            while (flag < cols * img.channels()) {
                for (int l = 0; l < SIZE; l++) {
                    if ((int)rp[l]>0 && (int) rp[l] >= (int) maskTemp.at<uchar>(k, l))
                        rp[l] = 255;
                    else
                        rp[l] = 0;
                }
                rp = rp + SIZE;
                flag += SIZE;
            }
        }
        lastrows=i+SIZE;
    }
    //处理边缘像素
    uchar* lastSet[rows-lastrows];
    for(int i=lastrows;i<rows;i++){
        lastSet[i-lastrows] = img.ptr<uchar>(i);
    }
    for (int k = 0; k < rows-lastrows-1; k++) {
        uchar* rp = lastSet[k];
        int flag = 0;
        while (flag < cols * img.channels()) {
            for (int l = 0; l < SIZE; l++) {
                if ( (int)rp[l]>0 && (int) rp[l] >= (int) maskTemp.at<uchar>(k, l))
                    rp[l] = 255;
                else
                    rp[l] = 0;
            }
            rp = rp + SIZE;
            flag += SIZE;
        }
    }

    img.row(0).setTo(Scalar(0));
    img.row(img.rows-1).setTo(Scalar(0));
    img.col(0).setTo(Scalar(0));
    img.col(img.cols-1).setTo(Scalar(0));

//	imshow("2", img);
    //图像平移到 (OffsetX,OffsetY)
    Mat reROI= re(Rect(OffsetX, OffsetY, cols, rows));
    Mat im=img.clone();
    im.copyTo(reROI,img);
    return re;

}
map<string,Mat> dither(const char* str, int Sizex, int Sizey, double Scale, int OffsetX,
        int OffsetY, Scalar Background, int TemplateSize) {

    struct stat fileStat;
    int r = stat(str, &fileStat);
    if (r != 0) {
        cout << "No such input image or dictionary." << endl;
    }
    map<string,Mat> m;

    if ((stat(str, &fileStat) == 0) && !S_ISDIR(fileStat.st_mode)) {
        //处理单个图像
        string sinstr=str;
        sinstr=sinstr.substr(sinstr.find_last_of('\\')+1);
        cout<<"--正在处理图像："+sinstr<<endl;
        Mat mat=dither_single(str, Sizex, Sizey, Scale, OffsetX, OffsetY, Background,
                TemplateSize);
        m.insert(map<string,Mat>::value_type(sinstr,mat));
    } else {
        //处理文件夹
        vector<string> file_vec;
        GetAllFiles(str,file_vec);
        for (vector<string>::const_iterator it = file_vec.begin();
                it < file_vec.end(); ++it) {
            string sinstr=(*it);
            sinstr=sinstr.substr(sinstr.find_last_of('\\')+1);
            cout<<"--正在处理图像："+sinstr<<endl;
            Mat mat=dither_single((*it).c_str(), Sizex, Sizey, Scale, OffsetX, OffsetY,
                    Background, TemplateSize);
            m.insert(map<string,Mat>::value_type(sinstr,mat));
        }
    }
    return m;
}

int main(int argc, char **argv) {

    cout<<"------图像抖动处理-------------------"<<endl;
    cout<<"------------------------------------"<<endl;
    cout<<"------------------------------------"<<endl;

    map<string, string> cofm;
    ReadConfig("config", cofm);
    ifstream fin("config");
    if (!fin){
       cout << "config 配置文件不存在。" << endl;
    }

    //参数
    const char* str=cofm.at("inputdir").c_str();
    int sizex=atoi(cofm.at("Sizex").c_str());
    int sizey=atoi(cofm.at("Sizey").c_str());
    double scale=atof(cofm.at("Scale").c_str());
    int offsetx=atoi(cofm.at("OffsetX").c_str());
    int offsety=atoi(cofm.at("OffsetY").c_str());
    int templatesize=atoi(cofm.at("TemplateSize").c_str());
    string outdir=cofm.at("outputdir");  //输出图像的目录

    //判断输出目录是否存在，不存在则创建
    struct stat fileStat;
    int r = stat(outdir.c_str(), &fileStat);
    if (r != 0) {
        //cout << "输出目录不存在。" << endl;
        _mkdir(outdir.c_str());
    }

    //调用dither函数
    map<string,Mat> images=dither(str,sizex,sizey,scale,offsetx,offsety,Scalar(0,0,0),templatesize);

    map<string,Mat>::const_iterator it = images.begin();
    while (it != images.end())
    {
        map<string,Mat>::value_type va= *it;
        imwrite(outdir+"\\图_"+va.first, va.second);
        cout<<"%%输出处理后的图像：图_"+va.first<<endl;
        it++;
    }

    cout<<"所有图像处理完毕。"<<endl;
    //getchar();
    waitKey();


}



