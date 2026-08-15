package presentation.ui.state

import domain.entity.Respuesta

data class UiState(
    val isLoading: Boolean = false,
    val respuesta: Respuesta? = null,
    val contador: Int = 0,
    val error: String? = null
)
