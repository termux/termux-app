package data.repository

import data.datasource.LocalDataSource
import data.datasource.RemoteDataSource
import domain.entity.Consulta
import domain.entity.Respuesta
import domain.repository.IConsultaRepository

class ConsultaRepositoryImpl(
    private val remoteDataSource: RemoteDataSource,
    private val localDataSource: LocalDataSource
) : IConsultaRepository {

    override suspend fun enviarConsulta(consulta: Consulta): Result<Respuesta> {
        return remoteDataSource.fetch(
            url = "https://api.example.com/consultas",
            consulta = consulta.contenido
        ).onSuccess {
            localDataSource.incrementarContador()
        }
    }

    override suspend fun obtenerContador(): Result<Int> {
        return Result.success(localDataSource.getContador())
    }

    override suspend fun incrementarContador(): Result<Unit> {
        return runCatching {
            localDataSource.incrementarContador()
        }
    }

    override fun getContadorSync(): Int {
        return localDataSource.getContador()
    }

    override fun incrementarContadorSync(): Int {
        return localDataSource.incrementarContador()
    }
}
