package domain.entity

data class Consulta(
    val contenido: String,
    val timestamp: Long = System.currentTimeMillis()
) {
    init {
        require(contenido.isNotBlank()) { "Consulta cannot be empty" }
        require(contenido.length <= 500) { "Consulta must be <= 500 characters" }
    }
}
