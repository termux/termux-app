package domain.usecase

import domain.repository.IConsultaRepository
import io.mockk.every
import io.mockk.mockk
import org.junit.Test
import kotlin.test.assertEquals

class GetContadorUseCaseTest {

    private val mockRepository = mockk<IConsultaRepository>()
    private val useCase = GetContadorUseCase(mockRepository)

    @Test
    fun `invoke should return contador from repository`() {
        every { mockRepository.getContadorSync() } returns 5

        val result = useCase()

        assertEquals(5, result)
    }

    @Test
    fun `invoke should return zero when no consultas sent`() {
        every { mockRepository.getContadorSync() } returns 0

        val result = useCase()

        assertEquals(0, result)
    }

    @Test
    fun `invoke should return updated contador`() {
        every { mockRepository.getContadorSync() } returns 42

        val result = useCase()

        assertEquals(42, result)
    }
}
