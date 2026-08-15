package domain.usecase

import domain.repository.IConsultaRepository

class IncrementarContadorUseCase(
    private val repository: IConsultaRepository
) {
    suspend operator fun invoke(): Result<Unit> {
        return repository.incrementarContador()
    }
}
