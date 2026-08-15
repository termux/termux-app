# Clean Architecture Implementation Guide

## Complete Refactoring from Legacy Code

This guide walks through the exact process of transforming a legacy "God Activity" into a clean, testable architecture.

## Before: Legacy God Activity (150+ lines)

```kotlin
// ❌ LEGACY - DO NOT USE
class LegacyActivity : AppCompatActivity() {
    private var contador = 0
    private var adapter: ArrayAdapter<String>? = null
    private var progressDialog: ProgressDialog? = null
    private val sharedPref by lazy { getSharedPreferences("prefs", MODE_PRIVATE) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        contador = sharedPref.getInt("contador", 0)
        updateUI()

        findViewById<Button>(R.id.button).setOnClickListener {
            val input = findViewById<EditText>(R.id.edit).text.toString()
            if (input.isEmpty()) {
                Toast.makeText(this, "Empty", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            progressDialog = ProgressDialog(this).apply {
                setTitle("Loading...")
                show()
            }

            object : AsyncTask<String, Void, String>() {
                override fun doInBackground(vararg params: String?): String? {
                    return try {
                        val url = URL("https://api.example.com/consultas")
                        val conn = url.openConnection() as HttpURLConnection
                        conn.requestMethod = "POST"
                        conn.outputStream.use { 
                            it.write("""{"consulta":"${params[0]}"}""".toByteArray())
                        }
                        conn.inputStream.bufferedReader().readText()
                    } catch (e: Exception) {
                        "Error: ${e.message}"
                    }
                }

                override fun onPostExecute(result: String?) {
                    progressDialog?.dismiss()
                    if (result?.startsWith("Error") == false) {
                        contador++
                        sharedPref.edit().putInt("contador", contador).apply()
                        findViewById<TextView>(R.id.response).text = result
                    } else {
                        findViewById<TextView>(R.id.error).text = result
                    }
                    updateUI()
                }
            }.execute(input)
        }
    }

    private fun updateUI() {
        findViewById<TextView>(R.id.contador).text = "Consultas: $contador"
    }

    // ⚠️ Memory leak on rotation: AsyncTask holds Activity reference
    // ⚠️ ProgressDialog leaks: not dismissed on configuration change
    // ⚠️ SharedPreferences accessed directly from UI thread
    // ⚠️ No separation of concerns
    // ⚠️ Zero testability
}
```

## After: Clean Architecture Implementation

### Step 1: Define Domain Entities

```kotlin
// ✓ domain/entity/Consulta.kt - Pure Kotlin, no Android
package domain.entity

data class Consulta(
    val contenido: String,
    val timestamp: Long = System.currentTimeMillis()
) {
    init {
        require(contenido.isNotBlank()) { "Consulta cannot be empty" }
        require(contenido.length <= 500) { "Consulta must be <= 500 characters" }
    }
}

// ✓ domain/entity/Respuesta.kt
data class Respuesta(
    val id: String,
    val contenido: String,
    val timestamp: Long = System.currentTimeMillis()
) {
    init {
        require(id.isNotBlank()) { "ID cannot be empty" }
        require(contenido.isNotBlank()) { "Content cannot be empty" }
    }
}
```

**Benefits**:
- Validation in constructor (fail-fast)
- Immutable (prevents bugs)
- No Android dependency (highly testable)
- Can be used in any Kotlin project

### Step 2: Define Repository Contract

```kotlin
// ✓ domain/repository/IConsultaRepository.kt - Interface only
package domain.repository

interface IConsultaRepository {
    suspend fun enviarConsulta(consulta: Consulta): Result<Respuesta>
    suspend fun obtenerContador(): Result<Int>
    suspend fun incrementarContador(): Result<Unit>
    fun getContadorSync(): Int
    fun incrementarContadorSync(): Int
}
```

**Why Interface?**
- Dependency Inversion Principle
- Easy to mock for testing
- Multiple implementations possible (API v1, v2, mock)
- Changes propagate transparently

### Step 3: Create Domain UseCases

```kotlin
// ✓ domain/usecase/EnviarConsultaUseCase.kt
package domain.usecase

class EnviarConsultaUseCase(
    private val repository: IConsultaRepository
) {
    suspend operator fun invoke(consulta: Consulta): Result<Respuesta> {
        return repository.enviarConsulta(consulta)
    }
}

// ✓ domain/usecase/GetContadorUseCase.kt
class GetContadorUseCase(
    private val repository: IConsultaRepository
) {
    operator fun invoke(): Int {
        return repository.getContadorSync()
    }
}

// ✓ domain/usecase/IncrementarContadorUseCase.kt
class IncrementarContadorUseCase(
    private val repository: IConsultaRepository
) {
    suspend operator fun invoke(): Result<Unit> {
        return repository.incrementarContador()
    }
}
```

