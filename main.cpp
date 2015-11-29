#include <stdio.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include </usr/local/include/glm/vec3.hpp> // glm::vec3
#include </usr/local/include/glm/vec4.hpp> // glm::vec4
#include </usr/local/include/glm/mat4x4.hpp> // glm::mat4
#include </usr/local/include/glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"

#define OBJFILE "/Users/applekey/Documents/datasets/monkey.obj"


std::vector<glm::vec3> LoadObj(const char * filename)
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

//void voxelOperator(unsigned int triId, const std::vector<glm::vec3> triangles, unsigned char * storage)
//{
//    unsigned int triangleVertId = triId * 3;
//}

struct voxelOperator {
    const std::vector<glm::vec3> triangles;
    unsigned char * storage;
    void operator()( const tbb::blocked_range<int>& range ) const {
        for( int i=range.begin(); i!=range.end(); ++i )
        {
            unsigned int triangleVertId = i * 3;
        }
    }
};


int main()
{
    //read in obj
    std::vector<glm::vec3> triangles = LoadObj(OBJFILE);
    voxelOperator vop;
    
    tbb::parallel_for( tbb::blocked_range<int>( 1, triangles.size()/3 ), vop );
    
    
    
}