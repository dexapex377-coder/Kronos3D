plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.kronos3d"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.kronos3d"
        minSdk = 29
        targetSdk = 34
        versionCode = 1
        versionName = "1.0.0"
        ndkVersion = "27.0.12077973"

        ndk {
            val abiFilter = project.properties["abi"]?.toString()
            if (abiFilter != null) {
                abiFilters.add(abiFilter)
            } else {
                abiFilters.addAll(listOf("arm64-v8a", "armeabi-v7a"))
            }
        }

        externalNativeBuild {
            cmake {
                cppFlags.addAll(listOf(
                    "-std=c++20",
                    "-fexceptions",
                    "-frtti",
                    "-O3",
                    "-ffast-math",
                    "-flto=thin",
                    "-DANDROID",
                    "-DKRONOS_GLES32",
                    "-DKRONOS_VULKAN",
                    "-DBLENDER_EMBEDDED",
                    "-DNDEBUG"
                ))
                arguments.addAll(listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DANDROID_TOOLCHAIN=clang",
                    "-DCMAKE_CXX_STANDARD=20",
                    "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
                ))
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            isShrinkResources = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            signingConfig = signingConfigs.getByName("debug")
        }
        debug {
            isDebuggable = true
            isMinifyEnabled = false
        }
    }

    signingConfigs {
        create("debug") {
            storeFile = file("debug.keystore")
            storePassword = "android"
            keyAlias = "androiddebugkey"
            keyPassword = "android"
        }
    }

    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
            version("3.22.1")
        }
    }

    packagingOptions {
        jniLibs {
            useLegacyPackaging = true
        }
    }

    buildFeatures {
        viewBinding = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("com.google.android.material:material:1.12.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    implementation("androidx.activity:activity-ktx:1.9.0")
}