**UseCase Pattern**:
- `operator fun invoke()` makes them callable like functions
- Single responsibility per UseCase
- Easy to test in isolation
- Reusable across features

### Step 4: Implement Data Layer

```kotlin
// ✓ data/datasource/RemoteDataSource.kt
package data.datasource

class RemoteDataSource {
    suspend fun fetch(url: String, consulta: String): Result<Respuesta> {
        return withContext(Dispatchers.IO) {
            runCatching {
                val connection = URL(url).openConnection() as HttpURLConnection
                connection.apply {
                    requestMethod = "POST"
                    connectTimeout = 5000      // 5 second timeout
                    readTimeout = 5000
                    setRequestProperty("Content-Type", "application/json")
                    doOutput = true
                }

                connection.outputStream.use { output ->
                    val json = """{"consulta":"$consulta"}"""
                    output.write(json.toByteArray())
                    output.flush()
                }

                val responseCode = connection.responseCode
                when {
                    responseCode in 200..299 -> {
                        val response = connection.inputStream
                            .bufferedReader()
                            .use { it.readText() }
                        parseResponse(response)
                    }
                    else -> throw Exception("HTTP Error: $responseCode")
                }
            }
        }
    }

    private fun parseResponse(json: String): Respuesta {
        val id = extractValue(json, "id")
        val contenido = extractValue(json, "contenido")
        return Respuesta(id, contenido)
    }

    private fun extractValue(json: String, key: String): String {
        val regex = """"$key":"([^"]*)"""".toRegex()
        return regex.find(json)?.groupValues?.get(1) ?: ""
    }
}

// ✓ data/datasource/LocalDataSource.kt
package data.datasource

class LocalDataSource(
    private val sharedPreferences: SharedPreferences
) {
    fun getContador(): Int {
        return sharedPreferences.getInt("contador", 0)
    }

    fun incrementarContador(): Int {
        val nuevo = getContador() + 1
        sharedPreferences.edit()
            .putInt("contador", nuevo)
            .apply()
        return nuevo
    }
}

// ✓ data/repository/ConsultaRepositoryImpl.kt
package data.repository

class ConsultaRepositoryImpl(
    private val remoteDataSource: RemoteDataSource,
    private val localDataSource: LocalDataSource
) : IConsultaRepository {

    override suspend fun enviarConsulta(consulta: Consulta): Result<Respuesta> {
        return remoteDataSource.fetch(API_URL, consulta.contenido)
            .onSuccess { localDataSource.incrementarContador() }
    }

    override suspend fun obtenerContador(): Result<Int> {
        return Result.success(localDataSource.getContador())
    }

    override suspend fun incrementarContador(): Result<Unit> {
        return runCatching { localDataSource.incrementarContador() }
    }

    override fun getContadorSync(): Int = localDataSource.getContador()
    override fun incrementarContadorSync(): Int = localDataSource.incrementarContador()

    companion object {
        private const val API_URL = "https://api.example.com/consultas"
    }
}
```

**Data Layer Responsibilities**:
- HTTP communication with timeouts
- Local storage management
- Error handling with Result type
- Transparent to presentation layer

### Step 5: Create ViewModel

```kotlin
// ✓ presentation/ui/state/UiState.kt
package presentation.ui.state

data class UiState(
    val isLoading: Boolean = false,
    val respuesta: Respuesta? = null,
    val contador: Int = 0,
    val error: String? = null
)

// ✓ presentation/viewmodel/MainViewModel.kt
package presentation.viewmodel

@HiltViewModel
class MainViewModel @Inject constructor(
    private val enviarConsultaUseCase: EnviarConsultaUseCase,
    private val getContadorUseCase: GetContadorUseCase,
    private val incrementarContadorUseCase: IncrementarContadorUseCase
) : ViewModel() {

    private val _uiState = MutableStateFlow(UiState(contador = 0))
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    init {
        cargarContador()
    }

    fun enviarConsulta(contenido: String) {
        if (contenido.isBlank()) {
            _uiState.value = _uiState.value.copy(
                error = "Consulta cannot be empty"
            )
            return
        }

        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(isLoading = true, error = null)
            try {
                val consulta = Consulta(contenido)
                enviarConsultaUseCase(consulta)
                    .onSuccess { respuesta ->
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            respuesta = respuesta,
                            contador = getContadorUseCase()
                        )
                    }
                    .onFailure { throwable ->
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            error = throwable.message ?: "Unknown error"
                        )
                    }
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    error = e.message ?: "Validation error"
                )
            }
        }
    }

    fun clearError() {
        _uiState.value = _uiState.value.copy(error = null)
    }

    private fun cargarContador() {
        _uiState.value = _uiState.value.copy(contador = getContadorUseCase())
    }
}
```

