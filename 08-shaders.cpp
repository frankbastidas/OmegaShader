//==============================================================================
/*
    Software License Agreement (BSD License)
    Copyright (c) 2003-2016, CHAI3D.
    (www.chai3d.org)

    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

    * Neither the name of CHAI3D nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE. 

    \author    <http://www.chai3d.org>
    \author    Francois Conti
    \version   3.2.0 $Rev: 1929 $
*/
//==============================================================================

//------------------------------------------------------------------------------
#include "chai3d.h"
//------------------------------------------------------------------------------
#include <GLFW/glfw3.h>
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled 
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;

const double SPHERE_RADIUS = 0.02;
//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// four cameras to render the world from different perspectives
cCamera* cameraView1;
cCamera* cameraView2;

// Four colored background
cBackground* background;
cBackground* background2;

// Four view panels
cViewPanel* viewPanel1;
cViewPanel* viewPanel2;

// Four framebuffer
cFrameBufferPtr frameBuffer1;
cFrameBufferPtr frameBuffer2;

//------------------------------------------------------------------------------
// STATES
//------------------------------------------------------------------------------
enum MouseState
{
    MOUSE_IDLE,
    MOUSE_SELECTION
};

// a light source
cSpotLight *light;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a virtual tool representing the haptic device in the scene
cToolCursor* tool;

// a line representing the velocity vector of the haptic device
cShapeLine* velocity;

// a virtual mesh like object
cMesh* object;

// sphere objects
cMesh* spheres;


// a font for rendering text
cFontPtr font;

// a font for rendering text of position tool
cFontPtr fontPos;

// a label to display the rate [Hz] at which the simulation is running
cLabel* labelRates;
cLabel* labelRates2;

// a label to display the rate [Hz] at which the simulation is running
cLabel* labelRatesPos;


// a flag that indicates if the haptic simulation is currently running
bool simulationRunning = false;

// a flag that indicates if the haptic simulation has terminated
bool simulationFinished = true;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = NULL;

// current width of window
int width  = 0;

// current height of window
int height = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;


// mouse position
double mouseX, mouseY;

// mouse state
MouseState mouseState = MOUSE_IDLE;

//------------------------------------------------------------------------------
// RELIEF MAPPING
//------------------------------------------------------------------------------

//Scale Relief
float heightScale;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// DECLARED MACROS
//------------------------------------------------------------------------------
// convert to resource path
#define RESOURCE_PATH(p)    (char*)((resourceRoot+string(p)).c_str())


//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window display is resized
void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void errorCallback(int error, const char* a_description);

// callback when a key is pressed
void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);

// callback to handle mouse click
void mouseButtonCallback(GLFWwindow* a_window, int a_button, int a_action, int a_mods);

// callback to handle mouse motion
void mouseMotionCallback(GLFWwindow* a_window, double a_posX, double a_posY);

// this function renders the scene
void updateGraphics(void);

// this function contains the main haptics simulation loop
void updateHaptics(void);

// this function closes the application
void close(void);


bool moveW = false;

//==============================================================================
/*
    DEMO:   08-shaders.cpp

    This example illustrates how to build a small mesh cube.
    The applications also presents the use of texture properties by defining 
    a texture image and associated texture coordinates for each of the vertices.
    A bump map is also used in combination with a shader to create a more
    realistic rendering of the surface.
*/
//==============================================================================

