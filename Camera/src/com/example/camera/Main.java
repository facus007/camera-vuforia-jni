package com.example.camera;



import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class Main extends Activity implements OnClickListener {

	private Button startButton;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		startButton = (Button) findViewById(R.id.button_start);
		startButton.setOnClickListener(this);
	}

	@Override
	public void onClick(View v) {
		
		if (R.id.button_start == v.getId()){
			startCameraActivity();
		}
	}

	private void startCameraActivity()
    {
        Intent i = new Intent(this, CameraActivity.class);
        startActivity(i);
    }
	
	
	
	
}