**ViewModel Benefits**:
- Survives configuration changes (rotations)
- Lifecycle-aware coroutines (viewModelScope)
- State management with StateFlow
- No memory leaks

### Step 6: Create Clean Activity

```kotlin
// ✓ presentation/ui/MainActivity.kt
@AndroidEntryPoint
class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding
    private val viewModel: MainViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupListeners()
        observeUiState()
    }

    private fun setupListeners() {
        binding.buttonEnviar.setOnClickListener {
            val contenido = binding.editTextConsulta.text.toString()
            viewModel.enviarConsulta(contenido)
        }

        binding.textViewError.setOnClickListener {
            viewModel.clearError()
            binding.textViewError.visibility = View.GONE
        }
    }

    private fun observeUiState() {
        lifecycleScope.launch {
            repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.uiState.collect { uiState ->
                    render(uiState)
                }
            }
        }
    }

    private fun render(uiState: UiState) {
        // Show/hide loading
        binding.progressBar.visibility = 
            if (uiState.isLoading) View.VISIBLE else View.GONE

        // Disable button while loading
        binding.buttonEnviar.isEnabled = !uiState.isLoading

        // Update counter
        binding.textViewContador.text = "Queries: ${uiState.contador}"

        // Show response
        uiState.respuesta?.let { respuesta ->
            binding.textViewRespuesta.text = "Response: ${respuesta.contenido}"
            binding.textViewRespuesta.visibility = View.VISIBLE
        }

        // Show error
        uiState.error?.let { error ->
            binding.textViewError.text = "Error: $error"
            binding.textViewError.visibility = View.VISIBLE
        } ?: run {
            binding.textViewError.visibility = View.GONE
        }
    }
}
```

**Activity Improvements**:
- Zero business logic
- ViewBinding (no findViewById)
- Reactive to ViewModel state
- Lifecycle-safe coroutines

### Step 7: Setup Dependency Injection

```kotlin
// ✓ di/DomainModule.kt
@Module
@InstallIn(SingletonComponent::class)
object DomainModule {
    @Provides
    @Singleton
    fun provideEnviarConsultaUseCase(
        repo: IConsultaRepository
    ): EnviarConsultaUseCase = EnviarConsultaUseCase(repo)

    @Provides
    @Singleton
    fun provideGetContadorUseCase(
        repo: IConsultaRepository
    ): GetContadorUseCase = GetContadorUseCase(repo)

    @Provides
    @Singleton
    fun provideIncrementarContadorUseCase(
        repo: IConsultaRepository
    ): IncrementarContadorUseCase = IncrementarContadorUseCase(repo)
}

// ✓ di/DataModule.kt
@Module
@InstallIn(SingletonComponent::class)
object DataModule {
    @Provides
    @Singleton
    fun provideSharedPreferences(
        @ApplicationContext context: Context
    ): SharedPreferences {
        return context.getSharedPreferences("prefs", Context.MODE_PRIVATE)
    }

    @Provides
    @Singleton
    fun provideRemoteDataSource(): RemoteDataSource = RemoteDataSource()

    @Provides
    @Singleton
    fun provideLocalDataSource(
        sp: SharedPreferences
    ): LocalDataSource = LocalDataSource(sp)

    @Provides
    @Singleton
    fun provideRepository(
        remote: RemoteDataSource,
        local: LocalDataSource
    ): IConsultaRepository = ConsultaRepositoryImpl(remote, local)
}

// ✓ LegacyApplication.kt
@HiltAndroidApp
class LegacyApplication : Application()
```

### Step 8: Write Unit Tests

