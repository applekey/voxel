#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "common.h"
class render
{
    
public:
    
    void drawCube(int x, int y, int z, int drawLevel)
    {
        //std::cout<<"x:"<<x<<" y:"<<y<<" z:"<<z<<std::endl;
        //return;
        glPushMatrix();
        float factor = 1/pow(2,drawLevel);
        static float alpha = 00;
        //attempt to rotate cube
        glRotatef(alpha, 0, 1, 0);
        glScalef(factor, factor, factor);
        glColor3f(1.0f, 1.0f, 0.0f);
        
        glTranslatef(x*2,y*2,z*2);

        glBegin(GL_QUADS);
        
        //Front
        glNormal3f(0.0f, 0.0f, 1.0f);
        //glNormal3f(-1.0f, 0.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, 1.0f);
        //glNormal3f(1.0f, 0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, 1.0f);
        //glNormal3f(1.0f, 0.0f, 1.0f);
        glVertex3f(1.0f, 1.0f, 1.0f);
        //glNormal3f(-1.0f, 0.0f, 1.0f);
        glVertex3f(-1.0f, 1.0f, 1.0f);
        
        //Right
        glNormal3f(1.0f, 0.0f, 0.0f);
        //glNormal3f(1.0f, 0.0f, -1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        //glNormal3f(1.0f, 0.0f, -1.0f);
        glVertex3f(1.0f, 1.0f, -1.0f);
        //glNormal3f(1.0f, 0.0f, 1.0f);
        glVertex3f(1.0f, 1.0f, 1.0f);
        //glNormal3f(1.0f, 0.0f, 1.0f);
        glVertex3f(1.0f, -1.0f, 1.0f);
        
        //Back
        glNormal3f(0.0f, 0.0f, -1.0f);
        //glNormal3f(-1.0f, 0.0f, -1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        //glNormal3f(-1.0f, 0.0f, -1.0f);
        glVertex3f(-1.0f, 1.0f, -1.0f);
        //glNormal3f(1.0f, 0.0f, -1.0f);
        glVertex3f(1.0f, 1.0f, -1.0f);
        //glNormal3f(1.0f, 0.0f, -1.0f);
        glVertex3f(1.0f, -1.0f, -1.0f);
        
        //Left
        glNormal3f(-1.0f, 0.0f, 0.0f);
        //glNormal3f(-1.0f, 0.0f, -1.0f);
        glVertex3f(-1.0f, -1.0f, -1.0f);
        //glNormal3f(-1.0f, 0.0f, 1.0f);
        glVertex3f(-1.0f, -1.0f, 1.0f);
        //glNormal3f(-1.0f, 0.0f, 1.0f);
        glVertex3f(-1.0f, 1.0f, 1.0f);
        //glNormal3f(-1.0f, 0.0f, -1.0f);
        glVertex3f(-1.0f, 1.0f, -1.0f);
        
        glEnd();
        glPopMatrix();
        
        
        
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
    
    void drawRecuse(struct node * p, int currentLevel, int drawLevel, uint64_t currentMorton)
    {
        if (currentLevel == drawLevel)
        {
            int x,y,z;
            mortonDecode(currentMorton,x,y,z);
            drawCube(x,y,z,drawLevel);
            return;
        }
        uint64_t nextLevelMorton = currentMorton<<3;
        
        int newlevel = currentLevel + 1;
        uint64_t mymortonCode = p->elementMortonIndex;
        for(uint64_t i = 8; i >= 1;i--)
        {
            uint64_t code = mymortonCode & (0b0010000000);
            if (code == 128)
            {
                uint64_t elemMort = nextLevelMorton + i-1;
                drawRecuse(p->children[i-1],newlevel,drawLevel,elemMort);
            }
            mymortonCode = mymortonCode <<1 ;
        }
    }
    
    int draw(struct node * head, int level)
    {
        GLFWwindow* window;
        
        /* Initialize the library */
        if (!glfwInit())
            return -1;
        
        /* Create a windowed mode window and its OpenGL context */
        window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
        if (!window)
        {
            glfwTerminate();
            return -1;
        }

        /* Make the window's context current */
        glfwMakeContextCurrent(window);
        
        GLint windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        
        glEnable(GL_LIGHTING); //Enable lighting
        glEnable(GL_LIGHT0); //Enable light #0
        glEnable(GL_LIGHT1); //Enable light #1
        glEnable(GL_NORMALIZE); //Automatically normalize normals
        
        /* Loop until the user closes the window */
        while (!glfwWindowShouldClose(window))
        {
            // Scale to window size
            GLint windowWidth, windowHeight;
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            glViewport(0, 0, windowWidth, windowHeight);
            
            // Draw stuff
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            glMatrixMode(GL_PROJECTION_MATRIX);
            glLoadIdentity();
            gluPerspective( 60, (double)windowWidth / (double)windowHeight, 0.1, 100 );
            
            glMatrixMode(GL_MODELVIEW_MATRIX);
            glTranslatef(-1,-2,-5);
            
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_COLOR_MATERIAL);
            
            //Add ambient light
            GLfloat ambientColor[] = {0.2f, 0.4f, 0.4f, 1.0f}; //Color(0.2, 0.2, 0.2)
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);
            
            //Add positioned light
            GLfloat lightColor0[] = {0.5f, 0.5f, 0.5f, 1.0f}; //Color (0.5, 0.5, 0.5)
            GLfloat lightPos0[] = {6.0f, 3.0f, 6.0f, 1.0f}; //Positioned at (4, 0, 8)
            glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
            glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
            
            //Add directed light
            GLfloat lightColor1[] = {1.0f, 0.2f, 0.2f, 1.0f}; //Color (0.5, 0.2, 0.2)
            //Coming from the direction (-1, 0.5, 0.5)
            GLfloat lightPos1[] = {-1.0f, 0.5f, 0.5f, 0.0f};
            glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1);
            glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);
            
            
            
            drawRecuse(head,0,level,0);
            
            
            
            // Update Screen
            glfwSwapBuffers(window);
            
            // Check for any input, or window movement
            glfwPollEvents();
        }
        
        glfwTerminate();
        return 0;

    }
};