#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <QCAR/QCAR.h>
#include <QCAR/CameraDevice.h>
#include <QCAR/Renderer.h>
#include <QCAR/VideoBackgroundConfig.h>
#include <QCAR/Trackable.h>
#include <QCAR/TrackableResult.h>
#include <QCAR/Tool.h>
#include <QCAR/Tracker.h>
#include <QCAR/TrackerManager.h>
#include <QCAR/ImageTracker.h>
#include <QCAR/ImageTarget.h>
#include <QCAR/MultiTarget.h>
#include <QCAR/CameraCalibration.h>
#include <QCAR/UpdateCallback.h>
#include <QCAR/DataSet.h>

#include "Texture.h"
#include "SampleUtils.h"
#include "CubeShaders.h"
#include "Cube.h"

// Utility for logging:
#define LOG_TAG    "QCAR"
#define LOG(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#ifdef __cplusplus
extern "C"
{
#endif

//Matriz de projeção usada para renderizar os objetos
QCAR::Matrix44F projectionMatrix;
unsigned int larguraDaTela = 0;
unsigned int alturaDaTela = 0;
bool isActivityInPortraitMode   = false;

// Textures:
int textureCount                = 0;
Texture** textures              = 0;

// OpenGL ES 2.0 specific:
unsigned int shaderProgramID    = 0;
GLint vertexHandle              = 0;
GLint normalHandle              = 0;
GLint textureCoordHandle        = 0;
GLint mvpMatrixHandle           = 0;
GLint texSampler2DHandle        = 0;

// Screen dimensions:
unsigned int screenWidth        = 0;
unsigned int screenHeight       = 0;

// Constants:
static const float kCubeScaleX    = 120.0f * 0.75f / 2.0f;
static const float kCubeScaleY    = 120.0f * 1.00f / 2.0f;
static const float kCubeScaleZ    = 120.0f * 0.50f / 2.0f;


QCAR::MultiTarget* mit          = NULL;

QCAR::DataSet* dataSet          = 0;


void iniciaAplicacao(JNIEnv* env, jobject obj, jint largura, jint altura);
void configureVideoBackground();
void updateRenderer(jint largura, jint altura);
void initRendering();
void initMIT();
QCAR::ImageTarget* findImageTarget(const char* name);

JNIEXPORT int JNICALL
Java_com_example_camera_CameraActivity_initTracker(JNIEnv *, jobject)
{
    LOG("initTracker");
    
    // Initialize the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* tracker = trackerManager.initTracker(QCAR::Tracker::IMAGE_TRACKER);
    
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
                    trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));
    if (tracker == NULL)
    {
        LOG("Failed to initialize ImageTracker.");
        return 0;
    }

    LOG("Successfully initialized ImageTracker.");
    
    // Create the data set:
    dataSet = imageTracker->createDataSet();
    if (dataSet == 0)
    {
        LOG("Failed to create a new tracking data.");
        return 0;
    }

    // Load the data set:
    if (!dataSet->load("FlakesBox.xml", QCAR::DataSet::STORAGE_APPRESOURCE))
    {
        LOG("Failed to load data set.");
        return 0;
    }

    LOG("Successfully loaded the data set.");
    
    return 1;
}


JNIEXPORT void JNICALL
Java_com_example_camera_CameraActivity_startCamera(JNIEnv *,
                                                                         jobject)
{
    LOG("startCamera");
    
    // Select the camera to open, set this to QCAR::CameraDevice::CAMERA_FRONT 
    // to activate the front camera instead.
    QCAR::CameraDevice::CAMERA camera = QCAR::CameraDevice::CAMERA_DEFAULT;

    // Initialize the camera:
    if (!QCAR::CameraDevice::getInstance().init(camera))
        return;

    // Configure the video background
    configureVideoBackground();

    // Select the default mode:
    if (!QCAR::CameraDevice::getInstance().selectVideoMode(
                                QCAR::CameraDevice::MODE_DEFAULT))
        return;

    // Start the camera:
    if (!QCAR::CameraDevice::getInstance().start())
        return;

    // Uncomment to enable flash
    //if(QCAR::CameraDevice::getInstance().setFlashTorchMode(true))
    //    LOG("IMAGE TARGETS : enabled torch");

    // Uncomment to enable infinity focus mode, or any other supported focus mode
    // See CameraDevice.h for supported focus modes
    //if(QCAR::CameraDevice::getInstance().setFocusMode(QCAR::CameraDevice::FOCUS_MODE_INFINITY))
    //    LOG("IMAGE TARGETS : enabled infinity focus");

    // Start the tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* imageTracker = trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);
    if(imageTracker != 0)
        imageTracker->start();
}