```kotlin
// ✓ src/test/kotlin/domain/usecase/EnviarConsultaUseCaseTest.kt
class EnviarConsultaUseCaseTest {
    private val mockRepo = mockk<IConsultaRepository>()
    private val useCase = EnviarConsultaUseCase(mockRepo)

    @Test
    fun `invoke returns success on valid consulta`() = runBlocking {
        val consulta = Consulta("Test")
        val expected = Respuesta("id1", "Response")
        
        coEvery { mockRepo.enviarConsulta(consulta) } returns Result.success(expected)
        
        val result = useCase(consulta)
        
        assertTrue(result.isSuccess)
        assertEquals(expected, result.getOrNull())
    }

    @Test
    fun `invoke returns failure on network error`() = runBlocking {
        val consulta = Consulta("Test")
        val exception = Exception("Network error")
        
        coEvery { mockRepo.enviarConsulta(consulta) } returns Result.failure(exception)
        
        val result = useCase(consulta)
        
        assertTrue(result.isFailure)
    }
}

// ✓ src/test/kotlin/presentation/viewmodel/MainViewModelTest.kt
class MainViewModelTest {
    private val mockEnviar = mockk<EnviarConsultaUseCase>()
    private val mockGet = mockk<GetContadorUseCase>()
    private val mockIncrementar = mockk<IncrementarContadorUseCase>()

    private lateinit var viewModel: MainViewModel

    @Before
    fun setup() {
        Dispatchers.setMain(Dispatchers.Unconfined)
        every { mockGet() } returns 0
        viewModel = MainViewModel(mockEnviar, mockGet, mockIncrementar)
    }

    @Test
    fun `enviarConsulta with empty string shows error`() = runBlocking {
        viewModel.enviarConsulta("")
        
        val state = viewModel.uiState.first()
        assertNotNull(state.error)
    }

    @Test
    fun `enviarConsulta successful updates respuesta`() = runBlocking {
        val respuesta = Respuesta("id1", "Response")
        coEvery { mockEnviar(any()) } returns Result.success(respuesta)
        every { mockGet() } returns 1

        viewModel.enviarConsulta("Test")

        kotlinx.coroutines.delay(100)
        val state = viewModel.uiState.first()
        assertEquals(respuesta, state.respuesta)
    }
}
```

## Comparison: Before vs After

| Aspect | Legacy | Clean Architecture |
|--------|--------|-------------------|
| **File Lines** | 150+ in one file | 30-60 lines per file |
| **Testability** | 0% (mixed UI/logic) | 95%+ (unit testable) |
| **Memory Leaks** | AsyncTask leaks | Zero (ViewModel lifecycle) |
| **Type Safety** | findViewById() | ViewBinding compile-time |
| **Reusability** | UI-only | Usecase/domain reusable |
| **Maintainability** | Nightmare | Clear layers |
| **Configuration Changes** | Crash/leak | Automatic survival |
| **Error Handling** | try-catch chain | Result type |
| **Testing Speed** | Requires emulator | JVM unit tests (2s) |
| **Dependencies** | Mixed concerns | Clear graph |

## Validation Checklist

- [ ] Domain layer has ZERO `import android.*`
- [ ] All UseCases use `operator fun invoke()`
- [ ] Entities have validation in `init` blocks
- [ ] ViewModel injects UseCases via constructor
- [ ] ViewModel exposes immutable StateFlow
- [ ] MainActivity uses ViewBinding
- [ ] MainActivity uses `repeatOnLifecycle`
- [ ] Hilt annotations on Application and Activity
- [ ] DomainModule provides all UseCases
- [ ] DataModule provides all DataSources and Repository
- [ ] All UseCase tests pass
- [ ] ViewModel tests cover happy path + errors
- [ ] No findViewById() in Activity
- [ ] No AsyncTask anywhere
- [ ] No ProgressDialog (use ProgressBar with state)
- [ ] Test coverage > 90%

## Common Pitfalls

### ❌ Accessing SharedPreferences from Activity

```kotlin
// Bad - direct access
val contador = getSharedPreferences("prefs", MODE_PRIVATE)
    .getInt("contador", 0)
```

### ✓ Through Domain Layer

```kotlin
// Good - through repository
val contador = getContadorUseCase()
```

### ❌ Importing Android in Domain

```kotlin
// Bad - breaks domain independence
package domain.usecase
import android.content.Context  // ❌ FORBIDDEN
```

### ✓ Pure Kotlin Only

```kotlin
// Good - pure kotlin only
package domain.usecase
import domain.repository.IConsultaRepository
import domain.entity.Consulta
```

### ❌ Mutable StateFlow Exposed

```kotlin
// Bad - external modification possible
val uiState: MutableStateFlow<UiState> = _uiState
```

### ✓ Immutable StateFlow Exposed

```kotlin
// Good - read-only outside viewmodel
val uiState: StateFlow<UiState> = _uiState.asStateFlow()
```

## Next Steps

1. Apply this structure to your existing app
2. Test each layer independently
3. Gradually migrate screens one by one
4. Monitor test coverage improvement
5. Review memory leaks with Android Profiler
6. Celebrate reduced maintenance burden
