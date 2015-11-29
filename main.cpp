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

#define OBJFILE "/Users/applekey/Documents/datasets/monkey.obj"
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
        //cout<<line<<endl;
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
    const glm::vec3 * voxelSize;
    std::vector<glm::vec3> * triangles;
    unsigned char * storage;
    glm::vec3 aabbLow;
    glm::vec3 aabbHigh;
    float aabbXRun;
    float aabbYRun;
    float aabbZRun;
    
    struct aabb
    {
        glm::vec3 lower;
        glm::vec3 upper;
    };
    
    struct aabb calculateBoundingBox(glm::vec3 vert1, glm::vec3 vert2, glm::vec3 vert3) const
    {
        struct aabb cd;
        
        cd.lower.x = fmin(vert1.x,(fmin(vert2.x, vert3.x)));
        cd.lower.y = fmin(vert1.y,(fmin(vert2.y, vert3.z)));
        cd.lower.z = fmin(vert1.z,(fmin(vert2.y, vert3.z)));
        
        cd.upper.x = fmax(vert1.x,(fmax(vert2.x, vert3.x)));
        cd.upper.y = fmax(vert1.y,(fmax(vert2.y, vert3.z)));
        cd.upper.z = fmax(vert1.z,(fmax(vert2.y, vert3.z)));
        
        return cd;
    }
    
    int min(int x, int y) const
    {
        return  y + ((x - y) & ((x - y) >>
                                (sizeof(int) * CHAR_BIT - 1)));
    }
    
    int largestAxis(float x, float y, float z) const
    {
        return fmin(fabs(x), fmax(fabs(y), fabs(z)));
    }
    
    
    void TriangleRasterizer(glm::vec3 vert1,glm::vec3 vert2,glm::vec3 vert3, unsigned char * output) const
    {
        //calculate smaller bounding box
        
        struct aabb triaabb = calculateBoundingBox(vert1,vert2,vert3);
        // conver to screen space
        
        
        
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
            
            // find the max area
//            glm::vec3 normal = glm::normalize(glm::cross(vert3 - vert1, vert2 - vert1));
//            int projectionAxis = largestAxis(normal.x,normal.y,normal.z);
            
            //rasterize the triangle
            
            
            
            
            
        }
    }
};
/*------------------------------main functions end --------------------*/

int main()
{
    //screen size
    glm::vec3 screenSize(SCREENSIZE,SCREENSIZE,SCREENSIZE);
    
    //intilize output
    assert(isPowerOfTwo(SCREENSIZE));
    unsigned char * output = new unsigned char [SCREENSIZE * SCREENSIZE * SCREENSIZE / 8];
    
    //AABB
    glm::vec3 aabbLow = glm::vec3(FLT_MIN,FLT_MIN,FLT_MIN);
    glm::vec3 aabbHigh = glm::vec3(FLT_MAX,FLT_MAX,FLT_MAX);
    
    //read in obj
    std::vector<glm::vec3> triangles = LoadObjFindAABB(OBJFILE, &aabbLow,&aabbHigh);
    struct voxelOperator vop;
    vop.triangles = &triangles;
    vop.voxelSize = &screenSize;
    vop.storage = output;
    vop.aabbLow = aabbLow;
    vop.aabbHigh = aabbHigh;
    vop.aabbXRun = aabbHigh.x - aabbLow.x;
    vop.aabbYRun = aabbHigh.y - aabbLow.y;
    vop.aabbZRun = aabbHigh.z - aabbLow.z;
    
    tbb::parallel_for( tbb::blocked_range<int>( 1, triangles.size()/3 ), vop );
    
    
    
    return 0;
    
}