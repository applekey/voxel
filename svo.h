
#include <iomanip>
class svo{

    unsigned char * svoHead;
public:
    svo()
    {
        
    }
    
    void WriteToFile(char * val)
    {
        FILE *f = fopen("file.txt", "w");
        if (f == NULL)
        {
            printf("Error opening file!\n");
            exit(1);
        }
        
        /* print some text */
        const char *text = "Write this to the file";
        fprintf(f, "Some text: %s\n", text);
        
        fclose(f);
    }
    
    int voxelLevels(int x)
    {
        int levels = 0;
        while (((x & 1) == 0) && x > 1)
        {
            levels +=1;
            x >>= 1;
        }
        return levels;
    }
    
    //http://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
    inline uint64_t mortonEncode_for(unsigned int x, unsigned int y, unsigned int z){
        uint64_t answer = 0;
        for (uint64_t i = 0; i < (sizeof(uint64_t)* CHAR_BIT)/3; ++i) {
            answer |= ((x & ((uint64_t)1 << i)) << 2*i) | ((y & ((uint64_t)1 << i)) << (2*i + 1)) | ((z & ((uint64_t)1 << i)) << (2*i + 2));
        }
        return answer;
    }
    
    void mortonDecode(uint64_t morton, int& x, int& y, int& z){
        x = 0;
        y = 0;
        z = 0;
        for (uint64_t i = 0; i < (sizeof(uint64_t) * CHAR_BIT)/3; ++i) {
            x |= ((morton & (uint64_t( 1ull ) << uint64_t((3ull * i) + 0ull))) >> uint64_t(((3ull * i) + 0ull)-i));
            y |= ((morton & (uint64_t( 1ull ) << uint64_t((3ull * i) + 1ull))) >> uint64_t(((3ull * i) + 1ull)-i));
            z |= ((morton & (uint64_t( 1ull ) << uint64_t((3ull * i) + 2ull))) >> uint64_t(((3ull * i) + 2ull)-i));
        }
    }
    
    unsigned char voxelAccess(unsigned char * voxel, int x, int y, int z, int * size)
    {
        int plane = size[0] * size[1];
        int stride = size[0];
        
        return voxel[z * plane + y * stride + x];
    }
    
    int AddressCalc(int i,int j,int k) // for testing only
    {
        return i + j* 8 + k * 64;
    }
    
    void BuildSVOUnitTest()
    {
        //create a dataset
        //
        unsigned char * voxSet = new unsigned char[8 * 8 * 8];
        int stride = 8;
        int plane = 64;
        
        for (int i = 0;i < 8*8*8;i++ )
        {
            voxSet[i] = 0;
        }
        
        voxSet[AddressCalc(0, 0, 0)] = 1;
        
        int testSize[3] = {8,8,8};
        
        BuildSVO(voxSet,testSize);
        
        
        
        
    }
    
    void BuildSVO(unsigned char * voxel, int * size)
    {
        assert(size[0] == size[1] && size[0] == size[2]);
        int levels = voxelLevels(size[0]);
        //create level queue
        unsigned char * lvlQueue = new unsigned char [levels * 8];
        
        int * lvlIndex = new int[levels];
        for(int i = 0; i < levels; i++)
        {
            lvlIndex[i] = 0;
        }
        
        uint64_t mortonCode = 0;
        
        for(int i = 0; i < size[0]*size[0]*size[0];i++)
        {
            int x,y,z;
            mortonDecode(mortonCode,x,y,z);
            
            //std::cout<<"x:"<<x<<" y:"<<y<<" z:"<<z<<std::endl;
            unsigned char vox = voxelAccess(voxel, x,y,z,size);
            
            //add new node to lowest level
            int * currentLevelIndex = &lvlIndex[levels-1];
            lvlQueue[(levels-1)*8 + *currentLevelIndex] = vox;
            *currentLevelIndex +=1;
            
            // check if level is full
            int currentLevel = levels - 1;
            while (currentLevel > 0 && *currentLevelIndex == 8)
            {
                bool empty = true;
                //check to see if currentlevel is empty
                unsigned char writeVal = 0;
                unsigned char increment = 1;
                for(int i =0;i<8;i++)
                {
                    unsigned char voxVal = lvlQueue[currentLevel*8 + i];
                    
                    if (voxVal != 0) {
                        empty = false;
                        writeVal += increment;
                        increment = increment<<1;
                    }
                    
                }
            
                if(!empty)
                {
                    //write current level's to file
                    std::cout<<"Level:"<<currentLevel<<" nodevals:"<<static_cast<unsigned>(writeVal)<<std::endl;
                }
                *currentLevelIndex = 0;
                //go up a level
                currentLevel -= 1;
                // insert entry into lvlQueue
                currentLevelIndex = &lvlIndex[currentLevel];
                lvlQueue[currentLevel*8 + *currentLevelIndex] = empty? 0:1;
                *currentLevelIndex +=1;
                
            }
            
            //std::cout<<x<<" "<<y<<" "<<z<<std::endl;
            mortonCode++;
        }
        
    }

};