int main(int argc, char* argv[])
{
    //--------------------------------------------------------------------------
    // INITIALIZATION
    //--------------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CHAI3D" << endl;
    cout << "Demo: 08-shaders" << endl;
    cout << "Copyright 2003-2016" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[f] - Enable/Disable full screen mode" << endl;
    cout << "[m] - Enable/Disable vertical mirroring" << endl;
    cout << "[q] - Exit application" << endl;
    cout << endl << endl;

    // parse first arg to try and locate resources
    string resourceRoot = string(argv[0]).substr(0,string(argv[0]).find_last_of("/\\")+1);


    //--------------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //--------------------------------------------------------------------------

    // initialize GLFW library
    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    // set error callback
    glfwSetErrorCallback(errorCallback);

    // compute desired size of window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int w = 0.8 * mode->height;
    int h = 0.5 * mode->height;
    int x = 0.5 * (mode->width - w);
    int y = 0.5 * (mode->height - h);

    // set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // set active stereo mode
    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    // create display context
    window = glfwCreateWindow(w, h, "CHAI3D", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    // get width and height of window
    glfwGetWindowSize(window, &width, &height);

    // set position of window
    glfwSetWindowPos(window, x, y);

    // set key callback
    glfwSetKeyCallback(window, keyCallback);

    // set resize callback
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    // set mouse position callback
    glfwSetCursorPosCallback(window, mouseMotionCallback);

    // set mouse button callback
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // set current display context
    glfwMakeContextCurrent(window);

    // sets the swap interval for the current display context
    glfwSwapInterval(swapInterval);

#ifdef GLEW_VERSION
    // initialize GLEW library
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif


    //--------------------------------------------------------------------------
    // WORLD
    //--------------------------------------------------------------------------
    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    // the color is defined by its (R,G,B) components.
    world->m_backgroundColor.setWhite();

    //--------------------------------------------------------------------------
    // CAMERA
    //--------------------------------------------------------------------------

    // create a camera and insert it into the virtual world
    //camera = new cCamera(world);
    camera = new cCamera(NULL);

    cameraView1 = new cCamera(world);

    world->addChild(cameraView1);

    // position and orient the camera
    cameraView1->set(cVector3d(1.43, 0.4, 0.860),    // camera position (eye)
                    cVector3d(-0.732, -0.252, -0.63),    // lookat position (target)
                    cVector3d(-0.602, -0.180, 0.773));   // direction of the "up" vector

// set the near and far clipping planes of the camera
// anything in front or behind these clipping planes will not be rendered
    cameraView1->setClippingPlanes(0.01, 10.0);

    // set stereo mode
    cameraView1->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    cameraView1->setStereoEyeSeparation(0.03);
    cameraView1->setStereoFocalLength(3.0);

    // set vertical mirrored display mode
    cameraView1->setMirrorVertical(mirroredDisplay);

    // set camera field of view 
    cameraView1->setFieldViewAngleDeg(45);

    //create camera 2

    cameraView2 = new cCamera(world);

    world->addChild(cameraView2);

    // position and orient the camera
    cameraView2->set(cVector3d(1.5, 0.0, -0.27),    // camera position (eye)
                    cVector3d(0.0, 0.0, 0.0),    // lookat position (target)
                    cVector3d(0.0, 0.0, 1.0));   // direction of the "up" vector

// set the near and far clipping planes of the camera
// anything in front or behind these clipping planes will not be rendered
    cameraView2->setClippingPlanes(0.01, 10.0);

    // set stereo mode
    cameraView2->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    cameraView2->setStereoEyeSeparation(0.03);
    cameraView2->setStereoFocalLength(3.0);

    // set vertical mirrored display mode
    cameraView2->setMirrorVertical(mirroredDisplay);

    // set camera field of view 
    cameraView2->setFieldViewAngleDeg(45);


    //--------------------------------------------------------------------------
    // LIGHT SOURCES
    //--------------------------------------------------------------------------

    // create a light source
    light = new cSpotLight(world);

    // add light to world
    world->addChild(light);

    // enable light source
    light->setEnabled(true);

    // position the light source
    light->setLocalPos(3.5, 2.0, 8.0);

    // define the direction of the light beam
    light->setDir(-3.5, -2.0, -8.0);

    // set light cone half angle
    light->setCutOffAngleDeg(20);


    //--------------------------------------------------------------------------
    // HAPTIC DEVICES / TOOLS
    //--------------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

    // get access to the first available haptic device
    handler->getDevice(hapticDevice, 0);

    // retrieve information about the current haptic device
    cHapticDeviceInfo hapticDeviceInfo = hapticDevice->getSpecifications();

    // if the device has a gripper, enable the gripper to simulate a user switch
    hapticDevice->setEnableGripperUserSwitch(true);

    // create a 3D tool and add it to the world
    tool = new cToolCursor(world);
    world->addChild(tool);

    // connect the haptic device to the tool
    tool->setHapticDevice(hapticDevice);

    // define the radius of the tool (sphere)
    double toolRadius = SPHERE_RADIUS;

    // define a radius for the tool
    tool->setRadius(toolRadius);

    // set color of proxy sphere
    tool->m_hapticPoint->m_sphereProxy->m_material->setWhite();

    // show proxy and device position of finger-proxy algorithm
    tool->setShowContactPoints(true, true, cColorf(0.0, 0.0, 0.0));

    // enable if objects in the scene are going to rotate of translate
    // or possibly collide against the tool. If the environment
    // is entirely static, you can set this parameter to "false"
    tool->enableDynamicObjects(false);

    // map the physical workspace of the haptic device to a larger virtual workspace.
    tool->setWorkspaceRadius(0.9);

    // start the haptic tool
    tool->start();

    // create small line to illustrate the velocity of the haptic device
    velocity = new cShapeLine(cVector3d(0, 0, 0),
        cVector3d(0, 0, 0));

    // insert line inside world
    world->addChild(velocity);

    //--------------------------------------------------------------------------
    // CREATE OBJECT
    //--------------------------------------------------------------------------

    // read the scale factor between the physical workspace of the haptic
    // device and the virtual workspace defined for the tool
    double workspaceScaleFactor = tool->getWorkspaceScaleFactor();

    // stiffness properties
    double maxStiffness = hapticDeviceInfo.m_maxLinearStiffness / workspaceScaleFactor;

    // create a virtual mesh
    object = new cMesh();

    // add object to world
    world->addChild(object);

    // set the position of the object at the center of the world
    //object->setLocalPos(0.0, 0.0, -0.75);
    object->setLocalPos(0.0, 0.0, -0.3);

    // create cube
    //cCreateBox(object, 0.8, 0.8, 0.8);
    
    // create plane
    cCreatePlane(object, 0.9, 0.9);

    // create a texture
    cTexture2dPtr texture = cTexture2d::create();
       
    bool fileload = texture->loadFromFile(RESOURCE_PATH("../resources/images/wood.png"));
    if (!fileload)
    {
        #if defined(_MSVC)
        fileload = texture->loadFromFile("../../../bin/resources/images/wood.png");
        #endif
    }
    if (!fileload)
    {
        cout << "Error - Texture image failed to load correctly." << endl;
       // close();
       // return (-1);
    }

    cTexture2dPtr texture2 = cTexture2d::create();
    texture2->setTextureUnit(GL_TEXTURE4);
    fileload = texture2->loadFromFile(RESOURCE_PATH("../../../resources/images/toy_box_disp.png"));
    if (!fileload)
    {
#if defined(_MSVC)
        fileload = texture2->loadFromFile("../../../bin/resources/images/toy_box_disp.png");
#endif
    }
    if (!fileload)
    {
        cout << "Error - Texture2 image failed to load correctly." << endl;
        // close();
        // return (-1);
    }


    // apply texture to object
    object->setTexture(texture);
    object->setTexture2(texture2);
    // enable texture rendering 
    object->setUseTexture(true);

    // Since we don't need to see our polygons from both sides, we enable culling.
    object->setUseCulling(true);

    // set material properties to light gray
    object->m_material->setWhite();

    // set material shininess
    object->m_material->setShininess(80);

    // compute collision detection algorithm
    object->createAABBCollisionDetector(toolRadius);

    // define a default stiffness for the object
    //object->m_material->setStiffness(0.5 * maxStiffness);
    object->m_material->setStiffness(0.5 * maxStiffness);

    // define some static friction
    object->m_material->setStaticFriction(0.0); //0.2

    // define some dynamic friction
    object->m_material->setDynamicFriction(0.0); //0.2

    // define some texture rendering
    object->m_material->setTextureLevel(1.0);

    // render triangles haptically on front side only
    object->m_material->setHapticTriangleSides(true, false);

    // create a normal texture
    cNormalMapPtr normalMap = cNormalMap::create();

    // load normal map from file
    fileload = normalMap->loadFromFile(RESOURCE_PATH("../resources/images/toy_box_normal.png"));
    if (!fileload)
    {
#if defined(_MSVC)
        fileload = normalMap->loadFromFile("../../../bin/resources/images/toy_box_normal.pngq");
#endif
    }
    if (!fileload)
    {
        cout << "Error - Texture image failed to load correctly." << endl;
        close();
        return (-1);
    }

    // assign normal map to object
    object->m_normalMap = normalMap;

    // compute tangent vectors
    object->computeBTN();   
    

    //--------------------------------------------------------------------------
    // CREATE SHADERS
    //--------------------------------------------------------------------------

    // create vertex shader
    cShaderPtr vertexShader = cShader::create(C_VERTEX_SHADER);
    
    string modeMappingV, modeMappingF;
    int modeM = 1;

    switch (modeM)
    {
    case 0:
       modeMappingV = RESOURCE_PATH("../resources/shaders/bump.vert");
        modeMappingF = RESOURCE_PATH("../resources/shaders/bump.frag");
#if defined(_MSVC)
        modeMappingV = "../../../bin/resources/shaders/bump.vert";
        modeMappingF = "../../../bin/resources/shaders/bump.frag";
#endif
        break;
    case 1:
        modeMappingV = RESOURCE_PATH("../resources/shaders/parallaxmapping.vert");
        modeMappingF = RESOURCE_PATH("../resources/shaders/parallaxmapping.frag");
#if defined(_MSVC)
        modeMappingV = "../../../bin/resources/shaders/parallaxmapping.vert";
        modeMappingF = "../../../bin/resources/shaders/parallaxmapping.frag";
#endif
        break;
    case 2:
        modeMappingV = RESOURCE_PATH("../resources/shaders/ReliefMapping.vert");
        modeMappingF = RESOURCE_PATH("../resources/shaders/ReliefMapping.frag");
#if defined(_MSVC)
        modeMappingV = "../../../bin/resources/shaders/ReliefMapping.vert";
        modeMappingF = "../../../bin/resources/shaders/ReliefMapping.frag";
#endif
        break;
    default:
        break;
    }

    // load vertex shader from file
    fileload = vertexShader->loadSourceFile(modeMappingV);
    /*if (!fileload)
    {
#if defined(_MSVC)
        fileload = vertexShader->loadSourceFile("../../../bin/resources/shaders/parallaxmapping.vert");
#endif
    }*/

    // create fragment shader
    cShaderPtr fragmentShader = cShader::create(C_FRAGMENT_SHADER);
    
    // load fragment shader from file
    fileload = fragmentShader->loadSourceFile(modeMappingF);
    /*if (!fileload)
    {
#if defined(_MSVC)
        fileload = fragmentShader->loadSourceFile("../../../bin/resources/shaders/parallaxmapping.frag");
#endif
    }*/

    // create program shader
    cShaderProgramPtr programShader = cShaderProgram::create();

    // assign vertex shader to program shader
    programShader->attachShader(vertexShader);

    // assign fragment shader to program shader
    programShader->attachShader(fragmentShader);
    
    // assign program shader to object
    object->setShaderProgram(programShader);
    
    // link program shader
    programShader->linkProgram();

    // set uniforms
    programShader->setUniformi("uColorMap", 0);
    //programShader->setUniformi("uColorMap2", 4);
    programShader->setUniformi("uDepthMap", 4);
    //programShader->setUniformi("uShadowMap", 0);
    programShader->setUniformi("uNormalMap", 2);
    programShader->setUniformf("uInvRadius", 0.0f);


    //--------------------------------------------------------------------------
   // CREATE SPHERES
   //--------------------------------------------------------------------------
    
    // create a virtual mesh
    spheres = new cMesh();

    // add object to world
    world->addChild(spheres);

    spheres->setLocalPos(tool->getDeviceGlobalPos());

    cCreateSphere(spheres, toolRadius);
    //cCreateBox(spheres, toolRadius*2, toolRadius*2, toolRadius*2);

   // create texture
    cTexture2dPtr texture3 = cTexture2d::create();

    // load texture file
    fileload = texture3->loadFromFile(RESOURCE_PATH("../resources/images/spheremap-3.jpg"));
    if (!fileload)
    {
#if defined(_MSVC)
        fileload = texture3->loadFromFile("../../../bin/resources/images/spheremap-3.jpg");
#endif
    }
    if (!fileload)
    {
        cout << "Error - Texture image failed to load correctly." << endl;
        close();
        return (-1);
    }


    // apply texture to object
    spheres->setTexture(texture3);
    // enable texture rendering 
    spheres->setUseTexture(true);

    // Since we don't need to see our polygons from both sides, we enable culling.
    spheres->setUseCulling(true);

    // set material properties to light gray
    spheres->m_material->setWhite();

    // set material shininess
    spheres->m_material->setShininess(80);

    // compute collision detection algorithm
    //spheres->createAABBCollisionDetector(toolRadius);

    // define a default stiffness for the object
    //object->m_material->setStiffness(0.5 * maxStiffness);
    spheres->m_material->setStiffness(0.8 * maxStiffness);

    // define some static friction
    spheres->m_material->setStaticFriction(0.0); //0.2

    // define some dynamic friction
    spheres->m_material->setDynamicFriction(0.0); //0.2

    // define some texture rendering
    spheres->m_material->setTextureLevel(1.0);

    
    // compute tangent vectors
    spheres->computeBTN();

    // create fragment shader
    cShaderPtr fragmentShader2 = cShader::create(C_FRAGMENT_SHADER);
    cShaderPtr vertexShader2 = cShader::create(C_VERTEX_SHADER);

    fileload = vertexShader2->loadSourceFile(RESOURCE_PATH("../resources/shaders/phong.vert"));
    if (!fileload)
{
#if defined(_MSVC)
        fileload = vertexShader2->loadSourceFile("../../../bin/resources/shaders/phong.vert");
#endif
    }
    fileload = fragmentShader2->loadSourceFile(RESOURCE_PATH("../resources/shaders/phong.frag"));
    if (!fileload){
#if defined(_MSVC)
    fileload = fragmentShader2->loadSourceFile("../../../bin/resources/shaders/phong.frag");
#endif
    }

    // create program shader
    cShaderProgramPtr programShader2 = cShaderProgram::create();
    // assign vertex shader to program shader
    programShader2->attachShader(vertexShader2);

    // assign fragment shader to program shader
    programShader2->attachShader(fragmentShader2);

    spheres->setShaderProgram(programShader2);

    // link program shader
    programShader2->linkProgram();
    // set uniforms
    //programShader2->setUniformi("uColorMap", 0);
    //programShader->setUniformi("uColorMap2", 4);
    //programShader2->setUniformi("uDepthMap", 4);
    programShader->setUniformi("uShadowMap", 0);
    //programShader2->setUniformi("uNormalMap", 2);
    //programShader2->setUniformf("uInvRadius", 0.0f);


    //tool->setShaderProgram(programShader2);
    // link program shader

    //--------------------------------------------------------------------------
// FRAMEBUFFERS
//--------------------------------------------------------------------------

    // create framebuffer for view 1
    frameBuffer1 = cFrameBuffer::create();
    frameBuffer1->setup(cameraView1);

    // create framebuffer for view 2
    frameBuffer2 = cFrameBuffer::create();
    frameBuffer2->setup(cameraView2);

    //--------------------------------------------------------------------------
// VIEW PANELS
//--------------------------------------------------------------------------

    // create and setup view panel 1
    viewPanel1 = new cViewPanel(frameBuffer1);
    camera->m_frontLayer->addChild(viewPanel1);

    // create and setup view panel 2
    viewPanel2 = new cViewPanel(frameBuffer2);
    camera->m_frontLayer->addChild(viewPanel2);

    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    font = NEW_CFONTCALIBRI20();

    // create a label to display the haptic and graphic rate of the simulation
    labelRates = new cLabel(font);
    cameraView1->m_frontLayer->addChild(labelRates);

    // set font color
    labelRates->m_fontColor.setWhite();

    // create a background
    background = new cBackground();
    cameraView1->m_backLayer->addChild(background);

    // set background properties
    background->setCornerColors(cColorf(0.3, 0.3, 0.3),
                                cColorf(0.3, 0.3, 0.3),
                                cColorf(0.1, 0.1, 0.1),
                                cColorf(0.1, 0.1, 0.1));
    //-----------------------------------------
    // create a label to display the haptic and graphic rate of the simulation
    labelRates2 = new cLabel(font);
    cameraView2->m_frontLayer->addChild(labelRates2);

    // set font color
    labelRates2->m_fontColor.setWhite();

    // create a background
    background2 = new cBackground();
    cameraView2->m_backLayer->addChild(background2);

    // set background properties
    background2->setCornerColors(cColorf(0.3, 0.3, 0.3),
                                cColorf(0.3, 0.3, 0.3),
                                cColorf(0.1, 0.1, 0.1),
                                cColorf(0.1, 0.1, 0.1));


    //--------------------------------------------------------------------------
    // START SIMULATION
    //--------------------------------------------------------------------------

    // create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);

    //object->setWireMode(true);
    //object->setShowNormals(true);
    
    //object->setShowTangents(true);
    //object->setNormalsProperties(0.5, cColorf(0.1, 0.8, 0.2));

    
    heightScale = 0.0;
    //object->heighC = 0.3125 * heightScale + 0.01;
    object->heighC = 0.45977 * heightScale + 0.01;

    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    // call window size callback at initialization
    windowSizeCallback(window, width, height);

    // main graphic loop
    while (!glfwWindowShouldClose(window))
    {
        // get width and height of window
        glfwGetWindowSize(window, &width, &height);

        // render graphics
        updateGraphics();

        // swap buffers
        glfwSwapBuffers(window);

        // process events
        glfwPollEvents();
        programShader->setUniformf("heightScale", heightScale);
        //programShader2->setUniformf("heightScale", heightScale);

        //print scale relieve
        cout << heightScale <<", "<< object->heighC << endl;

        // signal frequency counter
        freqCounterGraphics.signal(1);
    }

    // close window
    glfwDestroyWindow(window);

    // terminate GLFW library
    glfwTerminate();

    // exit
    return 0;
}

//------------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update window size
    width  = a_width;
    height = a_height;

    int halfW = width / 2;
    int halfH = height;
    int offset = 1;

    // update display panel sizes and positions
    viewPanel1->setLocalPos(0.0, 0.0);
    viewPanel1->setSize(halfW, halfH);

    viewPanel2->setLocalPos(halfW, 0.0);
    viewPanel2->setSize(halfW, halfH);

    // update frame buffer sizes
    frameBuffer1->setSize(halfW, halfH);
    frameBuffer2->setSize(halfW, halfH);
}

