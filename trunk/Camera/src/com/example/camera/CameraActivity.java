package com.example.camera;

import java.util.Vector;
import com.qualcomm.QCAR.QCAR;
import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.WindowManager;
import android.view.ViewGroup.LayoutParams;

public class CameraActivity extends Activity  {

	
	private GLSurfaceView glView;	
	private CameraRenderer renderer;
	private Vector<Texture> mTextures;
	
    // Display size of the device:
    private int mScreenWidth = 0;
    private int mScreenHeight = 0;
	
	/** Static initializer block to load native libraries on start-up. */
    static
    {
    	loadLibrary("QCAR");
        loadLibrary("Camera");
    }
    
  
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		// Load any sample specific textures:
        mTextures = new Vector<Texture>();
        loadTextures();        
        
		iniciaQCAR();
		
		initAplication();
		
		iniciarAplicacaoAR();
		
		initTracker();
		
		addGLView();
		
		onPostExecute();
	}
	
	private void initAplication() {
		int screenOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
		setRequestedOrientation(screenOrientation);
		
		storeScreenDimensions();
		
		// As long as this window is visible to the user, keep the device's
        // screen turned on and bright:
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
	}

	private void storeScreenDimensions() {
		
		 DisplayMetrics metrics = new DisplayMetrics();
	        getWindowManager().getDefaultDisplay().getMetrics(metrics);
	        mScreenWidth = metrics.widthPixels;
	        mScreenHeight = metrics.heightPixels;
		
	}

	private void iniciarAplicacaoAR() {
		
		initApplicationNative(mScreenWidth, mScreenHeight);
		            
		glView = new GLSurfaceView(this);
		
		glView.setEGLContextClientVersion(2);
		renderer = new CameraRenderer();
		glView.setRenderer(renderer);

				
	}
	
	private void iniciaQCAR() {
		
		QCAR.setInitParameters(this, QCAR.GL_20);
		QCAR.init();

	}
	private void addGLView() {
		
		addContentView(glView, new LayoutParams(LayoutParams.WRAP_CONTENT,
                LayoutParams.WRAP_CONTENT));
		
	}
	
	protected void onPostExecute()
    {
        
		// Done loading the tracker, update application status:
            onQCARInitializedNative();
 
    }

	public void onResume() {
		super.onResume();
		
		QCAR.onResume();
		startCamera();
		setProjectionMatrix();
		resumeGLView();
		
	}
	
	public void onPause() {
		super.onPause();
		pauseGLView();
		pararCamera();
		QCAR.onPause();
	}

	public void onDestroy() {
		super.onDestroy();
		finalizaTracker();
		QCAR.deinit();
	}
	
	private void pauseGLView() {
		glView.setVisibility(View.INVISIBLE);
		glView.onPause();
	}
	
	private void resumeGLView() {
		glView.setVisibility(View.VISIBLE);
		glView.onResume();
	}

    private void loadTextures()
    {
        mTextures.add(Texture.loadTextureFromApk("TextureWireframe.png",
                getAssets()));

    }

    
    public int getTextureCount()
    {
        return mTextures.size();
    }


    
    public Texture getTexture(int i)
    {
        return mTextures.elementAt(i);
    }
    
	public native int  initTracker(); 
	public native int  startCamera(); 
	private native void setProjectionMatrix();
	private native void iniciaAplicacaoNative(int largura, int altura);
	private native void pararCamera();
	private native void finalizaTracker();
	private native void initApplicationNative(int width, int height);
	public native void onQCARInitializedNative();
	
	public static boolean loadLibrary(String nLibName)
    {
        try
        {
            System.loadLibrary(nLibName);
            
            return true;
        }
        catch (UnsatisfiedLinkError ulee)
        {
            
        	ulee.printStackTrace();
        }
        catch (SecurityException se)
        {
           
        	se.printStackTrace();
        }

        return false;
    }
	
	
	
	
}
