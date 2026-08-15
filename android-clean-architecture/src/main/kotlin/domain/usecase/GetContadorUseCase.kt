package domain.usecase

import domain.repository.IConsultaRepository

class GetContadorUseCase(
    private val repository: IConsultaRepository
) {
    operator fun invoke(): Int {
        return repository.getContadorSync()
    }
}
