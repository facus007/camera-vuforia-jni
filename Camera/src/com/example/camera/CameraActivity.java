package com.example.camera;

import com.qualcomm.QCAR.QCAR;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.ViewGroup.LayoutParams;

public class CameraActivity extends Activity  {

	
	private GLSurfaceView glView;	
	private CameraRenderer renderer;
	
	/** Static initializer block to load native libraries on start-up. */
    static
    {
    	loadLibrary("QCAR");
        loadLibrary("Camera");
    }
    
  
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		//setContentView(R.layout.main);
		System.out.println("Teste Activity");
		iniciaQCAR();
		initTracker();
		iniciarAplicacaoAR();
		System.out.println("Tracker Iniciado!!!");
		System.out.println("Ligar Camera!!!");
		addGLView();
	}
			
	/*public void onCreate() {		
		iniciaQCAR();
		iniciaTracker();		
		iniciaAplicacaoAR();		
		carregaDadosDoTracker();
		addGLView();
	}

	public void onResume() {
		resumeGLView();
		iniciaCamera();
		setProjectionMatrix();
		QCAR.onResume();
	}*/
	/*public void onResume() {
		System.out.println("OnResume!!!");
	}*/
	
	private void iniciarAplicacaoAR() {
		DisplayMetrics metrics = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(metrics);
		iniciaAplicacaoNative(metrics.widthPixels, metrics.heightPixels);
		glView = new GLSurfaceView(this);
		System.out.println("glview ok!!!");
		renderer = new CameraRenderer();
		glView.setRenderer(renderer);

				
	}

	private void addGLView() {
		System.out.println("entrou no addGLView!!!");
		addContentView(glView, new LayoutParams(LayoutParams.WRAP_CONTENT,
                LayoutParams.WRAP_CONTENT));
		System.out.println("Add GLView!!!");
	}

	public void onResume() {
		super.onResume();
		System.out.println("OnResume!!!");
		resumeGLView();
		startCamera();
		setProjectionMatrix();
		QCAR.onResume();
		System.out.println("Saindo do onResume");
	}
	
	public void onPause() {
		super.onPause();
		pouseGLView();
		pararCamera();
		QCAR.onPause();
	}

	public void onDestroy() {
		super.onDestroy();
		finalizaTracker();
		QCAR.deinit();
	}
	
	private void pouseGLView() {
		glView.setVisibility(View.INVISIBLE);
		glView.onPause();
	}
	
	private void resumeGLView() {
		glView.setVisibility(View.VISIBLE);
		glView.onResume();
	}

	private void iniciaQCAR() {
		System.out.println("Iniciando QCAR");
		QCAR.setInitParameters(this, QCAR.GL_11);
		QCAR.init();
		System.out.println("QCAR iniciado com sucesso!!!");
	}

	public native int  initTracker(); 
	public native int  startCamera(); 
	private native void setProjectionMatrix();
	private native void iniciaAplicacaoNative(int largura, int altura);
	private native void pararCamera();
	private native void finalizaTracker();
	public static boolean loadLibrary(String nLibName)
    {
        try
        {
            System.loadLibrary(nLibName);
            //DebugLog.LOGI("Native library lib" + nLibName + ".so loaded");
            return true;
        }
        catch (UnsatisfiedLinkError ulee)
        {
            //DebugLog.LOGE("The library lib" + nLibName +                             ".so could not be loaded");
        	ulee.printStackTrace();
        }
        catch (SecurityException se)
        {
           // DebugLog.LOGE("The library lib" + nLibName +       ".so was not allowed to be loaded");
        	se.printStackTrace();
        }

        return false;
    }
	
	
	
	
}