//------------------------------------------------------------------------------

void errorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}

//------------------------------------------------------------------------------

void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    // filter calls that only include a key press
    if ((a_action != GLFW_PRESS) && (a_action != GLFW_REPEAT))
    {
        return;
    }

    // option - exit
    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);
    }

    // option - toggle fullscreen
    else if (a_key == GLFW_KEY_F)
    {
        // toggle state variable
        fullscreen = !fullscreen;

        // get handle to monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        // get information about monitor
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // set fullscreen or window mode
        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
    }

    // option - toggle vertical mirroring
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }
    // option - chage Scale of height Depth
    else if (a_key == GLFW_KEY_R)
    {
        if (heightScale > 0.0f)
            heightScale -= 0.0005f;
        else
            heightScale = 0.0f;
       // object->heighC = 0.3125 * heightScale + 0.01; 
        object->heighC = 0.45977 * heightScale + 0.01;
    }
    else if (a_key == GLFW_KEY_E)
    {
        if (heightScale < 1.0f)
            heightScale += 0.0005f;
        else
            heightScale = 1.0f;
        //object->heighC = 0.3125 * heightScale + 0.01;
        object->heighC = 0.45977 * heightScale + 0.01;
    }
    // option - chage Scale of height Depth
    else if (a_key == GLFW_KEY_T)
    {
        object->heighC -= 0.005f;
    }
    else if (a_key == GLFW_KEY_Y)
    {
        object->heighC += 0.005f;
    }
    //----------------MOVE-----------------
    else if (a_key == GLFW_KEY_W)
    {
        cameraView1->setLocalPos(cameraView1->getLocalPos()+cVector3d(-0.01,0.0,0.0));
    }
    else if (a_key == GLFW_KEY_S)
    {
        cameraView1->setLocalPos(cameraView1->getLocalPos() + cVector3d(0.01, 0.0, 0.0));
    }
    else if (a_key == GLFW_KEY_A)
    {
        cameraView1->setLocalPos(cameraView1->getLocalPos() + cVector3d(0.0, -0.01, 0.0));
    }
    else if (a_key == GLFW_KEY_D)
    {
        cameraView1->setLocalPos(cameraView1->getLocalPos() + cVector3d(0.0, 0.01, 0.0));
    }
    else if (a_key == GLFW_KEY_Z)
    {
        cameraView1->setLocalPos(cameraView1->getLocalPos() + cVector3d(0.0, 0.00, -0.01));
    }
    else if (a_key == GLFW_KEY_X)
    {
        cameraView1->setLocalPos(cameraView1->getLocalPos() + cVector3d(0.0, 0.0, 0.01));
    }
    else if (a_key == GLFW_KEY_U)
    {
        cout << cameraView1->getLocalPos().str(3) << endl;
        cout << cameraView1->getLookVector().str(3) << endl;
        cout << cameraView1->getUpVector().str(3) << endl;
    }
}

