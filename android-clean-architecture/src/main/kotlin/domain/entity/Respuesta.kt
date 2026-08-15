package domain.entity

data class Respuesta(
    val id: String,
    val contenido: String,
    val timestamp: Long = System.currentTimeMillis()
) {
    init {
        require(id.isNotBlank()) { "Response ID cannot be empty" }
        require(contenido.isNotBlank()) { "Response content cannot be empty" }
    }
}
