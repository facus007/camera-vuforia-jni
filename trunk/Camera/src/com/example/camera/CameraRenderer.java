package com.example.camera;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.qualcomm.QCAR.QCAR;

import android.opengl.GLSurfaceView.Renderer;



public class CameraRenderer implements Renderer {

	
	public CameraRenderer(){
		
	}
	
	
	@Override
	public void onDrawFrame(GL10 gl) {
		renderFrame();
		
	}

	@Override
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		System.out.println("Entro por aki....");
		updateRenderer(width, height);
		QCAR.onSurfaceChanged(width, height);
		
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		initRendering();
		QCAR.onSurfaceCreated();
		
	}

		
	public native void renderFrame();
	public native void updateRenderer(int width, int height);
	public native void initRendering();
}