void
configureVideoBackground()
{
    // Get the default video mode:
    QCAR::CameraDevice& cameraDevice = QCAR::CameraDevice::getInstance();
    QCAR::VideoMode videoMode = cameraDevice.
                                getVideoMode(QCAR::CameraDevice::MODE_DEFAULT);


    // Configure the video background
    QCAR::VideoBackgroundConfig config;
    config.mEnabled = true;
    config.mSynchronous = true;
    config.mPosition.data[0] = 0.0f;
    config.mPosition.data[1] = 0.0f;
    
   // LOG("Entrou aki......");
    //LOG("->>>>>>>>largura = %d, altura = %d",larguraDaTela,alturaDaTela);
    if (!isActivityInPortraitMode)
    {
        LOG("configureVideoBackground PORTRAIT");
        config.mSize.data[0] = videoMode.mHeight
                                * (alturaDaTela / (float)videoMode.mWidth);
        config.mSize.data[1] = alturaDaTela;

        if(config.mSize.data[0] < larguraDaTela)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = larguraDaTela;
            config.mSize.data[1] = larguraDaTela * 
                              (videoMode.mWidth / (float)videoMode.mHeight);
        }
    }
    else
    {
        LOG("configureVideoBackground LANDSCAPE");
        config.mSize.data[0] = larguraDaTela;
        config.mSize.data[1] = videoMode.mHeight * (larguraDaTela / (float)videoMode.mWidth);
        //
        if(config.mSize.data[1] < alturaDaTela)
        {
            LOG("Correcting rendering background size to handle missmatch between screen and video aspect ratios.");
            config.mSize.data[0] = alturaDaTela * (videoMode.mWidth / (float)videoMode.mHeight);
            config.mSize.data[1] = alturaDaTela;
        }
    }

    LOG("Configure Video Background : Video (%d,%d), Screen (%d,%d), mSize (%d,%d)", videoMode.mWidth, videoMode.mHeight, larguraDaTela, alturaDaTela, config.mSize.data[0], config.mSize.data[1]);

    // Set the config:
    QCAR::Renderer::getInstance().setVideoBackgroundConfig(config);
}

JNIEXPORT void JNICALL
Java_com_example_camera_CameraActivity_setProjectionMatrix(JNIEnv *, jobject){
	const QCAR::CameraCalibration& cameraCalibration =
	QCAR::CameraDevice::getInstance().getCameraCalibration();
	projectionMatrix = QCAR::Tool::getProjectionGL(cameraCalibration, 2.0f,
			2000.0f);
}


JNIEXPORT void JNICALL
Java_com_example_camera_CameraActivity_iniciaAplicacaoNative(
		JNIEnv* env, jobject obj, jint largura, jint altura)
{
    larguraDaTela = largura;
	alturaDaTela = altura;
	//larguraDaTela = 0;
    //alturaDaTela = 0;
//	LOG("iniciaAplicacaoNative->>>>>>>>largura = %d, altura = %d",larguraDaTela,alturaDaTela);
//	LOG("Java_com_aftersixapps_watcher_ARController_iniciaAplicacaoNative");
	//iniciaAplicacao(env, obj, largura, altura);
}

void iniciaAplicacao(JNIEnv* env, jobject obj, jint largura, jint altura) {
     
	larguraDaTela = largura;
	alturaDaTela = altura;
   // LOG("->>>>>>>>largura = %d, altura = %d",altura,largura);
	//QCAR::registerCallback(&qcarUpdate);
}