//------------------------------------------------------------------------------

void mouseButtonCallback(GLFWwindow* a_window, int a_button, int a_action, int a_mods)
{
    if (a_action == GLFW_PRESS)
    {
        // store mouse position
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // variable for storing collision information
        cCollisionRecorder recorder;
        cCollisionSettings settings;
        mouseState = MOUSE_SELECTION;
        //cout << "hola" << endl;
            
    }
    else
    {
        mouseState = MOUSE_IDLE;
    }
}

void mouseMotionCallback(GLFWwindow* a_window, double a_posX, double a_posY)
{
    if (mouseState == MOUSE_SELECTION)
    {
        
        double posRelX = (a_posX - (0.5 * width));
        double posRelY = ((height - a_posY) - (0.5 * height));
        double Xnew = mouseX - a_posX;
        double Ynew = mouseY - a_posY;
        //cout << Xnew << endl;
        cameraView1->rotateAboutLocalAxisDeg(cVector3d(0.0,0.0,1.0),Xnew/100.0);
        cameraView1->rotateAboutLocalAxisDeg(cVector3d(0.0, 1.0, 0.0), Ynew / 100.0);
        
        //cout << "hola" << endl;

        // get the vector that goes from the camera to the selected point (mouse click)
        /*cVector3d vCameraObject = selectedPoint - camera->getLocalPos();
        
        // get the vector that point in the direction of the camera. ("where the camera is looking at")
        cVector3d vCameraLookAt = camera->getLookVector();

        // compute the angle between both vectors
        double angle = cAngle(vCameraObject, vCameraLookAt);

        // compute the distance between the camera and the plane that intersects the object and 
        // which is parallel to the camera plane
        double distanceToObjectPlane = vCameraObject.length() * cos(angle);

        // convert the pixel in mouse space into a relative position in the world
        double factor = (distanceToObjectPlane * tan(0.5 * camera->getFieldViewAngleRad())) / (0.5 * height);
        double posRelX = factor * (a_posX - (0.5 * width));
        double posRelY = factor * ((height - a_posY) - (0.5 * height));

        // compute the new position in world coordinates
        cVector3d pos = camera->getLocalPos() +
            distanceToObjectPlane * camera->getLookVector() +
            posRelX * camera->getRightVector() +
            posRelY * camera->getUpVector();

        // compute position of object by taking in account offset
        cVector3d posObject = pos - selectedObjectOffset;

        // apply new position to object
        selectedObject->setLocalPos(posObject);

        // place cursor at the position of the mouse click
        sphereSelect->setLocalPos(pos);*/
    }
}

