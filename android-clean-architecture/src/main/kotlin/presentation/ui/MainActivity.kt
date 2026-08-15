package presentation.ui

import android.os.Bundle
import android.view.View
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.launch
import presentation.databinding.ActivityMainBinding
import presentation.viewmodel.MainViewModel

@AndroidEntryPoint
class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private val viewModel: MainViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        setupListeners()
        observeUiState()
    }

    private fun setupListeners() {
        binding.buttonEnviar.setOnClickListener {
            val contenido = binding.editTextConsulta.text.toString()
            viewModel.enviarConsulta(contenido)
        }

        binding.textViewError.setOnClickListener {
            viewModel.clearError()
            binding.textViewError.visibility = View.GONE
        }
    }

    private fun observeUiState() {
        lifecycleScope.launch {
            repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.uiState.collect { uiState ->
                    render(uiState)
                }
            }
        }
    }

    private fun render(uiState: presentation.ui.state.UiState) {
        binding.progressBar.visibility = if (uiState.isLoading) View.VISIBLE else View.GONE
        binding.buttonEnviar.isEnabled = !uiState.isLoading

        binding.textViewContador.text = "Consultas: ${uiState.contador}"

        uiState.respuesta?.let { respuesta ->
            binding.textViewRespuesta.text = "Respuesta: ${respuesta.contenido}"
            binding.textViewRespuesta.visibility = View.VISIBLE
        }

        uiState.error?.let { error ->
            binding.textViewError.text = "Error: $error"
            binding.textViewError.visibility = View.VISIBLE
        } ?: run {
            binding.textViewError.visibility = View.GONE
        }
    }
}
