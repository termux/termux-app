# Clean Architecture Implementation Guide

## Overview
This project implements a complete Clean Architecture pattern for Android using MVVM, ViewBinding, and Hilt dependency injection. The architecture eliminates the 150+ line "God Activity" anti-pattern and replaces it with a testable, maintainable, and scalable structure.

## Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      PRESENTATION LAYER                          │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  MainActivity (ViewBinding)                              │  │
│  │  - Renders UI based on StateFlow<UiState>               │  │
│  │  - Handles user interactions (clicks)                   │  │
│  │  - Uses repeatOnLifecycle for lifecycle-aware observer  │  │
│  └────────────────────┬─────────────────────────────────────┘  │
│                       │                                         │
│                       │ clicks                                  │
│                       ▼                                         │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  MainViewModel (injected with Hilt)                      │  │
│  │  - Manages UiState with StateFlow                        │  │
│  │  - Methods: enviarConsulta(), clearError()               │  │
│  │  - Injects UseCases via constructor                      │  │
│  └────────────────────┬─────────────────────────────────────┘  │
│                       │                                         │
│                       │ calls use cases                         │
└───────────────────────┼──────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────┐
│                       DOMAIN LAYER                               │
│           (Pure Kotlin - NO Android Dependencies)               │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  UseCases: EnviarConsultaUseCase, GetContadorUseCase,    │  │
│  │            IncrementarContadorUseCase                    │  │
│  │  - operator fun invoke() implementation                  │  │
│  │  - Delegates to repository                              │  │
│  └────────────────────┬─────────────────────────────────────┘  │
│                       │                                         │
│                       │ delegates to                            │
│                       ▼                                         │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Entities: Consulta, Respuesta                           │  │
│  │  - Validation in init blocks                            │  │
│  │  - Immutable data classes                               │  │
│  └──────────────────────────────────────────────────────────┘  │
│                       ▲                                         │
│                       │ implements                              │
│                       │                                         │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Repository Interface: IConsultaRepository               │  │
│  │  - suspend fun enviarConsulta(consulta): Result<>        │  │
│  │  - suspend fun obtenerContador(): Result<Int>            │  │
│  │  - suspend fun incrementarContador(): Result<Unit>       │  │
│  │  - fun getContadorSync(): Int                            │  │
│  │  - fun incrementarContadorSync(): Int                    │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────┬────────────────────────────────────────┘
                          │
                          │ implements
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                        DATA LAYER                                │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  ConsultaRepositoryImpl (implements IConsultaRepository)  │  │
│  │  - Combines RemoteDataSource & LocalDataSource           │  │
│  │  - Orchestrates data flow                                │  │
│  └────────────┬────────────────────────┬──────────────────────┘  │
│               │                        │                       │
│               │ calls                  │ calls                 │
│               ▼                        ▼                       │
│  ┌──────────────────────────┐  ┌──────────────────────────┐   │
│  │  RemoteDataSource        │  │  LocalDataSource         │   │
│  │  - fetch(url, consulta)  │  │  - getContador()         │   │
│  │  - HttpURLConnection     │  │  - incrementarContador() │   │
│  │  - 5s timeout            │  │  - SharedPreferences     │   │
│  │  - JSON parsing          │  │                          │   │
│  └────────────┬─────────────┘  └──────────────┬───────────┘   │
│               │                               │               │
│               │ HTTP POST                     │ read/write    │
│               ▼                               ▼               │
│  ┌──────────────────────────┐  ┌──────────────────────────┐   │
│  │  REST API                │  │  Device SharedPreferences│   │
│  │  https://api.example.com │  │  (contador storage)      │   │
│  │  /consultas              │  │                          │   │
│  └──────────────────────────┘  └──────────────────────────┘   │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘

Data Flow: User → MainActivity → MainViewModel → UseCases → 
          Repository → DataSources → External Services/Storage → 
          Response → Repository → ViewModel → StateFlow → MainActivity → UI
