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

#define OBJFILE "/Users/applekey/Documents/obj/sphere.obj"
#define SCREENSIZE 64

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
    
    //return 0 = x, 1 = y, 2 = z
    int largestAxis(float x, float y, float z) const
    {
        float max = fmax(x,fmax(y,z));
        if(max == x) return 0; // x
        if(max == y) return 1; // y
        if(max == z) return 2; // z
        return -1;
    }
    glm::vec3 swizzleVert(glm::vec3 vertex, int * mapping) const
    {
        float points[3] = {vertex.x,vertex.y,vertex.z};
        glm::vec3 swizPoint = glm::vec3(points[mapping[0]],points[mapping[1]],points[mapping[2]]);
        return swizPoint;
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
    
    struct aabb convertToScreenSpace(struct aabb vec, glm::vec3 faabbLower, glm::vec3 faabbRun, glm::vec3 fvoxelSize) const
    {
        assert(vec.lower.x >= faabbLower.x);
        assert(vec.lower.y >= faabbLower.y);
        
        // find the lower
        struct aabb screenIndex;
        screenIndex.lower.x = int((vec.lower.x - faabbLower.x)/(faabbRun.x) * voxelSize.x);
        screenIndex.lower.y = int((vec.lower.y - faabbLower.y)/(faabbRun.y) * voxelSize.y);
        screenIndex.lower.z = int((vec.lower.z - faabbLower.z)/(faabbRun.z) * voxelSize.z);
        
        //find upper
        screenIndex.upper.x = int((vec.upper.x - faabbLower.x)/(faabbRun.x) * voxelSize.x);
        screenIndex.upper.y = int((vec.upper.y - faabbLower.y)/(faabbRun.y) * voxelSize.y);
        screenIndex.upper.z = int((vec.upper.z - faabbLower.z)/(faabbRun.z) * voxelSize.z);
        
        return screenIndex;
    }
    
    glm::vec3 convertIndexToCoords(glm::vec3 index, glm::vec3 aabbLower, glm::vec3 aabbRun, glm::vec3 voxelSize, bool mid) const
    {
        
        assert(index.x >= 0);
        assert(index.y >= 0);
        
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
        
        //std::cout<<"x:"<<voxel.x<<" y:"<<voxel.y<<" z:"<<voxel.z<<std::endl;
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
        //find the largest axis
        glm::vec3 normal = glm::normalize(glm::cross(vert3 - vert1, vert2 - vert1));
        int projectionAxis = largestAxis(fabs(normal.x),fabs(normal.y),fabs(normal.z));
        
        //swizzle xyz
        int mapping[3];
        switch(projectionAxis)
        {
            case 0: //x normal
            {
                mapping[0] = 2;
                mapping[1] = 1;
                mapping[2] = 0;
                break;
            }
            case 1: //y normal
            {
                mapping[0] = 0;
                mapping[1] = 2;
                mapping[2] = 1;
                break;
            }
            case 2: //z normal
            {
                mapping[0] = 0;
                mapping[1] = 1;
                mapping[2] = 2;
                break;
            }
        }
        
        assert(vert1.x >= aabbLow.x);
        assert(vert1.y >= aabbLow.y);
        assert(vert1.z >= aabbLow.z);
        
        assert(vert2.x >= aabbLow.x);
        assert(vert2.y >= aabbLow.y);
        assert(vert2.z >= aabbLow.z);
        
        assert(vert3.x >= aabbLow.x);
        assert(vert3.y >= aabbLow.y);
        assert(vert3.z >= aabbLow.z);
        
        glm::vec3 swizzlePoints[3];
        swizzlePoints[0] = swizzleVert(vert1,mapping);
        swizzlePoints[1] = swizzleVert(vert2,mapping);
        swizzlePoints[2] = swizzleVert(vert3,mapping);
    
        //calculate triangle bounding square
        struct aabb triaabb = calculateBoundingBox(swizzlePoints[0],swizzlePoints[1],swizzlePoints[2]);
        // conver to screen space
        
        glm::vec3 saabbLow =swizzleVert(aabbLow,mapping);
        glm::vec3 saabbRun =swizzleVert(aabbRun,mapping);
        glm::vec3 svoxelSize =swizzleVert(voxelSize,mapping);
        

        struct aabb screenCords = convertToScreenSpace(triaabb,saabbLow,saabbRun,svoxelSize);
        
        //swizzle
        int vStart = screenCords.lower.x;
        int vEnd = screenCords.upper.x;
        int uStart = screenCords.lower.y;
        int uEnd = screenCords.upper.y;
        glm::vec3 direction = glm::vec3(0,0,1.0);
        
        assert(uStart >=0 && uEnd <=svoxelSize.x);
        assert(vStart >=0 && vEnd <=svoxelSize.y);
        
//        std::cout<<uStart<<" "<<uEnd<<std::endl;
//        std::cout<<vStart<<" "<<vEnd<<std::endl;
//        std::cout<<std::endl;
        
        for( int u = uStart ;u <= uEnd; u++)
        {
            for(int v = vStart;v <= vEnd ;v++)
            {
                glm::vec3 screenIndex;
                screenIndex.x = u;
                screenIndex.y = v;
                screenIndex.z = screenCords.lower.z;
                // shoot ray find intersection point
                glm::vec3 rayHitPosition;
                glm::vec3 po = convertIndexToCoords(screenIndex, saabbLow, saabbRun, svoxelSize,true);
                po.z =-99;
                //dilate vertpositions
                
                glm::vec3 e0 = glm::vec3( glm::vec2(vert2) - glm::vec2(vert1), 0 );
                glm::vec3 e1 = glm::vec3( glm::vec2(vert3) - glm::vec2(vert2), 0 );
                glm::vec3 e2 = glm::vec3( glm::vec2(vert1) - glm::vec2(vert3), 0 );
                
                glm::vec3 n0 = glm::cross( e0, direction );
                glm::vec3 n1 = glm::cross( e1, direction );
                glm::vec3 n2 = glm::cross( e2, direction );
                
                if(rayIntersectsTriangle(po, direction, swizzlePoints[0], swizzlePoints[1], swizzlePoints[2], &rayHitPosition))
                {
                    //unSwizzle
                    float rayHitPos[3] ={rayHitPosition.x,rayHitPosition.y,rayHitPosition.z};
                    rayHitPosition[mapping[0]] = rayHitPos[0];
                    rayHitPosition[mapping[1]] = rayHitPos[1];
                    rayHitPosition[mapping[2]] = rayHitPos[2];
                    
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
/*------------------------------main functions end -------------------------------*/

int main()
{
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
    
    tbb::task_scheduler_init init(1); // run 1 thread for debugging
    tbb::parallel_for( tbb::blocked_range<int>( 0, triangles.size()/3 ), vop );
    
    
    int z = 22;
    
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
        std::cout<<"--"<<std::endl;
    }
    
    
    return 0;
    
}