package di

import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import domain.repository.IConsultaRepository
import domain.usecase.EnviarConsultaUseCase
import domain.usecase.GetContadorUseCase
import domain.usecase.IncrementarContadorUseCase
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object DomainModule {

    @Provides
    @Singleton
    fun provideEnviarConsultaUseCase(
        repository: IConsultaRepository
    ): EnviarConsultaUseCase {
        return EnviarConsultaUseCase(repository)
    }

    @Provides
    @Singleton
    fun provideGetContadorUseCase(
        repository: IConsultaRepository
    ): GetContadorUseCase {
        return GetContadorUseCase(repository)
    }

    @Provides
    @Singleton
    fun provideIncrementarContadorUseCase(
        repository: IConsultaRepository
    ): IncrementarContadorUseCase {
        return IncrementarContadorUseCase(repository)
    }
}
