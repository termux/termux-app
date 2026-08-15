package presentation.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import dagger.hilt.android.lifecycle.HiltViewModel
import domain.entity.Consulta
import domain.usecase.EnviarConsultaUseCase
import domain.usecase.GetContadorUseCase
import domain.usecase.IncrementarContadorUseCase
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import presentation.ui.state.UiState
import javax.inject.Inject

@HiltViewModel
class MainViewModel @Inject constructor(
    private val enviarConsultaUseCase: EnviarConsultaUseCase,
    private val getContadorUseCase: GetContadorUseCase,
    private val incrementarContadorUseCase: IncrementarContadorUseCase
) : ViewModel() {

    private val _uiState = MutableStateFlow(UiState(contador = 0))
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    init {
        cargarContador()
    }

    fun enviarConsulta(contenido: String) {
        if (contenido.isBlank()) {
            _uiState.value = _uiState.value.copy(error = "Consulta cannot be empty")
            return
        }

        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(isLoading = true, error = null)
            try {
                val consulta = Consulta(contenido)
                enviarConsultaUseCase(consulta)
                    .onSuccess { respuesta ->
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            respuesta = respuesta,
                            contador = getContadorUseCase()
                        )
                    }
                    .onFailure { throwable ->
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            error = throwable.message ?: "Unknown error"
                        )
                    }
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    error = e.message ?: "Validation error"
                )
            }
        }
    }

    fun clearError() {
        _uiState.value = _uiState.value.copy(error = null)
    }

    private fun cargarContador() {
        _uiState.value = _uiState.value.copy(contador = getContadorUseCase())
    }
}