JNIEXPORT void JNICALL
Java_com_example_camera_CameraRenderer_renderFrame(JNIEnv *, jobject)
{
	//LOG("Java_com_qualcomm_QCARSamples_ImageTargets_GLRenderer_renderFrame");

    // Clear color and depth buffer 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get the state from QCAR and mark the beginning of a rendering section
    QCAR::State state = QCAR::Renderer::getInstance().begin();
    
    // Explicitly render the Video Background
    QCAR::Renderer::getInstance().drawVideoBackground();
    
    
   // Did we find any trackables this frame?
    if (state.getNumTrackableResults())
    {
        // Get the trackable:
        const QCAR::TrackableResult* result=NULL;
        int numResults=state.getNumTrackableResults();

        // Browse results searching for the MultiTarget
        for (int j=0;j<numResults;j++)
        {
            result = state.getTrackableResult(j);
            if (result->getType() == QCAR::TrackableResult::MULTI_TARGET_RESULT) break;
            result=NULL;
        }

        // If it was not found exit
        if (result==NULL)
        {
            // Clean up and leave
            glDisable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);

            QCAR::Renderer::getInstance().end();
            return;
        }
                
        QCAR::Matrix44F modelViewMatrix =
            QCAR::Tool::convertPose2GLMatrix(result->getPose());        
        QCAR::Matrix44F modelViewProjection;
        SampleUtils::scalePoseMatrix(kCubeScaleX, kCubeScaleY, kCubeScaleZ,
                                     &modelViewMatrix.data[0]);
        SampleUtils::multiplyMatrix(&projectionMatrix.data[0],
                                    &modelViewMatrix.data[0],
                                    &modelViewProjection.data[0]);

        glUseProgram(shaderProgramID);
         
        // Draw the cube:

        // We must detect if background reflection is active and adjust the culling direction.
        // If the reflection is active, this means the post matrix has been reflected as well,
        // therefore standard counter clockwise face culling will result in "inside out" models.
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        if(QCAR::Renderer::getInstance().getVideoBackgroundConfig().mReflection == QCAR::VIDEO_BACKGROUND_REFLECTION_ON)
            glFrontFace(GL_CW);  //Front camera
        else
            glFrontFace(GL_CCW);   //Back camera

        glVertexAttribPointer(vertexHandle, 3, GL_FLOAT, GL_FALSE, 0,
                              (const GLvoid*) &cubeVertices[0]);
        glVertexAttribPointer(normalHandle, 3, GL_FLOAT, GL_FALSE, 0,
                              (const GLvoid*) &cubeNormals[0]);
        glVertexAttribPointer(textureCoordHandle, 2, GL_FLOAT, GL_FALSE, 0,
                              (const GLvoid*) &cubeTexCoords[0]);
        
        glEnableVertexAttribArray(vertexHandle);
        glEnableVertexAttribArray(normalHandle);
        glEnableVertexAttribArray(textureCoordHandle);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]->mTextureID);
        glUniformMatrix4fv(mvpMatrixHandle, 1, GL_FALSE,
                           (GLfloat*)&modelViewProjection.data[0] );
        glUniform1i(texSampler2DHandle, 0 /*GL_TEXTURE0*/);
        glDrawElements(GL_TRIANGLES, NUM_CUBE_INDEX, GL_UNSIGNED_SHORT,
                       (const GLvoid*) &cubeIndices[0]);

        glDisable(GL_CULL_FACE);
    }
    
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    
    QCAR::Renderer::getInstance().end();
}


JNIEXPORT void JNICALL
Java_com_example_camera_CameraRenderer_updateRenderer(
		JNIEnv* env, jobject obj, jint largura, jint altura)
{
	LOG("Java_com_example_camera_CameraRenderer_updateRenderer");
	updateRenderer(largura, altura);
}


void updateRenderer(jint largura, jint altura) {
	larguraDaTela = largura;
	alturaDaTela = altura;

	configureVideoBackground();
}

JNIEXPORT void JNICALL
Java_com_example_camera_CameraRenderer_initRendering(JNIEnv *, jobject)
{
	LOG("Java_com_example_camera_CameraRenderer_initRendering");
	initRendering();
}

