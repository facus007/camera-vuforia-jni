#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef USE_OPENGL_ES_1_1
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

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
#include <QCAR/CameraCalibration.h>
#include <QCAR/UpdateCallback.h>
#include <QCAR/DataSet.h>

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

// Screen dimensions:
unsigned int screenWidth        = 0;
unsigned int screenHeight       = 0;


void iniciaAplicacao(JNIEnv* env, jobject obj, jint largura, jint altura);
void configureVideoBackground();
void updateRenderer(jint largura, jint altura);
void initRendering();

JNIEXPORT int JNICALL
Java_com_example_camera_CameraActivity_initTracker(JNIEnv *, jobject)
{
    LOG("Java_com_qualcomm_QCARSamples_ImageTargets_ImageTargets_initTracker");
    
    // Initialize the image tracker:
    QCAR::TrackerManager& trackerManager = QCAR::TrackerManager::getInstance();
    QCAR::Tracker* tracker = trackerManager.initTracker(QCAR::Tracker::IMAGE_TRACKER);
    if (tracker == NULL)
    {
        LOG("Failed to initialize ImageTracker.");
        return 0;
    }

    LOG("Successfully initialized ImageTracker.");
    return 1;
}


JNIEXPORT void JNICALL
Java_com_example_camera_CameraActivity_startCamera(JNIEnv *,
                                                                         jobject)
{
    LOG("Java_com_qualcomm_QCARSamples_ImageTargets_ImageTargets_startCamera");
    
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
    
    LOG("Entrou aki......");
    LOG("->>>>>>>>largura = %d, altura = %d",larguraDaTela,alturaDaTela);
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
	larguraDaTela = 0;
     alturaDaTela = 0;
	LOG("iniciaAplicacaoNative->>>>>>>>largura = %d, altura = %d",larguraDaTela,alturaDaTela);
	LOG("Java_com_aftersixapps_watcher_ARController_iniciaAplicacaoNative");
	//iniciaAplicacao(env, obj, largura, altura);
}

void iniciaAplicacao(JNIEnv* env, jobject obj, jint largura, jint altura) {
     
	larguraDaTela = largura;
	alturaDaTela = altura;
    LOG("->>>>>>>>largura = %d, altura = %d",altura,largura);
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
    
    
   // We must detect if background reflection is active and adjust the culling direction. 
    // If the reflection is active, this means the post matrix has been reflected as well,
    // therefore standard counter clockwise face culling will result in "inside out" models. 
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    if(QCAR::Renderer::getInstance().getVideoBackgroundConfig().mReflection == QCAR::VIDEO_BACKGROUND_REFLECTION_ON)
        glFrontFace(GL_CW);  //Front camera
    else
        glFrontFace(GL_CCW);   //Back camera
    
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
}



#ifdef __cplusplus
}
#endif
