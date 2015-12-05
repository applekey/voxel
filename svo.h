struct node
{
    int level;
    uint64_t elementMortonIndex;
    ////////////////////////////////
    struct node *  children[8];
    uint64_t offsets[8];
};

struct leaf
{
    uint64_t linksStartIndex;
    uint64_t offsets[8];
};

class svo{
    unsigned char * svoHead;
    FILE *f;
public:
    svo()
    {
        
    }
    
    uint64_t convertCharToMort(std::string val)
    {
        uint64_t startVal = 0;
        uint64_t add = 1;
        for(int i =0;i<8;i++)
        {
            if (val[i] == '1')
            {
                startVal += add;
            }
            add = add<<1;
        }
        return startVal;
    }
    
    void attachPrevNodeToCurrent(struct node * head, std::vector<struct node * > * nodeVec, uint64_t mortonCode )
    {
        uint64_t mymortonCode = mortonCode;
        for(int i = 7; i>=0 ;i--)
        {
            
            uint64_t code = mymortonCode & (0b0010000000);
            if (code == 128)
            {
                head->children[i] = nodeVec->back();
                nodeVec->pop_back();
            }
            mymortonCode = mymortonCode <<1 ;
        }
        assert(nodeVec->empty());
    }
    
    
    void teststrucnode(struct node * head, int & total, int level, int maxLevel)
    {
        assert(head->level == level);
        
        total ++;
        
        if (level == maxLevel)
        {
            return;
        }
        
        
        int newlevel = level + 1;
        uint64_t mymortonCode = head->elementMortonIndex;
        for(int i = 7; i >= 0;i--)
        {
            uint64_t code = mymortonCode & (0b0010000000);
            if (code == 128)
            {
                teststrucnode(head->children[i],total,newlevel,maxLevel);
            }
            mymortonCode = mymortonCode <<1 ;
        }
    }
    
    
    struct node * ReadIn (const char * fileName, int levels)
    {
        std::ifstream input(fileName);
        std::string line;
        
        
        int previousLevel = levels;
        std::vector<struct node * > * lvls = new std::vector<struct node * >[levels];
        
        while(std::getline(input, line ))
        {
            std::cout<<line<<'\n';
            // get level
            int level = (int)(line[6]) - '0';
            
            // grab morton code
            std::string mortCodeString = line.substr(17);
            uint64_t mortCode = convertCharToMort(mortCodeString);
            
            if(level == levels-1) // this is a root node
            {
                struct node * newNode = new struct node;
                newNode->level = level;
                newNode->elementMortonIndex = mortCode;
                lvls[level].push_back(newNode);
            }
            else //this is a leaf, remove previous nodes
            {
                //attach previous lvl elements to currentElement
                struct node * newNode = new struct node;
                newNode->level = level;
                newNode->elementMortonIndex = mortCode;
               
                std::vector<struct node * > *  prevLevel  = &lvls[level+1];
                
                attachPrevNodeToCurrent(newNode,prevLevel,mortCode);
                
                lvls[level+1].clear();
                lvls[level].push_back(newNode);

            }
        };
        return lvls[0][0];
    }
    
    void OpenFile()
    {
        f = fopen("file.txt", "w");
        if (f == NULL)
        {
            printf("Error opening file!\n");
            exit(1);
        }
    }
    
    void CloseFile()
    {
        fclose(f);
    }
    
    void WriteToFile(const char * val)
    {
        fprintf(f, "%s", val);
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
    
    int AddressCalc(int i,int j,int k, int size) // for testing only
    {
        int stride = size;
        int plane = size *size;
        
        return i + j* stride + k * plane;
    }
    
    void BuildSVOUnitTest()
    {
        int size =16;
        //create a dataset
        //
        unsigned char * voxSet = new unsigned char[size * size *size];
        
        
        for (int i = 0;i < size * size *size;i++ )
        {
            voxSet[i] = 0;
        }
        
        voxSet[AddressCalc(15, 0, 0,size)] = 1;
        
        voxSet[AddressCalc(1, 0, 1,size)] = 1;
        
        int testSize[3] = {size,size,size};
        
        BuildSVO(voxSet,testSize);
    }
    
    void BuildSVO(unsigned char * voxel, int * size)
    {
        
        assert(size[0] == size[1] && size[0] == size[2]);
        
        OpenFile();
        
        int levels = voxelLevels(size[0]);
        //create level queue
        unsigned char * lvlQueue = new unsigned char [levels * 8];
        
        int * lvlIndex = new int[levels];
        for(int i = 0; i < levels; i++)
        {
            lvlIndex[i] = 0;
        }
        
        uint64_t mortonCode = 0;
        
        unsigned int indexOffset = 0;
        
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
            char vall[10000] = "        ";
            while (currentLevel >= 0 && *currentLevelIndex == 8)
            {
                bool empty = true;
                //check to see if currentlevel is empty
                unsigned char writeVal = 0;
                unsigned char increment = 1;
                
                for(int i =0;i<8;i++)
                {
                    unsigned char voxVal = lvlQueue[currentLevel*8 + i];
                    vall[i] = '0';
                    if (voxVal != 0) {
                        empty = false;
                        //writeVal += increment;
                        vall[i] = '1';
                        
                    }
                    increment++;
                    
                }
            
                if(!empty)
                {
                    //write current level's to file
                    std::stringstream buffer;
                    
                    buffer<<"Level:"<<currentLevel<<" nodevals:"<<vall<<std::endl;
                    std::cout<<"Level:"<<currentLevel<<" nodevals:"<<vall<<std::endl;
                    WriteToFile(buffer.str().c_str());
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
        CloseFile();
    }

};