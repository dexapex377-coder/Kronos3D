package com.kronos3d

import android.app.Activity
import android.content.pm.ActivityInfo
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import android.view.MotionEvent
import android.view.WindowManager
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.kronos3d.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private var nativeInitialized = false
    private var lastTouchX = 0f
    private var lastTouchY = 0f
    private var firstTouch = true

    companion object {
        init {
            System.loadLibrary("kronos_jni")
        }
    }

    external fun nativeInit(surface: android.view.Surface, width: Int, height: Int): Boolean
    external fun nativeResize(width: Int, height: Int)
    external fun nativeRender()
    external fun nativeTouchEvent(action: Int, x: Float, y: Float, pressure: Float)
    external fun nativeCleanup()
    external fun nativeGetVersion(): String
    external fun nativeGetRendererInfo(): String

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        // Get version info
        val version = nativeGetVersion()
        val renderer = nativeGetRendererInfo()
        binding.tvVersion.text = "$version\nRenderer: $renderer"

        binding.glSurfaceView.apply {
            setEGLContextClientVersion(3) // OpenGL ES 3.x
            setPreserveEGLContextOnPause(true)
            setRenderer(object : GLSurfaceView.Renderer {
                override fun onSurfaceCreated(gl: android.opengl.GLSurfaceView?, config: android.graphics.EGLConfig) {
                    val surface = holder.surface
                    val width = binding.glSurfaceView.width
                    val height = binding.glSurfaceView.height
                    if (width > 0 && height > 0) {
                        nativeInitialized = nativeInit(surface, width, height)
                        runOnUiThread {
                            if (nativeInitialized) {
                                Toast.makeText(this@MainActivity, "Engine initialized: $renderer", Toast.LENGTH_SHORT).show()
                            } else {
                                Toast.makeText(this@MainActivity, "Engine initialization failed!", Toast.LENGTH_LONG).show()
                            }
                        }
                    }
                }

                override fun onSurfaceChanged(gl: android.opengl.GLSurfaceView?, width: Int, height: Int) {
                    nativeResize(width, height)
                }

                override fun onDrawFrame(gl: android.opengl.GLSurfaceView?) {
                    if (nativeInitialized) {
                        nativeRender()
                    }
                }
            })
            renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
        }

        binding.glSurfaceView.setOnTouchListener { v, event ->
            handleTouchEvent(event)
            true
        }
    }

    private fun handleTouchEvent(event: MotionEvent): Boolean {
        val action = event.actionMasked
        val pointerIndex = event.actionIndex
        val pointerId = event.getPointerId(pointerIndex)
        val x = event.getX(pointerIndex)
        val y = event.getY(pointerIndex)
        val pressure = event.getPressure(pointerIndex)

        when (action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                lastTouchX = x
                lastTouchY = y
                firstTouch = true
                nativeTouchEvent(action, x, y, pressure)
            }
            MotionEvent.ACTION_MOVE -> {
                for (i in 0 until event.pointerCount) {
                    val px = event.getX(i)
                    val py = event.getY(i)
                    val p = event.getPressure(i)
                    nativeTouchEvent(action, px, py, p)
                }
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_CANCEL -> {
                nativeTouchEvent(action, x, y, pressure)
            }
        }
        return true
    }

    override fun onPause() {
        super.onPause()
        binding.glSurfaceView.onPause()
    }

    override fun onResume() {
        super.onResume()
        binding.glSurfaceView.onResume()
    }

    override fun onDestroy() {
        super.onDestroy()
        if (nativeInitialized) {
            nativeCleanup()
            nativeInitialized = false
        }
    }
}