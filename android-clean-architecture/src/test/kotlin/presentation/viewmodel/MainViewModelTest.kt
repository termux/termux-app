package presentation.viewmodel

import domain.entity.Consulta
import domain.entity.Respuesta
import domain.usecase.EnviarConsultaUseCase
import domain.usecase.GetContadorUseCase
import domain.usecase.IncrementarContadorUseCase
import io.mockk.coEvery
import io.mockk.coVerify
import io.mockk.every
import io.mockk.mockk
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.test.resetMain
import kotlinx.coroutines.test.setMain
import org.junit.After
import org.junit.Before
import org.junit.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

class MainViewModelTest {

    private val mockEnviarUseCase = mockk<EnviarConsultaUseCase>()
    private val mockGetUseCase = mockk<GetContadorUseCase>()
    private val mockIncrementarUseCase = mockk<IncrementarContadorUseCase>()

    private lateinit var viewModel: MainViewModel

    @Before
    fun setup() {
        Dispatchers.setMain(Dispatchers.Unconfined)
        every { mockGetUseCase() } returns 0
        viewModel = MainViewModel(mockEnviarUseCase, mockGetUseCase, mockIncrementarUseCase)
    }

    @After
    fun tearDown() {
        Dispatchers.resetMain()
    }

    @Test
    fun `initial state should have contador 0`() = runBlocking {
        val state = viewModel.uiState.first()
        assertEquals(0, state.contador)
    }

    @Test
    fun `enviarConsulta with empty string should set error`() = runBlocking {
        viewModel.enviarConsulta("")
        val state = viewModel.uiState.first()
        assertNotNull(state.error)
    }

    @Test
    fun `enviarConsulta should set loading state`() = runBlocking {
        coEvery { mockEnviarUseCase(any()) } returns Result.success(Respuesta("id1", "Response"))
        every { mockGetUseCase() } returns 1

        viewModel.enviarConsulta("Test query")

        val states = mutableListOf<presentation.ui.state.UiState>()
        val job = kotlinx.coroutines.launch {
            viewModel.uiState.collect { states.add(it) }
        }
        kotlinx.coroutines.delay(100)
        job.cancel()

        assertTrue(states.any { it.isLoading })
    }

    @Test
    fun `enviarConsulta with valid input should call useCase`() = runBlocking {
        val consulta = Consulta("Test query")
        coEvery { mockEnviarUseCase(any()) } returns Result.success(Respuesta("id1", "Response"))
        every { mockGetUseCase() } returns 1

        viewModel.enviarConsulta("Test query")

        kotlinx.coroutines.delay(100)
        coVerify { mockEnviarUseCase(any()) }
    }

    @Test
    fun `enviarConsulta should update respuesta on success`() = runBlocking {
        val expectedRespuesta = Respuesta("id1", "Response")
        coEvery { mockEnviarUseCase(any()) } returns Result.success(expectedRespuesta)
        every { mockGetUseCase() } returns 1

        viewModel.enviarConsulta("Test query")

        kotlinx.coroutines.delay(100)
        val state = viewModel.uiState.first()
        assertEquals(expectedRespuesta, state.respuesta)
    }

    @Test
    fun `enviarConsulta should set error on failure`() = runBlocking {
        val exception = Exception("Network error")
        coEvery { mockEnviarUseCase(any()) } returns Result.failure(exception)

        viewModel.enviarConsulta("Test query")

        kotlinx.coroutines.delay(100)
        val state = viewModel.uiState.first()
        assertNotNull(state.error)
        assertTrue(state.error?.contains("Network error") ?: false)
    }

    @Test
    fun `clearError should remove error message`() = runBlocking {
        coEvery { mockEnviarUseCase(any()) } returns Result.failure(Exception("Error"))

        viewModel.enviarConsulta("Test query")
        kotlinx.coroutines.delay(100)
        viewModel.clearError()

        val state = viewModel.uiState.first()
        assertNull(state.error)
    }

    @Test
    fun `loading should be false after request completes`() = runBlocking {
        coEvery { mockEnviarUseCase(any()) } returns Result.success(Respuesta("id1", "Response"))
        every { mockGetUseCase() } returns 1

        viewModel.enviarConsulta("Test query")

        kotlinx.coroutines.delay(100)
        val state = viewModel.uiState.first()
        assertFalse(state.isLoading)
    }
}
