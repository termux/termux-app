package domain.usecase

import domain.repository.IConsultaRepository
import io.mockk.coEvery
import io.mockk.mockk
import kotlinx.coroutines.runBlocking
import org.junit.Test
import kotlin.test.assertTrue

class IncrementarContadorUseCaseTest {

    private val mockRepository = mockk<IConsultaRepository>()
    private val useCase = IncrementarContadorUseCase(mockRepository)

    @Test
    fun `invoke should return success on successful increment`() = runBlocking {
        coEvery { mockRepository.incrementarContador() } returns Result.success(Unit)

        val result = useCase()

        assertTrue(result.isSuccess)
    }

    @Test
    fun `invoke should return failure on error`() = runBlocking {
        val exception = Exception("Increment failed")
        coEvery { mockRepository.incrementarContador() } returns Result.failure(exception)

        val result = useCase()

        assertTrue(result.isFailure)
    }

    @Test
    fun `invoke should call repository increment method`() = runBlocking {
        coEvery { mockRepository.incrementarContador() } returns Result.success(Unit)

        useCase()

        io.mockk.verify { runBlocking { mockRepository.incrementarContador() } }
    }
}
