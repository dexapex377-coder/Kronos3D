package com.kronos3d

import android.os.Bundle
import android.util.Log
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.WindowManager
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.kronos3d.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {

    private lateinit var binding: ActivityMainBinding
    private var nativeInitialized = false
    private var rendering = false
    private var renderThread: Thread? = null

    companion object {
        const val TAG = "Kronos3D"
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

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        binding.tvVersion.text = "${nativeGetVersion()}\nRenderer: ${nativeGetRendererInfo()}"

        binding.surfaceView.holder.addCallback(this)
        binding.surfaceView.setOnTouchListener { _, event ->
            handleTouchEvent(event)
            true
        }
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        Log.i(TAG, "Surface created: ${holder.surface}")
        val surface = holder.surface
        val width = binding.surfaceView.width
        val height = binding.surfaceView.height
        if (width > 0 && height > 0) {
            nativeInitialized = nativeInit(surface, width, height)
            if (nativeInitialized) {
                rendering = true
                renderThread = Thread {
                    while (rendering && nativeInitialized) {
                        nativeRender()
                    }
                }
                renderThread?.start()
            }
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.i(TAG, "Surface changed: ${width}x${height}")
        nativeResize(width, height)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        Log.i(TAG, "Surface destroyed")
        rendering = false
        renderThread?.join(1000)
        renderThread = null
        if (nativeInitialized) {
            nativeCleanup()
            nativeInitialized = false
        }
    }

    private var lastTouchX = 0f
    private var lastTouchY = 0f

    private fun handleTouchEvent(event: MotionEvent): Boolean {
        val action = event.actionMasked
        val pointerIndex = event.actionIndex
        val x = event.getX(pointerIndex)
        val y = event.getY(pointerIndex)
        val pressure = event.getPressure(pointerIndex)

        when (action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                lastTouchX = x; lastTouchY = y
            }
            MotionEvent.ACTION_MOVE -> {
                for (i in 0 until event.pointerCount) {
                    nativeTouchEvent(action, event.getX(i), event.getY(i), event.getPressure(i))
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
        rendering = false
    }

    override fun onResume() {
        super.onResume()
        val holder = binding.surfaceView.holder
        if (holder.surface?.isValid == true && !nativeInitialized) {
            rendering = true
            renderThread = Thread {
                while (rendering) nativeRender()
            }
            renderThread?.start()
        }
    }

    override fun onDestroy() {
        rendering = false
        renderThread?.join(1000)
        if (nativeInitialized) {
            nativeCleanup()
            nativeInitialized = false
        }
        super.onDestroy()
    }
}