void initRendering() {
	// Define clear color
    glClearColor(0.0f, 0.0f, 0.0f, QCAR::requiresAlpha() ? 0.0f : 1.0f);
    
    // Now generate the OpenGL texture objects and add settings
    for (int i = 0; i < textureCount; ++i)
    {
        glGenTextures(1, &(textures[i]->mTextureID));
        glBindTexture(GL_TEXTURE_2D, textures[i]->mTextureID);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textures[i]->mWidth,
                textures[i]->mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                (GLvoid*)  textures[i]->mData);
    }
  
    shaderProgramID     = SampleUtils::createProgramFromBuffer(cubeMeshVertexShader,
                                                            cubeFragmentShader);

    vertexHandle        = glGetAttribLocation(shaderProgramID,
                                                "vertexPosition");
    normalHandle        = glGetAttribLocation(shaderProgramID,
                                                "vertexNormal");
    textureCoordHandle  = glGetAttribLocation(shaderProgramID,
                                                "vertexTexCoord");
    mvpMatrixHandle     = glGetUniformLocation(shaderProgramID,
                                                "modelViewProjectionMatrix");
    texSampler2DHandle  = glGetUniformLocation(shaderProgramID, 
                                                "texSampler2D");
}

JNIEXPORT void JNICALL
Java_com_example_camera_CameraActivity_finalizaTracker(JNIEnv *, jobject) {
	QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();

//desativaDataset();

	trackerManager.deinitTracker(QCAR::Tracker::IMAGE_TRACKER);
}

JNIEXPORT void JNICALL
Java_com_example_camera_CameraActivity_pararCamera(JNIEnv *, jobject) {
	QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
	QCAR::Tracker* imageTracker = trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);
	imageTracker->stop();

	QCAR::CameraDevice::getInstance().stop();
	QCAR::CameraDevice::getInstance().deinit();
}

JNIEXPORT void JNICALL
Java_com_example_camera_CameraActivity_onQCARInitializedNative(JNIEnv *, jobject)
{
    LOG("onQCARInitializedNative");

    // Validate the MultiTarget and setup programmatically if required:
    initMIT();

    // Get the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = static_cast<QCAR::ImageTracker*>(
        trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER));

    // Activate the data set:
    if (!imageTracker->activateDataSet(dataSet))
    {
        LOG("Failed to activate data set.");
        return;
    }

    LOG("Successfully activated the data set.");

   // QCAR::registerCallback(&myUpdateCallBack);
}


JNIEXPORT void JNICALL
Java_com_example_camera_CameraActivity_initApplicationNative(
                            JNIEnv* env, jobject obj, jint width, jint height)
{
    LOG("Java_com_example_camera_CameraActivity_initApplicationNative");
    
    // Store screen dimensions
    screenWidth = width;
    screenHeight = height;
        
    // Handle to the activity class:
    jclass activityClass = env->GetObjectClass(obj);

    jmethodID getTextureCountMethodID = env->GetMethodID(activityClass,
                                                    "getTextureCount", "()I");
    if (getTextureCountMethodID == 0)
    {
        LOG("Function getTextureCount() not found.");
        return;
    }

    textureCount = env->CallIntMethod(obj, getTextureCountMethodID);    
    if (!textureCount)
    {
        LOG("getTextureCount() returned zero.");
        return;
    }

    textures = new Texture*[textureCount];

    jmethodID getTextureMethodID = env->GetMethodID(activityClass,
        "getTexture", "(I)Lcom/example/camera/Texture;");

    if (getTextureMethodID == 0)
    {
        LOG("Function getTexture() not found.");
        return;
    }

    // Register the textures
    for (int i = 0; i < textureCount; ++i)
    {

        jobject textureObject = env->CallObjectMethod(obj, getTextureMethodID, i); 
        if (textureObject == NULL)
        {
            LOG("GetTexture() returned zero pointer");
            return;
        }

        textures[i] = Texture::create(env, textureObject);
    }
}

