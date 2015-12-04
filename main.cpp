#include <stdio.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <float.h>
#include </usr/local/include/glm/vec3.hpp> // glm::vec3
#include </usr/local/include/glm/vec4.hpp> // glm::vec4
#include </usr/local/include/glm/mat4x4.hpp> // glm::mat4
#include </usr/local/include/glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

// project files
#include "svo.h"


#define OBJFILE "/Users/applekey/Documents/obj/cone.obj"
#define SCREENSIZE 128

std::vector<glm::vec3> LoadObjFindAABB(const char * filename, glm::vec3 * aabbLow, glm::vec3 *  aabbHigh)
{
    std::ifstream in(filename, std::ios::in);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << std::endl;
        exit(1);
        
    }
    
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> meshVertices;
    std::vector<int> faceIndex;
    
    std::string line;
    while (std::getline(in, line))
    {
        if (line.substr(0,2)=="v "){
            std::istringstream v(line.substr(2));
            glm::vec3 vert;
            double x,y,z;
            v>>x;v>>y;v>>z;
            vert=glm::vec3(x,y,z);
            
            // x
            if ( x < aabbLow->x)
            {
                aabbLow->x = x;
            }
            
            if (x > aabbHigh->x)
            {
                aabbHigh->x = x;
            }
            
            // y
            if ( y < aabbLow->y)
            {
            aabbLow->y = y;
            }
            
            if (y > aabbHigh->y)
            {
                aabbHigh->y = y;
            }
            
            // z
            if ( z < aabbLow->z)
            {
                aabbLow->z = z;
            }
            
            if (z > aabbHigh->z)
            {
                aabbHigh->z = z;
            }
            
            vertices.push_back(vert);
        }
        else if(line.substr(0,2)=="f "){
            std::istringstream v(line.substr(2));
            int a,b,c;
            v>>a;v>>b;v>>c;
            a--;b--;c--;
            faceIndex.push_back(a);
            faceIndex.push_back(b);
            faceIndex.push_back(c);
        }
    }
    for(unsigned int i=0;i<faceIndex.size();i++)
    {
        glm::vec3 meshData;
        meshData=glm::vec3(vertices[faceIndex[i]].x,vertices[faceIndex[i]].y,vertices[faceIndex[i]].z);
        meshVertices.push_back(meshData);
        
    }
    return meshVertices;
}

int isPowerOfTwo (unsigned int x)
{
    while (((x & 1) == 0) && x > 1) /* While x is even and > 1 */
    x >>= 1;
    return (x == 1);
}

/*------------------------------main functions --------------------*/
struct voxelOperator {
    glm::vec3 voxelSize;
    std::vector<glm::vec3> * triangles;
    unsigned char * storage;
    glm::vec3 aabbLow;
    glm::vec3 aabbHigh;
    glm::vec3 aabbRun;
    
    //return 1 = x, 2 = y, 3 = z
    int largestAxis(float x, float y, float z) const
    {
        float max = fmax(x,fmax(y,z));
        if(max == x) return 1; // x
        if(max == y) return 2; // y
        if(max == z) return 3; // z
        return -1;
    }
    
    struct aabb
    {
        glm::vec3 lower;
        glm::vec3 upper;
    };
    
    struct aabb calculateBoundingBox(glm::vec3 vert1, glm::vec3 vert2, glm::vec3 vert3) const
    {
        struct aabb cd;
        
        cd.lower.x = fmin(vert1.x,(fmin(vert2.x, vert3.x)));
        cd.lower.y = fmin(vert1.y,(fmin(vert2.y, vert3.y)));
        cd.lower.z = fmin(vert1.z,(fmin(vert2.z, vert3.z)));
        
        cd.upper.x = fmax(vert1.x,(fmax(vert2.x, vert3.x)));
        cd.upper.y = fmax(vert1.y,(fmax(vert2.y, vert3.y)));
        cd.upper.z = fmax(vert1.z,(fmax(vert2.z, vert3.z)));
        
        return cd;
    }
    