//------------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // close haptic device
    tool->stop();

    // delete resources
    delete hapticsThread;
    delete world;
    delete handler;
}

//------------------------------------------------------------------------------

void updateGraphics(void)
{
    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////

    // update haptic and graphic rate data
    labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
                        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    // update position of label
    labelRates->setLocalPos((int)(0.5 * (width - labelRates->getWidth())), 15);

    // update haptic and graphic rate data
    labelRates2->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    // update position of label
    labelRates2->setLocalPos((int)(0.5 * (width - labelRates2->getWidth())), 15);

    cVector3d posA = tool->getDeviceGlobalPos();
    // update haptic and graphic rate data
    /*labelRatesPos->setText("Position Tool : " + posA.str(3));

    // update position of label
    labelRatesPos->setLocalPos((int)(0.5 * (width - labelRatesPos->getWidth())), 30);*/


    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////

    // update shadow maps (if any)
    //world->updateShadowMaps(false, mirroredDisplay);

    // render all framebuffers
    frameBuffer1->renderView();
    frameBuffer2->renderView();

    // render world
    camera->renderView(width, height);

    // wait until all GL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) cout << "Error: " << gluErrorString(err) << endl;
}

//------------------------------------------------------------------------------

void updateHaptics(void)
{
    // angular velocity of object
    cVector3d angVel(0.0, 0.2, 0.3);

    // reset clock
    cPrecisionClock clock;
    clock.reset();

    // simulation in now running
    simulationRunning  = true;
    simulationFinished = false;

    // main haptic simulation loop
    while(simulationRunning)
    {
        /////////////////////////////////////////////////////////////////////
        // SIMULATION TIME
        /////////////////////////////////////////////////////////////////////

        // stop the simulation clock
        clock.stop();

        // read the time increment in seconds
        double timeInterval = clock.getCurrentTimeSeconds();

        // restart the simulation clock
        clock.reset();
        clock.start();

        // signal frequency counter
        freqCounterHaptics.signal(1);


        /////////////////////////////////////////////////////////////////////
        // HAPTIC FORCE COMPUTATION
        /////////////////////////////////////////////////////////////////////

        // compute global reference frames for each object
        world->computeGlobalPositions(true);

        // update position and orientation of tool
        tool->updateFromDevice();

        // compute interaction forces
        tool->computeInteractionForces();

        // send forces to haptic device
        tool->applyToDevice();

        spheres->setLocalPos(tool->getDeviceGlobalPos());
        /////////////////////////////////////////////////////////////////////
        // DYNAMIC SIMULATION
        /////////////////////////////////////////////////////////////////////
        /*
        // some constants
        const double INERTIA = 0.4;
        const double MAX_ANG_VEL = 10.0;
        const double DAMPING = 0.1;

        // get position of cursor in global coordinates
        cVector3d toolPos = tool->getDeviceGlobalPos();

        // get position of object in global coordinates
        cVector3d objectPos = object->getGlobalPos();

        // compute a vector from the center of mass of the object (point of rotation) to the tool
        cVector3d v = cSub(toolPos, objectPos);

        // compute angular acceleration based on the interaction forces
        // between the tool and the object
        cVector3d angAcc(0,0,0);
        if (v.length() > 0.0)
        {
            // get the last force applied to the cursor in global coordinates
            // we negate the result to obtain the opposite force that is applied on the
            // object
            cVector3d toolForce = -tool->getDeviceGlobalForce();

            // compute the effective force that contributes to rotating the object.
            cVector3d force = toolForce - cProject(toolForce, v);

            // compute the resulting torque
            cVector3d torque = cMul(v.length(), cCross( cNormalize(v), force));

            // update rotational acceleration
            angAcc = (1.0 / INERTIA) * torque;
        }

        // update rotational velocity
        angVel.add(timeInterval * angAcc);

        // set a threshold on the rotational velocity term
        double vel = angVel.length();
        if (vel > MAX_ANG_VEL)
        {
            angVel.mul(MAX_ANG_VEL / vel);
        }

        // add some damping too
        angVel.mul(1.0 - DAMPING * timeInterval);

        // if user switch is pressed, set velocity to zero
        if (tool->getUserSwitch(0) == 1)
        {
            angVel.zero();
        }

        // compute the next rotation configuration of the object
        if (angVel.length() > C_SMALL)
        {
            object->rotateAboutGlobalAxisRad(cNormalize(angVel), timeInterval * angVel.length());
        }*/
    }
    
    // exit haptics thread
    simulationFinished = true;
}

//------------------------------------------------------------------------------
