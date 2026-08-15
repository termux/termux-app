package domain.usecase

import domain.entity.Consulta
import domain.entity.Respuesta
import domain.repository.IConsultaRepository
import io.mockk.coEvery
import io.mockk.mockk
import kotlinx.coroutines.runBlocking
import org.junit.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue

class EnviarConsultaUseCaseTest {

    private val mockRepository = mockk<IConsultaRepository>()
    private val useCase = EnviarConsultaUseCase(mockRepository)

    @Test
    fun `invoke should return success with respuesta on successful repository call`() = runBlocking {
        val consulta = Consulta("Test query")
        val expectedRespuesta = Respuesta("id1", "Test response")

        coEvery { mockRepository.enviarConsulta(consulta) } returns Result.success(expectedRespuesta)

        val result = useCase(consulta)

        assertTrue(result.isSuccess)
        assertEquals(expectedRespuesta, result.getOrNull())
    }

    @Test
    fun `invoke should return failure on repository error`() = runBlocking {
        val consulta = Consulta("Test query")
        val exception = Exception("Network error")

        coEvery { mockRepository.enviarConsulta(consulta) } returns Result.failure(exception)

        val result = useCase(consulta)

        assertTrue(result.isFailure)
        assertEquals("Network error", result.exceptionOrNull()?.message)
    }

    @Test
    fun `invoke should call repository with correct consulta`() = runBlocking {
        val consulta = Consulta("Test query")
        coEvery { mockRepository.enviarConsulta(consulta) } returns Result.success(
            Respuesta("id1", "Response")
        )

        useCase(consulta)

        io.mockk.verify { runBlocking { mockRepository.enviarConsulta(consulta) } }
    }
}
