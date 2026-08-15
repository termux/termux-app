package domain.usecase

import domain.entity.Consulta
import domain.entity.Respuesta
import domain.repository.IConsultaRepository

class EnviarConsultaUseCase(
    private val repository: IConsultaRepository
) {
    suspend operator fun invoke(consulta: Consulta): Result<Respuesta> {
        return repository.enviarConsulta(consulta)
    }
}