    struct aabb convertToScreenSpace(struct aabb vec, glm::vec3 aabbLower, glm::vec3 aabbRun, glm::vec3 voxelSize) const
    {
        // find the lower
        struct aabb screenIndex;
        screenIndex.lower.x = int((vec.lower.x - aabbLower.x)/(aabbRun.x) * voxelSize.x);
        screenIndex.lower.y = int((vec.lower.y - aabbLower.y)/(aabbRun.y) * voxelSize.y);
        screenIndex.lower.z = int((vec.lower.z - aabbLower.z)/(aabbRun.z) * voxelSize.z);
        
        //find upper
        screenIndex.upper.x = int((vec.upper.x - aabbLower.x)/(aabbRun.x) * voxelSize.x);
        screenIndex.upper.y = int((vec.upper.y - aabbLower.y)/(aabbRun.y) * voxelSize.y);
        screenIndex.upper.z = int((vec.upper.z - aabbLower.z)/(aabbRun.z) * voxelSize.z);
        
        return screenIndex;
    }
    
    glm::vec3 convertIndexToCoords(glm::vec3 index, glm::vec3 aabbLower, glm::vec3 aabbRun, glm::vec3 voxelSize, bool mid) const
    {
        //floating points per voxel
        glm::vec3 voxelLenghts = glm::vec3(aabbRun.x/voxelSize.x,aabbRun.y/voxelSize.y,aabbRun.z/voxelSize.z);
        if(mid)
        {
            return aabbLower + index * voxelLenghts + voxelLenghts/2.0f;
        }
        else
        {
            return aabbLower + index * voxelLenghts;
        }
    }
    
    glm::vec3 convertCoordToIndex(glm::vec3 coord, glm::vec3 aabbLower, glm::vec3 aabbRun, glm::vec3 voxelSize) const
    {
        //convert x
        glm::vec3 indexCoords;
        indexCoords.x = int((coord.x - aabbLower.x)/(aabbRun.x) * voxelSize.x);
        indexCoords.y = int((coord.y - aabbLower.y)/(aabbRun.y) * voxelSize.y);
        indexCoords.z = int((coord.z - aabbLower.z)/(aabbRun.z) * voxelSize.z);
        return indexCoords;
    }
    
    struct points
    {
    
        glm::vec3 a;
        glm::vec3 b;
        glm::vec3 c;
    };
    
    struct points Dilate(glm::vec3  vert1, glm::vec3  vert2, glm::vec3  vert3) const
    {
        float scalePercentage = 1.3;
        //find the center of 3 verts
        float centx = (vert1.x + vert2.x + vert3.x)/3.0;
        float centy = (vert1.y + vert2.y + vert3.y)/3.0;
        float centz = (vert1.z + vert2.z + vert3.z)/3.0;
        
        glm::mat4x4 translation = glm::mat4x4(0);
        translation[0][0] = 1.0;
        translation[1][1] = 1.0;
        translation[2][2] = 1.0;
        translation[3][0] = -centx;
        translation[3][1] = -centy;
        translation[3][2] = -centz;
        translation[3][3] = 1.0;
        
        glm::mat4x4 scale = glm::mat4(0);
        scale[3][3] = 1.0;
        scale[0][0] = scalePercentage;
        scale[1][1] = scalePercentage;
        scale[2][2] = scalePercentage;
        
        
        //translate verts back to center
        //glm::vec4 transform =scale*translation;
        glm::vec4 svert1 = scale*translation*glm::vec4(vert1,1.0);
        glm::vec4 svert2 = scale*translation*glm::vec4(vert2,1.0);
        glm::vec4 svert3 = scale*translation* glm::vec4(vert3,1.0);
        //translate back to orig
        
        assert(svert1 != svert2);
        assert(svert2 != svert3);
        assert(svert1 != svert3);
        
        translation[3][0] = centx;
        translation[3][1] = centy;
        translation[3][2] = centz;
        
        svert1 = translation*svert1;
        svert2 = translation*svert2;
        svert3 = translation*svert3;
        
        struct points rpoints;
        rpoints.a = glm::vec3(svert1/svert1.w);
        rpoints.b = glm::vec3(svert2/svert2.w);
        rpoints.c = glm::vec3(svert3/svert3.w);
        return rpoints;
    }
    
