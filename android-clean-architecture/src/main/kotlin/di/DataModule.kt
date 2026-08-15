package di

import android.content.Context
import android.content.SharedPreferences
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import data.datasource.LocalDataSource
import data.datasource.RemoteDataSource
import data.repository.ConsultaRepositoryImpl
import domain.repository.IConsultaRepository
import javax.inject.Singleton

@Module
@InstallIn(SingletonComponent::class)
object DataModule {

    @Provides
    @Singleton
    fun provideSharedPreferences(
        @ApplicationContext context: Context
    ): SharedPreferences {
        return context.getSharedPreferences("consultas_prefs", Context.MODE_PRIVATE)
    }

    @Provides
    @Singleton
    fun provideRemoteDataSource(): RemoteDataSource {
        return RemoteDataSource()
    }

    @Provides
    @Singleton
    fun provideLocalDataSource(
        sharedPreferences: SharedPreferences
    ): LocalDataSource {
        return LocalDataSource(sharedPreferences)
    }

    @Provides
    @Singleton
    fun provideConsultaRepository(
        remoteDataSource: RemoteDataSource,
        localDataSource: LocalDataSource
    ): IConsultaRepository {
        return ConsultaRepositoryImpl(remoteDataSource, localDataSource)
    }
}