void
initMIT()
{
    //
    // This function checks the current tracking setup for completeness. If
    // it finds that something is missing, then it creates it and configures it:
    // Any MultiTarget and Part elements missing from the config.xml file
    // will be created.
    //

    LOG("Beginning to check the tracking setup");

    // Configuration data - identical to what is in the config.xml file
    //
    // If you want to recreate the trackable assets using the on-line TMS server 
    // using the original images provided in the sample's media folder, use the
    // following trackable sizes on creation to get identical visual results:
    // create a cuboid with width = 90 ; height = 120 ; length = 60.
    
    const char* names[6]   = { "FlakesBox.Front", "FlakesBox.Back", "FlakesBox.Left", "FlakesBox.Right", "FlakesBox.Top", "FlakesBox.Bottom" };
    const float trans[3*6] = { 0.0f,  0.0f,  30.0f, 
                               0.0f,  0.0f, -30.0f,
                              -45.0f, 0.0f,  0.0f, 
                               45.0f, 0.0f,  0.0f,
                               0.0f,  60.0f, 0.0f,
                               0.0f, -60.0f, 0.0f };
    const float rots[4*6]  = { 1.0f, 0.0f, 0.0f,   0.0f,
                               0.0f, 1.0f, 0.0f, 180.0f,
                               0.0f, 1.0f, 0.0f, -90.0f,
                               0.0f, 1.0f, 0.0f,  90.0f,
                               1.0f, 0.0f, 0.0f, -90.0f,
                               1.0f, 0.0f, 0.0f,  90.0f };

    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = (QCAR::ImageTracker*)
        trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);

    if (imageTracker == 0 || dataSet == 0)
    {
        return;
    }

    // Go through all Trackables to find the MultiTarget instance
    //
    for(int i=0; i<dataSet->getNumTrackables(); i++)
    {
        if(dataSet->getTrackable(i)->getType()==QCAR::Trackable::MULTI_TARGET)
        {
            LOG("MultiTarget exists -> no need to create one");
            mit = reinterpret_cast<QCAR::MultiTarget*>(dataSet->getTrackable(i));
            break;
        }
    }

    // If no MultiTarget was found, then let's create one.
    if(mit==NULL)
    {
        LOG("No MultiTarget found -> creating one");
        mit = dataSet->createMultiTarget("FlakesBox");

        if(mit==NULL)
        {
            LOG("ERROR: Failed to create the MultiTarget - probably the Tracker is running");
            return;
        }
    }

    // Try to find each ImageTarget. If we find it, this actually means that it
    // is not part of the MultiTarget yet: ImageTargets that are part of a
    // MultiTarget don't show up in the list of Trackables.
    // Each ImageTarget that we found, is then made a part of the
    // MultiTarget and a correct pose (reflecting the pose of the
    // config.xml file) is set).
    // 
    int numAdded = 0;
    for(int i=0; i<6; i++)
    {
        
        if(QCAR::ImageTarget* it = findImageTarget(names[i]))
        {
            LOG("ImageTarget '%s' found -> adding it as to the MultiTarget",
                names[i]);

            int idx = mit->addPart(it);
            QCAR::Vec3F t(trans+i*3),a(rots+i*4);
            QCAR::Matrix34F mat;

            QCAR::Tool::setTranslation(mat, t);
            QCAR::Tool::setRotation(mat, a, rots[i*4+3]);
            mit->setPartOffset(idx, mat);
            numAdded++;
        }
    }

    LOG("Added %d ImageTarget(s) to the MultiTarget", numAdded);

    if(mit->getNumParts()!=6)
    {
        LOG("ERROR: The MultiTarget should have 6 parts, but it reports %d parts",
            mit->getNumParts());
    }

    LOG("Finished checking the tracking setup");
}

QCAR::ImageTarget* findImageTarget(const char* name)
{
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::ImageTracker* imageTracker = (QCAR::ImageTracker*)
                        trackerManager.getTracker(QCAR::Tracker::IMAGE_TRACKER);

    if (imageTracker != 0)
    {
        for(int i=0; i<dataSet->getNumTrackables(); i++)
        {
                LOG("ENTROU NO IF 1");
            if(dataSet->getTrackable(i)->getType()==QCAR::Trackable::IMAGE_TARGET)
            {
                if(!strcmp(dataSet->getTrackable(i)->getName(),name)){
                    LOG("ENTROU NO IF 2");
                    return reinterpret_cast<QCAR::ImageTarget*>(dataSet->getTrackable(i));
                }
            }
        }
    }
    return NULL;
}

#ifdef __cplusplus
}
#endif