```

## Architecture Layers

### 1. Presentation Layer (`presentation/`)

**Responsibility**: UI rendering and user interaction handling

**Files**:
- `MainActivity.kt`: Activity with ViewBinding, observes StateFlow with repeatOnLifecycle
- `MainViewModel.kt`: ViewModel with StateFlow<UiState>, injects UseCases via Hilt
- `UiState.kt`: Data class containing isLoading, respuesta, contador, error
- `activity_main.xml`: View layout definition

**Key Principles**:
- Zero business logic
- Pure reactive data binding
- Lifecycle-aware coroutine collection
- ViewBinding for type safety

### 2. Domain Layer (`domain/`)

**Responsibility**: Business logic and use cases - PURE KOTLIN

**Files**:
- `entity/Consulta.kt`: Immutable data class with validation
- `entity/Respuesta.kt`: Immutable data class with validation
- `repository/IConsultaRepository.kt`: Contract interface (no implementation)
- `usecase/EnviarConsultaUseCase.kt`: operator fun invoke() pattern
- `usecase/GetContadorUseCase.kt`: Retrieves counter
- `usecase/IncrementarContadorUseCase.kt`: Increments counter

**Key Principles**:
- ZERO Android imports
- ZERO javax.inject or dagger imports
- Only kotlin.* and kotlinx.coroutines.*
- Pure Kotlin data classes with init block validation
- Repository as interface (dependency inversion)
- Testable without mocking Android

### 3. Data Layer (`data/`)

**Responsibility**: Data access and external service integration

**Files**:
- `datasource/RemoteDataSource.kt`: HTTP calls with HttpURLConnection
- `datasource/LocalDataSource.kt`: SharedPreferences wrapper
- `repository/ConsultaRepositoryImpl.kt`: Implements IConsultaRepository

**Key Principles**:
- HttpURLConnection with 5-second timeout
- JSON parsing with runCatching for error handling
- SharedPreferences for local counter persistence
- No direct UI dependency

## Dependency Injection with Hilt

### Module Structure

**DomainModule** (`di/DomainModule.kt`):
```kotlin
@Provides fun provideEnviarConsultaUseCase(repo): EnviarConsultaUseCase
@Provides fun provideGetContadorUseCase(repo): GetContadorUseCase
@Provides fun provideIncrementarContadorUseCase(repo): IncrementarContadorUseCase
```

**DataModule** (`di/DataModule.kt`):
```kotlin
@Provides fun provideSharedPreferences(context): SharedPreferences
@Provides fun provideRemoteDataSource(): RemoteDataSource
@Provides fun provideLocalDataSource(sp): LocalDataSource
@Provides fun provideConsultaRepository(remote, local): IConsultaRepository
```

### Injection Flow

1. Application starts with `@HiltAndroidApp` on LegacyApplication
2. MainActivity annotated with `@AndroidEntryPoint`
3. MainViewModel injected with `@HiltViewModel` + constructor injection
4. Hilt resolves dependency graph automatically

## Domain Layer Restrictions

### ALLOWED IMPORTS
```kotlin
import kotlin.*
import kotlinx.coroutines.*
```

### FORBIDDEN IMPORTS
```kotlin
❌ import android.*              // No Android framework
❌ import androidx.*             // No Android X libraries
❌ import dagger.*               // No DI annotations
❌ import javax.inject.*         // No injection annotations
❌ import android.content.*      // No Context
❌ import android.os.*           // No Handler, Bundle, etc
❌ import android.util.*         // No Log
❌ import org.json.*             // No JSON parsing
❌ import java.net.*             // No URL, Socket (use at data layer only)
❌ import java.io.*              // No Stream classes
```

These restrictions ensure:
- True separation of concerns
- Testability without Android emulator
- Reusability across different platforms
- No circular dependencies

## Testing Strategy

### Unit Tests (JVM - No Emulator Required)

1. **UseCase Tests** (`src/test/kotlin/domain/usecase/`):
   - Test operator fun invoke() with mocked repositories
   - Happy path: valid input → success
   - Error path: exception → failure
   - Edge cases: empty, max length, null values

2. **ViewModel Tests** (`src/test/kotlin/presentation/viewmodel/`):
   - Test state transitions: idle → loading → success/error
   - Test user actions: enviarConsulta(), clearError()
   - Test StateFlow emission
   - Uses mockk for UseCase mocking

3. **Repository Tests** (coming):
   - Mock RemoteDataSource and LocalDataSource
   - Test result combination

### Test Coverage Target: >90%

### Test Execution

```bash
./gradlew test                    # Run all unit tests
./gradlew test --info             # Verbose output
./gradlew test:MainViewModelTest  # Run specific test
```

## Memory Leak Prevention

### Problems in Legacy Code
- AsyncTask holds Activity reference
- ProgressDialog leaks on orientation change
- Direct Handler usage without cleanup
- Fragment transactions without proper lifecycle

### Solutions in Clean Architecture
✓ **ViewModel**: Survives configuration changes, lives outside Activity
✓ **StateFlow with repeatOnLifecycle**: Automatically cancels in STOPPED state
✓ **Coroutines**: Job tracking and automatic cancellation
✓ **No AsyncTask**: Use suspend functions instead
✓ **No ProgressDialog**: Use reactive ProgressBar with state binding

## Scalability Features

This architecture supports:
- **Offline caching**: Add caching layer in data/ without changing domain
- **Query history**: Add database entity and DAO without touching presentation
- **Multiple APIs**: Create additional RemoteDataSource implementations
- **Analytics**: Inject into ViewModel without affecting business logic
- **Authentication**: Add auth layer in data/ with transparent token refresh
- **Feature toggles**: Implement in domain UseCase easily
- **Rate limiting**: Apply at repository level transparently

## Migration Path from Legacy

1. Extract current AsyncTask code
2. Create Entities with validation in domain/
3. Create Repository interface
4. Create RemoteDataSource with same API call
5. Create LocalDataSource wrapper
6. Create UseCases that delegate
7. Create ViewModel with StateFlow
8. Create new layouts with ViewBinding
9. Replace Activity with clean implementation
10. Update AndroidManifest.xml
11. Inject Hilt application and activity annotations
12. Test each layer independently

## Compilation Verification

Each layer compiles independently:

```bash
# Domain layer (pure Kotlin)
kotlinc -d domain.jar domain/entity/*.kt domain/usecase/*.kt domain/repository/*.kt

# Data layer (depends on domain)
kotlinc -cp domain.jar -d data.jar data/datasource/*.kt data/repository/*.kt

# Presentation (depends on domain + Android)
kotlinc -cp domain.jar:android.jar -d presentation.jar presentation/**/*.kt
```

No circular dependencies ✓
Clear dependency arrows (→) ✓
One-way dependencies ✓

## Code Organization

```
src/
├── main/
│   ├── kotlin/
│   │   ├── presentation/
│   │   │   ├── ui/
│   │   │   │   ├── MainActivity.kt
│   │   │   │   └── activity_main.xml
│   │   │   ├── ui/state/
│   │   │   │   └── UiState.kt
│   │   │   └── viewmodel/
│   │   │       └── MainViewModel.kt
│   │   ├── domain/
│   │   │   ├── entity/
│   │   │   │   ├── Consulta.kt
│   │   │   │   └── Respuesta.kt
│   │   │   ├── repository/
│   │   │   │   └── IConsultaRepository.kt
│   │   │   └── usecase/
│   │   │       ├── EnviarConsultaUseCase.kt
│   │   │       ├── GetContadorUseCase.kt
│   │   │       └── IncrementarContadorUseCase.kt
│   │   ├── data/
│   │   │   ├── datasource/
│   │   │   │   ├── RemoteDataSource.kt
│   │   │   │   └── LocalDataSource.kt
│   │   │   └── repository/
│   │   │       └── ConsultaRepositoryImpl.kt
│   │   ├── di/
│   │   │   ├── DomainModule.kt
│   │   │   └── DataModule.kt
│   │   └── LegacyApplication.kt
│   └── AndroidManifest.xml
└── test/
    └── kotlin/
        ├── domain/usecase/
        │   ├── EnviarConsultaUseCaseTest.kt
        │   ├── GetContadorUseCaseTest.kt
        │   └── IncrementarContadorUseCaseTest.kt
        └── presentation/viewmodel/
            └── MainViewModelTest.kt
```

## Acceptance Criteria

✓ No compilation errors  
✓ No lint warnings (Hilt, ViewBinding, coroutines)  
✓ >90% code coverage  
✓ Zero memory leaks (Activity/ViewModel lifecycle correct)  
✓ Unit tests run on JVM without emulator  
✓ Domain layer has zero Android imports  
✓ Dependency graph flows inward (presentation → domain ← data)  
✓ All UseCases testable with mocks  
✓ StateFlow observable with repeatOnLifecycle  
✓ Average 30-60 lines per file (vs 150+ in legacy)  

## Next Steps for Production

1. Add room database for persistent caching
2. Add interceptor for API error handling
3. Add location data source
4. Add analytics event tracking
5. Add biometric authentication
6. Add push notifications receiver
7. Add image caching layer
8. Add pagination support
9. Add offline-first sync
10. Add feature flag integration
