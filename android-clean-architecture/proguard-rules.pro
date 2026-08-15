# Hilt
-keep class dagger.hilt.android.** { *; }
-keep class hilt_aggregated_deps.** { *; }
-keep,allowobfuscation @interface dagger.hilt.android.lifecycle.HiltViewModel

# Kotlin
-keep class kotlin.** { *; }
-keep class kotlinx.coroutines.** { *; }

# AndroidX Lifecycle
-keep class androidx.lifecycle.** { *; }

# Keep data classes
-keep class domain.entity.** { *; }
-keep class presentation.ui.state.** { *; }

# Keep ViewBinding generated classes
-keep class **.databinding.** { *; }
