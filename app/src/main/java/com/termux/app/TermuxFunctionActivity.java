import android.app.Activity;
import android.os.Bundle;

import com.termux.R;

public final class TermuxFunctionActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.function);
    }
}
