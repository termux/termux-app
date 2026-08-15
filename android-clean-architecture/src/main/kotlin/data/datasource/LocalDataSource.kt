package data.datasource

import android.content.SharedPreferences

class LocalDataSource(
    private val sharedPreferences: SharedPreferences
) {
    fun getContador(): Int {
        return sharedPreferences.getInt(KEY_CONTADOR, 0)
    }

    fun incrementarContador(): Int {
        val nuevoContador = getContador() + 1
        sharedPreferences.edit().putInt(KEY_CONTADOR, nuevoContador).apply()
        return nuevoContador
    }

    companion object {
        private const val KEY_CONTADOR = "contador"
    }
}