    void MarkOutputVoxel(glm::vec3 voxel, glm::vec3 voxelSize, unsigned char * output) const
    {
        if (voxel.x >= voxelSize.x || voxel.y >= voxelSize.y || voxel.z >= voxelSize.z)
        {
            return;
        }
        
        assert(voxel.x < voxelSize.x
               && voxel.y < voxelSize.y
               && voxel.z < voxelSize.z);
        assert(voxel.x >=0 && voxel.y >=0 && voxel.z >=0);
        
        int plane = voxelSize.y * voxelSize.y;
        int stride = voxelSize.y;
        int outputIndex = voxel.x + voxel.y * stride + voxel.z*plane;
        
        assert(outputIndex <= (voxelSize.x * voxelSize.y * voxelSize.z));
        
        //std::cout<<"x:"<<voxel.x<<"y:"<<voxel.y<<"z:"<<voxel.z<<std::endl;
        output[outputIndex] = 1;
    }
    
    bool rayIntersectsTriangle(glm::vec3 p, glm::vec3 d, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm:: vec3 * hitLocation) const
    {
        
        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;
        glm::vec3 h = glm::cross(d, e2);
        
        // (D x E2) dot E1
        
        float a = glm::dot(e1,h);
        
        if (a > -0.00001 && a < 0.00001)
        {
            return false;
        }
        
        // 1/ ((D x E2) dot E1)
        float f = 1/a;
        
        glm::vec3 s = p - v0; //T
        
        float u = f * (glm::dot(s,h)); // D x E2 dot T
        
        if (u < 0.0 || u > 1.0)
        {
            return false;
        }
        
        glm::vec3 q = glm::cross(s,e1);
        float v = f * glm::dot(d,q);
        
        if (v < 0.0 || u + v > 1.0)
        {
            return false;
        }
        
        // at this stage we can compute t to find out where
        // the intersection point is on the line
        float t = f * glm::dot(e2,q);
        
        if (t > 0.00001)
        {
            // calculate hit location
            glm::vec3 hLoc =  p + d*t;
            hitLocation->x = hLoc.x;
            hitLocation->y = hLoc.y;
            hitLocation->z = hLoc.z;
            
            return true;
        }
        else
        {
            return false;
        }
    }
    
    void TriangleRasterizer(glm::vec3 vert1,glm::vec3 vert2,glm::vec3 vert3, unsigned char * output) const
    {
        //calculate smaller bounding box
        
        struct aabb triaabb = calculateBoundingBox(vert1,vert2,vert3);
        // conver to screen space
        glm::vec3 distance;
        struct aabb screenCords = convertToScreenSpace(triaabb,aabbLow,aabbRun,voxelSize);
        
        //find the largest axis
        glm::vec3 normal = glm::normalize(glm::cross(vert3 - vert1, vert2 - vert1));
        int projectionAxis = largestAxis(fabs(normal.x),fabs(normal.y),fabs(normal.z));
        
        int uStart = 0, uEnd = 0, vStart = 0,vEnd = 0;
        if (projectionAxis == 1) {uStart = screenCords.lower.y; uEnd = screenCords.upper.y; vStart = screenCords.lower.z; vEnd = screenCords.upper.z;}; //x
        if (projectionAxis == 2) {uStart = screenCords.lower.x; uEnd = screenCords.upper.x; vStart = screenCords.lower.z; vEnd = screenCords.upper.z;}; //y
        if (projectionAxis == 3) {uStart = screenCords.lower.x; uEnd = screenCords.upper.x; vStart = screenCords.lower.y; vEnd = screenCords.upper.y;}; //z
        
        //        std::cout<<uStart<<" "<<uEnd<<std::endl;
        //        std::cout<<vStart<<" "<<vEnd<<std::endl;
        //        std::cout<<std::endl;
        
        for( int u = uStart ;u <= uEnd; u++)
        {
            for(int v = vStart;v <= vEnd ;v++)
            {
                glm::vec3 screenIndex;
                glm::vec3 direction = glm::vec3(0,0,0);
                if(projectionAxis == 1)
                {
                    screenIndex.y = u;
                    screenIndex.z = v;
                    screenIndex.x = -999.0;
                    direction.x = 1.0;
                }
                else if(projectionAxis == 2)
                {
                    screenIndex.z = v;
                    screenIndex.x = u;
                    screenIndex.y =  -999.0;
                    direction.y = 1.0;
                }
                else if(projectionAxis == 3)
                {
                    screenIndex.x = u;
                    screenIndex.y = v;
                    screenIndex.z =  -999.0;
                    direction.z = 1.0;
                }
                else
                {
                    std::cout<<"Error projection axis"<<std::endl;
                }
                
                // shoot ray find intersection point
                glm::vec3 rayHitPosition;
                glm::vec3 po = convertIndexToCoords(screenIndex, aabbLow, aabbRun, voxelSize,true);
                
                //dilate triangle
                
                glm::vec3 vert1tmp = vert1;
                glm::vec3 vert2tmp = vert2;
                
                struct points rpoints = Dilate(vert1,vert2,vert3);
                //assert(rpoints.a!= rpoints.b);
                
                if(rayIntersectsTriangle(po, direction, rpoints.a, rpoints.b, rpoints.c, &rayHitPosition))
                {
                    glm::vec3 fillIndex = convertCoordToIndex(rayHitPosition,aabbLow,aabbRun,voxelSize);
                    MarkOutputVoxel(fillIndex,voxelSize,output);
                }
                
            }
        }
    }
    
    
    void operator()( const tbb::blocked_range<int>& range ) const {
        for( int i=range.begin(); i!=range.end(); ++i )
        {
            glm::vec3 * verts = &(*triangles)[0];
            unsigned int triangleVertId = i * 3;
            //verticies
            glm::vec3 vert1 = verts[triangleVertId];
            glm::vec3 vert2 = verts[triangleVertId + 1];
            glm::vec3 vert3 = verts[triangleVertId + 2];
            
            TriangleRasterizer(vert1,vert2,vert3,storage);
            
        }
    }
};
/*------------------------------main functions end --------------------*/

