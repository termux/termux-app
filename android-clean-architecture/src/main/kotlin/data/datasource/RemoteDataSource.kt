package data.datasource

import domain.entity.Respuesta
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.net.HttpURLConnection
import java.net.URL

class RemoteDataSource {
    suspend fun fetch(url: String, consulta: String): Result<Respuesta> = withContext(Dispatchers.IO) {
        runCatching {
            val connection = URL(url).openConnection() as HttpURLConnection
            connection.apply {
                requestMethod = "POST"
                connectTimeout = 5000
                readTimeout = 5000
                setRequestProperty("Content-Type", "application/json")
                doOutput = true
            }

            connection.outputStream.use { output ->
                val json = """{"consulta":"$consulta"}""".toByteArray()
                output.write(json)
                output.flush()
            }

            val responseCode = connection.responseCode
            if (responseCode == HttpURLConnection.HTTP_OK || responseCode == HttpURLConnection.HTTP_CREATED) {
                val response = connection.inputStream.bufferedReader().use { it.readText() }
                parseResponse(response)
            } else {
                throw Exception("HTTP Error: $responseCode")
            }
        }
    }

    private fun parseResponse(jsonResponse: String): Respuesta {
        val id = extractJsonValue(jsonResponse, "id")
        val contenido = extractJsonValue(jsonResponse, "contenido")
        return Respuesta(id, contenido)
    }

    private fun extractJsonValue(json: String, key: String): String {
        val pattern = """"$key":"([^"]*)"""".toRegex()
        return pattern.find(json)?.groupValues?.get(1) ?: ""
    }
}
