apply plugin: 'com.android.library'
apply plugin: 'maven-publish'

android {
    namespace = "com.termux.shared"

    compileSdkVersion project.properties.compileSdkVersion.toInteger()
    ndkVersion = System.getenv("JITPACK_NDK_VERSION") ?: project.properties.ndkVersion

    dependencies {
        implementation "androidx.appcompat:appcompat:1.6.1"
        implementation "androidx.annotation:annotation:1.9.0"
        implementation "androidx.core:core:1.13.1"
        implementation "com.google.android.material:material:1.12.0"
        implementation "com.google.guava:guava:24.1-jre"
        implementation "io.noties.markwon:core:$markwonVersion"
        implementation "io.noties.markwon:ext-strikethrough:$markwonVersion"
        implementation "io.noties.markwon:linkify:$markwonVersion"
        implementation "io.noties.markwon:recycler:$markwonVersion"
        implementation "org.lsposed.hiddenapibypass:hiddenapibypass:6.1"

        implementation "androidx.window:window:1.1.0"

        // Do not increment version higher than 2.5 or there
        // will be runtime exceptions on android < 8
        // due to missing classes like java.nio.file.Path.
        implementation "commons-io:commons-io:2.5"

        implementation project(":terminal-view")

        implementation "com.termux:termux-am-library:v2.0.0"
    }

    defaultConfig {
        compileSdkVersion project.properties.compileSdkVersion.toInteger()
        minSdkVersion project.properties.minSdkVersion.toInteger()
        targetSdkVersion project.properties.targetSdkVersion.toInteger()
        testInstrumentationRunner "androidx.test.runner.AndroidJUnitRunner"
        externalNativeBuild {
            ndkBuild {
                cppFlags ''
            }
        }
    }

    buildTypes {
        release {
            minifyEnabled false
            proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.pro'
        }
    }

    compileOptions {
        // Flag to enable support for the new language APIs
        coreLibraryDesugaringEnabled true

        sourceCompatibility JavaVersion.VERSION_1_8
        targetCompatibility JavaVersion.VERSION_1_8
    }

    externalNativeBuild {
        ndkBuild {
            path file('src/main/cpp/Android.mk')
        }
    }

    publishing {
        multipleVariants {
            withSourcesJar()
            withJavadocJar()
            allVariants()
        }
    }
}

dependencies {
    testImplementation "junit:junit:4.13.2"
    androidTestImplementation "androidx.test.ext:junit:1.1.5"
    coreLibraryDesugaring "com.android.tools:desugar_jdk_libs:1.1.5"
}

task sourceJar(type: Jar) {
    from android.sourceSets.main.java.srcDirs
    archiveClassifier = "sources"
}

afterEvaluate {
    publishing {
        publications {
            // Creates a Maven publication called "release".
            release(MavenPublication) {
                from components.default
                groupId = 'com.termux'
                artifactId = 'termux-shared'
                version = '0.118.0'
                artifact(sourceJar)
            }
        }
    }
}