int main()
{
    svo svc;
    svc.ReadIn("file.txt",7);
    
    return 0;
    
//    int sscreenSize[3] = {SCREENSIZE,SCREENSIZE,SCREENSIZE};
//    
//    
//    
//    sv.BuildSVOUnitTest();
//    
//    
//    return 0;
    //screen size
    glm::vec3 screenSize(SCREENSIZE,SCREENSIZE,SCREENSIZE);
    
    //intilize output
    assert(isPowerOfTwo(SCREENSIZE));
    unsigned int bufferSize =SCREENSIZE * SCREENSIZE * SCREENSIZE;
    unsigned char * output = new unsigned char [bufferSize];
    for(int i = 0;i < bufferSize;i++)
    {
        output[i] = 0;
    }
    //AABB
    glm::vec3 aabbLow = glm::vec3(FLT_MAX,FLT_MAX,FLT_MAX);
    
    glm::vec3 aabbHigh = glm::vec3(FLT_MIN,FLT_MIN,FLT_MIN);
    //read in obj
    std::vector<glm::vec3> triangles = LoadObjFindAABB(OBJFILE, &aabbLow,&aabbHigh);
    struct voxelOperator vop;
    vop.triangles = &triangles;
    vop.voxelSize = screenSize;
    vop.storage = output;
    vop.aabbLow = aabbLow;
    vop.aabbHigh = aabbHigh;
    vop.aabbRun = glm::vec3( aabbHigh.x - aabbLow.x, aabbHigh.y - aabbLow.y, aabbHigh.z - aabbLow.z);
    
    //tbb::task_scheduler_init init(1); // run 1 thread for debugging
    tbb::parallel_for( tbb::blocked_range<int>( 1, triangles.size()/3 ), vop );
    
    svo sv;
    int sscreenSize[3] = {SCREENSIZE,SCREENSIZE,SCREENSIZE};
    sv.BuildSVO(output, sscreenSize);
    int z = 55;
    
    for (int j =0; j < SCREENSIZE; j++) {
        for (int i =0; i < SCREENSIZE; i++) {
            
            int intOutput = output[j*SCREENSIZE + SCREENSIZE * SCREENSIZE * z+ i];
            if (intOutput == 0) {
                std::cout<<" ";
            }
            else
            {
                std::cout<<intOutput;
            }
        }
        std::cout<<std::endl;
    }
    
    
    return 0;
    
}