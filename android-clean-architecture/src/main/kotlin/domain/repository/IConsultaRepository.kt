package domain.repository

import domain.entity.Consulta
import domain.entity.Respuesta

interface IConsultaRepository {
    suspend fun enviarConsulta(consulta: Consulta): Result<Respuesta>
    suspend fun obtenerContador(): Result<Int>
    suspend fun incrementarContador(): Result<Unit>
    fun getContadorSync(): Int
    fun incrementarContadorSync(): Int
}
